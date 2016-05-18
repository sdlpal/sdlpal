#include "pch.h"

#include <wrl.h>
#include <string>
#include <DXGI.h>
#include <ppltasks.h>
#include "../SDLPal.Common/AsyncHelper.h"
#include "../SDLPal.Common/StringHelper.h"

extern "C" void TerminateOnError(const char *fmt, ...);

#define PAL_PATH_NAME	"SDLPAL"

static std::string g_savepath, g_basepath, g_configpath, g_screenshotpath;

extern HANDLE g_eventHandle;

extern "C"
LPCSTR UTIL_BasePath(VOID)
{
	if (g_basepath.empty())
	{
		auto mru_list = Windows::Storage::AccessCache::StorageApplicationPermissions::MostRecentlyUsedList;
		if (mru_list->Entries->Size > 0)
		{
			auto localfolder = mru_list->Entries->First()->Current.Metadata;
			if (localfolder->End()[-1] != L'\\') localfolder += "\\";
			ConvertString(localfolder, g_basepath);
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
	return UTIL_BasePath();
}

static BOOL UTIL_IsMobile(VOID)
{
	auto rc = Windows::ApplicationModel::Resources::Core::ResourceContext::GetForCurrentView();
	auto qv = rc->QualifierValues;
	return qv->HasKey("DeviceFamily") && qv->Lookup("DeviceFamily") == "Mobile";
}

extern "C"
BOOL UTIL_GetScreenSize(DWORD *pdwScreenWidth, DWORD *pdwScreenHeight)
{
	DXGI_OUTPUT_DESC desc;
	IDXGIFactory1* pFactory = nullptr;
	IDXGIAdapter1* pAdapter = nullptr;
	IDXGIOutput* pOutput = nullptr;
	DWORD retval = FALSE;
	
#if (_WIN32_WINNT >= 0x0A00) && (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP)
	if (!UTIL_IsMobile()) return FALSE;
#endif

	if (!pdwScreenWidth || !pdwScreenHeight) return FALSE;

	if (FAILED(CreateDXGIFactory1(IID_IDXGIFactory1, (void**)&pFactory))) goto UTIL_WP_GetScreenSize_exit;

	if (FAILED(pFactory->EnumAdapters1(0, &pAdapter))) goto UTIL_WP_GetScreenSize_exit;

	if (FAILED(pAdapter->EnumOutputs(0, &pOutput))) goto UTIL_WP_GetScreenSize_exit;

	if (SUCCEEDED(pOutput->GetDesc(&desc)))
	{
#if (_WIN32_WINNT < 0x0A00) && (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP)
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
BOOL UTIL_TouchEnabled(VOID)
{
	return (ref new Windows::Devices::Input::TouchCapabilities())->TouchPresent;
}

#include "SDL.h"
#include "SDL_endian.h"
#include <setjmp.h>

jmp_buf exit_jmp_buf;

extern "C"
INT UTIL_Platform_Init(int argc, char* argv[])
{
	if (setjmp(exit_jmp_buf) == 1) return -1;

	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeRight");
	SDL_SetHint(SDL_HINT_WINRT_HANDLE_BACK_BUTTON, "1");

	return 0;
}

extern "C"
VOID UTIL_Platform_Quit(VOID)
{
	//throw ref new Platform::Exception(0);
	longjmp(exit_jmp_buf, 1);
}
