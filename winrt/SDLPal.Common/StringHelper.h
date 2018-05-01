/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2018 SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// StringHelper.h: UWP support library for SDLPal.
// Author: Lou Yihua @ 2016
//

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
	if (src)
	{
		int len = MultiByteToWideChar(CP_ACP, 0, src, -1, nullptr, 0);
		auto wc = new wchar_t[len];
		MultiByteToWideChar(CP_ACP, 0, src, -1, wc, len);
		auto dst = ref new Platform::String(wc);
		delete[] wc;
		return dst;
	}
	else
		return "";
}

static Platform::String^ ConvertString(const std::string& src)
{
	return ConvertString(src.c_str());
}
