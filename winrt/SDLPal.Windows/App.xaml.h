//
// App.xaml.h
// App 类的声明。
//

#pragma once

#include "App.g.h"

namespace SDLPal
{
	/// <summary>
	/// 提供特定于应用程序的行为，以补充默认的应用程序类。
	/// </summary>
	ref class App sealed
	{
	protected:
		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e) override;

	internal:
		App();

		void SetMainPage(Windows::UI::Xaml::Controls::Page^ page) { _main_page = page; }

	private:
		Windows::UI::Xaml::Controls::Page^ _main_page;

		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
		void OnNavigationFailed(Platform::Object ^sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs ^e);
	};
}

extern HANDLE g_eventHandle;
