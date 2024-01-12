#include "device.h"
#include "opengl.h"
#include <nn/vi.h>
#include <nn/nn_Assert.h>
#include <nn/os.h>
#include <nn/oe.h>
#include <nn/time.h>
#include <nv/nv_MemoryManagement.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "log.h"
#include "profiler.h"

using namespace xs::device;

namespace
{
	void* NvAllocateFunction(size_t size, size_t alignment, void* userPtr)
	{
		NN_UNUSED(userPtr);
		// According to specifications of aligned_alloc(), we need to coordinate the size parameter to become the integral multiple of alignment.
		return aligned_alloc(alignment, nn::util::align_up(size, alignment));
	}
	void NvFreeFunction(void* addr, void* userPtr)
	{
		NN_UNUSED(userPtr);
		free(addr);
	}
	void* NvReallocateFunction(void* addr, size_t newSize, void* userPtr)
	{
		NN_UNUSED(userPtr);
		return realloc(addr, newSize);
	}

	void* NvDevtoolsAllocateFunction(size_t size, size_t alignment, void* userPtr)
	{
		NN_UNUSED(userPtr);
		// According to specifications of aligned_alloc(), we need to coordinate the size parameter to become the integral multiple of alignment.
		return aligned_alloc(alignment, nn::util::align_up(size, alignment));
	}
	void NvDevtoolsFreeFunction(void* addr, void* userPtr)
	{
		NN_UNUSED(userPtr);
		free(addr);
	}
	void* NvDevtoolsReallocateFunction(void* addr, size_t newSize, void* userPtr)
	{
		NN_UNUSED(userPtr);
		return realloc(addr, newSize);
	}

	void logOpenGLVersionInfo()
	{
		const auto vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
		const auto renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
		const auto version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
		const auto shaderVersion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));

		xs::log::info("OpenGL Vendor {}", vendor);
		xs::log::info("OpenGL Renderer {}", renderer);
		xs::log::info("OpenGL Version {}", version);
		xs::log::info("OpenGL Shader Version {}", shaderVersion);
	}
}

namespace xs::device::internal
{
	EGLDisplay					display;
	EGLSurface					surface;
	EGLContext					context;
	nn::vi::NativeWindowHandle  nativeWindowHandle;
	nn::vi::Display*			nnDisplay = nullptr;
	nn::vi::Layer*				layer = nullptr;
	int							width;
	int							height;
}

using namespace xs::device::internal;

void xs::device::initialize()
{
	nn::time::Initialize();

	// Set memory allocator for graphics subsystem. This function must be called before using any graphics API's.
	nv::SetGraphicsAllocator(NvAllocateFunction, NvFreeFunction, NvReallocateFunction, NULL);

	//Set memory allocator for graphics developer tools and NVN debug layer. This function must be called before using any graphics developer features.
	nv::SetGraphicsDevtoolsAllocator(NvDevtoolsAllocateFunction, NvDevtoolsFreeFunction, NvDevtoolsReallocateFunction, NULL);

	// Donate memory for graphics driver to work in. This function must be called before using any graphics API's.
	size_t graphicsSystemMemorySize = 8 * 1024 * 1024;
	void* graphicsHeap = malloc(graphicsSystemMemorySize);
	nv::InitializeGraphics(graphicsHeap, graphicsSystemMemorySize);

	// Initialize the application operating environment (OE) library Enable notifications of changes in operating mode.
	nn::oe::Initialize();
	nn::oe::SetOperationModeChangedNotificationEnabled(true);

	// On startup check if device is in console or handheld mode by getting the default resolution
	nn::oe::GetDefaultDisplayResolution(&width, &height);

	// Initialize Video Interface (VI) system to display to the target's screen
	nn::vi::Initialize();

	nn::Result result = nn::vi::OpenDefaultDisplay(&nnDisplay);
	NN_ASSERT(result.IsSuccess());

	result = nn::vi::CreateLayer(&layer, nnDisplay, width, height);
	NN_ASSERT(result.IsSuccess());

	result = nn::vi::GetNativeWindow(&nativeWindowHandle, layer);
	NN_ASSERT(result.IsSuccess());

	// Initialize EGL
	EGLBoolean eglResult;

	display = ::eglGetDisplay(EGL_DEFAULT_DISPLAY);
	NN_ASSERT(display != NULL, "eglGetDisplay failed.");

	eglResult = ::eglInitialize(display, 0, 0);
	NN_ASSERT(eglResult, "eglInitialize failed.");

	EGLint configAttribs[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 8,
		EGL_NONE
	};
	EGLint numConfigs = 0;
	EGLConfig config;
	eglResult = ::eglChooseConfig(display, configAttribs, &config, 1, &numConfigs);
	NN_ASSERT(eglResult && numConfigs == 1, "eglChooseConfig failed.");

	surface = ::eglCreateWindowSurface(display, config, static_cast<NativeWindowType>(nativeWindowHandle), 0);
	NN_ASSERT(surface != EGL_NO_SURFACE, "eglCreateWindowSurface failed.");

	// Set the current rendering API.
	eglResult = eglBindAPI(EGL_OPENGL_API);
	NN_ASSERT(eglResult, "eglBindAPI failed.");

	// Create new context and set it as current.
	EGLint contextAttribs[] = {
		EGL_CONTEXT_MAJOR_VERSION, 4,
		EGL_CONTEXT_MINOR_VERSION, 6,		
		/* For debug callback */
		// TODO: Make this non-debug in release
		EGL_CONTEXT_FLAGS_KHR,
#if defined(DEBUG) || 1
		EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
		EGL_NONE
#else
		//EGL_CONTEXT_OPENGL_NO_ERROR_KHR,
		//EGL_NONE
#endif
	};
	context = ::eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
	NN_ASSERT(context != EGL_NO_CONTEXT, "eglCreateContext failed. %d", eglGetError());

	eglResult = ::eglMakeCurrent(display, surface, surface, context);
	NN_ASSERT(eglResult, "eglMakeCurrent failed.");

	eglResult = ::eglSwapInterval(display, 1);
	NN_ASSERT(eglResult, "eglSwapInterval failed.");

	if (!gladLoadGLLoader(GLADloadproc(eglGetProcAddress)))
	{
		log::critical("Failed to initialize OpenGL context");
		NN_ASSERT(false);
	}

	if (eglResult == EGL_FALSE)
	{
		xs::log::error("Failed to initialize EGL");
		xs::device::shutdown();
	}

	logOpenGLVersionInfo();
	
#if defined(DEBUG)	
#else
#endif
}

void xs::device::shutdown()
{
	EGLBoolean eglResult;

	eglResult = ::eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	NN_ASSERT(eglResult, "eglMakeCurrent failed.");
	eglResult = ::eglTerminate(display);
	NN_ASSERT(eglResult, "eglTerminate failed.");
	eglResult = ::eglReleaseThread();
	NN_ASSERT(eglResult, "eglReleaseThread failed.");

	if (eglResult == EGL_FALSE)
		xs::log::error("Failed to terminate EGL");

	nn::vi::DestroyLayer(layer);
	nn::vi::CloseDisplay(nnDisplay);
	nn::vi::Finalize();

	nv::FinalizeGraphics();
}

void xs::device::begin_frame() {}

void xs::device::end_frame()
{
	XS_PROFILE_FUNCTION();
	::eglSwapBuffers(display, surface);
	glClearColor(0.0, 0.0, 0.0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void xs::device::poll_events() {}

bool xs::device::can_close()
{
	return false;
}

bool xs::device::request_close()
{
	return false;
}

bool xs::device::should_close()
{
	return false;
}

int xs::device::get_width()
{
	return internal::width;
}

int xs::device::get_height()
{
	return internal::height;
}
