#include "pch.h"

#include <wrl.h>
#include <windows.h>
#include "../SDLPal.Common/AsyncHelper.h"
#include "../../global.h"
#include "App.xaml.h"

HANDLE g_eventHandle;

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	if (FAILED(Windows::Foundation::Initialize(RO_INIT_MULTITHREADED))) {
		return 1;
	}

	g_eventHandle = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);

	PAL_LoadConfig(TRUE);

	if (gConfig.fLaunchSetting)
	{
		Windows::UI::Xaml::Application::Start(
			ref new Windows::UI::Xaml::ApplicationInitializationCallback(
				[](Windows::UI::Xaml::ApplicationInitializationCallbackParams^ p) {
					auto app = ref new SDLPal::App();
				}
			)
		);
	}
	else
		SDL_WinRTRunApp(SDL_main, NULL);

	return 0;
}
