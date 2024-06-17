#include "std.h"
#include "gxruntime.h"
#include "zmouse.h"

#ifndef SPI_SETMOUSESPEED
#define SPI_SETMOUSESPEED	113
#endif

static const int static_ws = WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
static const int scaled_ws = WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

static string app_title;
static string app_close;
static gxRuntime* runtime;
static bool busy, suspended;
static volatile bool run_flag;
static DDSURFACEDESC2 desktop_desc;

typedef int (_stdcall *LibFunc)(const void* in, int in_sz, void* out, int out_sz);

struct gxDll
{
	HINSTANCE hinst;
	map<string, LibFunc> funcs;
};

static map<string, gxDll*> libs;

static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

static Debugger* debugger;

static set<gxTimer*> timers;

enum
{
	WM_STOP = WM_APP + 1, WM_RUN, WM_END
};

////////////////////
// STATIC STARTUP //
////////////////////
gxRuntime* gxRuntime::openRuntime(HINSTANCE hinst, const string& cmd_line, Debugger* d)
{
	if (runtime) return 0;

	//create debugger
	debugger = d;

	//create WNDCLASS
	WNDCLASS wndclass;
	memset(&wndclass, 0, sizeof(wndclass));
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.lpfnWndProc = ::windowProc;
	wndclass.hInstance = hinst;
	wndclass.lpszClassName = "Blitz Runtime Class";
	wndclass.hCursor = (HCURSOR)LoadCursor(0,IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	RegisterClass(&wndclass);

	busy = suspended = false;
	run_flag = true;

	const char* app_t = " ";
	int ws = WS_CAPTION, ws_ex = 0;

	HWND hwnd = CreateWindowEx(ws_ex, "Blitz Runtime Class", app_t, ws, 0, 0, 0, 0, 0, 0, 0, 0);

	UpdateWindow(hwnd);

	runtime = d_new gxRuntime(hinst, cmd_line, hwnd);
	return runtime;
}

void gxRuntime::closeRuntime(gxRuntime* r)
{
	if (!runtime || runtime != r) return;

	map<string, gxDll*>::const_iterator it;
	for (it = libs.begin(); it != libs.end(); ++it)
	{
		FreeLibrary(it->second->hinst);
	}
	libs.clear();

	delete runtime;
	runtime = 0;
}

//////////////////////////
// RUNTIME CONSTRUCTION //
//////////////////////////
typedef int (_stdcall *SetAppCompatDataFunc)(int x, int y);

gxRuntime::gxRuntime(HINSTANCE hi, const string& cl, HWND hw):
		hinst(hi), cmd_line(cl), hwnd(hw),
		pointer_visible(true),
		fileSystem(0)
{
	CoInitialize(0);

	TIMECAPS tc;
	timeGetDevCaps(&tc, sizeof(tc));
	timeBeginPeriod(tc.wPeriodMin);

	memset(&osinfo, 0, sizeof(osinfo));
	osinfo.dwOSVersionInfoSize = sizeof(osinfo);
	GetVersionEx(&osinfo);
}

gxRuntime::~gxRuntime()
{
	while (timers.size()) freeTimer(*timers.begin());

	TIMECAPS tc;
	timeGetDevCaps(&tc, sizeof(tc));
	timeEndPeriod(tc.wPeriodMin);
	DestroyWindow(hwnd);
	UnregisterClass("Blitz Runtime Class", hinst);

	CoUninitialize();
}

/////////////
// SUSPEND //
/////////////
void gxRuntime::suspend()
{
	busy = true;

#if BB_BLITZ3D_ENABLED
	pauseAudio();
    backupGraphics();
    unacquireInput();
#endif

	suspended = true;
	busy = false;

	if (debugger) debugger->debugStop();
}

////////////
// RESUME //
////////////
void gxRuntime::resume()
{
	suspended = false;
	busy = false;

	if (debugger) debugger->debugRun();
}

///////////////////
// FORCE SUSPEND //
///////////////////
void gxRuntime::forceSuspend()
{
	suspend();
}

//////////////////
// FORCE RESUME //
//////////////////
void gxRuntime::forceResume()
{
	resume();
}

/////////////////
// WINDOW PROC //
/////////////////
LRESULT gxRuntime::windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (runtime) return runtime->windowProc(hwnd, msg, wparam, lparam);
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

//////////////////////////////
//STOP FROM EXTERNAL SOURCE //
//////////////////////////////
void gxRuntime::asyncStop()
{
	PostMessage(hwnd, WM_STOP, 0, 0);
}

//////////////////////////////
//RUN  FROM EXTERNAL SOURCE //
//////////////////////////////
void gxRuntime::asyncRun()
{
	PostMessage(hwnd, WM_RUN, 0, 0);
}

//////////////////////////////
// END FROM EXTERNAL SOURCE //
//////////////////////////////
void gxRuntime::asyncEnd()
{
	PostMessage(hwnd, WM_END, 0, 0);
}

//////////
// IDLE //
//////////
bool gxRuntime::idle()
{
	for (;;)
	{
		MSG msg;
		if (suspended && run_flag)
		{
			GetMessage(&msg, 0, 0, 0);
		}
		else
		{
			if (!PeekMessage(&msg, 0, 0, 0,PM_REMOVE)) return run_flag;
		}
		switch (msg.message)
		{
			case WM_STOP:
				if (!suspended) forceSuspend();
				break;
			case WM_RUN:
				if (suspended) forceResume();
				break;
			case WM_END:
				debugger = 0;
				run_flag = false;
				break;
			default:
				DispatchMessage(&msg);
		}
	}
	return run_flag;
}

///////////
// DELAY //
///////////
bool gxRuntime::delay(int ms)
{
	int t = timeGetTime() + ms;
	for (;;)
	{
		if (!idle()) return false;
		int d = t - timeGetTime(); //how long left to wait
		if (d <= 0) return true;
		if (d > 100) d = 100;
		Sleep(d);
	}
}

///////////////
// DEBUGSTMT //
///////////////
void gxRuntime::debugStmt(int pos, const char* file)
{
	if (debugger) debugger->debugStmt(pos, file);
}

///////////////
// DEBUGSTOP //
///////////////
void gxRuntime::debugStop()
{
	if (!suspended) forceSuspend();
}

////////////////
// DEBUGENTER //
////////////////
void gxRuntime::debugEnter(void* frame, void* env, const char* func)
{
	if (debugger) debugger->debugEnter(frame, env, func);
}

////////////////
// DEBUGLEAVE //
////////////////
void gxRuntime::debugLeave()
{
	if (debugger) debugger->debugLeave();
}

////////////////
// DEBUGERROR //
////////////////
void gxRuntime::debugError(const char* t)
{
	if (!debugger) return;
	Debugger* d = debugger;
	asyncEnd();
	if (!suspended)
	{
		forceSuspend();
	}
	d->debugMsg(t, true);
}

///////////////
// DEBUGINFO //
///////////////
void gxRuntime::debugInfo(const char* t)
{
	if (!debugger) return;
	Debugger* d = debugger;
	asyncEnd();
	if (!suspended)
	{
		forceSuspend();
	}
	d->debugMsg(t, false);
}

//////////////
// DEBUGLOG //
//////////////
void gxRuntime::debugLog(const char* t)
{
	if (debugger) debugger->debugLog(t);
}

/////////////////////////
// RETURN COMMAND LINE //
/////////////////////////
string gxRuntime::commandLine()
{
	return cmd_line;
}

/////////////
// EXECUTE //
/////////////
bool gxRuntime::execute(const string& cmd_line)
{
	if (!cmd_line.size()) return false;

	//convert cmd_line to cmd and params
	string cmd = cmd_line, params;
	while (cmd.size() && cmd[0] == ' ') cmd = cmd.substr(1);
	if (cmd.find('\"') == 0)
	{
		int n = cmd.find('\"', 1);
		if (n != string::npos)
		{
			params = cmd.substr(n + 1);
			cmd = cmd.substr(1, n - 1);
		}
	}
	else
	{
		int n = cmd.find(' ');
		if (n != string::npos)
		{
			params = cmd.substr(n + 1);
			cmd = cmd.substr(0, n);
		}
	}
	while (params.size() && params[0] == ' ') params = params.substr(1);
	while (params.size() && params[params.size() - 1] == ' ') params = params.substr(0, params.size() - 1);

	SetForegroundWindow(GetDesktopWindow());

	return (int)ShellExecute(GetDesktopWindow(), 0, cmd.c_str(), params.size() ? params.c_str() : 0, 0,SW_SHOW) > 32;
}

///////////////
// APP TITLE //
///////////////
void gxRuntime::setTitle(const string& t, const string& e)
{
	app_title = t;
	app_close = e;
	SetWindowText(hwnd, app_title.c_str());
}

//////////////////
// GETMILLISECS //
//////////////////
int gxRuntime::getMilliSecs()
{
	return timeGetTime();
}

/////////////////////
// POINTER VISIBLE //
/////////////////////
void gxRuntime::setPointerVisible(bool vis)
{
	if (pointer_visible == vis) return;

	pointer_visible = vis;

	//force a WM_SETCURSOR
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
}

gxFileSystem* gxRuntime::openFileSystem(int flags)
{
	if (fileSystem) return 0;

	fileSystem = d_new gxFileSystem();
	return fileSystem;
}

void gxRuntime::closeFileSystem(gxFileSystem* f)
{
	if (!fileSystem || fileSystem != f) return;

	delete fileSystem;
	fileSystem = 0;
}
gxTimer* gxRuntime::createTimer(int hertz)
{
	gxTimer* t = d_new gxTimer(this, hertz);
	timers.insert(t);
	return t;
}

void gxRuntime::freeTimer(gxTimer* t)
{
	if (!timers.count(t)) return;
	timers.erase(t);
	delete t;
}

static string toDir(string t)
{
	if (t.size() && t[t.size() - 1] != '\\') t += '\\';
	return t;
}

string gxRuntime::systemProperty(const std::string& p)
{
	char buff[MAX_PATH + 1];
	string t = tolower(p);
	if (t == "cpu")
	{
		return "Intel";
	}
	else if (t == "os")
	{
		switch (osinfo.dwMajorVersion)
		{
			case 3:
				switch (osinfo.dwMinorVersion)
				{
					case 51: return "Windows NT 3.1";
				}
				break;
			case 4:
				switch (osinfo.dwMinorVersion)
				{
					case 0: return "Windows 95";
					case 10: return "Windows 98";
					case 90: return "Windows ME";
				}
				break;
			case 5:
				switch (osinfo.dwMinorVersion)
				{
					case 0: return "Windows 2000";
					case 1: return "Windows XP";
					case 2: return "Windows Server 2003";
				}
				break;
			case 6:
				switch (osinfo.dwMinorVersion)
				{
					case 0: return "Windows Vista";
					case 1: return "Windows 7";
				}
				break;
		}
	}
	else if (t == "appdir")
	{
		if (GetModuleFileName(0, buff,MAX_PATH))
		{
			string t = buff;
			int n = t.find_last_of('\\');
			if (n != string::npos) t = t.substr(0, n);
			return toDir(t);
		}
	}
	else if (t == "apphwnd")
	{
		return itoa((int)hwnd);
	}
	else if (t == "apphinstance")
	{
		return itoa((int)hinst);
	}
	else if (t == "windowsdir")
	{
		if (GetWindowsDirectory(buff,MAX_PATH)) return toDir(buff);
	}
	else if (t == "systemdir")
	{
		if (GetSystemDirectory(buff,MAX_PATH)) return toDir(buff);
	}
	else if (t == "tempdir")
	{
		if (GetTempPath(MAX_PATH, buff)) return toDir(buff);
	}
	return "";
}


int gxRuntime::callDll(const std::string& dll, const std::string& func, const void* in, int in_sz, void* out,
					   int out_sz)
{
	map<string, gxDll*>::const_iterator lib_it = libs.find(dll);

	if (lib_it == libs.end())
	{
		HINSTANCE h = LoadLibrary(dll.c_str());
		if (!h) return 0;
		gxDll* t = d_new gxDll;
		t->hinst = h;
		lib_it = libs.insert(make_pair(dll, t)).first;
	}

	gxDll* t = lib_it->second;
	map<string, LibFunc>::const_iterator fun_it = t->funcs.find(func);

	if (fun_it == t->funcs.end())
	{
		LibFunc f = (LibFunc)GetProcAddress(t->hinst, func.c_str());
		if (!f) return 0;
		fun_it = t->funcs.insert(make_pair(func, f)).first;
	}

	static void* save_esp;

	_asm{
			mov [save_esp],esp
	};

	int n = fun_it->second(in, in_sz, out, out_sz);

	_asm{
			mov esp,[save_esp]
	};

	return n;
}
