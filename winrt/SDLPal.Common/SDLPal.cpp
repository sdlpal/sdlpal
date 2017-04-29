#include "pch.h"

#include <wrl.h>
#include <windows.h>
#include "../SDLPal.Common/AsyncHelper.h"
#include "../../global.h"
#include "../../palcfg.h"
#include "App.xaml.h"
extern "C" {
#include "../../native_midi/native_midi.h"
}

HANDLE g_eventHandle = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	if (FAILED(Windows::Foundation::Initialize(RO_INIT_MULTITHREADED))) {
		return 1;
	}

	PAL_LoadConfig(TRUE);

	bool last_crashed = false;
	try
	{
		// Check the 'running' file to determine whether the program is abnormally terminated last time.
		// If so, force to launch the setting page.
		auto file = AWait(Windows::Storage::ApplicationData::Current->LocalFolder->GetFileAsync("running"), g_eventHandle);
		// When there is a debugger, ignore the last crash state, as it can be recovered by the debugger.
		if (!IsDebuggerPresent())
		{
			gConfig.fLaunchSetting = TRUE;
			last_crashed = true;
		}
		AWait(file->DeleteAsync(), g_eventHandle);
	}
	catch (Platform::Exception^)
	{
	}

	if (gConfig.fLaunchSetting || last_crashed)
	{
		Windows::UI::Xaml::Application::Start(
			ref new Windows::UI::Xaml::ApplicationInitializationCallback(
				[last_crashed](Windows::UI::Xaml::ApplicationInitializationCallbackParams^ p) {
					auto app = ref new SDLPal::App();
					app->LastCrashed = last_crashed;
				}
			)
		);
	}
	else
		SDL_WinRTRunApp(SDL_main, NULL);

	return 0;
}
