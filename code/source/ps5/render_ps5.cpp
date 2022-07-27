#include <render.h>
#include <tools.h>
#include "../render_internal.h"
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
#include <vectormath.h>
#include <agc/toolkit/toolkit.h>

#include <libsysmodule.h>
#include <png_dec.h>

#include "Camera.h"
#include "Instance.h"

const uint32_t BUFFERING = 2;
const uint32_t SCREEN_WIDTH = 1920;
const uint32_t SCREEN_HEIGHT = 1080;

using namespace glm;

namespace xs::render::internal
{
	struct image
	{
		sce::Agc::Core::Texture texture;
		int		width = -1;
		int		height = -1;
		int		channels = -1;
		std::size_t	string_id = 0;
	};

	struct Vert
	{
		float x, y;
		float s, t;
		float r, g, b;		
	};

	int width = -1;
	int height = -1;

	int videoHandle = -1;
	sce::Agc::Label* flipLabels = nullptr;
	sce::Agc::CxRenderTarget rts[BUFFERING];
	sce::Agc::Core::BasicContext ctxs[BUFFERING];	// There are other context types
	sce::Agc::CxRenderTargetMask rtMask;
	sce::Agc::CxViewport vport;
	//sce::Agc::Core::Texture texture;
	sce::Agc::Core::Sampler sampler;
	// sce::Agc::Core::Buffer vertBuffer;
	sce::Agc::Core::Encoder::EncoderValue clearColor;
	sce::Agc::Shader* gs;	// TODO: Why pointer?
	sce::Agc::Shader* ps;	// TODO: Why pointer?
	int frame = 0;
	std::vector<image>		images = {};
	vec2 offset = vec2(0.0f, 0.0f);

	uint8_t* alloc_direct_mem(sce::Agc::SizeAlign sizeAlign);
	int create_scanout_buffers(const sce::Agc::CxRenderTarget* rts, uint32_t count);	
}

using namespace xs::render::internal;

// These symbols point to the headers and code of the shader binaries linked into the sample's elf.
// They are declared inside the shader code. For example, the Shader::ps_header and Shader::ps_text symbols
// were declared in the shader by putting the attribute [CxxSymbol("Shader::ps")] in front of the pixel 
// shader's entry point.
namespace Shader
{
	extern char  ps_header[];
	extern const char  ps_text[];
	extern char  gs_header[];
	extern const char  gs_text[];
}

// This is a temporary utility function to allocate direct memory. It's not important to understand how this works.
uint8_t* xs::render::internal::alloc_direct_mem(sce::Agc::SizeAlign sizeAlign)
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
		xs::log::error("sceKernelAllocateMainDirectMemory error:0x%x size:0x%zx\n", ret, size);
		return nullptr;
	}

	void* ptr = NULL;
	char namedStr[32];
	snprintf_s(namedStr, sizeof(namedStr), "agc_basic_context %d_%zuKB", allocCount++, size >> 10);
	ret = sceKernelMapNamedDirectMemory(&ptr, size, SCE_KERNEL_PROT_GPU_RW | SCE_KERNEL_PROT_CPU_RW, 0, offsetOut, alignment, namedStr);
	SCE_AGC_ASSERT_MSG(ret == SCE_OK, "Unable to map memory");
	return (uint8_t*)ptr;
}

int xs::render::internal::create_scanout_buffers(const sce::Agc::CxRenderTarget* rts, uint32_t count)
{
	// First we need to select what we want to display on, which in this case is the TV, also known as SCE_VIDEO_OUT_BUS_TYPE_MAIN.
	int videoHandle = sceVideoOutOpen(SCE_USER_SERVICE_USER_ID_SYSTEM, SCE_VIDEO_OUT_BUS_TYPE_MAIN, 0, NULL);
	SCE_AGC_ASSERT_MSG(videoHandle >= 0, "sceVideoOutOpen() returns handle=%d\n", videoHandle);

	// Next we need to inform scan-out about the format of our buffers. This can be done by directly talking to VideoOut or
	// by letting Agc::Core do the translation. To do so, we first need to get a RenderTargetSpec, which we can extract from
	// the list of CxRenderTargets passed into the function.
	sce::Agc::Core::RenderTargetSpec spec;
	SceError error = sce::Agc::Core::translate(&spec, &rts[0]);
	SCE_AGC_ASSERT(error == SCE_OK);

	// Next, we use this RenderTargetSpec to create a SceVideoOutBufferAttribute2 which tells VideoOut how it should interpret
	// our buffers. VideoOut needs to know how the color data in the target should be interpreted, and since our pixel shader has
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
		// We could manually call into VideoOut to set up the scan-out buffers, but Agc::Core provides a helper for this.
		addresses[i].data = rts[i].getDataAddress();
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

void CreateRenderTargets(sce::Agc::CxRenderTarget* rts, sce::Agc::Core::RenderTargetSpec* spec, uint32_t count)
{
	// First, retrieve the size of the render target. We can of course do this before we have any pointers.
	sce::Agc::SizeAlign rtSize = sce::Agc::Core::getSize(spec);
	// Then we can allocate the required memory backing and assign it to the spec.
	spec->m_dataAddress = alloc_direct_mem(rtSize);
	memset((void*)spec->m_dataAddress, 0x80, rtSize.m_size);

	// We can now initialize the render target. This will check that the dataAddress is properly aligned.
	SceError error = sce::Agc::Core::initialize(&rts[0], spec);
	SCE_AGC_ASSERT_MSG(error == SCE_OK, "Failed to initialize RenderTarget.");
	sce::Agc::Core::registerResource(&rts[0], "Color %d", 0);		// For debugging

	// Now that we have the first RT set up, we can create the others. They are identical to the first material, except for the RT memory.
	for (uint32_t i = 1; i < count; ++i)
	{
		// You can just memcpy the CxRenderTarget, but doing so of course sidesteps the alignment checks in initialize().
		memcpy(&rts[i], &rts[0], sizeof(rts[0]));
		rts[i].setDataAddress(alloc_direct_mem(rtSize));
		memset(rts[i].getDataAddress(), 0x80, rtSize.m_size);
		sce::Agc::Core::registerResource(&rts[i], "Color %d", i);	// For debugging
	}
}

void printImageInfo(ScePngDecImageInfo& imageInfo)
{
	const char* csText;
	switch (imageInfo.colorSpace) {
	case SCE_PNG_DEC_COLOR_SPACE_GRAYSCALE:
		csText = "Grayscale";
		break;
	case SCE_PNG_DEC_COLOR_SPACE_RGB:
		csText = "RGB";
		break;
	case SCE_PNG_DEC_COLOR_SPACE_CLUT:
		csText = "CLUT";
		break;
	case SCE_PNG_DEC_COLOR_SPACE_GRAYSCALE_ALPHA:
		csText = "GrayscaleAlpha";
		break;
	case SCE_PNG_DEC_COLOR_SPACE_RGBA:
		csText = "RGBA";
		break;
	default:
		csText = "unknown";
	}

	printf("==== PNG image info. ====\n");
	printf("imageSize = %u x %u\n", imageInfo.imageWidth, imageInfo.imageHeight);
	printf("colorSpace = %s\n", csText);
	printf("bitDepth = %u\n", imageInfo.bitDepth);
	printf("imageFlag = 0x%x\n", imageInfo.imageFlag);
}

int LoadPNGTexture(const char* inFileName, sce::Agc::Core::Texture& outTexture)
{
	ScePngDecParseParam		parseParam;
	ScePngDecImageInfo		imageInfo;
	ScePngDecCreateParam	createParam;
	ScePngDecHandle			handle;
	ScePngDecDecodeParam	decodeParam;
	int32_t ret = 0;

	// read PNG image from file
	std::FILE* fp;
	long fileSize;

	fp = std::fopen(inFileName, "rb");
	if (fp == NULL) {
		return -1;
	}
	std::fseek(fp, 0L, SEEK_END);
	fileSize = std::ftell(fp);
	if (fileSize < 1) {
		std::fclose(fp);
		return -1;
	}
	std::fseek(fp, 0L, SEEK_SET);

	void* texelMemAddr = alloc_direct_mem({ (size_t)fileSize, sce::Agc::Alignment::kBuffer });
	if (nullptr == texelMemAddr) {
		fclose(fp);
		return 1;
	}
	if (std::fread(texelMemAddr, 1, fileSize, fp) != fileSize) {
		// Deallocate memory #todo: Check how to dealloc this as off_t of direct memory is discarded in AllocDMem function
		sceKernelReleaseFlexibleMemory(texelMemAddr, fileSize);
		std::fclose(fp);
		return -1;
	}
	std::fclose(fp);

	// get image info.
	parseParam.pngMemAddr = texelMemAddr;
	parseParam.pngMemSize = fileSize;
	parseParam.reserved0 = 0;
	ret = scePngDecParseHeader(&parseParam, &imageInfo);
	if (ret < 0) {
		printf("Error: scePngDecParseHeader(), ret 0x%08x\n", ret);
		return ret;
	}
	printImageInfo(imageInfo);
	// allocate memory for output image
	sce::Agc::Core::TextureSpec textureSpec;
	textureSpec.init();
	textureSpec.m_type = sce::Agc::Core::Texture::Type::k2d;
	textureSpec.m_width = imageInfo.imageWidth;
	textureSpec.m_height = imageInfo.imageHeight;
	textureSpec.m_format = sce::Agc::Core::DataFormat({ sce::Agc::Core::TypedFormat::k8_8_8_8UNorm, sce::Agc::Core::Swizzle::kRGBA_R4S4 });

	const sce::Agc::SizeAlign textureSizeAlign = sce::Agc::Core::getSize(&textureSpec);
	textureSpec.m_dataAddress = alloc_direct_mem(textureSizeAlign);
	if (textureSpec.m_dataAddress == nullptr)
	{
		ret = 1; // Alloc Dmem failed to allocate memory
		printf("Error: Allocate GPU Memory, ret 0x%08x\n", ret);
		return ret;
	}

	ret = sce::Agc::Core::initialize(&outTexture, &textureSpec);
	if (ret < 0) {
		printf("Error: Agc::Core::initialize( sce::Agc::Core::Texture*, sce::Agc::Core::TextureSpec ), ret 0x%08x\n", ret);
		return ret;
	}

	// query memory size for PNG decoder
	createParam.thisSize = sizeof(createParam);
	createParam.attribute = imageInfo.bitDepth >> 4;
	createParam.maxImageWidth = imageInfo.imageWidth;
	size_t decoderSize = scePngDecQueryMemorySize(&createParam);
	if (decoderSize < 0) {
		printf("Error: scePngDecQueryMemorySize(), ret %lu\n", decoderSize);
		return ret;
	}
	// allocate memory for PNG decoder
	void* decoderMemory = alloc_direct_mem({ decoderSize, 0 });
	if (decoderMemory == nullptr) {
		printf("Error: decoderMemory.allocate(), ret 0x%08x\n", ret);
		return ret;
	}
	// create PNG decoder
	ret = scePngDecCreate(&createParam, decoderMemory, decoderSize, &handle);
	if (ret < 0) {
		printf("Error: scePngDecCreate(), ret 0x%08x\n", ret);
		return ret;
	}

	// decode PNG image
	size_t pngDataSize = outTexture.getWidth() * outTexture.getHeight() * 4;
	void* pngDataBuff = alloc_direct_mem({ pngDataSize , 0 });
	if (ret < 0) {
		printf("Error: imageMemory.allocate(), ret 0x%08x\n", ret);
		scePngDecDelete(handle);
		return ret;
	}
	decodeParam.pngMemAddr = texelMemAddr;
	decodeParam.pngMemSize = fileSize;
	decodeParam.imageMemAddr = pngDataBuff;
	decodeParam.imageMemSize = pngDataSize;
	decodeParam.imagePitch = outTexture.getWidth() * 4;
	decodeParam.pixelFormat = SCE_PNG_DEC_PIXEL_FORMAT_R8G8B8A8;
	decodeParam.alphaValue = 255;
	ret = scePngDecDecode(handle, &decodeParam, NULL);
	if (ret < 0) {
		printf("Error: scePngDecDecode(), ret 0x%08x\n", ret);
		scePngDecDelete(handle);
		return ret;
	}
	// delete PNG decoder
	ret = scePngDecDelete(handle);
	if (ret < 0) {
		printf("Error: scePngDecDelete(), ret 0x%08x\n", ret);
		return ret;
	}

	sce::AgcGpuAddress::SurfaceSummary surfaceSummary;
	ret = sce::Agc::Core::translate(&surfaceSummary, &textureSpec);
	if (ret < 0) {
		printf("Error: sce::Agc::Core::translate(), ret 0x%08x\n", ret);
		return ret;
	}
	ret = sce::AgcGpuAddress::tileSurface(outTexture.getDataAddress(), textureSizeAlign.m_size,
		pngDataBuff, pngDataSize, &surfaceSummary, 0, 0);
	if (ret < 0) {
		printf("Error: sce::AgcGpuAddress::tileSurface(), ret 0x%08x\n", ret);
		return ret;
	}
	return 0;
}


void xs::render::initialize()
{
	// This function always needs to be called before any other Agc call.
	SceError error = sce::Agc::init();
	SCE_AGC_ASSERT(error == SCE_OK);

	error = sce::Agc::Toolkit::init();
	SCE_AGC_ASSERT(error == SCE_OK);	

	// Load png decoder module
	error = sceSysmoduleLoadModule(SCE_SYSMODULE_PNG_DEC);
	SCE_AGC_ASSERT(error == SCE_OK);

	// Be able to see the names for AGC resources in Razor GPU
	size_t resourceRegistrationBufferSize;
	error = sce::Agc::ResourceRegistration::queryMemoryRequirements(&resourceRegistrationBufferSize, 128, 64);
	if (error != SCE_AGC_ERROR_RESOURCE_REGISTRATION_NO_PA_DEBUG)
	{
		SCE_AGC_ASSERT(error == SCE_OK);
		error = sce::Agc::ResourceRegistration::init(alloc_direct_mem({ resourceRegistrationBufferSize, sce::Agc::Alignment::kResourceRegistration }), resourceRegistrationBufferSize, 64);
		SCE_AGC_ASSERT(error == SCE_OK);
		error = sce::Agc::ResourceRegistration::registerDefaultOwner(nullptr);
		SCE_AGC_ASSERT(error == SCE_OK);
	}

	// Everything with Cx is Context State

	// Set up the RenderTarget spec.
	sce::Agc::Core::RenderTargetSpec rtSpec;
	rtSpec.init();
	rtSpec.m_width = SCREEN_WIDTH;
	rtSpec.m_height = SCREEN_HEIGHT;
	rtSpec.m_format = { sce::Agc::Core::TypedFormat::k8_8_8_8Srgb, sce::Agc::Core::Swizzle::kRGBA_R4S4 };
	rtSpec.m_tileMode = sce::Agc::CxRenderTarget::TileMode::kRenderTarget; // Read docs on this

	// Precompute the color value we use to clear the render target
	clearColor = sce::Agc::Core::Encoder::encode(rtSpec.getFormat(), { 0 });


	// Now we create a number of render targets from this spec. These are our scanout buffers.	
	CreateRenderTargets(rts, &rtSpec, BUFFERING);

	// These labels are currently unused, but the intent is to use them for flip tracking.
	flipLabels = (sce::Agc::Label*)alloc_direct_mem({ sizeof(sce::Agc::Label) * BUFFERING, sce::Agc::Alignment::kLabel });

	// We need the videoout handle to flip.
	videoHandle = create_scanout_buffers(rts, BUFFERING);

	// Create a context for each buffered frame.
	const uint32_t dcb_size = 1024 * 1024 * 8; // 8 MB

	// Set up to contexts, one for each target
	for (uint32_t i = 0; i < BUFFERING; ++i)
	{
		// Contexts are manually initialized. The reason is that we have no fixed requirements
		// for how the components have to be hooked up, so there is considerable freedom for the 
		// developer here.
		// In this case, we simply make all components use the DCB for their storage.

		ctxs[i].m_dcb.init(
			alloc_direct_mem({ dcb_size, 4 }),
			dcb_size,
			nullptr,		// Mem alloc function ptr
			nullptr);
		ctxs[i].m_bdr.init(
			&ctxs[i].m_dcb,
			&ctxs[i].m_dcb);
		ctxs[i].m_sb.init(
			256, // This is the size of a chunk in the StateBuffer, defining the largest size of a load packet's payload.
			&ctxs[i].m_dcb,
			&ctxs[i].m_dcb);
		flipLabels[i].m_value = 1; // 1 means "not used by GPU"
		sce::Agc::Core::registerResource(&ctxs[i].m_dcb, "Context %d", i);
		sce::Agc::Core::registerResource(&flipLabels[i], "Flip label %d", i);
	}


	// Initialize some state. We don't have to do this every loop through the frame, so we do it here.
	// Which render target you want to use and which channels
	rtMask = sce::Agc::CxRenderTargetMask().init().setMask(0, 0xf);

	// Set up a viewport using a helper function from Core.	
	sce::Agc::Core::setViewport(&vport, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, -1.0f, 1.0f);

	// We will also pass the frame number into the GS to drive animation.
	sce::Agc::ShUserDataGs frame_reg;
	frame_reg.init();

	// First, we load the shaders, since the size of the shader's register blocks is not known.	
	error = sce::Agc::createShader(&gs, Shader::gs_header, Shader::gs_text);
	SCE_AGC_ASSERT(error == SCE_OK);
	sce::Agc::Core::registerResource(gs, "Shader::gs");
	error = sce::Agc::createShader(&ps, Shader::ps_header, Shader::ps_text);
	SCE_AGC_ASSERT(error == SCE_OK);
	sce::Agc::Core::registerResource(ps, "Shader::ps");
}

void xs::render::shutdown()
{
	//TODO: Free allocated memory!
	// sceKernelReleaseDirectMemory(void*,size) for alloc_direct_mem allocations
}

void xs::render::render()
{
	SceError error;

	// First we identify the back buffer.
	const uint32_t buffer = frame++ % BUFFERING;

	// Check if the command buffer has been fully processed, if so it's safe for us to overwrite it on the CPU.
	while (flipLabels[buffer].m_value != 1)
	{
		sceKernelUsleep(1000);
	}

	// We can now set the flip label to 0, which the GPU will set back to 1 when it's done.
	flipLabels[buffer].m_value = 0;

	sce::Agc::Core::BasicContext& ctx = ctxs[buffer];
	// First we reset the context, since we're writing a completely new DCB.
	// This is actually quite wasteful, since we could reuse the previous data, but the
	// point of this code is to demonstrate a Gnm-like approach to writing DCBs.
	ctx.reset();

	// This will stall the Command Processor (CP) until the buffer is no longer being displayed.
	// Note that we're actually pulling the DCB out of the context and accessing it
	// directly here. This is very much how Agc's contexts work. They do not hide away the underlying
	// components but mostly just try to remove redundant work.
	ctx.m_dcb.waitUntilSafeForRendering(videoHandle, buffer);


	// Clear both color and depth targets by just using toolkit functions.
	sce::Agc::Toolkit::Result tk0 = sce::Agc::Toolkit::clearRenderTargetCs(&ctx.m_dcb, &rts[buffer], clearColor);
	ctx.resetToolkitChangesAndSyncToGl2(tk0);

	// The contexts provide their own functions to set shaders, which are there to make sure all
	// components are properly made aware of shader changes.

	ctx.setShaders(nullptr, gs, ps, sce::Agc::UcPrimitiveType::Type::kTriStrip);

	// Setting state can be done in several ways, such as by directly interacting with the DCB.
	// For the most part, contexts are designed to use StateBuffers (SBs), which allow the user
	// to have their indirect state turn into something that behaves a lot like direct state.
	// The easiest way to pass state into the StateBuffer is with the setState template method. This method
	// will look for a static const RegisterType member called m_type to determine what type of register
	// is being set and automatically determines the size of the state from the type being passed in.
	ctx.m_sb.setState(rtMask);
	ctx.m_sb.setState(vport);
	ctx.m_sb.setState(rts[buffer]);

	sce::Agc::Core::VertexAttribute attributes[3] =
	{
		{
			0, // m_vbTableIndex
			sce::Agc::Core::VertexAttribute::Format::k32_32Float,
			0, // m_offset
			sce::Agc::Core::VertexAttribute::Index::kVertexId
		},		
		{
			0, // m_vbTableIndex
			sce::Agc::Core::VertexAttribute::Format::k32_32Float,
			sizeof(float) * 2, // m_offset
			sce::Agc::Core::VertexAttribute::Index::kVertexId
		},
		{
			0, // m_vbTableIndex
			sce::Agc::Core::VertexAttribute::Format::k32_32_32Float,
			sizeof(float) * 4, // m_offset
			sce::Agc::Core::VertexAttribute::Index::kVertexId
		},
	};

	sampler
		.init()
		.setXyFilterMode(sce::Agc::Core::Sampler::FilterMode::kPoint)
		.setWrapMode(sce::Agc::Core::Sampler::WrapMode::kClampLastTexel);

	std::vector<sce::Agc::Core::Buffer> vertBuffers;
	for (const auto& spe : sprite_queue)
	{
		const auto& sprite = sprites[spe.sprite_id];
		const auto& image = images[sprite.image_id];

		float from_x = 0.0f;
		float from_y = 0.0f;
		float to_x = image.width * (sprite.to.x - sprite.from.x) * spe.scale;
		float to_y = image.height * (sprite.to.y - sprite.from.y) * spe.scale;

		float from_u = sprite.from.x;
		float from_v = sprite.from.y;
		float to_u = sprite.to.x;
		float to_v = sprite.to.y;

		if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::flip_x))
			std::swap(from_u, to_u);

		if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::flip_y))
			std::swap(from_v, to_v);

		vec4 add_color = to_vec4(spe.add_color);
		vec4 mul_color = to_vec4(spe.mul_color);

		// Allocate on the dcb
		vertBuffers.push_back({});
		sce::Agc::Core::Buffer& vertBuffer = vertBuffers.back();
		Vert* verData = (Vert*)ctx.m_dcb.allocateTopDown({ sizeof(Vert) * 4, sce::Agc::Alignment::kBuffer });
		verData[0] = { from_x, from_y,	from_u, to_v, 	1.0f, 0.0f, 0.0f };
		verData[1] = { from_x, to_y,	from_u, from_v, 	0.0f, 1.0f, 0.0f };
		verData[2] = { to_x, from_y,	to_u, to_v, 	0.0f, 1.0f, 0.0f };
		verData[3] = { to_x, to_y,		to_u, from_v, 	0.0f, 0.0f, 1.0f };

		for (int i = 0; i < 4; i++)
		{
			verData[i].x += (float)spe.x;
			verData[i].y += (float)spe.y;
		}

		sce::Agc::Core::initializeRegularBuffer(&vertBuffer, verData, sizeof(Vert), 4);

		Camera* camera = (Camera*)ctx.m_dcb.allocateTopDown(sizeof(Camera), sce::Agc::Alignment::kBuffer);
		camera->x = 0.0f;
		camera->y = 0.0f;
		camera->res_x = 640.0f * 0.5f;
		camera->res_y = 360.0f * 0.5f;

		ctx.m_bdr.getStage(sce::Agc::ShaderType::kGs)
			.setVertexBuffers(0, 1, &vertBuffer)
			.setVertexAttributes(0, 3, attributes)
			.setUserSrtBuffer(&camera, sizeof(camera));

		ctx.m_bdr.getStage(sce::Agc::ShaderType::kPs)
			.setTextures(0, 1, &image.texture)
			.setSamplers(0, 1, &sampler);


		// In this example, we're actually drawing two triangles. The state only differs in what is in
		// frame_reg. Because we're not calling into the Binder or StateBuffer in between these draws, they will 
		// not write anything to the DCB and thus will incur no GPU cost.
		ctx.drawIndexAuto(4);
	}

	// Submit a flip via the GPU.
	// Note: on PlayStation®5, RenderTargets write into the GL2 cache, but the scan-out
	// does not snoop any GPU caches. As such, it is necessary to flush these writes to memory before they can
	// be displayed. This flush is performed internally by setFlip() so we don't need to do it 
	// on the application side.
	ctx.m_dcb.setFlip(videoHandle, buffer, SCE_VIDEO_OUT_FLIP_MODE_VSYNC, 0);

	// The last thing we do in the command buffer is write 1 to the flip label to signal that command buffer 
	// processing has finished. 
	//
	// While Agc provides access to the lowest level of GPU synchronization faculties, it also provides
	// functionality that builds the correct synchronization steps in an easier fashion.
	// Since synchonization should be relatively rare, spending a few CPU cycles on letting the library
	// work out what needs to be done is generally a good idea.
	sce::Agc::Core::gpuSyncEvent(
		&ctx.m_dcb,
		// The SyncWaitMode controls how the GPU's Command Processor (CP) handles the synchronization.
		// By setting this to kAsynchronous, we tell the CP that it doesn't have to wait for this operation
		// to finish before it can start the next frame. Instead, we could ask it to drain all graphics work
		// first, but that would be more aggressive than we need to be here.
		sce::Agc::Core::SyncWaitMode::kAsynchronous,
		// Since we are making the label write visible to the CPU, it is not necessary to flush any caches 
		// and we set the cache op to 'kNone'.
		sce::Agc::Core::SyncCacheOp::kNone,
		// Write the flip label and make it visible to the CPU.
		sce::Agc::Core::SyncLabelVisibility::kCpu,
		&flipLabels[buffer],
		// We write the value "1" to the flip label.
		1);

	// Finally, we submit the work to the GPU. Since this is the only work on the GPU, we set its priority to normal.
	// The only reason to set the priority to kInterruptPriority is to make a submit expel work from the GPU we have previously
	// submitted. 
	error = sce::Agc::submitGraphics(
		sce::Agc::GraphicsQueue::kNormal,
		ctx.m_dcb.getSubmitPointer(),
		ctx.m_dcb.getSubmitSize());
	SCE_AGC_ASSERT(error == SCE_OK);

	// If the application is suspended, it will happen during this call. As a side-effect, this is equivalent to
	// calling resetQueue(ResetQueueOp::kAllAccessible).
	error = sce::Agc::suspendPoint();
	SCE_AGC_ASSERT(error == SCE_OK);
}

void xs::render::clear()
{
	// lines_count = 0;
	// triangles_count = 0;
	sprite_queue.clear();
}

void xs::render::set_offset(double x, double y) {}

int xs::render::load_image(const std::string& image_file)
{
	// Find image first
	auto id = std::hash<std::string>{}(image_file);
	for (int i = 0; i < images.size(); i++)
		if (images[i].string_id == id)
			return i;

	auto buffer = fileio::read_binary_file(image_file);
	internal::image img;
	img.string_id = id;

	auto path = fileio::get_path(image_file);
	LoadPNGTexture(path.c_str(), img.texture);

	img.width = img.texture.getWidth();
	img.height = img.texture.getHeight();
	
	const auto i = images.size();
	images.push_back(img);
	return static_cast<int>(i);
}

int xs::render::create_sprite(int image_id, double x0, double y0, double x1, double y1)
{
	for (int i = 0; i < sprites.size(); i++)
	{
		const auto& s = sprites[i];
		if (s.image_id == image_id &&
			s.from.x == x0 && s.from.y == y0 &&
			s.to.x == x1 && s.to.y == y1)
			return i;
	}

	const auto i = sprites.size();
	sprite s = { image_id, { x0, y0 }, { x1, y1 } };
	sprites.push_back(s);
	return static_cast<int>(i);

}

void xs::render::render_sprite(
	int sprite_id,
	double x,
	double y,
	double scale,
	double rotation,
	color mutiply,
	color add,
	unsigned int flags)
{
	sprite_queue.push_back({
	sprite_id,
	x + offset.x,
	y + offset.y,
	scale,
	rotation,
	mutiply,
	add,
	flags });
}

int xs::render::load_font(const std::string& font_file, double size)
{
	return 0;
}

void xs::render::render_text(
	int font_id,
	const std::string& text,
	double x,
	double y,
	color multiply,
	color add,
	unsigned int flags)
{
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
