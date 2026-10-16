// Windows headers have to come first, winsock2.h must precede windows.h
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using sock_t = SOCKET;
static const sock_t invalid_sock = INVALID_SOCKET;
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
using sock_t = int;
static const sock_t invalid_sock = -1;
#endif

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "debugger.hpp"
#include "device.hpp"
#include "fileio.hpp"
#include "log.hpp"
#include "script.hpp"
#include "json/json.hpp"

#ifdef _MSC_VER
#pragma warning(disable:4200)
#endif

extern "C" {
#include "wren.h"
#include "wren_vm.h"
}

#ifdef _MSC_VER
#pragma warning(default:4200)
#endif

using namespace std;
using namespace xs;
using json = nlohmann::json;

#if WREN_DEBUGGER

namespace
{
	// The one and only thread of a Wren program, as far as the client knows.
	constexpr int thread_id = 1;

	// How many line events pass between checks for incoming requests while the
	// game is running. Checking costs a system call, so it is not done on every
	// line.
	constexpr int poll_interval = 512;

	// A stack frame as shown to the client.
	struct frame_info
	{
		int index;			// Index in the frames of the fiber
		ObjFn* fn;
		CallFrame* frame;
		int line;
		string name;
		string path;
	};

	// Socket -----------------------------------------------------------------

	void close_socket(sock_t& s)
	{
		if (s == invalid_sock)
			return;
#if defined(_WIN32)
		closesocket(s);
#else
		close(s);
#endif
		s = invalid_sock;
	}

	void set_non_blocking(sock_t s)
	{
#if defined(_WIN32)
		u_long mode = 1;
		ioctlsocket(s, FIONBIO, &mode);
#else
		fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
#endif
	}

	// Waits up to [timeout_ms] until [s] can be read.
	bool readable(sock_t s, int timeout_ms)
	{
		fd_set set;
		FD_ZERO(&set);
		FD_SET(s, &set);
		timeval tv{ timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
		return select((int)s + 1, &set, nullptr, nullptr, &tv) > 0;
	}

	// State ------------------------------------------------------------------

	sock_t listener = invalid_sock;
	sock_t client = invalid_sock;
	string inbox;					// Bytes received but not yet parsed
	int next_seq = 1;

	WrenVM* vm = nullptr;
	bool is_enabled = false;
	bool ready_for_window = false;	// Events can be pumped
	bool configured = false;		// Client sent configurationDone
	bool launched = false;			// Client used launch (not attach)
	bool paused = false;
	bool pause_requested = false;
	bool break_on_error = true;
	int poll_counter = 0;

	// What the client asked for after a stop.
	enum class resume_mode { none, run };
	resume_mode resume = resume_mode::none;

	// How far the game runs before stopping again. Stepping compares the
	// number of frames of the fiber with the number when the step started.
	enum class step_mode { none, in, over, out };
	step_mode step = step_mode::none;
	const ObjFiber* step_fiber = nullptr;
	int step_depth = 0;

	// Breakpoint lines per normalized source path.
	unordered_map<string, set<int>> breakpoints;

	// Normalized source path of each module, by module object. Empty if the
	// module has no source file, like the built-in ones.
	unordered_map<const ObjModule*, string> module_paths;

	// The error that stopped the game, if that is why we are paused.
	string stop_error;
	bool stopped_on_instruction = true;

	string normalize(const string& path)
	{
		string res = filesystem::path(path).lexically_normal().generic_string();
#if defined(_WIN32)
		transform(res.begin(), res.end(), res.begin(),
			[](unsigned char c) { return (char)tolower(c); });
#endif
		return res;
	}

	// Returns the normalized source path of the module of [fn], or an empty
	// string if the module is not backed by a file.
	const string& module_path(const ObjModule* module)
	{
		// Functions made by wrenMakeCallHandle() belong to no module.
		static const string none;
		if (module == nullptr)
			return none;

		auto itr = module_paths.find(module);
		if (itr != module_paths.end())
			return itr->second;

		string path;
		if (module->name != nullptr)
		{
			const string abs = script::module_path(module->name->value);
			if (!abs.empty())
				path = normalize(abs);
		}
		return module_paths.emplace(module, path).first->second;
	}

	// Transport --------------------------------------------------------------

	void send(const json& message)
	{
		if (client == invalid_sock)
			return;

		const string body = message.dump();
		const string packet = "Content-Length: " + to_string(body.size()) + "\r\n\r\n" + body;
		size_t sent = 0;
		while (sent < packet.size())
		{
			const int n = (int)::send(client, packet.data() + sent, (int)(packet.size() - sent), 0);
			if (n <= 0)
			{
				close_socket(client);
				return;
			}
			sent += n;
		}
	}

	void respond(const json& request, bool success, const json& body = json::object(), const string& message = "")
	{
		json res = {
			{ "seq", next_seq++ },
			{ "type", "response" },
			{ "request_seq", request["seq"] },
			{ "success", success },
			{ "command", request["command"] },
			{ "body", body }
		};
		if (!success)
			res["message"] = message;
		send(res);
	}

	void event(const string& name, const json& body = json::object())
	{
		send({
			{ "seq", next_seq++ },
			{ "type", "event" },
			{ "event", name },
			{ "body", body }
		});
	}

	// Takes the next complete message out of the inbox.
	bool next_message(json& out)
	{
		const size_t header_end = inbox.find("\r\n\r\n");
		if (header_end == string::npos)
			return false;

		const string header = inbox.substr(0, header_end);
		const size_t key = header.find("Content-Length:");
		if (key == string::npos)
		{
			inbox.erase(0, header_end + 4);
			return false;
		}

		const size_t length = (size_t)atoi(header.c_str() + key + 15);
		if (inbox.size() < header_end + 4 + length)
			return false;

		const string body = inbox.substr(header_end + 4, length);
		inbox.erase(0, header_end + 4 + length);
		out = json::parse(body, nullptr, false);
		return !out.is_discarded();
	}

	// Reads what is available on the client socket. Waits up to [timeout_ms]
	// for something to arrive. Returns false when the connection is gone.
	bool receive(int timeout_ms)
	{
		if (client == invalid_sock)
			return false;

		if (!readable(client, timeout_ms))
			return true;

		char buffer[4096];
		const int n = (int)recv(client, buffer, sizeof(buffer), 0);
		if (n <= 0)
		{
			close_socket(client);
			return false;
		}
		inbox.append(buffer, n);
		return true;
	}

	void accept_client()
	{
		if (listener == invalid_sock || client != invalid_sock)
			return;
		if (!readable(listener, 0))
			return;

		client = accept(listener, nullptr, nullptr);
		if (client == invalid_sock)
			return;

		set_non_blocking(client);
		inbox.clear();
		log::info("Debugger client connected.");
	}

	// Stack ------------------------------------------------------------------

	// Collects the frames of the current fiber that have a source file,
	// innermost first.
	vector<frame_info> collect_frames()
	{
		vector<frame_info> frames;
		if (vm == nullptr || vm->fiber == nullptr)
			return frames;

		const ObjFiber* fiber = vm->fiber;
		for (int i = fiber->numFrames - 1; i >= 0; i--)
		{
			CallFrame* frame = &fiber->frames[i];
			ObjFn* fn = frame->closure->fn;
			const string& path = module_path(fn->module);
			if (path.empty())
				continue;

			// A frame that called something has its ip after the call. The
			// innermost frame has its ip on the next instruction to run, unless
			// it stopped on an error.
			int offset = (int)(frame->ip - fn->code.data);
			if (i != fiber->numFrames - 1 || !stopped_on_instruction)
				offset = max(offset - 1, 0);

			frame_info info;
			info.index = i;
			info.fn = fn;
			info.frame = frame;
			info.line = fn->debug->sourceLines.data[offset];
			info.name = fn->debug->name != nullptr ? fn->debug->name : "?";
			info.path = path;
			frames.push_back(info);
		}
		return frames;
	}

	// Requests ---------------------------------------------------------------

	void handle_stack_trace(const json& req)
	{
		const auto frames = collect_frames();
		json list = json::array();
		for (size_t i = 0; i < frames.size(); i++)
		{
			const auto& f = frames[i];
			list.push_back({
				{ "id", (int)i },
				{ "name", f.name },
				{ "line", f.line },
				{ "column", 1 },
				{ "source", {
					{ "name", filesystem::path(f.path).filename().string() },
					{ "path", f.path } } }
			});
		}
		respond(req, true, { { "stackFrames", list }, { "totalFrames", (int)frames.size() } });
	}

	void handle_set_breakpoints(const json& req)
	{
		const auto& args = req["arguments"];
		const string path = normalize(args["source"]["path"].get<string>());

		set<int> lines;
		json verified = json::array();
		if (args.contains("breakpoints"))
		{
			for (const auto& bp : args["breakpoints"])
			{
				const int line = bp["line"].get<int>();
				lines.insert(line);
				verified.push_back({ { "verified", true }, { "line", line } });
			}
		}

		if (lines.empty())
			breakpoints.erase(path);
		else
			breakpoints[path] = lines;

		respond(req, true, { { "breakpoints", verified } });
	}

	void handle_exception_info(const json& req)
	{
		respond(req, true, {
			{ "exceptionId", "RuntimeError" },
			{ "description", stop_error },
			{ "breakMode", "unhandled" }
		});
	}

	void handle(const json& req)
	{
		const string command = req.value("command", "");

		if (command == "initialize")
		{
			respond(req, true, {
				{ "supportsConfigurationDoneRequest", true },
				{ "supportsExceptionInfoRequest", true },
				{ "exceptionBreakpointFilters", json::array({
					{ { "filter", "uncaught" }, { "label", "Uncaught Errors" }, { "default", true } } }) }
			});
			event("initialized");
		}
		else if (command == "launch" || command == "attach")
		{
			launched = command == "launch";
			respond(req, true);
		}
		else if (command == "configurationDone")
		{
			configured = true;
			respond(req, true);
		}
		else if (command == "setBreakpoints")
		{
			handle_set_breakpoints(req);
		}
		else if (command == "setExceptionBreakpoints")
		{
			const auto& args = req["arguments"];
			break_on_error = args.contains("filters") &&
				find(args["filters"].begin(), args["filters"].end(), "uncaught") != args["filters"].end();
			respond(req, true);
		}
		else if (command == "threads")
		{
			respond(req, true, { { "threads", json::array({ { { "id", thread_id }, { "name", "main" } } }) } });
		}
		else if (command == "stackTrace")
		{
			handle_stack_trace(req);
		}
		else if (command == "scopes")
		{
			respond(req, true, { { "scopes", json::array() } });
		}
		else if (command == "variables")
		{
			respond(req, true, { { "variables", json::array() } });
		}
		else if (command == "exceptionInfo")
		{
			handle_exception_info(req);
		}
		else if (command == "continue")
		{
			respond(req, true, { { "allThreadsContinued", true } });
			step = step_mode::none;
			resume = resume_mode::run;
		}
		else if (command == "next" || command == "stepIn" || command == "stepOut")
		{
			step = command == "next" ? step_mode::over :
				   command == "stepIn" ? step_mode::in : step_mode::out;
			step_fiber = vm->fiber;
			step_depth = vm->fiber->numFrames;
			respond(req, true);
			resume = resume_mode::run;
		}
		else if (command == "pause")
		{
			pause_requested = true;
			respond(req, true);
		}
		else if (command == "disconnect" || command == "terminate")
		{
			const bool terminate = launched ||
				(req.contains("arguments") && req["arguments"].value("terminateDebuggee", false));
			respond(req, true);
			close_socket(client);
			resume = resume_mode::run;
			if (terminate)
			{
				if (ready_for_window)
					device::request_close();
				else
					exit(0);
			}
		}
		else
		{
			respond(req, false, json::object(), "Unsupported request: " + command);
		}
	}

	// Handles every complete request that has arrived.
	void process_requests()
	{
		json req;
		while (next_message(req))
		{
			if (req.value("type", "") == "request")
				handle(req);
		}
	}

	// Handles requests, waiting up to [timeout_ms] for new ones.
	void service(int timeout_ms)
	{
		accept_client();
		receive(timeout_ms);
		process_requests();
	}

	// Blocks until the client has finished configuring.
	void wait_for_configuration()
	{
		log::info("Waiting for a debugger client...");
		while (!configured && is_enabled)
		{
			service(50);
			if (ready_for_window)
			{
				device::poll_events();
				if (device::should_close())
					return;
			}
		}
	}

	// Pauses the game until the client resumes it.
	void stop(const char* reason, const string& text)
	{
		if (client == invalid_sock)
			return;

		paused = true;
		resume = resume_mode::none;
		pause_requested = false;
		event("stopped", {
			{ "reason", reason },
			{ "threadId", thread_id },
			{ "text", text },
			{ "allThreadsStopped", true }
		});

		while (resume == resume_mode::none && client != invalid_sock)
		{
			service(16);
			if (ready_for_window)
			{
				device::poll_events();
				if (device::should_close())
					break;
			}
		}
		paused = false;
	}

	// Called by the VM -------------------------------------------------------

	void hook(WrenVM* hooked_vm, WrenDebugEvent ev, const char* message)
	{
		if (client == invalid_sock)
			return;

		if (ev == WREN_DEBUG_EVENT_ERROR)
		{
			if (break_on_error)
			{
				step = step_mode::none;
				stop_error = message != nullptr ? message : "Runtime error";
				stopped_on_instruction = false;
				stop("exception", stop_error);
				stopped_on_instruction = true;
			}
			return;
		}

		if (++poll_counter >= poll_interval)
		{
			poll_counter = 0;
			service(0);
		}

		const ObjFiber* fiber = hooked_vm->fiber;
		const CallFrame* frame = &fiber->frames[fiber->numFrames - 1];
		const ObjFn* fn = frame->closure->fn;

		// Code without a source file, like the core library, can not be debugged.
		const string& path = module_path(fn->module);
		if (path.empty())
			return;

		const int offset = (int)(frame->ip - fn->code.data);
		const int line = fn->debug->sourceLines.data[offset];

		if (pause_requested)
		{
			step = step_mode::none;
			stop("pause", "");
			return;
		}

		const auto itr = breakpoints.find(path);
		if (itr != breakpoints.end() && itr->second.count(line) > 0)
		{
			step = step_mode::none;
			stop("breakpoint", "");
			return;
		}

		if (step != step_mode::none)
		{
			// Another fiber means the step went through Fiber.call or yield,
			// which is the place the user wants to see next.
			const int depth = fiber->numFrames;
			const bool done =
				fiber != step_fiber ||
				step == step_mode::in ||
				(step == step_mode::over && depth <= step_depth) ||
				(step == step_mode::out && depth < step_depth);
			if (done)
			{
				step = step_mode::none;
				stop("step", "");
			}
		}
	}
}

bool debugger::enable(int port)
{
#if defined(_WIN32)
	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
#endif

	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener == invalid_sock)
	{
		log::error("Debugger: can not create a socket.");
		return false;
	}

	int yes = 1;
	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

	// Only local debugging, the debugger must not be reachable from the network.
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons((unsigned short)port);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

	if (bind(listener, (sockaddr*)&addr, sizeof(addr)) != 0 || listen(listener, 1) != 0)
	{
		log::error("Debugger: can not listen on port {}.", port);
		close_socket(listener);
		return false;
	}

	set_non_blocking(listener);
	is_enabled = true;
	log::info("Debugger listening on port {}.", port);
	return true;
}

bool debugger::enabled()
{
	return is_enabled;
}

void debugger::attach(WrenVM* new_vm)
{
	if (!is_enabled)
		return;

	vm = new_vm;
	module_paths.clear();
	step = step_mode::none;
	wrenSetDebugHook(vm, &hook);

	// Only the first VM waits, after a reload the client is already set up.
	if (!configured)
		wait_for_configuration();
}

void debugger::detach()
{
	if (vm != nullptr)
		wrenSetDebugHook(vm, nullptr);
	vm = nullptr;
	module_paths.clear();
}

void debugger::update()
{
	if (!is_enabled)
		return;
	service(0);
}

void debugger::window_ready()
{
	ready_for_window = true;
}

void debugger::shutdown()
{
	if (!is_enabled)
		return;

	event("terminated");
	close_socket(client);
	close_socket(listener);
	is_enabled = false;
}

#else // WREN_DEBUGGER

bool debugger::enable(int) { log::error("This build of xs has no debugger."); return false; }
bool debugger::enabled() { return false; }
void debugger::attach(WrenVM*) {}
void debugger::detach() {}
void debugger::update() {}
void debugger::window_ready() {}
void debugger::shutdown() {}

#endif
