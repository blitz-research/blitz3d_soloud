
#include "stdafx.h"
#include "prefs.h"
#include "libs.h"
#include "resource.h"

#include <mmsystem.h>

char _credits[] =
	"\r\n"
	"Programming and design: Mark Sibly\r\n\r\n"
	"Documentation: Mark Sibly, Simon Harrison, Paul Gerfen, Shane Monroe and the Blitz Doc Team\r\n\r\n"
	"Testing and support: James Boyd, Simon Armstrong and the Blitz Dev Team\r\n\r\n"
	"FreeImage Image loader courtesy of Floris van den berg\r\n\r\n";

class Dialog : public CDialog {
	bool _quit;
public:
	Dialog() : _quit(false) {}

	afx_msg void OnOK() {
		_quit = true;
	}

	void wait() {
		_quit = false;
		MSG msg;
		while (!_quit && GetMessage(&msg, 0, 0, 0)) {
			if (!AfxGetApp()->PreTranslateMessage(&msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		EndDialog(0);
	}

	void wait(int n) {
		int _expire = (int) timeGetTime() + n;
		for (;;) {
			int tm = _expire - (int) timeGetTime();
			if (tm < 0) tm = 0;
			MsgWaitForMultipleObjects(0, 0, false, tm, QS_ALLEVENTS);

			MSG msg;
			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
				if (!AfxGetApp()->PreTranslateMessage(&msg)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			if (!tm) return;
		}
	}
};

void aboutBlitz(bool delay) {

	AfxGetMainWnd()->EnableWindow(0);

	Dialog about;

	about.Create(IDD_ABOUT);

	int bcc_ver=compiler_ver&0xffff;
	int ide_ver=VERSION&0xffff;
	int lnk_ver=linker_ver&0xffff;
	int run_ver=runtime_ver&0xffff;
	string bcc_v=itoa(ide_ver/1000)+"."+itoa(ide_ver%1000);
	string ide_v=itoa(ide_ver/1000)+"."+itoa(ide_ver%1000);
	string lnk_v=itoa(lnk_ver/1000)+"."+itoa(lnk_ver%1000);
	string run_v=itoa(run_ver/1000)+"."+itoa(run_ver%1000);

	string credits = _credits;

	if(runtime_ver>>16==3) {
		credits+="LibSGD Copyright Mark Sibly";
		run_v+=" (LibSGD Build)";
	}else if(runtime_ver>>16 == 2) {
		credits+="SoLoud Audio engine Copyright 2013-2018 Jari Komppa\r\n\r\n";
		run_v+=" (SoLoud Build)";
	}else if(runtime_ver>>16 == 1) {
		credits+="FMOD Audio engine Copyright Firelight Technologies Pty Ltd\r\n\r\n";
		run_v+=" (FMOD Build)";
	}

	about.GetDlgItem(IDC_CREDITS)->SetWindowText(credits.c_str());

	string t="Blitz3D IDE V"+ide_v;
	about.GetDlgItem( IDC_PRODUCT )->SetWindowText( t.c_str() );

	t="Compiler V"+bcc_v +" Linker V"+lnk_v;
	about.GetDlgItem(IDC_PRODUCT2)->SetWindowText(t.c_str());

	t="Runtime V"+run_v;
	about.GetDlgItem(IDC_VERSION)->SetWindowText(t.c_str());

	about.wait();
	about.EndDialog(0);
	AfxGetMainWnd()->EnableWindow(1);
}
