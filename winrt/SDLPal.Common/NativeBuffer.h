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
// NativeBuffer.h: UWP support library for SDLPal.
// Author: Lou Yihua @ 2017
//

#pragma once

#include <wrl.h>
#include <wrl/implements.h>
#include <windows.storage.streams.h>
#include <robuffer.h>
#include <stdint.h>

class NativeBuffer :
	public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::WinRtClassicComMix>,
	ABI::Windows::Storage::Streams::IBuffer,
	Windows::Storage::Streams::IBufferByteAccess>
{
public:
	virtual ~NativeBuffer()
	{
	}

	STDMETHODIMP RuntimeClassInitialize(byte *buffer, UINT totalSize)
	{
		m_length = totalSize;
		m_buffer = buffer;

		return S_OK;
	}

	STDMETHODIMP Buffer(byte **value)
	{
		*value = m_buffer;

		return S_OK;
	}

	STDMETHODIMP get_Capacity(UINT32 *value)
	{
		*value = m_length;

		return S_OK;
	}

	STDMETHODIMP get_Length(UINT32 *value)
	{
		*value = m_length;

		return S_OK;
	}

	STDMETHODIMP put_Length(UINT32 value)
	{
		m_length = value;

		return S_OK;
	}

	static Windows::Storage::Streams::IBuffer^ GetIBuffer(byte *buffer, uint32_t totalSize)
	{
		Microsoft::WRL::ComPtr<NativeBuffer> nativeBuffer;
		Microsoft::WRL::Details::MakeAndInitialize<NativeBuffer>(&nativeBuffer, buffer, totalSize);
		auto obj = reinterpret_cast<IInspectable*>(nativeBuffer.Get());
		return reinterpret_cast<Windows::Storage::Streams::IBuffer^>(obj);
	}

private:
	UINT32 m_length;
	byte *m_buffer;
};
