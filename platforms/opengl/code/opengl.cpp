#include <string>
#include "opengl.hpp"
#include "log.hpp"

#if defined(DEBUG)

void xs::gl_label(GLenum type, GLuint name, const std::string& label)
{
	std::string typeString;
	switch (type)
	{
	case GL_BUFFER:
		typeString = "buffer";
		break;
	case GL_SHADER:
		typeString = "shader";
		break;
	case GL_PROGRAM:
		typeString = "program";
		break;
	case GL_VERTEX_ARRAY:
		typeString = "vetex array";
		break;
	case GL_QUERY:
		typeString = "query";
		break;
	case GL_PROGRAM_PIPELINE:
		typeString = "program pipeline";
		break;
	case GL_TRANSFORM_FEEDBACK:
		typeString = "transform feedback";
		break;
	case GL_SAMPLER:
		typeString = "sampler";
		break;
	case GL_TEXTURE:
		typeString = "texture";
		break;
	case GL_RENDERBUFFER:
		typeString = "renderbuffer";
		break;
	case GL_FRAMEBUFFER:
		typeString = "framebuffer";
		break;
	default:
		typeString = "unknown";
		break;
	}

	const std::string temp = "[" + typeString + ":" + std::to_string(name) + "] " + label;
	glObjectLabel(type, name, static_cast<GLsizei>(temp.length()), temp.c_str());
}

#ifdef PLATFORM_PC
#ifdef APIENTRY
#undef APIENTRY
#endif
#include <Windows.h>
#endif

static void APIENTRY debug_callback_func(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const GLvoid* userParam)
{
	// Skip some less useful info
	if (id == 131218)	// http://stackoverflow.com/questions/12004396/opengl-debug-context-performance-warning
		return;

	// UNUSED(length);
	// UNUSED(userParam);
	std::string source_string;
	std::string type_string;
	std::string severity_string;

	// The AMD variant of this extension provides a less detailed classification of the error,
	// which is why some arguments might be "Unknown".
	switch (source) {
	case GL_DEBUG_CATEGORY_API_ERROR_AMD:
	case GL_DEBUG_SOURCE_API: {
		source_string = "API";
		break;
	}
	case GL_DEBUG_CATEGORY_APPLICATION_AMD:
	case GL_DEBUG_SOURCE_APPLICATION: {
		source_string = "Application";
		break;
	}
	case GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD:
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: {
		source_string = "Window System";
		break;
	}
	case GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD:
	case GL_DEBUG_SOURCE_SHADER_COMPILER: {
		source_string = "Shader Compiler";
		break;
	}
	case GL_DEBUG_SOURCE_THIRD_PARTY: {
		source_string = "Third Party";
		break;
	}
	case GL_DEBUG_CATEGORY_OTHER_AMD:
	case GL_DEBUG_SOURCE_OTHER: {
		source_string = "Other";
		break;
	}
	default: {
		source_string = "Unknown";
		break;
	}
	}

	switch (type) {
	case GL_DEBUG_TYPE_ERROR: {
		type_string = "Error";
		break;
	}
	case GL_DEBUG_CATEGORY_DEPRECATION_AMD:
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: {
		type_string = "Deprecated Behavior";
		break;
	}
	case GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD:
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: {
		type_string = "Undefined Behavior";
		break;
	}
	case GL_DEBUG_TYPE_PORTABILITY_ARB: {
		type_string = "Portability";
		break;
	}
	case GL_DEBUG_CATEGORY_PERFORMANCE_AMD:
	case GL_DEBUG_TYPE_PERFORMANCE: {
		type_string = "Performance";
		break;
	}
	case GL_DEBUG_CATEGORY_OTHER_AMD:
	case GL_DEBUG_TYPE_OTHER: {
		type_string = "Other";
		break;
	}
	default: {
		type_string = "Unknown";
		break;
	}
	}

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH: {
		severity_string = "High";
		break;
	}
	case GL_DEBUG_SEVERITY_MEDIUM: {
		severity_string = "Medium";
		break;
	}
	case GL_DEBUG_SEVERITY_LOW: {
		severity_string = "Low";
		break;
	}
	default: {
		severity_string = "Unknown";
		return;
	}
	}

	xs::log::warn("GL Debug Callback:\n source: {}:{} \n type: {}:{} \n id: {} \n severity: {}:{} \n  message: {}",
		source, source_string.c_str(), type, type_string.c_str(), id, severity, severity_string.c_str(), message);
	// ASSERT(type != GL_DEBUG_TYPE_ERROR, "GL Error occurs.");
}

#if defined(PLATFORM_PC)
static void APIENTRY DebugCallbackFuncAMD(
	GLuint id,
	GLenum category,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	void* userParam)
{
	debug_callback_func(
		GL_DEBUG_CATEGORY_API_ERROR_AMD,
		category,
		id,
		severity,
		length,
		message,
		userParam);
}

void xs::init_debug_messages()
{
	// Query the OpenGL function to register your callback function.
	PFNGLDEBUGMESSAGECALLBACKPROC	 _glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)wglGetProcAddress("glDebugMessageCallback");
	PFNGLDEBUGMESSAGECALLBACKARBPROC _glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC)wglGetProcAddress("glDebugMessageCallbackARB");
	PFNGLDEBUGMESSAGECALLBACKAMDPROC _glDebugMessageCallbackAMD = (PFNGLDEBUGMESSAGECALLBACKAMDPROC)wglGetProcAddress("glDebugMessageCallbackAMD");

	glDebugMessageCallback(debug_callback_func, nullptr);

	// Register your callback function.
	if (_glDebugMessageCallback != nullptr)
	{
		_glDebugMessageCallback(debug_callback_func, nullptr);
	}
	else if (_glDebugMessageCallbackARB != nullptr)
	{
		_glDebugMessageCallbackARB(debug_callback_func, nullptr);
	}

	// Additional AMD support
	if (_glDebugMessageCallbackAMD != nullptr)
	{
		_glDebugMessageCallbackAMD(DebugCallbackFuncAMD, nullptr);
	}

	// Enable synchronous callback. This ensures that your callback function is called
	// right after an error has occurred. This capability is not defined in the AMD
	// version.
	if ((_glDebugMessageCallback != nullptr) || (_glDebugMessageCallbackARB != nullptr))
	{
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	}

	glDebugMessageControl(
		GL_DONT_CARE,
		GL_DONT_CARE,
		GL_DEBUG_SEVERITY_LOW,
		0,
		nullptr,
		GL_FALSE);

	glDebugMessageControl(
		GL_DONT_CARE,
		GL_DONT_CARE,
		GL_DEBUG_SEVERITY_NOTIFICATION,
		0,
		nullptr,
		GL_FALSE);
}


#elif defined(PLATFORM_SWITCH)


void init_debug_messages()  
{
	glDebugMessageCallback(debug_callback_func, NULL);
}

#endif

#endif

