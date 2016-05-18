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
static Windows::Storage::StorageFolder ^g_basefolder, ^g_savefolder, ^g_configfolder, ^g_screenshotfolder;

extern HANDLE g_eventHandle;

extern "C"
LPCSTR UTIL_BasePath(VOID)
{
	if (g_basepath.empty())
	{
		g_basefolder = Windows::Storage::ApplicationData::Current->LocalFolder;
		auto localfolder = g_basefolder->Path;
		if (localfolder->End()[-1] != L'\\') localfolder += "\\";
		ConvertString(localfolder, g_basepath);
	}
	return g_basepath.c_str();
}

extern "C"
LPCSTR UTIL_SavePath(VOID)
{
	if (g_savepath.empty())
	{
		g_savefolder = Windows::Storage::ApplicationData::Current->LocalFolder;
		auto localfolder = g_savefolder->Path;
		if (localfolder->End()[-1] != L'\\') localfolder += "\\";
		ConvertString(localfolder, g_savepath);
	}
	return g_savepath.c_str();
}

extern "C"
LPCSTR UTIL_ConfigPath(VOID)
{
	if (g_configpath.empty())
	{
		g_configfolder = Windows::Storage::ApplicationData::Current->LocalFolder;
		auto localfolder = g_configfolder->Path;
		if (localfolder->End()[-1] != L'\\') localfolder += "\\";
		ConvertString(localfolder, g_configpath);
	}
	return g_configpath.c_str();
}

extern "C"
LPCSTR UTIL_ScreenShotPath(VOID)
{
	if (g_screenshotpath.empty())
	{
		Windows::Storage::StorageFolder^ folder = nullptr;

		try { folder = AWait(Windows::Storage::KnownFolders::PicturesLibrary->GetFolderAsync("SDLPAL"), g_eventHandle); }
		catch (Platform::Exception^) {}
		if (folder == nullptr)
		{
			try { folder = AWait(Windows::Storage::KnownFolders::PicturesLibrary->CreateFolderAsync("SDLPAL"), g_eventHandle); }
			catch (Platform::Exception^) {}
		}
		if (folder)
		{
			g_screenshotfolder = folder;
			auto localfolder = g_screenshotfolder->Path;
			if (localfolder->End()[-1] != L'\\') localfolder += "\\";
			ConvertString(localfolder, g_screenshotpath);
		}
		else
		{
			g_screenshotpath = UTIL_SavePath();
			g_screenshotfolder = g_savefolder;
		}
	}
	return g_screenshotpath.c_str();
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

#if WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP
	if (!UTIL_IsMobile()) return FALSE;
#endif

	if (!pdwScreenWidth || !pdwScreenHeight) return FALSE;

	if (FAILED(CreateDXGIFactory1(IID_IDXGIFactory1, (void**)&pFactory))) goto UTIL_WP_GetScreenSize_exit;

	if (FAILED(pFactory->EnumAdapters1(0, &pAdapter))) goto UTIL_WP_GetScreenSize_exit;

	if (FAILED(pAdapter->EnumOutputs(0, &pOutput))) goto UTIL_WP_GetScreenSize_exit;

	if (SUCCEEDED(pOutput->GetDesc(&desc)))
	{
		*pdwScreenWidth = (desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top);
		*pdwScreenHeight = (desc.DesktopCoordinates.right - desc.DesktopCoordinates.left);
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

extern "C"
INT UTIL_Platform_Init(int argc, char* argv[])
{
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeRight");
	SDL_SetHint(SDL_HINT_WINRT_HANDLE_BACK_BUTTON, "1");

	return 0;
}

extern "C"
VOID UTIL_Platform_Quit(VOID)
{
	Windows::ApplicationModel::Core::CoreApplication::Exit();
}
