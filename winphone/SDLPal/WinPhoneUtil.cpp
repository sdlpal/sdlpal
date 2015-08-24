#include <wrl.h>
#include <string>
#include <DXGI.h>
#include <ppltasks.h>

#define PAL_PATH_NAME	"SDLPAL"

static std::string g_savepath, g_basepath;

template<typename T>
static bool WaitOnTask(T task, int64 wait_timeout)
{
	auto start = GetTickCount64();
	while (!task.is_done() && (int64)(GetTickCount64() - start) <= wait_timeout)
		Sleep(1);
	return task.is_done();
}

static void ConvertString(Platform::String^ src, std::string& dst)
{
	int len = WideCharToMultiByte(CP_ACP, 0, src->Begin(), -1, nullptr, 0, nullptr, nullptr);
	dst.resize(len - 1);
	WideCharToMultiByte(CP_ACP, 0, src->Begin(), -1, (char*)dst.data(), len, nullptr, nullptr);
}

static bool CheckGamePath(Windows::Storage::StorageFolder^ root)
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

	/* Try to get the path */
	auto foldertask = concurrency::create_task(root->GetFolderAsync(PAL_PATH_NAME));
	if (!WaitOnTask(foldertask, 500)) return false;		// Wait for 500ms max

	try
	{
		/* Check the access right of necessary files */
		auto folder = foldertask.get();
		for (int i = 0; i < 13; i++)
		{
			auto filetask = concurrency::create_task(foldertask.get()->GetFileAsync(required_files[i]));
			if (!WaitOnTask(filetask, 500) || _waccess_s(filetask.get()->Path->Begin(), 2)) return false;
		}
		for (int i = 0; i < 2; i++)
		{
			auto filetask = concurrency::create_task(foldertask.get()->GetFileAsync(optional_required_files[i]));
			try { if (WaitOnTask(filetask, 500) && _waccess_s(filetask.get()->Path->Begin(), 2) == 0) return true; }
			catch(Platform::Exception^) {}
		}
	}
	catch(Platform::Exception^)
	{ /* Accessing SD card failed, or required file is missing, or access is denied */ }
	return false;
}

extern "C"
LPCSTR UTIL_BasePath(VOID)
{
	if (g_basepath.empty())
	{
		auto enumtask = concurrency::create_task(Windows::Storage::KnownFolders::RemovableDevices->GetFoldersAsync());
		if (WaitOnTask(enumtask, 1000))		// Wait for 1000ms max
		{
			auto folderiter = enumtask.get()->First();
			while (folderiter->HasCurrent)
			{
				if (CheckGamePath(folderiter->Current))
				{
					/* Folder examination succeeded */
					auto folder = folderiter->Current->Path;
					if (folder->End()[-1] != L'\\') folder += "\\";
					folder += PAL_PATH_NAME "\\";
					ConvertString(folder, g_basepath);

					/* Check whether the folder is writable */
					FILE* fp;
					if (_wfopen_s(&fp, (folder + "sdlpal.rpg")->Begin(), L"w") == 0)
					{
						g_savepath = g_basepath;
						fclose(fp);
					}
					break;
				}
				folderiter->MoveNext();
			}
		}

		if (g_basepath.empty())
		{
			g_basepath.assign("Assets\\Data\\");
		}
	}
	return g_basepath.c_str();
}

extern "C"
LPCSTR UTIL_SavePath(VOID)
{
	if (g_savepath.empty())
	{
		auto localfolder = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
		if (localfolder->End()[-1] != L'\\') localfolder += "\\";
		ConvertString(localfolder, g_savepath);
	}
	return g_savepath.c_str();
}

extern "C"
BOOL UTIL_GetScreenSize(DWORD *pdwScreenWidth, DWORD *pdwScreenHeight)
{
	DXGI_OUTPUT_DESC desc;
	IDXGIFactory1* pFactory = nullptr;
	IDXGIAdapter1* pAdapter = nullptr;
	IDXGIOutput* pOutput = nullptr;
	DWORD retval = FALSE;

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
