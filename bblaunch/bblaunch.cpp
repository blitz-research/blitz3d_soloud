
#include <string>
#include <windows.h>

using namespace std;

static const char *bb_err=
"Unable to run Blitz Basic";

static const char *md_err=
"Your desktop must be in high-colour mode to use Blitz Basic.\n\n"
"You can change your display settings from the control panel."; 

static string getAppDir(){
    char buff[MAX_PATH];
    if( GetModuleFileName( 0,buff,MAX_PATH ) ){
            string t=buff;
            int n=t.find_last_of( '\\' );
            if( n!=string::npos ) t=t.substr( 0,n );
            return t;
    }
    return "";
}

static void fail( const char *p ){
    ::MessageBox( 0,p,"Blitz Basic Error",MB_SETFOREGROUND|MB_TOPMOST|MB_ICONERROR );
    ExitProcess(-1);
}

static int desktopDepth(){
    HDC hdc=GetDC( GetDesktopWindow() );
    return GetDeviceCaps( hdc,BITSPIXEL );
}

int _stdcall WinMain( HINSTANCE inst,HINSTANCE prev,char *cmd,int show ){

    if( desktopDepth()<16 ) fail( md_err );

    //Ugly hack to get application dir...
    string t=getAppDir();
    putenv( ("blitzpath="+t).c_str() );
    SetCurrentDirectory( t.c_str() );
    t=t+"\\bin\\ide.exe "+cmd;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si,sizeof(si));si.cb=sizeof(si);
	if( !CreateProcess( 0,(char*)t.c_str(),0,0,0,0,0,0,&si,&pi ) ){
		::MessageBox( 0,bb_err,"Blitz Basic Error",MB_SETFOREGROUND|MB_TOPMOST|MB_ICONERROR );
		ExitProcess(-1);
	}

	//wait for BB to start
	WaitForInputIdle( pi.hProcess,INFINITE );

    // Close process and thread handles. 
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

    return 0;
}
