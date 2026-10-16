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
		int offset;			// Offset in the bytecode of fn the frame is at
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
			info.offset = offset;
			info.line = fn->debug->sourceLines.data[offset];
			info.name = fn->debug->name != nullptr ? fn->debug->name : "?";
			info.path = path;
			frames.push_back(info);
		}
		return frames;
	}

	// Variables --------------------------------------------------------------

	struct named_value
	{
		string name;
		Value value;
	};

	// What a variablesReference points to. The ids are only valid while the
	// game is paused.
	struct var_ref
	{
		enum class kind { locals, upvalues, module, value } what;
		frame_info frame;	// For locals, upvalues and module
		Value value;		// For value
	};
	vector<var_ref> refs;

	int add_ref(var_ref::kind what, const frame_info& frame, Value value = NULL_VAL)
	{
		refs.push_back({ what, frame, value });
		return (int)refs.size();	// 0 means no children, so ids start at 1
	}

	string class_name(Value value)
	{
		const ObjClass* c = wrenGetClassInline(vm, value);
		return c->name->value;
	}

	string type_name(Value value)
	{
		if (IS_NULL(value)) return "Null";
		if (IS_BOOL(value)) return "Bool";
		if (IS_NUM(value)) return "Num";
		if (IS_CLOSURE(value) || IS_FN(value)) return "Fn";
		if (IS_CLASS(value)) return "Class";
		if (IS_OBJ(value)) return class_name(value);
		return "Undefined";
	}

	string describe(Value value)
	{
		if (IS_NULL(value)) return "null";
		if (IS_BOOL(value)) return AS_BOOL(value) ? "true" : "false";
		if (IS_NUM(value))
		{
			char buffer[32];
			snprintf(buffer, sizeof(buffer), "%.14g", AS_NUM(value));
			return buffer;
		}
		if (IS_STRING(value))
		{
			const ObjString* s = AS_STRING(value);
			string res = "\"";
			for (uint32_t i = 0; i < s->length && i < 200; i++)
			{
				switch (s->value[i])
				{
				case '\n': res += "\\n"; break;
				case '\t': res += "\\t"; break;
				case '"': res += "\\\""; break;
				default: res += s->value[i];
				}
			}
			return res + (s->length > 200 ? "...\"" : "\"");
		}
		if (IS_LIST(value)) return "List(" + to_string(AS_LIST(value)->elements.count) + ")";
		if (IS_MAP(value)) return "Map(" + to_string(AS_MAP(value)->count) + ")";
		if (IS_RANGE(value))
		{
			const ObjRange* r = AS_RANGE(value);
			char buffer[64];
			snprintf(buffer, sizeof(buffer), "%.14g%s%.14g", r->from, r->isInclusive ? ".." : "...", r->to);
			return buffer;
		}
		if (IS_CLASS(value)) return string("class ") + AS_CLASS(value)->name->value;
		if (IS_CLOSURE(value))
			return string("Fn ") + AS_CLOSURE(value)->fn->debug->name;
		if (IS_OBJ(value)) return class_name(value);
		return "undefined";
	}

	// The names of the fields of an instance, from the class chain. The slots
	// of inherited fields come first.
	vector<named_value> instance_fields(Value value)
	{
		// Walk from the class up, then list the base class fields first
		vector<vector<named_value>> per_class;
		const ObjInstance* instance = AS_INSTANCE(value);
		const ObjClass* c = instance->obj.classObj;
		for (; c != nullptr; c = c->superclass)
		{
			per_class.emplace_back();
			auto& own_fields = per_class.back();
			const int base = c->superclass != nullptr ? c->superclass->numFields : 0;
			const ObjList* names = nullptr;
			if (c->module != nullptr && c->module->classFields != nullptr)
			{
				const Value found = wrenMapGet((ObjMap*)c->module->classFields, OBJ_VAL(c->name));
				if (!IS_UNDEFINED(found))
					names = AS_LIST(found);
			}
			const int own = c->numFields - base;
			for (int i = 0; i < own; i++)
			{
				const string name = names != nullptr && i < names->elements.count
					? AS_CSTRING(names->elements.data[i])
					: "field " + to_string(base + i);
				own_fields.push_back({ name, instance->fields[base + i] });
			}
		}

		vector<named_value> res;
		for (auto itr = per_class.rbegin(); itr != per_class.rend(); ++itr)
			res.insert(res.end(), itr->begin(), itr->end());
		return res;
	}

	// The variables that are inside [value], for lists, maps and instances.
	vector<named_value> children_of(Value value)
	{
		vector<named_value> res;
		constexpr int max_children = 1000;
		if (IS_INSTANCE(value))
		{
			res = instance_fields(value);
		}
		else if (IS_LIST(value))
		{
			const ObjList* list = AS_LIST(value);
			for (int i = 0; i < list->elements.count && i < max_children; i++)
				res.push_back({ "[" + to_string(i) + "]", list->elements.data[i] });
		}
		else if (IS_MAP(value))
		{
			const ObjMap* map = AS_MAP(value);
			for (uint32_t i = 0; i < map->capacity && (int)res.size() < max_children; i++)
			{
				const MapEntry& entry = map->entries[i];
				if (!IS_UNDEFINED(entry.key))
					res.push_back({ "[" + describe(entry.key) + "]", entry.value });
			}
		}
		return res;
	}

	bool has_children(Value value)
	{
		return IS_INSTANCE(value) || IS_LIST(value) || IS_MAP(value);
	}

	// Locals that hold a value at the current position of [f], and "this".
	vector<named_value> frame_locals(const frame_info& f)
	{
		vector<named_value> res;
		const Value receiver = f.frame->stackStart[0];
		if (!IS_CLOSURE(receiver))
			res.push_back({ "this", receiver });

		const DebugVariableBuffer& locals = f.fn->debug->locals;
		for (int i = 0; i < locals.count; i++)
		{
			const DebugVariable& v = locals.data[i];
			if (v.startIp <= f.offset && f.offset < v.endIp)
				res.push_back({ v.name, f.frame->stackStart[v.slot] });
		}
		return res;
	}

	vector<named_value> frame_upvalues(const frame_info& f)
	{
		vector<named_value> res;
		const DebugVariableBuffer& upvalues = f.fn->debug->upvalues;
		for (int i = 0; i < upvalues.count; i++)
		{
			const ObjUpvalue* upvalue = f.frame->closure->upvalues[upvalues.data[i].slot];
			if (upvalue != nullptr)
				res.push_back({ upvalues.data[i].name, *upvalue->value });
		}
		return res;
	}

	// The variables of the module of [f], without the classes.
	vector<named_value> module_variables(const frame_info& f)
	{
		vector<named_value> res;
		const ObjModule* module = f.fn->module;
		for (int i = 0; i < module->variables.count; i++)
		{
			const Value value = module->variables.data[i];
			if (IS_UNDEFINED(value) || IS_CLASS(value))
				continue;
			res.push_back({ module->variableNames.data[i]->value, value });
		}
		return res;
	}

	json to_json(const named_value& v, const frame_info& frame)
	{
		return {
			{ "name", v.name },
			{ "value", describe(v.value) },
			{ "type", type_name(v.value) },
			{ "variablesReference", has_children(v.value) ? add_ref(var_ref::kind::value, frame, v.value) : 0 }
		};
	}

	// Finds the value of an expression like `a`, `_field`, `a.b.c` or `list[1]`.
	bool evaluate_path(const frame_info& f, const string& expression, Value& result)
	{
		size_t i = 0;
		auto skip_space = [&]() { while (i < expression.size() && isspace((unsigned char)expression[i])) i++; };
		auto identifier = [&]() {
			const size_t start = i;
			while (i < expression.size() && (isalnum((unsigned char)expression[i]) || expression[i] == '_')) i++;
			return expression.substr(start, i - start);
		};

		skip_space();
		const string first = identifier();
		if (first.empty())
			return false;

		auto find = [&](const vector<named_value>& list, const string& name, Value& out) {
			for (const auto& v : list)
				if (v.name == name) { out = v.value; return true; }
			return false;
		};

		Value current = NULL_VAL;
		if (!find(frame_locals(f), first, current) &&
			!find(frame_upvalues(f), first, current) &&
			!find(module_variables(f), first, current))
		{
			// `_field` is short for a field of this
			Value self = NULL_VAL;
			if (first[0] != '_' || !find(frame_locals(f), "this", self) || !IS_INSTANCE(self) ||
				!find(instance_fields(self), first, current))
				return false;
		}

		for (skip_space(); i < expression.size(); skip_space())
		{
			if (expression[i] == '.')
			{
				i++;
				skip_space();
				const string name = identifier();
				if (name.empty() || !IS_INSTANCE(current))
					return false;
				const auto fields = instance_fields(current);
				if (!find(fields, name, current) && !find(fields, "_" + name, current))
					return false;
			}
			else if (expression[i] == '[')
			{
				const size_t close = expression.find(']', i);
				if (close == string::npos || !IS_LIST(current))
					return false;
				const int index = atoi(expression.substr(i + 1, close - i - 1).c_str());
				const ObjList* list = AS_LIST(current);
				if (index < 0 || index >= list->elements.count)
					return false;
				current = list->elements.data[index];
				i = close + 1;
			}
			else
				return false;
		}

		result = current;
		return true;
	}

	// Requests ---------------------------------------------------------------

	void handle_scopes(const json& req)
	{
		const auto frames = collect_frames();
		const int id = req["arguments"]["frameId"].get<int>();
		if (id < 0 || id >= (int)frames.size())
		{
			respond(req, false, json::object(), "Unknown frame");
			return;
		}

		const frame_info& f = frames[id];
		json scopes = json::array();
		scopes.push_back({ { "name", "Locals" }, { "variablesReference", add_ref(var_ref::kind::locals, f) }, { "expensive", false } });
		if (f.fn->debug->upvalues.count > 0)
			scopes.push_back({ { "name", "Closure" }, { "variablesReference", add_ref(var_ref::kind::upvalues, f) }, { "expensive", false } });
		scopes.push_back({ { "name", "Module" }, { "variablesReference", add_ref(var_ref::kind::module, f) }, { "expensive", true } });
		respond(req, true, { { "scopes", scopes } });
	}

	void handle_variables(const json& req)
	{
		const int id = req["arguments"]["variablesReference"].get<int>();
		if (id < 1 || id > (int)refs.size())
		{
			respond(req, true, { { "variables", json::array() } });
			return;
		}

		const var_ref ref = refs[id - 1];
		vector<named_value> values;
		switch (ref.what)
		{
		case var_ref::kind::locals: values = frame_locals(ref.frame); break;
		case var_ref::kind::upvalues: values = frame_upvalues(ref.frame); break;
		case var_ref::kind::module: values = module_variables(ref.frame); break;
		case var_ref::kind::value: values = children_of(ref.value); break;
		}

		json list = json::array();
		for (const auto& v : values)
			list.push_back(to_json(v, ref.frame));
		respond(req, true, { { "variables", list } });
	}

	void handle_evaluate(const json& req)
	{
		const auto& args = req["arguments"];
		const auto frames = collect_frames();
		const int id = args.value("frameId", 0);
		Value result = NULL_VAL;
		if (id < 0 || id >= (int)frames.size() ||
			!evaluate_path(frames[id], args.value("expression", ""), result))
		{
			respond(req, false, json::object(),
				"Only variables can be evaluated, like a, _field, a.b.c or list[0]");
			return;
		}

		respond(req, true, {
			{ "result", describe(result) },
			{ "type", type_name(result) },
			{ "variablesReference", has_children(result) ? add_ref(var_ref::kind::value, frames[id], result) : 0 }
		});
	}

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
				{ "supportsEvaluateForHovers", true },
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
			handle_scopes(req);
		}
		else if (command == "variables")
		{
			handle_variables(req);
		}
		else if (command == "evaluate")
		{
			handle_evaluate(req);
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
		refs.clear();
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
