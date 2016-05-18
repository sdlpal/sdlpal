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
	public:
		App();

		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e) override;

		void SetMainPage(Windows::UI::Xaml::Controls::Page^ page);

	protected:
		virtual void OnActivated(Windows::ApplicationModel::Activation::IActivatedEventArgs^ args) override;

	private:
		Windows::UI::Xaml::Media::Animation::TransitionCollection^ _transitions;
		Windows::Foundation::EventRegistrationToken _firstNavigatedToken;
		Windows::UI::Xaml::Controls::Page^ _main_page;

		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
		void RootFrame_FirstNavigated(Platform::Object^ sender, Windows::UI::Xaml::Navigation::NavigationEventArgs^ e);
	};

}

extern HANDLE g_eventHandle;
