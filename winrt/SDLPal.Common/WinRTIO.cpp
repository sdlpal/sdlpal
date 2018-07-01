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
// WinRTIO.cpp: UWP C-style file I/O wrapper for SDLPal.
// Author: Lou Yihua @ 2016-2017
//

#include <wrl.h>
#include <string>
#include <map>
#include <DXGI.h>
#include <shcore.h>
#include <unordered_set>
#include "AsyncHelper.h"
#include "StringHelper.h"

#pragma comment(lib, "ShCore.lib")

#define PAL_PATH_NAME	"SDLPAL"

static const LARGE_INTEGER liZero = { 0 };
static const void* const _SIGNATURE = &liZero;

/*========================*/

typedef struct WRT_FILE WRT_FILE;

struct WRT_FILE
{
	const void* sig;
	IStream* stream;
	CRITICAL_SECTION cs;
	bool readable;
	bool writable;
	bool binary;

	WRT_FILE() : sig(&liZero), stream(nullptr), readable(false), writable(false), binary(false) { InitializeCriticalSectionEx(&cs, 4000, 0); }
	WRT_FILE(Windows::Storage::Streams::IRandomAccessStream^ s, bool r, bool w, bool b)
		: sig(&liZero), stream(nullptr), readable(r), writable(w), binary(b)
	{
		HRESULT hr;
		InitializeCriticalSectionEx(&cs, 4000, 0);
		if (FAILED(hr = CreateStreamOverRandomAccessStream(s, IID_PPV_ARGS(&stream))))
			throw ref new Platform::Exception(hr);
	}
	~WRT_FILE() { if (stream) stream->Release(); DeleteCriticalSection(&cs); }
};

static std::map<std::string, Windows::Storage::StorageFile^> g_specialFiles;
static std::map<std::string, Windows::Storage::StorageFolder^> g_specialFolders;

class CriticalSection
{
public:
	CriticalSection(CRITICAL_SECTION& cs) : m_cs(cs) { EnterCriticalSection(&m_cs); }
	~CriticalSection() { LeaveCriticalSection(&m_cs); }

private:
	CRITICAL_SECTION& m_cs;
};

class Event
{
public:
	Event() : _eventHandle(CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS)) {}
	~Event() { CloseHandle(_eventHandle); }

	operator HANDLE() { return _eventHandle; }

private:
	HANDLE _eventHandle;
};

static const std::string get_directory(const char* _Filename)
{
	auto ptr = _Filename;
	while (*ptr == '/' || *ptr == '\\') ptr++;

	std::string directory((ptr != _Filename) ? "\\" : "");
	while (*ptr)
	{
		std::string temp;
		auto pos = ptr;
		while (*pos && *pos != '/' && *pos != '\\') pos++;
		if (*pos)
		{
			directory.append(ptr, pos - ptr);
			directory.append("\\");
		}
		while (*pos == '/' || *pos == '\\') pos++;
		ptr = pos;
	}

	return directory;
}

static const std::string get_filename(const char* _Filename)
{
	auto ptr = _Filename + strlen(_Filename);
	while (ptr > _Filename && *ptr != '/' && *ptr != '\\') ptr--;
	if (*ptr == '/' || *ptr == '\\') ptr++;
	return ptr;
}

extern "C" std::map<std::string, Windows::Storage::StorageFile^>* get_special_files_map() { return &g_specialFiles; }
extern "C" std::map<std::string, Windows::Storage::StorageFolder^>* get_special_folders_map() { return &g_specialFolders; }

extern "C"
errno_t WRT_fopen_s(WRT_FILE ** pFile, const char * _Filename, const char * _Mode)
{
	if (nullptr == _Filename || nullptr == _Mode || nullptr == pFile) return EINVAL;

	*pFile = nullptr;

	bool r, w, b = false;
	switch (*_Mode)
	{
	case 'a': w = true; r = false; break;
	case 'w': w = true; r = false; break;
	case 'r': w = false; r = true; break;
	default: return EINVAL;
	}
	for (size_t i = 1; i < strlen(_Mode); i++)
	{
		switch (_Mode[i])
		{
		case '+': r = w = true; break;
		case 'b': b = true; break;
		case 't': b = false; break;
		default: return EINVAL;
		}
	}

	try
	{
		Event eventHandle;

		// If the file belongs to so-called 'special files' (i.e., specified in configuration file), then return its object directly
		if (g_specialFiles.find(_Filename) != g_specialFiles.end())
		{
			*pFile = new WRT_FILE(AWait(g_specialFiles[_Filename]->OpenAsync(w ? Windows::Storage::FileAccessMode::ReadWrite : Windows::Storage::FileAccessMode::Read), eventHandle), r, w, b);
			if (*_Mode == 'a')
			{
				(*pFile)->stream->Seek(LARGE_INTEGER{ 0,0 }, STREAM_SEEK_END, nullptr);
			}
			return 0;
		}

		auto directory = get_directory(_Filename);
		auto filename = get_filename(_Filename);

		Windows::Storage::StorageFolder^ folder = nullptr;

		// If the file's folder belongs to so-called 'special folders' (i.e., specified in configuration file), then use the cache folder object
		for (auto i = g_specialFolders.begin(); directory.length() > 0 && i != g_specialFolders.end(); i++)
		{
			if (_strnicmp(i->first.c_str(), directory.c_str(), i->first.size()) == 0)
			{
				folder = i->second;
				if (directory.length() > i->first.length())
				{
					size_t pos = i->first.length(), next = directory.find('\\', pos);
					while (next != std::string::npos)
					{
						folder = AWait(folder->GetFolderAsync(ConvertString(std::string(&directory[pos], next - pos))), eventHandle);
						next = directory.find('\\', pos = next + 1);
					}
					if (pos < directory.length())
					{
						folder = AWait(folder->GetFolderAsync(ConvertString(std::string(&directory[pos], directory.length() - pos))), eventHandle);
					}
				}
			}
		}

		// The try get folder directly by its full path
		if (!folder && directory.length())
		{
			folder = AWait(Windows::Storage::StorageFolder::GetFolderFromPathAsync(ConvertString(directory)), eventHandle);
		}

		// As a last sort, use app's local folder
		if (!folder)
		{
			folder = Windows::Storage::ApplicationData::Current->LocalFolder;
		}

		Windows::Storage::StorageFile^ file = nullptr;
		switch (*_Mode)
		{
		case 'a':
			file = AWait(folder->CreateFileAsync(ConvertString(filename), Windows::Storage::CreationCollisionOption::OpenIfExists), eventHandle);
			break;
		case 'w':
			file = AWait(folder->CreateFileAsync(ConvertString(filename), Windows::Storage::CreationCollisionOption::ReplaceExisting), eventHandle);
			break;
		case 'r':
			file = AWait(folder->GetFileAsync(ConvertString(filename)), eventHandle);
			break;
		}
		if (file)
		{
			*pFile = new WRT_FILE(AWait(file->OpenAsync(w ? Windows::Storage::FileAccessMode::ReadWrite : Windows::Storage::FileAccessMode::Read), eventHandle), r, w, b);
			if (*_Mode == 'a')
			{
				(*pFile)->stream->Seek(LARGE_INTEGER{ 0,0 }, STREAM_SEEK_END, nullptr);
			}
		}
	}
	catch (Platform::AccessDeniedException^)
	{
		return EACCES;
	}
	catch (Platform::Exception^)
	{
		return EIO;
	}

	return 0;
}

extern "C"
WRT_FILE* WRT_fopen(const char * _Filename, const char * _Mode)
{
	WRT_FILE* ret;
	int err = WRT_fopen_s(&ret, _Filename, _Mode);
	_set_errno(err);
	return err ? nullptr : ret;
}

extern "C"
int WRT_fclose(WRT_FILE * _File)
{
	if (!_File || _File->sig != _SIGNATURE) return fclose((FILE*)_File);
	delete _File;
	return 0;
}

extern "C"
int WRT_feof(WRT_FILE * _File)
{
	if (!_File || _File->sig != _SIGNATURE) return feof((FILE*)_File);

	STATSTG st;
	ULARGE_INTEGER uli;
	CriticalSection cs(_File->cs);

	if (SUCCEEDED(_File->stream->Seek(liZero, STREAM_SEEK_CUR, &uli)) &&
		SUCCEEDED(_File->stream->Stat(&st, STATFLAG_NONAME)))
		return uli.QuadPart >= st.cbSize.QuadPart;
	else
		return 1;
}

extern "C"
int WRT_fseeki64(WRT_FILE * _File, long long _Offset, int _Origin)
{
	if (!_File || _File->sig != _SIGNATURE) return _fseeki64((FILE*)_File, _Offset, _Origin);

	CriticalSection cs(_File->cs);
	LARGE_INTEGER liOffset;
	liOffset.QuadPart = _Offset;
	return SUCCEEDED(_File->stream->Seek(liOffset, _Origin, NULL)) ? 0 : -1;
}

extern "C"
int WRT_fseek(WRT_FILE * _File, long _Offset, int _Origin)
{
	return WRT_fseeki64(_File, _Offset, _Origin);
}

extern "C"
long long WRT_ftelli64(WRT_FILE * _File)
{
	if (!_File || _File->sig != _SIGNATURE) return _ftelli64((FILE*)_File);

	CriticalSection cs(_File->cs);
	unsigned long long pos;
	return SUCCEEDED(_File->stream->Seek(liZero, STREAM_SEEK_CUR, (PULARGE_INTEGER)&pos)) ? pos : -1;
}

extern "C"
size_t WRT_ftell(WRT_FILE * _File)
{
	return (size_t)WRT_ftelli64(_File);
}

extern "C"
size_t WRT_fread(void * _DstBuf, size_t _ElementSize, size_t _Count, WRT_FILE * _File)
{
	if (!_File || _File->sig != _SIGNATURE) return fread(_DstBuf, _ElementSize, _Count, (FILE*)_File);
	if (!_File->readable || _ElementSize == 0 || _Count == 0 || !_DstBuf) return 0;

	CriticalSection cs(_File->cs);
	unsigned long cbRead;
	return SUCCEEDED(_File->stream->Read(_DstBuf, (ULONG)(_ElementSize * _Count), &cbRead)) ? cbRead / _ElementSize : 0;
}

extern "C"
size_t WRT_fwrite(const void * _Str, size_t _Size, size_t _Count, WRT_FILE * _File)
{
	if (!_File || _File->sig != _SIGNATURE) return fwrite(_Str, _Size, _Count, (FILE*)_File);
	if (!_File->writable || !_Str || _Size == 0 || _Count == 0) return 0;

	CriticalSection cs(_File->cs);
	unsigned long cbWrite;
	return SUCCEEDED(_File->stream->Write(_Str, (ULONG)(_Size * _Count), &cbWrite)) ? cbWrite / _Size : 0;
}

extern "C"
int WRT_fflush(WRT_FILE * _File)
{
	if (!_File || _File->sig != _SIGNATURE) return fflush((FILE*)_File);
	if (!_File->writable) return 0;

	CriticalSection cs(_File->cs);
	return SUCCEEDED(_File->stream->Commit(STGC_DEFAULT)) ? 0 : EOF;
}

static int WRT_fgetc_nolock(WRT_FILE * _File)
{
	unsigned long cbRead;
	unsigned char buf;
	return _File->stream->Read(&buf, 1, &cbRead) == S_OK ? buf : EOF;
}

extern "C"
int WRT_fgetc(WRT_FILE * _File)
{
	if (!_File || _File->sig != _SIGNATURE) return fgetc((FILE*)_File);

	CriticalSection cs(_File->cs);
	return _File->readable ? WRT_fgetc_nolock(_File) : EOF;
}

extern "C"
char* WRT_fgets(char * _Buf, int _MaxCount, WRT_FILE * _File)
{
	if (!_File || _File->sig != _SIGNATURE) return fgets(_Buf, _MaxCount, (FILE*)_File);
	if (!_File->readable || !_Buf || _MaxCount <= 0) return nullptr;

	CriticalSection cs(_File->cs);
	int n = 0, flag = 0, ch;
	while (n < _MaxCount - 1 && EOF != (ch = WRT_fgetc_nolock(_File)))
	{
		if (flag)
		{
			if (ch != '\n')
				_Buf[n++] = '\r';
			flag = 0;
		}
		if (ch != '\r')
		{
			_Buf[n++] = ch;
			if (ch == '\n') break;
		}
		else
			flag = 1;
	}
	if (n > 0)
	{
		_Buf[n] = '\0';
		return _Buf;
	}
	else
		return nullptr;
}

extern "C"
int WRT_fputc(int _Ch, WRT_FILE * _File)
{
	if (!_File || _File->sig != _SIGNATURE) return fputc(_Ch, (FILE*)_File);

	CriticalSection cs(_File->cs);
	unsigned long cbWrite;
	return _File->writable && _File->stream->Write(&_Ch, 1, &cbWrite) == S_OK ? cbWrite : EOF;
}

extern "C"
int WRT_fputs(const char * _Str, WRT_FILE * _File)
{
	if (!_File || _File->sig != _SIGNATURE) return fputs(_Str, (FILE*)_File);
	if (!_File->writable || !_Str) return EOF;

	const char *start = _Str;
	size_t length = strlen(_Str);
	unsigned long cbWrite;
	const char crlf[] = { '\r', '\n' };
	CriticalSection cs(_File->cs);
	while (true)
	{
		const char *ptr = start;
		while (*ptr && *ptr != '\n') ptr++;
		if (_File->stream->Write(start, (ULONG)(ptr - start), &cbWrite) != S_OK)
			return EOF;
		if (*ptr == '\n')
		{
			if (_File->stream->Write(crlf, 2, &cbWrite) != S_OK)
				return EOF;
			else
				start = ptr + 1;
		}
		else
			return (int)length;
	}
}

extern "C"
int WRT_access(const char * const _FileName, int _AccessMode)
{
	if (!_FileName || (_AccessMode & ~0x6) != 0)
	{
		_set_errno(EINVAL);
		return -1;
	}

	try
	{
		auto file = AWait(Windows::Storage::StorageFile::GetFileFromPathAsync(ConvertString(_FileName)));

		if ((file->Attributes & Windows::Storage::FileAttributes::Directory) != Windows::Storage::FileAttributes::Directory &&
			(file->Attributes & Windows::Storage::FileAttributes::ReadOnly) == Windows::Storage::FileAttributes::ReadOnly &&
			(_AccessMode & 0x2) != 0)
		{
			throw ref new Platform::AccessDeniedException();
		}

		return 0;
	}
	catch (AccessDeniedException^ e)
	{
		_set_errno(EACCES);
		return -1;
	}
	catch (Exception^ e)
	{
		if (e->HResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			_set_errno(ENOENT);
		}
		else
		{
			_set_errno(EINVAL);
		}
		return -1;
	}
}
