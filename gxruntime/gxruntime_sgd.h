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

	gxFileSystem *fileSystem{};

	static gxRuntime *openRuntime( HINSTANCE hinst,const std::string &cmd_line,Debugger *debugger );
	static void closeRuntime( gxRuntime *runtime );

	void asyncStop();
	void asyncRun();
	void asyncEnd();

	void suspend();
	void resume();

	bool idle();
	bool delay( int ms );

	bool execute( const std::string &cmd );
	std::string commandLine();
	std::string systemProperty( const std::string &t );

	void debugStop();
	void debugStmt( int pos,const char *file );
	void debugEnter( void *frame,void *env,const char *func );
	void debugLeave();
	void debugInfo( const char *t );
	void debugError( const char *t );
	void debugLog( const char *t );

	gxFileSystem *openFileSystem( int flags );
	void closeFileSystem( gxFileSystem *filesys );

	void setTitle( const std::string &t,const std::string &e );

	int  getMilliSecs();
	gxTimer *createTimer( int hertz );
	void freeTimer( gxTimer *timer );

	int callDll( const std::string &dll,const std::string &func,const void *in,int in_sz,void *out,int out_sz );
};

#endif
