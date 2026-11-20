#pragma once
#include <string>

typedef struct WrenVM WrenVM;

/// Debugger for Wren scripts. It speaks the Debug Adapter Protocol (DAP) over
/// TCP, so any DAP client (VS Code through xs-vscode, Neovim, Helix, ...) can
/// set breakpoints, step, and inspect the call stack and variables.
///
/// It is built on the debug hook of the Wren VM (WREN_DEBUGGER) and only runs
/// for unpackaged games started with --debug-port.
namespace xs::debugger
{
	/// Starts listening on [port]. Call before script::configure().
	bool enable(int port);

	/// Returns true when a debugger was enabled with enable().
	bool enabled();

	/// Hooks the debugger into a freshly created [vm]. The first time, it waits
	/// until a client has connected and finished configuring its breakpoints.
	void attach(WrenVM* vm);

	/// Unhooks the debugger. Call before the VM is freed.
	void detach();

	/// Handles incoming requests. Call once per frame.
	void update();

	/// Tells the debugger the window exists, so it can keep it responsive
	/// while the game is paused.
	void window_ready();

	/// Closes the connection and stops listening.
	void shutdown();
}
