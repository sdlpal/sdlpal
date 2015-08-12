#include <wrl.h>
#include <string>

//#include <stdio.h>
//#include <share.h>

//static const char *
//GetRootPath()
//{
//	static char buf[1024] = "";
//	if (buf[0] == '\0')
//	{
//		Platform::String^ localfolder = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
//		const char16 *begin = localfolder->Begin();
//		WideCharToMultiByte(CP_ACP, 0, begin, -1, buf, 1024, NULL, FALSE);
//	}
//	return buf;
//}
//
//static const char *
//GetInstallPath()
//{
//	static char buf[1024] = "";
//	if (buf[0] == '\0')
//	{
//		Platform::String^ installfolder = Windows::ApplicationModel::Package::Current->InstalledLocation->Path;
//		const char16 *begin = installfolder->Begin();
//		WideCharToMultiByte(CP_ACP, 0, begin, -1, buf, 1024, NULL, FALSE);
//	}
//	return buf;
//}
//
//extern "C" FILE *
//MY_fopen(const char *path, const char *mode)
//{
//	return fopen(path, mode);
//
//	const char *p = GetRootPath();
//	char buf[1024];
//	_snprintf_s(buf, 1024, "%s\\%s", p, path);
//	FILE *fp = _fsopen(buf, mode, _SH_DENYNO);
//	if (fp == NULL)
//	{
//		p = GetInstallPath();
//		_snprintf_s(buf, 1024, "%s\\%s", p, path);
//		fp = _fsopen(buf, mode, _SH_DENYNO);
//	}
//	if (fp == NULL)
//	{
//		p = GetRootPath();
//		_snprintf_s(buf, 1024, "%s\\Shared\\%s", p, path);
//		fp = _fsopen(buf, mode, _SH_DENYNO);
//	}
//	return fp;
//}

static std::string g_savepath;

extern "C"
LPCSTR UTIL_WP_SavePath(VOID)
{
	Platform::String^ localfolder = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
	int len = WideCharToMultiByte(CP_ACP, 0, localfolder->Begin(), -1, nullptr, 0, nullptr, nullptr);
	g_savepath.resize(len);
	WideCharToMultiByte(CP_ACP, 0, localfolder->Begin(), -1, (char*)g_savepath.data(), len, nullptr, nullptr);
	const_cast<char*>(g_savepath.data())[len - 1] = '\\';
	return g_savepath.c_str();
}
