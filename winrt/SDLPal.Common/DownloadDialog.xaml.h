//
// DownloadDialog.xaml.h
// DownloadDialog 类的声明
//

#pragma once

#include "DownloadDialog.g.h"

namespace SDLPal
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class DownloadDialog sealed
	{
	public:
		DownloadDialog(Windows::ApplicationModel::Resources::ResourceLoader^ ldr, Windows::Storage::StorageFolder^ folder, Windows::Storage::Streams::IRandomAccessStream^ stream, Platform::String^ msgfile, double w, double h, bool from_url);

		property Windows::UI::Xaml::Controls::ContentDialogResult Result;

	private:
		Windows::ApplicationModel::Resources::ResourceLoader^ m_resLdr;
		Windows::Storage::StorageFolder^ m_folder;
		Windows::Storage::Streams::IRandomAccessStream^ m_stream;
		Platform::String^ m_msgfile;
		double m_width, m_height;
		uint64_t m_totalBytes;
		bool m_Closable, m_Downloading;

		Platform::String^ FormatProgress();
		void DoDownload(Platform::String^ url);

		void OnPrimaryButtonClick(Windows::UI::Xaml::Controls::ContentDialog^ sender, Windows::UI::Xaml::Controls::ContentDialogButtonClickEventArgs^ args);
		void OnClosing(Windows::UI::Xaml::Controls::ContentDialog^ sender, Windows::UI::Xaml::Controls::ContentDialogClosingEventArgs^ args);
		void OnOpened(Windows::UI::Xaml::Controls::ContentDialog^ sender, Windows::UI::Xaml::Controls::ContentDialogOpenedEventArgs^ args);
		void OnNavigateStart(Windows::UI::Xaml::Controls::WebView^ sender, Windows::UI::Xaml::Controls::WebViewNavigationStartingEventArgs^ args);
		void OnDOMContentLoaded(Windows::UI::Xaml::Controls::WebView^ sender, Windows::UI::Xaml::Controls::WebViewDOMContentLoadedEventArgs^ args);
		void OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);
		void OnClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
