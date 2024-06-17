
#ifndef GXRUNTIME_SGD_H
#define GXRUNTIME_SGD_H

#include <windows.h>
#include <string>
#include <vector>

#include "gxfilesystem.h"
#include "gxtimer.h"

#include "../debugger/debugger.h"

class gxRuntime{
public:

	HWND hwnd;
	HINSTANCE hinst;

	gxFileSystem *fileSystem;

	void moveMouse( int x,int y );

	LRESULT windowProc( HWND hwnd,UINT msg,WPARAM w,LPARAM l );

private:
	gxRuntime( HINSTANCE hinst,const std::string &cmd_line,HWND hwnd );
	~gxRuntime();

	void suspend();
	void forceSuspend();
	void resume();
	void forceResume();

	RECT t_rect;
	int t_style;
	std::string cmd_line;
	bool pointer_visible;
	std::string app_title;
	std::string app_close;

	/***** APP INTERFACE *****/
public:
	static gxRuntime *openRuntime( HINSTANCE hinst,const std::string &cmd_line,Debugger *debugger );
	static void closeRuntime( gxRuntime *runtime );

	void asyncStop();
	void asyncRun();
	void asyncEnd();

	/***** GX INTERFACE *****/
public:

	//return true if program should continue, or false for quit.
	bool idle();
	bool delay( int ms );

	bool execute( const std::string &cmd );
	void setTitle( const std::string &title,const std::string &close );
	int  getMilliSecs();
	void setPointerVisible( bool vis );

	std::string commandLine();

	std::string systemProperty( const std::string &t );

	void debugStop();
	void debugProfile( int per );
	void debugStmt( int pos,const char *file );
	void debugEnter( void *frame,void *env,const char *func );
	void debugLeave();
	void debugInfo( const char *t );
	void debugError( const char *t );
	void debugLog( const char *t );

	gxFileSystem *openFileSystem( int flags );
	void closeFileSystem( gxFileSystem *filesys );

	gxTimer *createTimer( int hertz );
	void freeTimer( gxTimer *timer );

	int callDll( const std::string &dll,const std::string &func,const void *in,int in_sz,void *out,int out_sz );

	OSVERSIONINFO osinfo;
};

#endif
