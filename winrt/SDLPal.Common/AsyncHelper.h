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
// AsyncHelper.h: UWP support library for SDLPal.
//
// Original author: David Huang @ 1st July, 2014
//                  (C) 2014 Light Studio. All Rights Reserved.
//
// Modified by: Lou Yihua @ 2016
//

#pragma once
#include <ppltasks.h>
#include <Windows.h>
#include <synchapi.h>
using namespace Platform;

template <typename TResult>
inline TResult AWait(Windows::Foundation::IAsyncOperation<TResult>^ asyncOp, HANDLE eventHandle)
{
	TResult result;
	Exception^ exceptionObj = nullptr;
	ResetEvent(eventHandle);
	asyncOp->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<TResult>([&]
		(Windows::Foundation::IAsyncOperation<TResult>^ asyncInfo, Windows::Foundation::AsyncStatus asyncStatus)
	{
		try
		{
			result = asyncInfo->GetResults();
		}
		catch (Exception^ e)
		{
			exceptionObj = e;
		}
		SetEvent(eventHandle);
	});
	WaitForSingleObjectEx(eventHandle, INFINITE, FALSE);
	if (exceptionObj != nullptr) throw exceptionObj;
	return result;
}

template <typename TResult, typename TProgress>
inline TResult AWait(Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>^ asyncOp, HANDLE eventHandle)
{
	TResult result;
	Exception^ exceptionObj = nullptr;
	ResetEvent(eventHandle);
	asyncOp->Completed = ref new Windows::Foundation::AsyncOperationWithProgressCompletedHandler<TResult, TProgress>([&]
		(Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>^ asyncInfo, Windows::Foundation::AsyncStatus asyncStatus)
	{
		try
		{
			result = asyncInfo->GetResults();
		}
		catch (Exception^ e)
		{
			exceptionObj = e;
		}
		SetEvent(eventHandle);
	});
	WaitForSingleObjectEx(eventHandle, INFINITE, FALSE);
	if (exceptionObj != nullptr) throw exceptionObj;
	return result;
}


inline void AWait(Windows::Foundation::IAsyncAction^ asyncAc, HANDLE eventHandle)
{
	Exception^ exceptionObj = nullptr;
	asyncAc->Completed = ref new Windows::Foundation::AsyncActionCompletedHandler([&]
		(Windows::Foundation::IAsyncAction^ asyncInfo, Windows::Foundation::AsyncStatus asyncStatus)
	{
		try
		{
			asyncInfo->GetResults();
		}
		catch (Exception^ e)
		{
			exceptionObj = e;
		}
		SetEvent(eventHandle);
	});
	WaitForSingleObjectEx(eventHandle, INFINITE, FALSE);
	if (exceptionObj != nullptr) throw exceptionObj;
}

template <typename TResult>
inline TResult AWait(Windows::Foundation::IAsyncOperation<TResult>^ asyncOp)
{
	HANDLE handle = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);
	auto result = AWait(asyncOp, handle);
	CloseHandle(handle);
	return result;
}

template <typename TResult, typename TProgress>
inline TResult AWait(Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>^ asyncOp)
{
	HANDLE handle = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);
	auto result = AWait(asyncOp, handle);
	CloseHandle(handle);
	return result;
}

inline void AWait(Windows::Foundation::IAsyncAction^ asyncAc)
{
	HANDLE handle = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);
	AWait(asyncAc, handle);
	CloseHandle(handle);
}
