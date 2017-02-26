#include "pch.h"

#include <wrl.h>
#include <string>
#include <DXGI.h>
#include <ppltasks.h>
#include "../SDLPal.Common/AsyncHelper.h"
#include "../SDLPal.Common/StringHelper.h"
#include "../../src/main.h"

#include "SDL.h"
#include "SDL_endian.h"

static std::string g_basepath, g_configpath;

extern HANDLE g_eventHandle;

extern "C"
LPCSTR UTIL_BasePath(VOID)
{
	if (g_basepath.empty())
	{
		auto mru_list = Windows::Storage::AccessCache::StorageApplicationPermissions::MostRecentlyUsedList;
		for each (auto entry in mru_list->Entries)
		{
			if (dynamic_cast<Windows::Storage::StorageFolder^>(AWait(mru_list->GetItemAsync(entry.Token), g_eventHandle)) != nullptr)
			{
				auto localfolder = entry.Metadata;
				if (localfolder->End()[-1] != L'\\') localfolder += "\\";
				ConvertString(localfolder, g_basepath);
				break;
			}
		}
	}
	return g_basepath.c_str();
}

extern "C"
LPCSTR UTIL_SavePath(VOID)
{
	return UTIL_BasePath();
}

extern "C"
LPCSTR UTIL_ConfigPath(VOID)
{
	if (g_configpath.empty())
	{
		auto localfolder = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
		if (localfolder->End()[-1] != L'\\') localfolder += "\\";
		ConvertString(localfolder, g_configpath);
	}
	return g_configpath.c_str();
}

extern "C"
LPCSTR UTIL_ScreenShotPath(VOID)
{
	return gConfig.pszGamePath;
}

extern "C"
BOOL UTIL_GetScreenSize(DWORD *pdwScreenWidth, DWORD *pdwScreenHeight)
{
	DXGI_OUTPUT_DESC desc;
	IDXGIFactory1* pFactory = nullptr;
	IDXGIAdapter1* pAdapter = nullptr;
	IDXGIOutput* pOutput = nullptr;
	DWORD retval = FALSE;

#if NTDDI_VERSION >= NTDDI_WIN10
	if (Windows::System::Profile::AnalyticsInfo::VersionInfo->DeviceFamily != L"Windows.Mobile") return FALSE;
#endif

	if (!pdwScreenWidth || !pdwScreenHeight) return FALSE;

	if (FAILED(CreateDXGIFactory1(IID_IDXGIFactory1, (void**)&pFactory))) goto UTIL_WP_GetScreenSize_exit;

	if (FAILED(pFactory->EnumAdapters1(0, &pAdapter))) goto UTIL_WP_GetScreenSize_exit;

	if (FAILED(pAdapter->EnumOutputs(0, &pOutput))) goto UTIL_WP_GetScreenSize_exit;

	if (SUCCEEDED(pOutput->GetDesc(&desc)))
	{
#if (NTDDI_VERSION < NTDDI_WIN10) && (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP)
		*pdwScreenWidth = (desc.DesktopCoordinates.right - desc.DesktopCoordinates.left);
		*pdwScreenHeight = (desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top);
#else
		*pdwScreenWidth = (desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top);
		*pdwScreenHeight = (desc.DesktopCoordinates.right - desc.DesktopCoordinates.left);
#endif
		retval = TRUE;
	}

UTIL_WP_GetScreenSize_exit:
	if (pOutput) pOutput->Release();
	if (pAdapter) pAdapter->Release();
	if (pFactory) pFactory->Release();

	return retval;
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

extern "C"
BOOL UTIL_TouchEnabled(VOID)
{
	return (ref new Windows::Devices::Input::TouchCapabilities())->TouchPresent;
}

static Windows::Storage::StorageFile^ g_running_file = nullptr;

static void CreateRunningFile()
{
	// Create the 'running' file for crash detection.
	try { g_running_file = AWait(Windows::Storage::ApplicationData::Current->LocalFolder->CreateFileAsync("running", Windows::Storage::CreationCollisionOption::OpenIfExists), g_eventHandle); }
	catch (Platform::Exception^) {}
}

static void DeleteRunningFile()
{
	// Delete the 'running' file on normal exit.
	try { if (g_running_file) AWait(g_running_file->DeleteAsync()); g_running_file = nullptr; }
	catch (Platform::Exception^) {}
}

static int SDLCALL WinRT_EventFilter(void *userdata, SDL_Event * event)
{
	switch (event->type)
	{
	case SDL_APP_DIDENTERFOREGROUND:
		CreateRunningFile();
		break;
	case SDL_APP_DIDENTERBACKGROUND:
	case SDL_APP_TERMINATING:
		// Enter background or exiting, treat as normal exit
		DeleteRunningFile();
		break;
	}
	return 0;
}

static int input_event_filter(const SDL_Event *lpEvent, PALINPUTSTATE *state)
{
	if (lpEvent->type == SDL_KEYDOWN &&
		lpEvent->key.keysym.sym == SDLK_AC_BACK &&
		!gpGlobals->fInMainGame)
	{
		PAL_Shutdown(0);
	}
	return 0;
}

extern "C"
INT UTIL_Platform_Init(int argc, char* argv[])
{
	CreateRunningFile();

	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeRight");
	SDL_SetHint(SDL_HINT_WINRT_HANDLE_BACK_BUTTON, "1");

	SDL_AddEventWatch(WinRT_EventFilter, nullptr);

	return 0;
}

extern "C"
VOID UTIL_Platform_Quit(VOID)
{
	DeleteRunningFile();
}
