#include "pch.h"

#include <wrl.h>
#include <string>
#include <DXGI.h>
#include <ppltasks.h>
#include "../SDLPal.Common/AsyncHelper.h"
#include "../SDLPal.Common/StringHelper.h"

extern "C" void TerminateOnError(const char *fmt, ...);

#define PAL_PATH_NAME	"SDLPAL"

static std::string g_savepath, g_basepath, g_configpath;
static Windows::Storage::StorageFolder ^g_basefolder, ^g_savefolder, ^g_configfolder;

static Windows::Storage::StorageFolder^ CheckGamePath(Windows::Storage::StorageFolder^ root, HANDLE eventHandle)
{
	Platform::String^ required_files[] = {
		L"ABC.MKF", L"BALL.MKF", L"DATA.MKF", L"F.MKF", L"FBP.MKF",
		L"FIRE.MKF", L"GOP.MKF", L"MAP.MKF", L"MGO.MKF", L"PAT.MKF",
		L"RGM.MKF", L"RNG.MKF", L"SSS.MKF"
	};
	Platform::String^ optional_required_files[] = {
		L"VOC.MKF", L"SOUNDS.MKF"
	};
	/* The words.dat & m.msg may be configurable in the future, so not check here */

	try
	{
		/* Try to get the path */
		auto folder = AWait(root->GetFolderAsync(PAL_PATH_NAME), eventHandle);

		/* Check the access right of necessary files */
		for (int i = 0; i < 13; i++)
		{
			if (!AWait(AWait(folder->GetFileAsync(required_files[i]), eventHandle)->OpenReadAsync(), eventHandle)->CanRead)
				return nullptr;
		}

		for (int i = 0; i < 2; i++)
		{
			try {
				if (AWait(AWait(folder->GetFileAsync(optional_required_files[i]), eventHandle)->OpenReadAsync(), eventHandle)->CanRead)
					return folder;
			}
			catch (Platform::Exception^) {}
		}
	}
	catch (Platform::Exception^)
	{ /* Accessing SD card failed, or required file is missing, or access is denied */
	}
	return nullptr;
}

extern "C"
LPCSTR UTIL_BasePath(VOID)
{
	//if (g_basepath.empty())
	//{
	//	Windows::Storage::StorageFolder^ folder = nullptr;
	//	HANDLE eventHandle = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);
	//	auto folders = AWait(Windows::Storage::KnownFolders::RemovableDevices->GetFoldersAsync())->First();
	//	while (folders->HasCurrent)
	//	{
	//		if (folder = CheckGamePath(folders->Current, eventHandle))
	//			break;
	//		else
	//			folders->MoveNext();
	//	}

	//	if (!folder)
	//	{
	//		Windows::Storage::StorageFolder^ search_folders[] = {
	//			Windows::Storage::KnownFolders::PicturesLibrary,
	//			Windows::Storage::KnownFolders::SavedPictures,
	//			//Windows::Storage::KnownFolders::MusicLibrary,
	//			//Windows::Storage::KnownFolders::VideosLibrary,
	//		};
	//		for (int i = 0; i < sizeof(search_folders) / sizeof(search_folders[0]); i++)
	//		{
	//			if (folder = CheckGamePath(search_folders[i], eventHandle))
	//				break;
	//		}
	//	}

	//	CloseHandle(eventHandle);

	//	if (folder)
	//	{
	//		/* Folder examination succeeded */
	//		auto path = folder->Path;
	//		if (path->End()[-1] != L'\\') path += "\\";
	//		ConvertString(path, g_basepath);
	//		g_basefolder = folder;

	//		/* Check whether the folder is writable */
	//		FILE* fp = fopen(ConvertString(path + "sdlpal.rpg").c_str(), "wb");
	//		if (fp)
	//		{
	//			g_savepath = g_basepath;
	//			g_savefolder = g_basefolder;
	//			fclose(fp);
	//		}
	//	}
	//	else
	//	{
	//		TerminateOnError("Could not find PAL folder.\n");
	//		//ConvertString(Windows::ApplicationModel::Package::Current->InstalledLocation->Path + "\\Assets\\Data\\", g_basepath);
	//	}
	//}
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

# include <setjmp.h>

static jmp_buf g_exit_jmp_env;
# define LONGJMP_EXIT_CODE          0xff

extern "C"
INT UTIL_Platform_Init(int argc, char* argv[])
{
	//
	// In windows phone, calling exit(0) directly will cause an abnormal exit.
	// By using setjmp/longjmp to avoid this.
	//
	if (setjmp(g_exit_jmp_env) == LONGJMP_EXIT_CODE) return -1;

	// We should first check the SD card before running actual codes
	UTIL_BasePath();

	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeRight");
	SDL_SetHint(SDL_HINT_WINRT_HANDLE_BACK_BUTTON, "1");

	return 0;
}

extern "C"
VOID UTIL_Platform_Quit(VOID)
{
	longjmp(g_exit_jmp_env, LONGJMP_EXIT_CODE);
}
