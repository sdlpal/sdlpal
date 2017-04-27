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
