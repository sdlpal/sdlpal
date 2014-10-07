#include <wrl.h>
#include <stdio.h>

static const char *
GetRootPath()
{
	static char buf[1024] = "";
	if (buf[0] == '\0')
	{
		Platform::String^ localfolder = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
		const char16 *begin = localfolder->Begin();
		WideCharToMultiByte(CP_ACP, 0, begin, -1, buf, 1024, NULL, FALSE);
	}
	return buf;
}

static const char *
GetInstallPath()
{
	static char buf[1024] = "";
	if (buf[0] == '\0')
	{
		Platform::String^ installfolder = Windows::ApplicationModel::Package::Current->InstalledLocation->Path;
		const char16 *begin = installfolder->Begin();
		WideCharToMultiByte(CP_ACP, 0, begin, -1, buf, 1024, NULL, FALSE);
	}
	return buf;
}

extern "C" FILE *
MY_fopen(const char *path, const char *mode)
{
	const char *p = GetRootPath();
	char buf[1024];
	_snprintf_s(buf, 1024, "%s\\%s", p, path);
	FILE *fp = _fsopen(buf, mode, 0x40);
	if (fp == NULL)
	{
		p = GetInstallPath();
		_snprintf_s(buf, 1024, "%s\\%s", p, path);
		fp = _fsopen(buf, mode, 0x40);
	}
	if (fp == NULL)
	{
		p = GetRootPath();
		_snprintf_s(buf, 1024, "%s\\Shared\\%s", p, path);
		fp = _fsopen(buf, mode, 0x40);
	}
	return fp;
}
