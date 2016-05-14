#define UNICODE
#define _UNICODE
#define _CRT_SECURE_NO_WARNINGS

#include <tchar.h>
#include <Windows.h>
#include <CommCtrl.h>
#include <string>
#include "resource.h"
#include "../global.h"
#include "../util.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define ComboBox_AddString(hwndDlg, idCtrl, lpsz) \
            (BOOL)SNDMSG(GetDlgItem((hwndDlg), (idCtrl)), CB_ADDSTRING, (WPARAM)(0), (LPARAM)(lpsz))
#define ComboBox_SetCurSel(hwndDlg, idCtrl, index) \
            (BOOL)SNDMSG(GetDlgItem((hwndDlg), (idCtrl)), CB_SETCURSEL, (WPARAM)(index), (LPARAM)(0))
#define ComboBox_GetCurSel(hwndDlg, idCtrl) \
            (int)SNDMSG(GetDlgItem((hwndDlg), (idCtrl)), CB_GETCURSEL, (WPARAM)(0), (LPARAM)(0))
#define TrackBar_SetRange(hwndDlg, idCtrl, min, max, redraw) \
            (BOOL)SNDMSG(GetDlgItem((hwndDlg), (idCtrl)), TBM_SETRANGE, (WPARAM)(redraw), (LPARAM)(MAKELONG((min), (max))))
#define TrackBar_SetPos(hwndDlg, idCtrl, pos, redraw) \
            (BOOL)SNDMSG(GetDlgItem((hwndDlg), (idCtrl)), TBM_SETPOS, (WPARAM)(redraw), (LPARAM)(pos))
#define TrackBar_GetPos(hwndDlg, idCtrl) \
            (int)SNDMSG(GetDlgItem((hwndDlg), (idCtrl)), TBM_GETPOS, (WPARAM)(0), (LPARAM)(0))
#define EnableDlgItem(hwnd, nIDControl, bEnable) \
			EnableWindow(GetDlgItem((hwnd), (nIDControl)), (bEnable))

void SaveSettings(HWND hwndDlg, BOOL fWriteFile)
{
	if (fWriteFile)
	{
		std::string msgfile;
		char buffer[40];
		int len = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MSGFILE)) + 1;
		msgfile.assign(len, ' ');
		GetDlgItemTextA(hwndDlg, IDC_MSGFILE, (char*)msgfile.data(), len);

		FILE *fp = UTIL_OpenFileForMode("sdlpal.cfg", "w");
		fprintf(fp, "DOS=%d\n", IsDlgButtonChecked(hwndDlg, IDC_DOS));
		if (IsDlgButtonChecked(hwndDlg, IDC_DOS))
			fprintf(fp, "UseEmbeddedFonts=%d\n", IsDlgButtonChecked(hwndDlg, IDC_EMBEDFONT));
		else if (!IsDlgButtonChecked(hwndDlg, IDC_CUSTOM))
			fprintf(fp, "CodePage=%d\n", IsDlgButtonChecked(hwndDlg, IDC_CHS));
		if (IsDlgButtonChecked(hwndDlg, IDC_CUSTOM))
			fprintf(fp, "MessageFileName=%s\n", msgfile.c_str());
		GetDlgItemTextA(hwndDlg, IDC_CD, buffer, 40); fprintf(fp, "CD=%s\n", buffer);
		GetDlgItemTextA(hwndDlg, IDC_BGM, buffer, 40); fprintf(fp, "MUSIC=%s\n", buffer);
		if (ComboBox_GetCurSel(hwndDlg, IDC_BGM) == MUSIC_RIX)
		{
			GetDlgItemTextA(hwndDlg, IDC_OPL, buffer, 40); fprintf(fp, "OPL=%s\n", buffer);
			fprintf(fp, "UseSurroundOPL=%d\n", IsDlgButtonChecked(hwndDlg, IDC_SURROUNDOPL));
			fprintf(fp, "OPLSampleRate=%u\n", GetDlgItemInt(hwndDlg, IDC_OPLSR, nullptr, FALSE));
			fprintf(fp, "SurroundOPLOffset=%d\n", GetDlgItemInt(hwndDlg, IDC_OPLOFFSET, nullptr, TRUE));
		}
		fprintf(fp, "Stereo=%d\n", IsDlgButtonChecked(hwndDlg, IDC_STEREO));
		fprintf(fp, "ResampleQuality=%d\n", TrackBar_GetPos(hwndDlg, IDC_QUALITY));
		fprintf(fp, "Volume=%d\n", TrackBar_GetPos(hwndDlg, IDC_VOLUME));
		fprintf(fp, "AudioBufferSize=%u\n", GetDlgItemInt(hwndDlg, IDC_AUDIOBUFFER, nullptr, FALSE));
		fprintf(fp, "SampleRate=%u\n", GetDlgItemInt(hwndDlg, IDC_SAMPLERATE, nullptr, FALSE));
		fprintf(fp, "WindowWidth=%u\n", GetDlgItemInt(hwndDlg, IDC_WIDTH, nullptr, FALSE));
		fprintf(fp, "WindowHeight=%u\n", GetDlgItemInt(hwndDlg, IDC_HEIGHT, nullptr, FALSE));
		fprintf(fp, "KeepAspectRatio=%d\n", IsDlgButtonChecked(hwndDlg, IDC_ASPECTRATIO));
		fclose(fp);
	}
	else
	{
		PAL_LoadConfig(FALSE);

		gConfig.fIsWIN95 = !IsDlgButtonChecked(hwndDlg, IDC_DOS);
		gConfig.fUseEmbeddedFonts = !gConfig.fIsWIN95 && IsDlgButtonChecked(hwndDlg, IDC_EMBEDFONT);
		if (!IsDlgButtonChecked(hwndDlg, IDC_CUSTOM))
		{
			if (IsDlgButtonChecked(hwndDlg, IDC_CHS) && gConfig.fIsWIN95)
				gConfig.iCodePage = CP_GBK;
			else
				gConfig.iCodePage = CP_BIG5;
			free(gConfig.pszMsgName);
			gConfig.pszMsgName = nullptr;
		}
		else
		{
			int length = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MSGFILE));
			gConfig.pszMsgName = (char*)realloc(gConfig.pszMsgName, length + 1);
			GetDlgItemTextA(hwndDlg, IDC_MSGFILE, gConfig.pszMsgName, length + 1);
		}
		gConfig.fKeepAspectRatio = IsDlgButtonChecked(hwndDlg, IDC_ASPECTRATIO);
		gConfig.dwScreenWidth = GetDlgItemInt(hwndDlg, IDC_WIDTH, nullptr, FALSE);
		gConfig.dwScreenHeight = GetDlgItemInt(hwndDlg, IDC_HEIGHT, nullptr, FALSE);
		gConfig.eCDType = (MUSICTYPE)(ComboBox_GetCurSel(hwndDlg, IDC_CD) + MUSIC_MP3);
		gConfig.eMusicType = (MUSICTYPE)ComboBox_GetCurSel(hwndDlg, IDC_BGM);
		gConfig.eOPLType = (OPLTYPE)(ComboBox_GetCurSel(hwndDlg, IDC_OPL) + OPL_DOSBOX_OLD);
		gConfig.iAudioChannels = IsDlgButtonChecked(hwndDlg, IDC_STEREO) ? 2 : 1;
		gConfig.iSampleRate = GetDlgItemInt(hwndDlg, IDC_SAMPLERATE, nullptr, FALSE);
		gConfig.wAudioBufferSize = GetDlgItemInt(hwndDlg, IDC_AUDIOBUFFER, nullptr, FALSE);
		gConfig.iVolume = TrackBar_GetPos(hwndDlg, IDC_VOLUME);
		gConfig.iResampleQuality = TrackBar_GetPos(hwndDlg, IDC_QUALITY);
		if (gConfig.eMusicType == MUSIC_RIX)
		{
			gConfig.fUseSurroundOPL = IsDlgButtonChecked(hwndDlg, IDC_SURROUNDOPL);
			gConfig.iOPLSampleRate = GetDlgItemInt(hwndDlg, IDC_OPLSR, nullptr, FALSE);
			gConfig.dSurroundOPLOffset = GetDlgItemInt(hwndDlg, IDC_OPLOFFSET, nullptr, TRUE);
		}
	}
}

void ResetControls(HWND hwndDlg)
{
	TCHAR buffer[100];

	EnableDlgItem(hwndDlg, IDC_CHS, gConfig.fIsWIN95);
	EnableDlgItem(hwndDlg, IDC_EMBEDFONT, !gConfig.fIsWIN95);
	EnableDlgItem(hwndDlg, IDC_MSGFILE, gConfig.pszMsgName ? TRUE : FALSE);
	EnableDlgItem(hwndDlg, IDC_OPL, gConfig.eMusicType == MUSIC_RIX);
	EnableDlgItem(hwndDlg, IDC_SURROUNDOPL, gConfig.eMusicType == MUSIC_RIX);
	EnableDlgItem(hwndDlg, IDC_OPLOFFSET, gConfig.eMusicType == MUSIC_RIX);
	EnableDlgItem(hwndDlg, IDC_OPLSR, gConfig.eMusicType == MUSIC_RIX);

	CheckRadioButton(hwndDlg, IDC_DOS, IDC_WIN95, gConfig.fIsWIN95 ? IDC_WIN95 : IDC_DOS);
	CheckRadioButton(hwndDlg, IDC_CHT, IDC_CUSTOM, gConfig.pszMsgName ? IDC_CUSTOM : IDC_CHT + gConfig.iCodePage);

	CheckDlgButton(hwndDlg, IDC_EMBEDFONT, gConfig.fUseEmbeddedFonts);
	CheckDlgButton(hwndDlg, IDC_ASPECTRATIO, gConfig.fKeepAspectRatio);
	CheckDlgButton(hwndDlg, IDC_SURROUNDOPL, gConfig.fUseSurroundOPL);
	CheckDlgButton(hwndDlg, IDC_STEREO, gConfig.iAudioChannels == 2);

	ComboBox_SetCurSel(hwndDlg, IDC_CD, gConfig.eCDType - MUSIC_MP3);
	ComboBox_SetCurSel(hwndDlg, IDC_BGM, gConfig.eMusicType);
	ComboBox_SetCurSel(hwndDlg, IDC_OPL, gConfig.eOPLType - OPL_DOSBOX);

	SetDlgItemText(hwndDlg, IDC_WIDTH, _ultot(gConfig.dwScreenWidth, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_HEIGHT, _ultot(gConfig.dwScreenHeight, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_SAMPLERATE, _itot(gConfig.iSampleRate, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_OPLSR, _itot(gConfig.iOPLSampleRate, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_OPLOFFSET, _itot((int)gConfig.dSurroundOPLOffset, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_AUDIOBUFFER, _itot(gConfig.wAudioBufferSize, buffer, 10));

	SetDlgItemTextA(hwndDlg, IDC_MSGFILE, gConfig.pszMsgName);

	TrackBar_SetRange(hwndDlg, IDC_QUALITY, 0, 4, FALSE);
	TrackBar_SetPos(hwndDlg, IDC_QUALITY, gConfig.iResampleQuality, TRUE);
	TrackBar_SetRange(hwndDlg, IDC_VOLUME, 0, 100, FALSE);
	TrackBar_SetPos(hwndDlg, IDC_VOLUME, gConfig.iVolume, TRUE);
}

INT_PTR InitProc(HWND hwndDlg, HWND hwndCtrl, LPARAM lParam)
{
	TCHAR curdir[MAX_PATH * 2];
	GetCurrentDirectory(MAX_PATH * 2, curdir);

	InitCommonControls();

	ComboBox_AddString(hwndDlg, IDC_CD, TEXT("MP3"));
	ComboBox_AddString(hwndDlg, IDC_CD, TEXT("OGG"));

	ComboBox_AddString(hwndDlg, IDC_BGM, TEXT("RIX"));
	ComboBox_AddString(hwndDlg, IDC_BGM, TEXT("MIDI"));
	ComboBox_AddString(hwndDlg, IDC_BGM, TEXT("MP3"));
	ComboBox_AddString(hwndDlg, IDC_BGM, TEXT("OGG"));

	ComboBox_AddString(hwndDlg, IDC_OPL, TEXT("DOSBOX"));
	ComboBox_AddString(hwndDlg, IDC_OPL, TEXT("MAME"));

	SetDlgItemText(hwndDlg, IDC_GAMEPATH, curdir);

	ResetControls(hwndDlg);

	WINDOWINFO wi = { sizeof(WINDOWINFO) };
	RECT rcWork;
	if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0) && GetWindowInfo(hwndDlg, &wi))
	{
		int x = ((rcWork.right - rcWork.left) - (wi.rcWindow.right - wi.rcWindow.left)) / 2;
		int y = ((rcWork.bottom - rcWork.top) - (wi.rcWindow.bottom - wi.rcWindow.top)) / 2;
		SetWindowPos(hwndDlg, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
	}

	return TRUE;
}

INT_PTR ButtonProc(HWND hwndDlg, WORD idControl, HWND hwndCtrl)
{
	switch (idControl)
	{
	case IDOK:
		SaveSettings(hwndDlg, FALSE);
		EndDialog(hwndDlg, IDOK);
		return TRUE;
	case IDCANCEL:
		EndDialog(hwndDlg, IDCANCEL);
		return TRUE;

	case IDC_SAVE:
		SaveSettings(hwndDlg, TRUE);
		return TRUE;
	case IDC_DEFAULT:
		PAL_LoadConfig(FALSE);
		ResetControls(hwndDlg);
		return TRUE;

	case IDC_DOS:
	case IDC_WIN95:
		if (IsDlgButtonChecked(hwndDlg, IDC_DOS))
		{
			EnableDlgItem(hwndDlg, IDC_EMBEDFONT, TRUE);
			EnableDlgItem(hwndDlg, IDC_CHS, FALSE);
		}
		else
		{
			EnableDlgItem(hwndDlg, IDC_EMBEDFONT, FALSE);
			EnableDlgItem(hwndDlg, IDC_CHS, TRUE);
		}
		return TRUE;

	case IDC_CHT:
	case IDC_CHS:
	case IDC_CUSTOM:
		EnableDlgItem(hwndDlg, IDC_MSGFILE, IsDlgButtonChecked(hwndDlg, IDC_CUSTOM));
		return TRUE;

	default: return FALSE;
	}
}

INT_PTR ComboBoxProc(HWND hwndDlg, WORD idControl, HWND hwndCtrl)
{
	switch (idControl)
	{
	case IDC_BGM:
		EnableDlgItem(hwndDlg, IDC_OPL, ComboBox_GetCurSel(hwndDlg, IDC_BGM) == MUSIC_RIX);
		EnableDlgItem(hwndDlg, IDC_SURROUNDOPL, ComboBox_GetCurSel(hwndDlg, IDC_BGM) == MUSIC_RIX);
		EnableDlgItem(hwndDlg, IDC_OPLOFFSET, ComboBox_GetCurSel(hwndDlg, IDC_BGM) == MUSIC_RIX);
		EnableDlgItem(hwndDlg, IDC_OPLSR, ComboBox_GetCurSel(hwndDlg, IDC_BGM) == MUSIC_RIX);
		return TRUE;

	default:
		return FALSE;
	}
}

INT_PTR CommandProc(HWND hwndDlg, WORD command, WORD idControl, HWND hwndCtrl)
{
	switch (command)
	{
	case BN_CLICKED: return ButtonProc(hwndDlg, idControl, hwndCtrl);
	case CBN_SELCHANGE: return ComboBoxProc(hwndDlg, idControl, hwndCtrl);
	default: return FALSE;
	}
}

INT_PTR CALLBACK LauncherDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG: return InitProc(hwndDlg, (HWND)wParam, lParam);
	case WM_COMMAND: return CommandProc(hwndDlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
	case WM_CLOSE: return EndDialog(hwndDlg, 0);
	default: return FALSE;
	}
}

extern "C" int UTIL_Platform_Init(int argc, char* argv[])
{
	auto module = GetModuleHandle(nullptr);
	auto lang = GetThreadUILanguage();
	if (PRIMARYLANGID(lang) == LANG_CHINESE)
	{
		if (SUBLANGID(lang) == SUBLANG_CHINESE_SIMPLIFIED || SUBLANGID(lang) == SUBLANG_CHINESE_SINGAPORE)
			lang = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
		else
			lang = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
	}
	else
		lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
	auto dlg = (LPCDLGTEMPLATE)LockResource(LoadResource(module, FindResourceEx(module, RT_DIALOG, MAKEINTRESOURCE(IDD_LAUNCHER), lang)));
	if (DialogBoxIndirect(GetModuleHandle(nullptr), dlg, nullptr, LauncherDialogProc) != IDOK)
		return -1;
	else
		return 0;
}

extern "C" VOID UTIL_Platform_Quit(VOID) {}

extern "C" BOOL
UTIL_GetScreenSize(
	DWORD *pdwScreenWidth,
	DWORD *pdwScreenHeight
)
{
	return (pdwScreenWidth && pdwScreenHeight && *pdwScreenWidth && *pdwScreenHeight);
}
