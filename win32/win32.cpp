#define UNICODE
#define _UNICODE
#define _CRT_SECURE_NO_WARNINGS

#include <tchar.h>
#include <Windows.h>
#include <CommCtrl.h>
#include <ShlObj.h>
#include <string>
#include "resource.h"
#include "../global.h"
#include "../util.h"
#include "../palcfg.h"
#include "../resampler.h"

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

HINSTANCE g_hInstance;
WORD g_wLanguage;

int WINAPI LoadStringEx(
	HINSTANCE hInstance,
	UINT      uID,
	LANGID    wLang,
	LPTSTR    lpBuffer,
	int       nBufferMax
)
{
	auto hrc = FindResourceEx(hInstance, RT_STRING, MAKEINTRESOURCE((uID >> 4) + 1), wLang);
	if (nullptr == hrc) return 0;

	auto begin = (LPCWSTR)LockResource(LoadResource(hInstance, hrc));
	for (int idx = 0; idx < (int)(uID & 0xf); idx++)
		begin += *begin + 1;
	if (nBufferMax == 0)
	{
		*((LPCWSTR*)lpBuffer) = begin;
		return sizeof(LPCWSTR);
	}
	else
	{
		wcsncpy(lpBuffer, begin + 1, min(nBufferMax, *begin));
		if (nBufferMax <= *begin)
		{
			lpBuffer[nBufferMax - 1] = '\0';
			return nBufferMax - 1;
		}
		else
		{
			lpBuffer[*begin] = '\0';
			return *begin;
		}
	}
}

void SaveSettings(HWND hwndDlg, BOOL fWriteFile)
{
	int textLen;

	if (IsDlgButtonChecked(hwndDlg, IDC_CHS))
		gConfig.uCodePage = CP_GBK;
	else
		gConfig.uCodePage = CP_BIG5;

	if ((textLen = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MSGFILE))) > 0)
	{
		gConfig.pszMsgFile = (char*)realloc(gConfig.pszMsgFile, textLen + 1);
		GetDlgItemTextA(hwndDlg, IDC_MSGFILE, gConfig.pszMsgFile, textLen + 1);
	}
	else
	{
		free(gConfig.pszMsgFile); gConfig.pszMsgFile = nullptr;
	}
	if ((textLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_GAMEPATH))) > 0)
	{
		gConfig.pszGamePath = (char*)realloc(gConfig.pszGamePath, textLen + 1);
		GetDlgItemTextA(hwndDlg, IDC_GAMEPATH, gConfig.pszGamePath, textLen + 1);
	}
	else
	{
		free(gConfig.pszGamePath); gConfig.pszGamePath = nullptr;
	}

	gConfig.fFullScreen = IsDlgButtonChecked(hwndDlg, IDC_FULLSCREEN);
	gConfig.fUseTouchOverlay = IsDlgButtonChecked(hwndDlg, IDC_TOUCHOVERLAY);
	gConfig.fUseEmbeddedFonts = IsDlgButtonChecked(hwndDlg, IDC_EMBEDFONT);
	gConfig.fKeepAspectRatio = IsDlgButtonChecked(hwndDlg, IDC_ASPECTRATIO);
	gConfig.eCDType = (MUSICTYPE)(ComboBox_GetCurSel(hwndDlg, IDC_CD) + MUSIC_MP3);
	gConfig.eMusicType = (MUSICTYPE)ComboBox_GetCurSel(hwndDlg, IDC_BGM);
	gConfig.eOPLType = (OPLTYPE)(ComboBox_GetCurSel(hwndDlg, IDC_OPL));
	gConfig.iAudioChannels = IsDlgButtonChecked(hwndDlg, IDC_STEREO) ? 2 : 1;
	gConfig.iSampleRate = GetDlgItemInt(hwndDlg, IDC_SAMPLERATE, nullptr, FALSE);
	gConfig.wAudioBufferSize = GetDlgItemInt(hwndDlg, IDC_AUDIOBUFFER, nullptr, FALSE);
	gConfig.iMusicVolume = TrackBar_GetPos(hwndDlg, IDC_MUSICVOLUME);
	gConfig.iSoundVolume = TrackBar_GetPos(hwndDlg, IDC_SOUNDVOLUME);
	gConfig.iResampleQuality = TrackBar_GetPos(hwndDlg, IDC_QUALITY);
	if (gConfig.eMusicType == MUSIC_RIX)
	{
		gConfig.fUseSurroundOPL = IsDlgButtonChecked(hwndDlg, IDC_SURROUNDOPL);
		gConfig.iOPLSampleRate = GetDlgItemInt(hwndDlg, IDC_OPLSR, nullptr, FALSE);
	}

	if (fWriteFile) PAL_SaveConfig();
}

void ResetControls(HWND hwndDlg)
{
	TCHAR buffer[100];

	EnableDlgItem(hwndDlg, IDC_OPL, gConfig.eMusicType == MUSIC_RIX);
	EnableDlgItem(hwndDlg, IDC_SURROUNDOPL, gConfig.eMusicType == MUSIC_RIX);
	EnableDlgItem(hwndDlg, IDC_OPLSR, gConfig.eMusicType == MUSIC_RIX);

	CheckRadioButton(hwndDlg, IDC_CHT, IDC_CHS, IDC_CHT + gConfig.uCodePage);

	CheckDlgButton(hwndDlg, IDC_FULLSCREEN, gConfig.fFullScreen);
	CheckDlgButton(hwndDlg, IDC_TOUCHOVERLAY, gConfig.fUseTouchOverlay);
	CheckDlgButton(hwndDlg, IDC_EMBEDFONT, gConfig.fUseEmbeddedFonts);
	CheckDlgButton(hwndDlg, IDC_ASPECTRATIO, gConfig.fKeepAspectRatio);
	CheckDlgButton(hwndDlg, IDC_SURROUNDOPL, gConfig.fUseSurroundOPL);
	CheckDlgButton(hwndDlg, IDC_STEREO, gConfig.iAudioChannels == 2);

	ComboBox_SetCurSel(hwndDlg, IDC_CD, gConfig.eCDType - MUSIC_MP3);
	ComboBox_SetCurSel(hwndDlg, IDC_BGM, gConfig.eMusicType);
	ComboBox_SetCurSel(hwndDlg, IDC_OPL, gConfig.eOPLType);

	SetDlgItemText(hwndDlg, IDC_SAMPLERATE, _itot(gConfig.iSampleRate, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_OPLSR, _itot(gConfig.iOPLSampleRate, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_AUDIOBUFFER, _itot(gConfig.wAudioBufferSize, buffer, 10));

	if (gConfig.pszGamePath) SetDlgItemTextA(hwndDlg, IDC_GAMEPATH, gConfig.pszGamePath);
	if (gConfig.pszMsgFile) SetDlgItemTextA(hwndDlg, IDC_MSGFILE, gConfig.pszMsgFile);

	TrackBar_SetPos(hwndDlg, IDC_QUALITY, gConfig.iResampleQuality, TRUE);
	TrackBar_SetPos(hwndDlg, IDC_MUSICVOLUME, gConfig.iMusicVolume, TRUE);
	TrackBar_SetPos(hwndDlg, IDC_SOUNDVOLUME, gConfig.iSoundVolume, TRUE);
}

INT_PTR InitProc(HWND hwndDlg, HWND hwndCtrl, LPARAM lParam)
{
	InitCommonControls();

	ComboBox_AddString(hwndDlg, IDC_CD, TEXT("MP3"));
	ComboBox_AddString(hwndDlg, IDC_CD, TEXT("OGG"));

	ComboBox_AddString(hwndDlg, IDC_BGM, TEXT("MIDI"));
	ComboBox_AddString(hwndDlg, IDC_BGM, TEXT("RIX"));
	ComboBox_AddString(hwndDlg, IDC_BGM, TEXT("MP3"));
	ComboBox_AddString(hwndDlg, IDC_BGM, TEXT("OGG"));

	ComboBox_AddString(hwndDlg, IDC_OPL, TEXT("DOSBOX"));
	ComboBox_AddString(hwndDlg, IDC_OPL, TEXT("MAME"));
	ComboBox_AddString(hwndDlg, IDC_OPL, TEXT("DOSBOXNEW"));

	TrackBar_SetRange(hwndDlg, IDC_QUALITY, RESAMPLER_QUALITY_MIN, RESAMPLER_QUALITY_MAX, FALSE);
	TrackBar_SetRange(hwndDlg, IDC_MUSICVOLUME, 0, PAL_MAX_VOLUME, FALSE);
	TrackBar_SetRange(hwndDlg, IDC_SOUNDVOLUME, 0, PAL_MAX_VOLUME, FALSE);

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
		gConfig.fLaunchSetting = FALSE;
		SaveSettings(hwndDlg, TRUE);
		EndDialog(hwndDlg, IDOK);
		return TRUE;

	case IDCANCEL:
		EndDialog(hwndDlg, IDCANCEL);
		return TRUE;

	case IDC_DEFAULT:
		PAL_LoadConfig(FALSE);
		ResetControls(hwndDlg);
		return TRUE;

	case IDC_BRGAME:
	{
		TCHAR szName[MAX_PATH * 2], szTitle[200];
		BROWSEINFO bi = { hwndDlg, nullptr, szName, szTitle, BIF_USENEWUI, nullptr, NULL, 0 };
		LoadStringEx(g_hInstance, idControl, g_wLanguage, szTitle, 200);
		auto pidl = SHBrowseForFolder(&bi);
		if (pidl)
		{
			SHGetPathFromIDList(pidl, szName);
			int n = _tcslen(szName);
			if (szName[n - 1] != '\\') _tcscat(szName, L"\\");
			SetDlgItemText(hwndDlg, IDC_GAMEPATH, szName);
		}
		return TRUE;
	}

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
	g_hInstance = GetModuleHandle(nullptr);
#ifndef __MINGW32__ // mingw's windows sdk dont support GetThreadUILanguage, since it's a vista+ API
	g_wLanguage = GetThreadUILanguage();
	if (PRIMARYLANGID(g_wLanguage) == LANG_CHINESE)
	{
		if (SUBLANGID(g_wLanguage) == SUBLANG_CHINESE_SIMPLIFIED || SUBLANGID(g_wLanguage) == SUBLANG_CHINESE_SINGAPORE)
			g_wLanguage = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
		else
			g_wLanguage = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
	}
	else
#endif
		g_wLanguage = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
	auto dlg = (LPCDLGTEMPLATE)LockResource(LoadResource(g_hInstance, FindResourceEx(g_hInstance, RT_DIALOG, MAKEINTRESOURCE(IDD_LAUNCHER), g_wLanguage)));
	if (gConfig.fLaunchSetting && DialogBoxIndirect(GetModuleHandle(nullptr), dlg, nullptr, LauncherDialogProc) != IDOK)
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

extern "C"
BOOL UTIL_IsAbsolutePath(LPCSTR  lpszFileName)
{
	char szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szFname[_MAX_FNAME], szExt[_MAX_EXT];
	if (_splitpath_s(lpszFileName, szDrive, szDir, szFname, szExt) == 0)
		return (strlen(szDrive) > 0 && (szDir[0] == '\\' || szDir[0] == '/'));
	else
		return FALSE;
}
