#pragma once

#include <wrl.h>
#include <string>

static void ConvertString(Platform::String^ src, std::string& dst)
{
	int len = WideCharToMultiByte(CP_ACP, 0, src->Begin(), -1, nullptr, 0, nullptr, nullptr);
	dst.resize(len - 1);
	WideCharToMultiByte(CP_ACP, 0, src->Begin(), -1, (char*)dst.data(), len, nullptr, nullptr);
}

static std::string ConvertString(Platform::String^ src)
{
	int len = WideCharToMultiByte(CP_ACP, 0, src->Begin(), -1, nullptr, 0, nullptr, nullptr);
	std::string dst(len - 1, ' ');
	WideCharToMultiByte(CP_ACP, 0, src->Begin(), -1, (char*)dst.data(), len, nullptr, nullptr);
	return dst;
}

static std::string ConvertString(const wchar_t* src)
{
	int len = WideCharToMultiByte(CP_ACP, 0, src, -1, nullptr, 0, nullptr, nullptr);
	std::string dst(len - 1, ' ');
	WideCharToMultiByte(CP_ACP, 0, src, -1, (char*)dst.data(), len, nullptr, nullptr);
	return dst;
}

static std::string ConvertString(const std::wstring& src)
{
	return ConvertString(src.c_str());
}

static void ConvertString(const std::string& src, std::wstring& dst)
{
	int len = MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, nullptr, 0);
	dst.resize(len - 1);
	MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, (wchar_t*)dst.data(), len);
}

static Platform::String^ ConvertString(const char* src)
{
	int len = MultiByteToWideChar(CP_ACP, 0, src, -1, nullptr, 0);
	auto wc = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, src, -1, wc, len);
	auto dst = ref new Platform::String(wc);
	delete[] wc;
	return dst;
}

static Platform::String^ ConvertString(const std::string& src)
{
	return ConvertString(src.c_str());
}
