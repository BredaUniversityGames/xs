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
//#include <video_out.h>

const uint32_t BUFFERING = 2;			// This defines how many DCBs and scan-out buffers we have.
//const uint32_t SCREEN_WIDTH = 3840;
//const uint32_t SCREEN_HEIGHT = 2160;

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

void xs::render::render_sprite_ex(
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


