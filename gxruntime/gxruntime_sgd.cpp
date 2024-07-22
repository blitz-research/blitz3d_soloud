#include "std.h"
#include "gxruntime.h"

#ifndef SPI_SETMOUSESPEED
#include "zmouse.h"
#define SPI_SETMOUSESPEED	113
#endif

namespace {

	typedef int (_stdcall *LibFunc)(const void *in,	int in_sz, void *out, int out_sz);

	struct gxDll {
		HINSTANCE hinst{};
		map<string, LibFunc> funcs;
	};

	enum {
		WM_STOP = WM_APP + 1, WM_RUN, WM_END
	};

	HINSTANCE hinstance;
	std::string cmd_line;
	Debugger *debugger;

	HWND hwnd;
	volatile bool suspended;
	volatile bool run_flag;

	gxRuntime *runtime;

	map<string, gxDll *> libs;

	set<gxTimer *> timers;

	OSVERSIONINFO osinfo;

	LRESULT CALLBACK windowProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
		if (!runtime || !run_flag) return DefWindowProc(h, msg, w, l);
		switch (msg) {    //NOLINT
			case WM_STOP:
				if (!suspended) {
					suspended = true;
					if (debugger) debugger->debugStop();
				}
				return 0;
			case WM_RUN:
				runtime->resume();
				return 0;
			case WM_END:
				debugger = 0;
				run_flag = false;
				return 0;
			default:;
		}
		return DefWindowProc(h, msg, w, l);
	}
}

gxRuntime *gxRuntime::openRuntime(HINSTANCE hinst, const string &cmdline, Debugger *d) {
	if (runtime) return nullptr;

	::hinstance = hinst;
	::cmd_line = cmdline;
	::debugger = d;

	CoInitialize(0);

	TIMECAPS tc;
	timeGetDevCaps(&tc, sizeof(tc));
	timeBeginPeriod(tc.wPeriodMin);

	//create dummy window for debugger to communicate with
	WNDCLASS wndclass;
	memset(&wndclass, 0, sizeof(wndclass));
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.lpfnWndProc = ::windowProc;
	wndclass.hInstance = hinstance;
	wndclass.lpszClassName = "Blitz Runtime Class";
	wndclass.hCursor = (HCURSOR) LoadCursor(0, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	RegisterClass(&wndclass);
	hwnd = CreateWindowEx(0, "Blitz Runtime Class", " ", 0, 0, 0, 0, 0, 0, 0, 0, 0);

	memset(&osinfo, 0, sizeof(osinfo));
	osinfo.dwOSVersionInfoSize = sizeof(osinfo);
	GetVersionEx(&osinfo);

	runtime = d_new gxRuntime();

	run_flag = true;

	return runtime;
}

void gxRuntime::closeRuntime(gxRuntime *r) {
	if (!runtime || runtime != r) return;

	while (!timers.empty()) runtime->freeTimer(*timers.begin());
	timers.clear();

	map<string, gxDll *>::const_iterator it;
	for (it = libs.begin(); it != libs.end(); ++it) {
		FreeLibrary(it->second->hinst);
	}
	libs.clear();

	delete runtime;
	runtime = 0;

	DestroyWindow(hwnd);
	UnregisterClass("Blitz Runtime Class", hinstance);

	TIMECAPS tc;
	timeGetDevCaps(&tc, sizeof(tc));
	timeEndPeriod(tc.wPeriodMin);

	CoUninitialize();
}

void gxRuntime::suspend() {
	if(suspended) return;
	suspended = true;
	if (debugger) debugger->debugStop();
}

void gxRuntime::resume() {
	if(!suspended) return;
	suspended = false;
	if (debugger) debugger->debugRun();
}

void gxRuntime::asyncStop() {
	PostMessage(hwnd, WM_STOP, 0, 0);
}

void gxRuntime::asyncRun() {
	PostMessage(hwnd, WM_RUN, 0, 0);
}

void gxRuntime::asyncEnd() {
	PostMessage(hwnd, WM_END, 0, 0);
}

bool gxRuntime::idle() {
	while (suspended && run_flag) {
		MSG msg;
		GetMessage(&msg, 0, 0, 0);
		DispatchMessage(&msg);
	}
	return run_flag;
}

bool gxRuntime::delay(int ms) {
	int t = timeGetTime() + ms;
	for (;;) {
		if (!idle()) return false;
		int d = t - timeGetTime(); //how long left to wait
		if (d <= 0) return true;
		if (d > 100) d = 100;
		Sleep(d);
	}
}

void gxRuntime::debugStmt(int pos, const char *file) {
	if (debugger) debugger->debugStmt(pos, file);
}

void gxRuntime::debugStop() {
	suspend();
}

void gxRuntime::debugEnter(void *frame, void *env, const char *func) {
	if (debugger) debugger->debugEnter(frame, env, func);
}

void gxRuntime::debugLeave() {
	if (debugger) debugger->debugLeave();
}

void gxRuntime::debugError(const char *t) {
	if (!debugger) return;
	Debugger *d = debugger;
	asyncEnd();
	suspend();
	d->debugMsg(t, true);
}

void gxRuntime::debugInfo(const char *t) {
	if (!debugger) return;
	Debugger *d = debugger;
	asyncEnd();
	suspend();
	d->debugMsg(t, false);
}

void gxRuntime::debugLog(const char *t) {
	if (debugger) debugger->debugLog(t);
}

string gxRuntime::commandLine() {
	return cmd_line;
}

bool gxRuntime::execute(const string &cmd_line) {
	if (!cmd_line.size()) return false;

	//convert cmd_line to cmd and params
	string cmd = cmd_line, params;
	while (cmd.size() && cmd[0] == ' ') cmd = cmd.substr(1);
	if (cmd.find('\"') == 0) {
		int n = cmd.find('\"', 1);
		if (n != string::npos) {
			params = cmd.substr(n + 1);
			cmd = cmd.substr(1, n - 1);
		}
	} else {
		int n = cmd.find(' ');
		if (n != string::npos) {
			params = cmd.substr(n + 1);
			cmd = cmd.substr(0, n);
		}
	}
	while (params.size() && params[0] == ' ') params = params.substr(1);
	while (params.size() && params[params.size() - 1] == ' ') params = params.substr(0, params.size() - 1);

	SetForegroundWindow(GetDesktopWindow());

	return (int) ShellExecute(GetDesktopWindow(), 0, cmd.c_str(), params.size() ? params.c_str() : 0, 0, SW_SHOW) > 32;
}

gxFileSystem *gxRuntime::openFileSystem(int flags) {
	if (fileSystem) return 0;

	fileSystem = d_new gxFileSystem();
	return fileSystem;
}

void gxRuntime::closeFileSystem(gxFileSystem *f) {
	if (!fileSystem || fileSystem != f) return;

	delete fileSystem;
	fileSystem = 0;
}

void gxRuntime::setTitle( const string &t,const string &e ) {
}

int gxRuntime::getMilliSecs() {
	return timeGetTime();
}

gxTimer *gxRuntime::createTimer(int hertz) {
	gxTimer *t = d_new gxTimer(this, hertz);
	timers.insert(t);
	return t;
}

void gxRuntime::freeTimer(gxTimer *t) {
	if (!timers.count(t)) return;
	timers.erase(t);
	delete t;
}

static string toDir(string t) {
	if (t.size() && t[t.size() - 1] != '\\') t += '\\';
	return t;
}

string gxRuntime::systemProperty(const std::string &p) {
	char buff[MAX_PATH + 1];
	string t = tolower(p);
	if (t == "cpu") {
		return "Intel";
	} else if (t == "os") {
		switch (osinfo.dwMajorVersion) {
			case 3:
				switch (osinfo.dwMinorVersion) {
					case 51:
						return "Windows NT 3.1";
				}
				break;
			case 4:
				switch (osinfo.dwMinorVersion) {
					case 0:
						return "Windows 95";
					case 10:
						return "Windows 98";
					case 90:
						return "Windows ME";
				}
				break;
			case 5:
				switch (osinfo.dwMinorVersion) {
					case 0:
						return "Windows 2000";
					case 1:
						return "Windows XP";
					case 2:
						return "Windows Server 2003";
				}
				break;
			case 6:
				switch (osinfo.dwMinorVersion) {
					case 0:
						return "Windows Vista";
					case 1:
						return "Windows 7";
				}
				break;
		}
	} else if (t == "appdir") {
		if (GetModuleFileName(0, buff, MAX_PATH)) {
			string t = buff;
			int n = t.find_last_of('\\');
			if (n != string::npos) t = t.substr(0, n);
			return toDir(t);
		}
	} else if (t == "apphwnd") {
		return itoa((int) hwnd);
	} else if (t == "apphinstance") {
		return itoa((int) hinstance);
	} else if (t == "windowsdir") {
		if (GetWindowsDirectory(buff, MAX_PATH)) return toDir(buff);
	} else if (t == "systemdir") {
		if (GetSystemDirectory(buff, MAX_PATH)) return toDir(buff);
	} else if (t == "tempdir") {
		if (GetTempPath(MAX_PATH, buff)) return toDir(buff);
	}
	return "";
}


int gxRuntime::callDll(const std::string &dll, const std::string &func, const void *in, int in_sz, void *out,
					   int out_sz) {
	map<string, gxDll *>::const_iterator lib_it = libs.find(dll);

	if (lib_it == libs.end()) {
		HINSTANCE h = LoadLibrary(dll.c_str());
		if (!h) return 0;
		gxDll *t = d_new gxDll;
		t->hinst = h;
		lib_it = libs.insert(make_pair(dll, t)).first;
	}

	gxDll *t = lib_it->second;
	map<string, LibFunc>::const_iterator fun_it = t->funcs.find(func);

	if (fun_it == t->funcs.end()) {
		LibFunc f = (LibFunc) GetProcAddress(t->hinst, func.c_str());
		if (!f) return 0;
		fun_it = t->funcs.insert(make_pair(func, f)).first;
	}

	static void *save_esp;

	_asm{
		mov[save_esp], esp
	};

	int n = fun_it->second(in, in_sz, out, out_sz);

	_asm{
		mov esp,[save_esp]
	};

	return n;
}
