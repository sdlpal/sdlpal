/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2017, SDLPAL development team.
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
// native_avi.cpp: Native Windows Runtime AVI player for SDLPal.
//         @Author: Lou Yihua, 2017
//

#include "pch.h"
#include "AsyncHelper.h"
#include "StringHelper.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfmediaengine.h>
#include <wrl.h>
#include <d3d11.h>
#include <list>
#include <map>
#include "../../3rd/SDL/include/SDL_syswm.h"
#include "../../video.h"
#include "../../util.h"
#include "../../input.h"

using namespace Microsoft::WRL;

class bstr_t
{
public:
	bstr_t(const wchar_t* s) : m_bstr(SysAllocString(s)) {}
	~bstr_t() { SysFreeString(m_bstr); }

	operator BSTR() { return m_bstr; }

private:
	BSTR m_bstr;
};

class CUnknown
	: public IUnknown
{
public:
	CUnknown() : m_refcount(1) { }
	virtual ~CUnknown() { }

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject)
	{
		HRESULT hr = S_OK;
		if (riid == IID_IUnknown)
			*ppvObject = this, AddRef();
		else
			hr = E_NOINTERFACE;
		return hr;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return (ULONG)InterlockedIncrement(&m_refcount);
	}

	virtual ULONG STDMETHODCALLTYPE Release(void)
	{
		LONG ref = InterlockedDecrement(&m_refcount);
		if (0 == ref)
		{
			delete this;
		}
		return ref;
	}

private:
	volatile LONG m_refcount;
};

class CAVIPlayer
	: public CUnknown
	, public IMFMediaEngineNotify
	, public IMFMediaEngineExtension
{
#define FAIL_RETURN(x) do { HRESULT hr; if (hr = (x)) return hr; } while(0)
	struct Event
	{
		DWORD event;
		DWORD_PTR param1;
		DWORD param2;

		Event(DWORD a, DWORD_PTR b, DWORD c) : event(a), param1(b), param2(c) {}
	};

	class CMFAsyncCallback
		: public CUnknown
		, public IMFAsyncCallback
	{
	public:
		CMFAsyncCallback(IMFByteStream* pByteStream, IMFSourceResolver* resolver, IUnknown* punkState, IMFAsyncCallback* pCallback)
			: m_path(nullptr), m_pByteStream(pByteStream), m_punkState(punkState), m_pCallback(pCallback), m_resolver(resolver)
		{
		}

		CMFAsyncCallback(BSTR bstrURL, IUnknown* punkState, IMFAsyncCallback* pCallback)
			: m_path(bstrURL ? ref new Platform::String(bstrURL) : nullptr)
			, m_pByteStream(nullptr), m_punkState(punkState)
			, m_pCallback(pCallback), m_resolver(nullptr)
		{
		}

		CMFAsyncCallback(BSTR bstrURL, IMFSourceResolver* resolver, IUnknown* punkState, IMFAsyncCallback* pCallback)
			: m_path(bstrURL ? ref new Platform::String(bstrURL) : nullptr)
			, m_pByteStream(nullptr), m_punkState(punkState)
			, m_pCallback(pCallback), m_resolver(resolver)
		{
		}

		virtual ~CMFAsyncCallback()
		{
			if (m_path) delete m_path;
		}

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject)
		{
			HRESULT hr = S_OK;
			if (riid == IID_IMFAsyncCallback)
				*ppvObject = static_cast<IMFAsyncCallback*>(this), AddRef();
			else
				hr = CUnknown::QueryInterface(riid, ppvObject);
			return hr;
		}

		virtual ULONG STDMETHODCALLTYPE AddRef(void) { return CUnknown::AddRef(); }
		virtual ULONG STDMETHODCALLTYPE Release(void) { return CUnknown::Release(); }

		virtual HRESULT STDMETHODCALLTYPE GetParameters(
			__RPC__out DWORD *pdwFlags,
			__RPC__out DWORD *pdwQueue)
		{
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE Invoke(__RPC__in_opt IMFAsyncResult *pAsyncResult)
		{
			HRESULT hr;
			if (m_path != nullptr)
			{
				hr = CreateMFByteStreamFromURL(m_path, m_pByteStream.ReleaseAndGetAddressOf());
				if (m_resolver.Get() == nullptr)
				{
					return InvokeCallback(m_pByteStream.Get(), hr);
				}
			}

			ComPtr<IUnknown> result;
			MF_OBJECT_TYPE type;
			hr = m_resolver->CreateObjectFromByteStream(m_pByteStream.Get(), nullptr, MF_RESOLUTION_MEDIASOURCE | MF_RESOLUTION_READ, nullptr, &type, result.GetAddressOf());
			return InvokeCallback(result.Get(), hr);
		}

	protected:
		HRESULT InvokeCallback(IUnknown* pObject, HRESULT hr)
		{
			ComPtr<IMFAsyncResult> pResult;
			HRESULT nhr = MFCreateAsyncResult(pObject, m_pCallback.Get(), m_punkState.Get(), pResult.GetAddressOf());
			if (SUCCEEDED(nhr))
			{
				pResult->SetStatus(hr);
				nhr = MFInvokeCallback(pResult.Get());
			}
			return nhr;
		}

		static HRESULT CreateMFByteStreamFromURL(Platform::String^ path, IMFByteStream** ppByteStream)
		{
			HRESULT hr;
			if (!ppByteStream) return E_POINTER;
			try
			{
				auto file = AWait(Windows::Storage::StorageFile::GetFileFromPathAsync(path));
				auto stream = AWait(file->OpenAsync(Windows::Storage::FileAccessMode::Read));
				hr = MFCreateMFByteStreamOnStreamEx(reinterpret_cast<IUnknown*>(stream), ppByteStream);
				delete stream;
				delete file;
			}
			catch (Exception ^e)
			{
				hr = e->HResult;
			}
			return hr;
		}

	private:
		Platform::String^ m_path;
		ComPtr<IMFByteStream> m_pByteStream;
		ComPtr<IUnknown> m_punkState;
		ComPtr<IMFAsyncCallback> m_pCallback;
		ComPtr<IMFSourceResolver> m_resolver;
	};

public:
	CAVIPlayer()
		: m_event(CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS))
		, m_playing(false)
	{
		InitializeCriticalSection(&m_cs);
	}

	~CAVIPlayer()
	{
		DeleteCriticalSection(&m_cs);
	}

	HRESULT Initialize()
	{
		ComPtr<ID3D10Multithread> mtctx;
		ComPtr<IMFMediaEngineClassFactory> mfmecf;
		ComPtr<IMFAttributes> mfattr;

		static const D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_1
		};

		FAIL_RETURN(D3D11CreateDevice(
			nullptr,                    // specify nullptr to use the default adapter
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,                    // leave as nullptr if hardware is used
			D3D11_CREATE_DEVICE_VIDEO_SUPPORT | D3D11_CREATE_DEVICE_BGRA_SUPPORT,
			featureLevels,
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,          // always set this to D3D11_SDK_VERSION
			m_device.ReleaseAndGetAddressOf(),
			nullptr,
			m_context.ReleaseAndGetAddressOf()
		));

		FAIL_RETURN(m_context.As(&mtctx));
		FAIL_RETURN(mtctx->SetMultithreadProtected(TRUE));

		UINT resetToken;
		FAIL_RETURN(MFCreateDXGIDeviceManager(&resetToken, m_manager.ReleaseAndGetAddressOf()));
		FAIL_RETURN(m_manager->ResetDevice(m_device.Get(), resetToken));

		FAIL_RETURN(CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_ALL, IID_PPV_ARGS(mfmecf.GetAddressOf())));
		FAIL_RETURN(MFCreateAttributes(mfattr.GetAddressOf(), 1));
		FAIL_RETURN(mfattr->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, static_cast<CUnknown*>(this)));
		FAIL_RETURN(mfattr->SetUnknown(MF_MEDIA_ENGINE_EXTENSION, static_cast<CUnknown*>(this)));
		FAIL_RETURN(mfattr->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, m_manager.Get()));
		FAIL_RETURN(mfattr->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_B8G8R8A8_UNORM));
		FAIL_RETURN(mfmecf->CreateInstance(0, mfattr.Get(), m_engine.ReleaseAndGetAddressOf()));
		FAIL_RETURN(MFCreateSourceResolver(m_resolver.GetAddressOf()));

		m_playing = false;
		ResetEvent(m_event);
		m_events.clear();

		return S_OK;
	}

	//HRESULT 

	HRESULT Load(Platform::String^ filePath, DWORD* cx, DWORD* cy)
	{
		if (!filePath || !cx || !cy) return E_POINTER;

		if (m_engine)
		{
			ResetEvent(m_event);
			FAIL_RETURN(m_engine->SetSource(bstr_t(filePath->Data())));
			FAIL_RETURN(m_engine->Load());

			bool abort = false, ready = false;
			while (WaitForSingleObject(m_event, INFINITE) == WAIT_OBJECT_0 && !abort && !ready)
			{
				EnterCriticalSection(&m_cs);
				for (auto i = m_events.begin(); i != m_events.end(); i = m_events.erase(i))
				{
					switch (i->event)
					{
					case MF_MEDIA_ENGINE_EVENT_ERROR:
						UTIL_LogOutput(LOGLEVEL_ERROR, "Native AVI player encountered error: %d (%08x)!", i->param1, i->param2);
						break;
					case MF_MEDIA_ENGINE_EVENT_ABORT:
						abort = true;
						break;
					case MF_MEDIA_ENGINE_EVENT_CANPLAY:
						ready = true;
						break;
					}
				}
				LeaveCriticalSection(&m_cs);
			}
			if (abort || !ready) return E_FAIL;

			FAIL_RETURN(hr = m_engine->GetNativeVideoSize(cx, cy));

			D3D11_TEXTURE2D_DESC desc = {
				*cx,
				*cy,
				1,
				1,
				DXGI_FORMAT_B8G8R8A8_UNORM,
				{ 1, 0 },
				D3D11_USAGE_DEFAULT,
				D3D11_BIND_RENDER_TARGET,
				0,
				0
			};
			FAIL_RETURN(m_device->CreateTexture2D(&desc, nullptr, m_render.ReleaseAndGetAddressOf()));

			desc.Usage = D3D11_USAGE_STAGING;
			desc.BindFlags = 0;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			FAIL_RETURN(m_device->CreateTexture2D(&desc, nullptr, m_reader.ReleaseAndGetAddressOf()));

			return S_OK;
		}

		return E_FAIL;
	}

	template<typename _Callback>
	HRESULT Play(_Callback callback)
	{
		HRESULT hr = E_FAIL;
		if (m_engine && SUCCEEDED(hr = m_engine->Play()))
		{
			MFVideoNormalizedRect rect = { 0.0f, 0.0f, 1.0f, 1.0f };
			RECT rcTarget = { 0 };
			MFARGB bkgColor = { 0 };
			bool playing = true, skip = false;
			m_engine->GetNativeVideoSize((DWORD*)&rcTarget.right, (DWORD*)&rcTarget.bottom);
			while (playing && !skip)
			{
				LONGLONG pts;
				if (m_engine->OnVideoStreamTick(&pts) == S_OK &&
					m_engine->TransferVideoFrame(m_render.Get(), &rect, &rcTarget, &bkgColor) == S_OK)
				{
					D3D11_MAPPED_SUBRESOURCE res;
					// new frame available at the media engine so get it 
					m_context->CopyResource(m_reader.Get(), m_render.Get());
					m_context->Flush();
					m_context->Map(m_reader.Get(), 0, D3D11_MAP_READ, 0, &res);
					skip = callback(res.pData, res.RowPitch);
					m_context->Unmap(m_reader.Get(), 0);
				}

				if (WaitForSingleObject(m_event, 0) == WAIT_OBJECT_0)
				{
					EnterCriticalSection(&m_cs);
					for (auto i = m_events.begin(); i != m_events.end(); i = m_events.erase(i))
					{
						if (i->event == MF_MEDIA_ENGINE_EVENT_ENDED)
						{
							playing = false;
							break;
						}
					}
					LeaveCriticalSection(&m_cs);
				}
			}
			hr = skip ? S_FALSE : S_OK;
		}
		return hr;
	}

	HRESULT Pause()
	{
		if (m_engine)
		{
			HRESULT hr = m_engine->Pause();
			if (m_playing && SUCCEEDED(hr)) m_playing = false;
			return hr;
		}
		else
		{
			m_playing = false;
			return E_NOT_VALID_STATE;
		}
	}

	HRESULT Shutdown()
	{
		if (m_engine)
		{
			HRESULT hr = m_engine->Shutdown();
			if (m_playing && SUCCEEDED(hr)) m_playing = false;
			return hr;
		}
		else
		{
			m_playing = false;
			return E_NOT_VALID_STATE;
		}
	}

	bool IsPlaying() const
	{
		return m_playing;
	}


	// ***************************************************************************************
	// IUnknown
	// ***************************************************************************************
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject)
	{
		HRESULT hr = S_OK;
		if (!ppvObject) return E_POINTER;
		if (riid == IID_IMFMediaEngineNotify)
			*ppvObject = static_cast<IMFMediaEngineNotify*>(this);
		else if (riid == IID_IMFMediaEngineExtension)
			*ppvObject = static_cast<IMFMediaEngineExtension*>(this);
		else
			hr = CUnknown::QueryInterface(riid, ppvObject);
		return hr;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return 1;
	}

	virtual ULONG STDMETHODCALLTYPE Release(void)
	{
		return 1;
	}


	// ***************************************************************************************
	// IMFMediaEngineNotify
	// ***************************************************************************************
	virtual HRESULT STDMETHODCALLTYPE EventNotify(DWORD event, DWORD_PTR param1, DWORD param2)
	{
		EnterCriticalSection(&m_cs);
		if (event == MF_MEDIA_ENGINE_EVENT_ERROR)
		{
			ComPtr<IMFMediaError> error;
			m_engine->GetError(&error);
			m_events.push_back(Event(event, error->GetErrorCode(), error->GetExtendedErrorCode()));
		}
		else
		{
			m_events.push_back(Event(event, param1, param2));
		}
		LeaveCriticalSection(&m_cs);
		SetEvent(m_event);
		return S_OK;
	}


	// ***************************************************************************************
	// IMFMediaEngineExtension
	// ***************************************************************************************
	virtual HRESULT STDMETHODCALLTYPE CanPlayType(
		_In_  BOOL AudioOnly,
		_In_  BSTR MimeType,
		_Out_  MF_MEDIA_ENGINE_CANPLAY *pAnswer)
	{
		if (!pAnswer) return E_POINTER;
		if (_wcsicmp(MimeType, L"video/avi") == 0)
			*pAnswer = MF_MEDIA_ENGINE_CANPLAY_PROBABLY;
		else
			*pAnswer = MF_MEDIA_ENGINE_CANPLAY_NOT_SUPPORTED;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE BeginCreateObject(
		_In_  BSTR bstrURL,
		_In_opt_  IMFByteStream *pByteStream,
		_In_  MF_OBJECT_TYPE type,
		_Outptr_  IUnknown **ppIUnknownCancelCookie,
		_In_  IMFAsyncCallback *pCallback,
		_In_opt_  IUnknown *punkState)
	{
		if (!pCallback) return E_POINTER;
		switch (type)
		{
		case MF_OBJECT_BYTESTREAM:
			if (!bstrURL) return E_POINTER;
			break;
		case MF_OBJECT_MEDIASOURCE:
			if (!bstrURL && !pByteStream) return E_POINTER;
			break;
		default:
			return E_INVALIDARG;
		}

		ComPtr<IMFAsyncCallback> pcb;

		if (type == MF_OBJECT_BYTESTREAM)
		{
			if (!pByteStream)
			{
				pcb.Attach(new CMFAsyncCallback(bstrURL, punkState, pCallback));
			}
			else
			{
				return E_INVALIDARG;
			}
		}

		if (type == MF_OBJECT_MEDIASOURCE)
		{
			if (!pByteStream)
			{
				pcb.Attach(new CMFAsyncCallback(bstrURL, m_resolver.Get(), punkState, pCallback));
			}
			else
			{
				pcb.Attach(new CMFAsyncCallback(pByteStream, m_resolver.Get(), punkState, pCallback));
			}
		}

		return MFPutWorkItem2(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, 0, pcb.Get(), nullptr);
	}

	virtual HRESULT STDMETHODCALLTYPE CancelObjectCreation(
		_In_  IUnknown *pIUnknownCancelCookie)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE EndCreateObject(
		_In_  IMFAsyncResult *pResult,
		_Outptr_  IUnknown **ppObject)
	{
		return pResult->GetObject(ppObject);
	}


private:
	HANDLE m_event;
	CRITICAL_SECTION m_cs;

	ComPtr<ID3D11Device> m_device;
	ComPtr<ID3D11DeviceContext> m_context;
	ComPtr<IMFDXGIDeviceManager> m_manager;
	ComPtr<IMFMediaEngine> m_engine;
	ComPtr<IMFSourceResolver> m_resolver;
	ComPtr<ID3D11Texture2D> m_render, m_reader;

	std::list<Event> m_events;
	bool m_playing;

#undef FAIL_RETURN
};

extern "C" BOOL PAL_PlayAVI_Native(const char *lpszPath)
{
	CAVIPlayer player;
	HRESULT hr;
	DWORD cx, cy;

	if (FAILED(hr = player.Initialize())) return FALSE;
	if (FAILED(hr = player.Load(ConvertString(lpszPath), &cx, &cy))) return FALSE;

	PAL_ClearKeyState();

	SDL_Surface* surface = SDL_CreateRGBSurface(SDL_SWSURFACE, cx, cy, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	hr = player.Play([surface](const void* data, UINT pitch)->bool {
		if (pitch == surface->pitch)
		{
			memcpy(surface->pixels, data, surface->pitch * surface->h);
		}
		else
		{
			auto dst = (uint8_t*)surface->pixels - surface->pitch, src = (uint8_t*)data - pitch;
			auto length = surface->w << 2;
			for (int i = 0; i < surface->h; i++, memcpy(dst += surface->pitch, src += pitch, length));
		}
		VIDEO_DrawSurfaceToScreen(surface);
		UTIL_Delay(1);
		return (g_InputState.dwKeyPress & (kKeyMenu | kKeySearch)) != 0;
	});
	SDL_FreeSurface(surface);

	if (FAILED(hr)) return FALSE;

	if (hr == S_FALSE)
	{
		//
		// Simulate a short delay (like the original game)
		//
		UTIL_Delay(500);
	}

	hr = player.Pause();
	hr = player.Shutdown();

	return TRUE;
}
