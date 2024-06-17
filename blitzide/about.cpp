
#include "stdafx.h"
#include "prefs.h"
#include "libs.h"
#include "resource.h"

#include <mmsystem.h>

char _credits[]=
"\r\n"
"Programming and design: Mark Sibly\r\n\r\n"
"Documentation: Mark Sibly, Simon Harrison, Paul Gerfen, Shane Monroe and the Blitz Doc Team\r\n\r\n"
"Testing and support: James Boyd, Simon Armstrong and the Blitz Dev Team\r\n\r\n"
#if BB_FMOD_ENABLED
"FMOD Audio engine Copyright Firelight Technologies Pty Ltd\r\n\r\n"
#else
"SoLoud Audio engine Copyright (c) 2013-2018 Jari Komppa\r\n\r\n"
#endif
"FreeImage Image loader courtesy of Floris van den berg\r\n\r\n";
//"Please visit www.blitzbasic.com for all your Blitz related needs!";

class Dialog : public CDialog{
	bool _quit;
public:
	Dialog():_quit(false){}

	afx_msg void OnOK(){
		_quit=true;
	}

	void wait(){
		_quit=false;
		MSG msg;
		while( !_quit && GetMessage( &msg,0,0,0 ) ){
			if( !AfxGetApp()->PreTranslateMessage(&msg) ){
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
		EndDialog(0);
	}

	void wait( int n ){
		int _expire=(int)timeGetTime()+n;
		for(;;){
			int tm=_expire-(int)timeGetTime();
			if( tm<0 ) tm=0;
			MsgWaitForMultipleObjects( 0,0,false,tm,QS_ALLEVENTS );

			MSG msg;
			if( PeekMessage( &msg,0,0,0,PM_REMOVE ) ){
				if( !AfxGetApp()->PreTranslateMessage(&msg) ){
					TranslateMessage( &msg );
					DispatchMessage( &msg );
				}
			}
			if( !tm ) return;
		}
	}
};

void aboutBlitz( bool delay ){

	AfxGetMainWnd()->EnableWindow(0);

	Dialog about;

	about.Create( IDD_ABOUT );

	string credits;

	credits+=_credits;

	about.GetDlgItem( IDC_CREDITS )->SetWindowText( credits.c_str() );

        std::string t;

#if BB_LIBSGD_ENABLED
        t="LibSGD";
#elif BB_FMOD_ENABLED
        t="FMOD";
#else
        t="SoLoud";
#endif
	about.GetDlgItem( IDC_PRODUCT )->SetWindowText( ("Blitz3D ("+t+" Build)").c_str() );

        t="Version "+itoa(VERSION/1000)+"."+itoa(VERSION%1000);

	about.GetDlgItem( IDC_VERSION )->SetWindowText( t.c_str() );

	about.GetDlgItem( IDC_PROGRESS1 )->ShowWindow( SW_HIDE );
	about.wait();
	about.EndDialog(0);
	AfxGetMainWnd()->EnableWindow(1);
}
