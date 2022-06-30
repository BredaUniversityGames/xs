#include <render.h>
#include <ios>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#pragma clang diagnostic ignored "-Wreturn-type"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_easy_font.h>
#pragma clang diagnostic pop

#include <configuration.h>
#include <fileio.h>
#include <log.h>


// In order for any of the asserts in Agc headers to work (ex. registerstructs.h) SCE_AGC_DEBUG must be defined before including those headers.
// Since the extra validation adds an extra cost, we only define SCE_AGC_DEBUG for Debug builds.
#if defined(_DEBUG) && !defined(SCE_AGC_DEBUG)
#define SCE_AGC_DEBUG
#endif

// Adding this define before including Agc will enable the C++ nodiscard feature, which will return a warning anytime that you do not
// use the return value from a function that has been marked with the [[nodiscard]] attribute. In Agc, this attribute is added to all 
// functions that return a SceError whenever this define exists, which means that adding this define will make it easier to make sure 
// that you are handling any and all error codes that are returned by Agc.
#define SCE_AGC_CHECK_ERROR_CODES
//#include <stdio.h>
//#include <stdlib.h>
#include <kernel.h>
#include <agc.h>
#include <video_out.h>

const uint32_t BUFFERING = 2;			// This defines how many DCBs and scan-out buffers we have.
const uint32_t SCREEN_WIDTH = 3840;
const uint32_t SCREEN_HEIGHT = 2160;

// These symbols point to the headers and code of the shader binaries linked into the sample's elf.
// They are declared inside the shader code. For example, the Shader::ps_header and Shader::ps_text symbols
// were declared in the shader by putting the attribute [CxxSymbol("Shader::ps")] in front of the pixel 
// shader's entry point.
namespace Shader
{
	extern char ps_header[];
	extern const char ps_text[];
	extern char gs_header[];
	extern const char gs_text[];
}

using namespace glm;

// This is a temporary utility function to allocate direct memory. It's not important to understand how this works.
uint8_t* allocDmem(sce::Agc::SizeAlign sizeAlign)
{
	if (!sizeAlign.m_size)
	{
		return nullptr;
	}

	static uint32_t allocCount = 0;
	off_t offsetOut;

	const size_t alignment = (sizeAlign.m_align + 0xffffu) & ~0xffffu;
	const uint64_t size = (sizeAlign.m_size + 0xffffu) & ~0xffffu;

	int32_t ret = sceKernelAllocateMainDirectMemory(size, alignment, SCE_KERNEL_MTYPE_C_SHARED, &offsetOut);
	if (ret) {
		printf("sceKernelAllocateMainDirectMemory error:0x%x size:0x%zx\n", ret, size);
		return nullptr;
	}

	void* ptr = NULL;
	char namedStr[32];
	snprintf_s(namedStr, sizeof(namedStr), "agc_single_triangle %d_%zuKB", allocCount++, size >> 10);
	ret = sceKernelMapNamedDirectMemory(&ptr, size, SCE_KERNEL_PROT_GPU_RW | SCE_KERNEL_PROT_CPU_RW, 0, offsetOut, alignment, namedStr);
	SCE_AGC_ASSERT_MSG(ret == SCE_OK, "Unable to map memory");
	return (uint8_t*)ptr;
}



// This Material struct will be how we pass around the entire indirect state of a frame. There are two
// important things to note here, the first begin that everything is stored in opaque Register arrays 
// and that there are three different kinds of registers.
// The opaque nature of the Registers here is simply for convenience, since it means that we don't need
// to actually know what state is in a Material to use it.
// The three types however are crucially important. The GPU expects all indirect state to be grouped by
// this type and mixing two or more types in a single indirect state update will fail.
// Of course, it's possible to simply issue three state updates back to back, which is exactly what we
// do in this sample.
struct Material
{
	sce::Agc::CxRegister* m_cxRegs;
	sce::Agc::ShRegister* m_shRegs;
	sce::Agc::UcRegister* m_ucRegs;
	uint32_t m_numCxRegs;
	uint32_t m_numShRegs;
	uint32_t m_numUcRegs;
	sce::Agc::DrawModifier m_drawMod;
};

// Much of our indirect state comes from the shader binaries and looks quite similar in shape to the 
// Material struct above. Some of the state we want to set up ourselves, however, so we want it to be
// properly typed. Later, we will overlay this struct on top of the m_cxRegs member of the Material to
// access it.
struct CxState
{
	static const sce::Agc::RegisterType m_type = sce::Agc::RegisterType::kCx;
	sce::Agc::CxRenderTarget rt;
	sce::Agc::CxRenderTargetMask rt_mask;
	sce::Agc::CxViewport vport;
	sce::Agc::CxShaderLinkage linkage;
};


// Before we can render anything, we want to build our materials, which encompass all indirect state we want to set.
// It is theoretically possible to kick off the command buffer before the material is ready and have the CP wait for the
// CPU to be done, but this is out of scope for this sample.
//
// Since every frame will need to write to its own backbuffer, we also want one material per backbuffer, again, this is just
// for convenience here and we could reuse the same Material and patch it between frames.
//
// One array per register type gives the minimal number of GPU operations to set up the state, which is very efficient for
// the GPU, but not necessarily very convenient to write on the CPU. It is possible to break up the state into multiple arrays
// of indirect state per type and Material and to perform multiple state setting operations, at the cost of some CP cycles.
// Simply for the purpose of demonstration, this sample writes the minimal number of commands to the CP and thus combines all
// state.

bool createMaterials(Material* mat, uint32_t count)
{
	// First, we load the shaders, since the size of the shader's register blocks is not known. This is generally the only part
	// of the state that has an unknown size, making the question of how to handle shader state key to managing state.
	// In this example, we combine all state together into one array per type, which is the most efficient approach for the
	// GPU, but also generally the most complex in terms of CPU code.
	sce::Agc::Shader* gs, * ps;
	SceError err = sce::Agc::createShader(&gs, Shader::gs_header, Shader::gs_text);
	SCE_AGC_ASSERT_MSG_RETURN(err == SCE_OK, false, "Unable to create Gs.");
	sce::Agc::Core::registerResource(gs, "Shader::gs");

	err = sce::Agc::createShader(&ps, Shader::ps_header, Shader::ps_text);
	SCE_AGC_ASSERT_MSG_RETURN(err == SCE_OK, false, "Unable to create Ps.");
	sce::Agc::Core::registerResource(ps, "Shader::ps");

	// Next, we compute the sizes of the material's state blocks. Only Cx state has some entries that are not determined
	// by the shader and are defined in the CxState struct. Sh and Uc state on the other hand can be treated as fully
	// opaque in this example.
	const uint32_t customCxSize = SCE_AGC_REG_COUNT(CxState());
	const uint32_t cxStateSize = gs->m_numCxRegisters + ps->m_numCxRegisters + customCxSize;
	const uint32_t shStateSize = gs->m_numShRegisters + ps->m_numShRegisters;
	const uint32_t ucStateSize = SCE_AGC_REG_COUNT(sce::Agc::UcPrimitiveState());

	// Since our dmem allocations are very coarse, we just want this to be a single allocation.
	// There is no requirement for the three arrays to be consecutive in memory.
	uint32_t materialSize = (cxStateSize + shStateSize + ucStateSize);
	// We only need multiple copies of the Cx state, since all other state can be shared between the different materials.
	// Admittedly, reusing the Sh and Uc states in this example is being unnecessarily "clever", but in a real title
	// reusing indirect state can be a tremendous win in terms of memory and CPU time, since we can just set the shared
	// state up once and then only ever need to insert the state update packets into the command buffer.
	uint8_t* materialMemory = allocDmem({ (materialSize + (count - 1) * cxStateSize) * sizeof(sce::Agc::CxRegister), sce::Agc::Alignment::kRegister });

	// Since we have one allocation we need to then split the memory between the three pointers. With a better memory manager, you would probably not do this.
	mat->m_cxRegs = &((sce::Agc::CxRegister*)materialMemory)[0];
	mat->m_shRegs = &((sce::Agc::ShRegister*)materialMemory)[cxStateSize];
	mat->m_ucRegs = &((sce::Agc::UcRegister*)materialMemory)[cxStateSize + shStateSize];
	mat->m_numCxRegs = cxStateSize;
	mat->m_numShRegs = shStateSize;
	mat->m_numUcRegs = ucStateSize;
	mat->m_drawMod = gs->m_specials->m_drawModifier;

	// We put the CxState at the beginning of m_cxRegs, so it's easy to access. It will be initialized later.
	CxState* cxState = (CxState*)mat->m_cxRegs;

	// Copy the shader's state. All indirect state can be safely memcpy'd. There are no internal pointers.
	memcpy(mat->m_cxRegs + customCxSize, gs->m_cxRegisters, gs->m_numCxRegisters * sizeof(sce::Agc::CxRegister));
	memcpy(mat->m_cxRegs + customCxSize + gs->m_numCxRegisters, ps->m_cxRegisters, ps->m_numCxRegisters * sizeof(sce::Agc::CxRegister));

	memcpy(mat->m_shRegs, gs->m_shRegisters, gs->m_numShRegisters * sizeof(sce::Agc::ShRegister));
	memcpy(mat->m_shRegs + gs->m_numShRegisters, ps->m_shRegisters, ps->m_numShRegisters * sizeof(sce::Agc::ShRegister));

	sce::Agc::UcPrimitiveState* ucLinkage = (sce::Agc::UcPrimitiveState*)mat->m_ucRegs;
	SceError error = sce::Agc::Core::linkShaders(&cxState->linkage, ucLinkage, nullptr, gs, ps, sce::Agc::UcPrimitiveType::Type::kTriList);
	SCE_AGC_ASSERT_MSG(error == SCE_OK, "Failed to link shaders.");

	// Initialize the render target mask, which defines which RenderTargets we want to write to.
	// For every slot (MRT0-7), there is a 4 bit mask of channels. Since we want to write to all channels of MRT0,
	// we set the mask to 0xf.
	cxState->rt_mask
		.init()
		.setMask(0, 0xf);

	// Set up a viewport using a helper function from Agc::Core.
	sce::Agc::Core::setViewport(&cxState->vport, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, -1.0f, 1.0f);

	// Set up the RenderTarget spec. This describes the kind of RenderTarget we want in a form that is easier to manage than
	// the raw CxRenderTarget, which is the actual GPU state representing a RenderTarget.
	sce::Agc::Core::RenderTargetSpec rtSpec;
	rtSpec.init();
	rtSpec.m_width = SCREEN_WIDTH;
	rtSpec.m_height = SCREEN_HEIGHT;
	// The generic format type in Agc is Core::DataFormat. It is a struct that combines an actual encoding with a channel
	// swizzle. For convenience, it is usually just constructed using aggregate initialization like this.
	rtSpec.m_format = { sce::Agc::Core::TypedFormat::k8_8_8_8Srgb, sce::Agc::Core::Swizzle::kRGBA_R4S4 };
	// The default tile mode for RenderTargets is kRenderTarget, which is also supported by scan-out.
	rtSpec.m_tileMode = sce::Agc::CxRenderTarget::TileMode::kRenderTarget;

	// Once we have the basic spec set up, we can compute the size of the memory backing, allowing us to allocate memory.
	sce::Agc::SizeAlign rtSize = sce::Agc::Core::getSize(&rtSpec);

	// getSize calls always return a SizeAlign type that combines both the size and the minimum alignment.
	// Note that we're adding the pointer to the RenderTargetSpec here, which is different from how this worked on PlayStation®4.
	// The reason for this change is that it allows us to validate the whole RenderTarget when we initialize it.
	rtSpec.m_dataAddress = allocDmem(rtSize);

	// We can now initialize the render target, which will check that the dataAddress is properly aligned.
	// This process of defining a spec, getting the required sizes, adding the pointers to the spec, and finally
	// calling initialize() is shared with DepthRenderTargets, Buffers, and Textures.
	error = sce::Agc::Core::initialize(&cxState->rt, &rtSpec);
	SCE_AGC_ASSERT_MSG(error == SCE_OK, "Failed to initialize RenderTarget.");
	sce::Agc::Core::registerResource(&cxState->rt, "Color %d", 0);

	// This is just to make sure we don't scan out uninitialized memory on the first frame.
	memset((void*)rtSpec.m_dataAddress, 0x80, rtSize.m_size);

	// Now that we have the first material set up, we can create the others. They are identical to the first material, except for the RT memory.
	for (uint32_t i = 1; i < count; ++i)
	{
		mat[i].m_cxRegs = (sce::Agc::CxRegister*)materialMemory + materialSize + (i - 1) * cxStateSize;
		mat[i].m_shRegs = mat[0].m_shRegs;
		mat[i].m_ucRegs = mat[0].m_ucRegs;
		mat[i].m_numCxRegs = mat[0].m_numCxRegs;
		mat[i].m_numShRegs = mat[0].m_numShRegs;
		mat[i].m_numUcRegs = mat[0].m_numUcRegs;
		mat[i].m_drawMod = mat[0].m_drawMod;

		memcpy(mat[i].m_cxRegs, mat[0].m_cxRegs, mat[0].m_numCxRegs * sizeof(sce::Agc::CxRegister));

		CxState* cxStateCopy = (CxState*)mat[i].m_cxRegs;

		// At this point, we could update the spec and call initialize() again, which is probably the right thing to do,
		// even if it has extra CPU cost. To demonstrate directly manipulating the CxRenderTarget, we're actually just using
		// its accessors to set the DataAddress. Note that the setter takes a pointer, while the CxRenderTarget cannot actually
		// store arbitrary pointers and requires an alignment of at least 256. The setter takes care of this and asserts if the
		// pointer is invalid, but it will not verify that it fulfills the alignment restrictions of the tilemode, the way
		// initialize would do.
		cxStateCopy->rt.setDataAddress(allocDmem(rtSize));
		sce::Agc::Core::registerResource(&cxStateCopy->rt, "Color %d", i);

		// All register structs have getters as well as setters, which again return addresses as full pointers.
		memset(cxStateCopy->rt.getDataAddress(), 0x80, rtSize.m_size);
	}
	return true;
}


int createScanoutBuffers(const Material* materials, uint32_t count)
{
	// First we need to select what we want to display on, which in this case is the TV, also known as SCE_VIDEO_OUT_BUS_TYPE_MAIN.
	int videoHandle = sceVideoOutOpen(SCE_USER_SERVICE_USER_ID_SYSTEM, SCE_VIDEO_OUT_BUS_TYPE_MAIN, 0, NULL);
	SCE_AGC_ASSERT_MSG(videoHandle >= 0, "sceVideoOutOpen() returns handle=%d\n", videoHandle);

	// Next we need to inform scan-out about the format of our buffers. This can be done by directly talking to VideoOut or
	// by letting Agc::Core do the translation. To do so, we first need to get a RenderTargetSpec, which we can extract from
	// the list of CxRenderTargets passed into the function.
	sce::Agc::Core::RenderTargetSpec spec;

	// To make finding the CxRenderTarget easy, we put the CxState at the beginning of the m_cxRegs array, so we grab the first one.
	const CxState* cxState = (const CxState*)materials[0].m_cxRegs;
	SceError error = sce::Agc::Core::translate(&spec, &cxState->rt);
	SCE_AGC_ASSERT(error == SCE_OK);

	// Next, we use this RenderTargetSpec to create a SceVideoOutBufferAttribute2 which tells VideoOut how it should interpret
	// our buffers. VideoOut needs to know how the color data in the target should be interpreted, and since out pixel shader has
	// been writing linear values into an sRGB RenderTarget, the data VideoOut will find in memory are sRGB encoded.
	SceVideoOutBufferAttribute2 attribute;
	error = sce::Agc::Core::translate(&attribute, &spec, sce::Agc::Core::Colorimetry::kSrgb, sce::Agc::Core::Colorimetry::kBt709);
	SCE_AGC_ASSERT(error == SCE_OK);

	// Ideally, all buffers should be registered with VideoOut in a single call to sceVideoOutRegisterBuffers2.
	// The reason for this is that the buffers provided in each call get associated with one attribute slot in the API.
	// Even if consecutive calls pass the same SceVideoOutBufferAttribute2 into the function, they still get assigned
	// new attribute slots. When processing a flip, there is significant extra cost associated with switching attribute
	// slots, which should be avoided.
	SceVideoOutBuffers* addresses = (SceVideoOutBuffers*)calloc(count, sizeof(SceVideoOutBuffers));
	for (uint32_t i = 0; i < count; ++i)
	{
		cxState = (const CxState*)materials[i].m_cxRegs;
		// We could manually call into VideoOut to set up the scan-out buffers, but Agc::Core provides a helper for this.
		addresses[i].data = cxState->rt.getDataAddress();
	}

	// VideoOut internally groups scan-out buffers in sets. Every buffer in a set has the same attributes and switching (flipping) between
	// buffers of the same set is a light-weight operation. Switching to a buffer from a different set is significantly more expensive
	// and should be avoided. If an application wants to change the attributes of a scan-out buffer or wants to unregister buffers,
	// these operations are done on whole sets and affect every buffer in the set. This sample only registers one set of buffers and never
	// modifies the set.
	const int32_t setIndex = 0; // Call sceVideoOutUnregisterBuffers with this.
	error = sceVideoOutRegisterBuffers2(videoHandle, setIndex, 0, addresses, count, &attribute, SCE_VIDEO_OUT_BUFFER_ATTRIBUTE_CATEGORY_UNCOMPRESSED, nullptr);
	SCE_AGC_ASSERT(error == SCE_OK);
	free(addresses);

	return videoHandle;
}



namespace xs::render::internal
{
	struct image
	{
		//GLuint	gl_id = 0;
		int		width = -1;
		int		height = -1;
		int		channels = -1;
	};

	int width = -1;
	int height = -1;
}

using namespace xs::render::internal;

void xs::render::initialize()
{
	width = configuration::width;
	height = configuration::height;

	// This function always needs to be called before any other Agc call.
	SceError error = sce::Agc::init();
	SCE_AGC_ASSERT(error == SCE_OK);

	size_t resourceRegistrationBufferSize;
	error = sce::Agc::ResourceRegistration::queryMemoryRequirements(&resourceRegistrationBufferSize, 128, 64);

	if (error != SCE_AGC_ERROR_RESOURCE_REGISTRATION_NO_PA_DEBUG)
	{
		SCE_AGC_ASSERT(error == SCE_OK);
		uint8_t* mem = allocDmem({ resourceRegistrationBufferSize, sce::Agc::Alignment::kResourceRegistration });
		error = sce::Agc::ResourceRegistration::init( mem, resourceRegistrationBufferSize, 64);
		SCE_AGC_ASSERT(error == SCE_OK);
		error = sce::Agc::ResourceRegistration::registerDefaultOwner(nullptr);
		SCE_AGC_ASSERT(error == SCE_OK);
	}

	// The actual DCB we're creating is quite small, but we're allocating 1MB in case someone wants to 
	// modify the loop and not worry about running out of memory.
	const uint32_t dcb_size = 1024 * 1024;

	// Since we want to multi-buffer our rendering, we allocate a DCB for each buffer.
	sce::Agc::DrawCommandBuffer dcbs[BUFFERING];

	// The flip labels are used to track if a command buffer has been fully consumed by the GPU
	// and can be re-used.
	sce::Agc::Label* flipLabels = (sce::Agc::Label*)allocDmem({ sizeof(sce::Agc::Label) * BUFFERING, sizeof(uint64_t) });

	// Each buffer gets its own set of state. We could easily reuse the state, but this
	// makes is a bit easier to modify later in this sample.
	Material mats[BUFFERING];

	// First, we create the draw state, which includes allocating RenderTargets.
	createMaterials(mats, BUFFERING);

	// The RenderTargets needs to be registered with the VideoOut library.
	int videoHandle = createScanoutBuffers(mats, BUFFERING);

	// Initialize the DCBs.
	for (uint32_t i = 0; i < BUFFERING; ++i)
	{
		dcbs[i].init(
			allocDmem({ dcb_size, sce::Agc::Alignment::kCommandBuffer }),
			dcb_size,
			nullptr, // This would be the out-of-memory callback.
			nullptr); // This would be the user defined payload for the out-of-memory callback.
		flipLabels[i].m_value = 1; // 1 means "not used by GPU"

		sce::Agc::Core::registerResource(&dcbs[i], "DCB %d", i);
		sce::Agc::Core::registerResource(&flipLabels[i], "Flip label %d", i);
	}

	// These are pointers to timestamps. The timestamps themselves are stored in the DCB's storage, 
	// as shown later.
	volatile uint64_t* timestamps[2 * BUFFERING] = { 0 };


	// This is our frame loop.
	for (uint32_t i = 0; ; ++i)
	{
		// First we identify the back buffer.
		const uint32_t buffer = i % BUFFERING;

		// Check if the command buffer has been fully processed, if so it's safe for us to overwrite it on the CPU.
		while (flipLabels[buffer].m_value != 1)
		{
			sceKernelUsleep(1000);
		}

		// Grab the DCB of the backbuffer
		sce::Agc::DrawCommandBuffer& dcb = dcbs[buffer];

		// If we actually have timestamps for this buffer, read them out.
		if (timestamps[2 * buffer] && ((i % 128) == 0))
		{
			uint64_t refclk = *timestamps[2 * buffer + 1] - *timestamps[2 * buffer];
			double ms = refclk / (100000.0); // REFCLK is 100MHz and we want millisceonds.
			// This is our very low-tech frame time indicator.
			printf("Duration = %.3fms.\n", ms);
		}

		// We can now set the flip label to 0, which the GPU will set back to 1 when it's done.
		flipLabels[buffer].m_value = 0;

		// Before writing to it, we empty the DCB.
		// This is actually quite wasteful, since we could reuse the previous buffers.
		dcb.resetBuffer();

		// First thing we do is add a timestamp using a helper from Agc::Core.
		// This allocates storage for the timestamp from the end of the allocator passed in as its
		// second parameter, which in this case is just the DCB.
		timestamps[2 * buffer] = sce::Agc::Core::writeTimestamp(&dcb, &dcb, sce::Agc::Core::TimestampType::kBottomOfPipe);

		// This will stall the Command Processor (CP) until the buffer is no longer being displayed.
		dcb.waitUntilSafeForRendering(videoHandle, buffer);

		// We now load the material state we have prepared. Although it is quite
		// sizeable, between two Shaders, a RenderTarget, and assorted state, we
		// can do this using only three calls.
		dcb.setCxRegistersIndirect(mats[buffer].m_cxRegs, mats[buffer].m_numCxRegs);
		dcb.setShRegistersIndirect(mats[buffer].m_shRegs, mats[buffer].m_numShRegs);
		dcb.setUcRegistersIndirect(mats[buffer].m_ucRegs, mats[buffer].m_numUcRegs);

		// To keep this sample minimal, communication to the geometry and pixel shaders
		// is done just through user data, which is a relatively small amount of hardware
		// registers the shader can read directly. In this sample, we're using only one
		// of these to pass a single 32b value to the shaders.
		sce::Agc::ShUserDataGs frame_reg;

		frame_reg
			.init() // Like all registers, the ShUserDataGs needs to first have init() called on it.
			.setData(0xffffffff); // When the geometry shader sees 0xffffffff it performs a clear.

		// Sh state can be set directly, which means it's copied into the DCB.
		// This really only makes sense when performing small Sh state updates, but then it
		// can be quite fast and convenient.
		dcb.setShRegisterDirect(frame_reg.m_regs[0]);

		// The first draw is our clear. There is no index buffer, because the geometry shader
		// will use the vertex ID to compute the vertex positions.
		dcb.drawIndexAuto(3, mats[buffer].m_drawMod);

		// Next we set our user data to the frame number. This means that every 2^32 frames,
		// we will get an empty frame since the user data will be 0xffffffff in this draw as well.
		frame_reg.setData(i); // Pass in frame number to drive animation.
		dcb.setShRegisterDirect(frame_reg.m_regs[0]);

		// The draw of our actual triangle.
		dcb.drawIndexAuto(3, mats[buffer].m_drawMod);

		// Once the triangle is written (i.e. once the draw has reached the "bottom of the pipe", we take 
		// another timestamp.
		timestamps[2 * buffer + 1] = sce::Agc::Core::writeTimestamp(&dcb, &dcb, sce::Agc::Core::TimestampType::kBottomOfPipe);

		// Submit a flip via the GPU.
		// Note: on PlayStation®5, RenderTargets write into the GL2 cache, but the scan-out
		// does not snoop any GPU caches. As such, it is necessary to flush these writes to memory before they can
		// be displayed. This flush is performed internally by setFlip() so we don't need to do it 
		// on the application side.
		dcb.setFlip(videoHandle, buffer, SCE_VIDEO_OUT_FLIP_MODE_VSYNC, 0);

		// The last thing we do in the command buffer is write 1 to the flip label to signal that command buffer 
		// processing has finished. We also perform a cache flush here, since our timestamps are in the GL2 cache 
		// and we want to see those from the CPU.
		//
		// While Agc provides access to the lowest level of GPU synchronization faculties, it also provides
		// functionality that builds the correct synchronization steps in an easier fashion.
		// Since synchonization should be relatively rare, spending a few CPU cycles on letting the library
		// work out what needs to be done is generally a good idea.
		sce::Agc::Core::gpuSyncEvent(
			&dcb,
			// The SyncWaitMode controls how the GPU's Command Processor (CP) handles the synchronization.
			// By setting this to kAsynchronous, we tell the CP that it doesn't have to wait for this operation
			// to finish before it can start the next frame. Instead, we could ask it to drain all graphics work
			// first, but that would be more aggressive than we need to be here.
			sce::Agc::Core::SyncWaitMode::kAsynchronous,
			// Request dirty cache lines in GL2 to be written to memory. 
			sce::Agc::Core::SyncCacheOp::kGl2Writeback,
			// Write the flip label and make it visible to the CPU.
			sce::Agc::Core::SyncLabelVisibility::kCpu,
			&flipLabels[buffer],
			// We write the value "1" to the flip label.
			1);

		// Finally, we submit the work to the GPU. Since this is the only work on the GPU, we set its priority to normal.
		// The only reason to set the priority to kInterruptPriority is to make a submit expel work from the GPU we have perviously
		// submitted. 
		error = sce::Agc::submitGraphics(
			sce::Agc::GraphicsQueue::kNormal,
			dcb.getSubmitPointer(),
			dcb.getSubmitSize());
		SCE_AGC_ASSERT_MSG(error == SCE_OK, "Submit failed for frame %d", i);

		// If the application is suspended, it will happen during this call. As a side-effect, this is equivalent to
		// calling resetQueue(ResetQueueOp::kAllAccessible).
		error = sce::Agc::suspendPoint();
		SCE_AGC_ASSERT(error == SCE_OK);
	}

}

void xs::render::shutdown()
{
}

void xs::render::render()
{
}

void xs::render::clear()
{
}

void xs::render::set_offset(double x, double y) {}

int xs::render::load_image(const std::string& image_file)
{
	return -1;
}


void xs::render::begin(primitive p)
{
}

void xs::render::vertex(double x, double y)
{
}

void xs::render::end()
{
}

void xs::render::set_color(double r, double g, double b, double a)
{
}

void xs::render::set_color(color c)
{
}

void xs::render::line(double x0, double y0, double x1, double y1)
{
}

void xs::render::text(const std::string& text, double x, double y, double size)
{
}

int xs::render::create_sprite(int image_id, double x0, double y0, double x1, double y1)
{
	return -1;
}

void xs::render::render_sprite(
	int image_id,
	double x,
	double y,
	double rotation,
	double size,
	xs::render::color mutiply,
	xs::render::color add,
	unsigned int flags)
{}



void xs::render::render_sprite(int image_id, double x, double y, sprite_anchor anchor)
{}


