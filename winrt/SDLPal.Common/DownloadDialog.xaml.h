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
		DownloadDialog(Platform::String^ link, Windows::ApplicationModel::Resources::ResourceLoader^ ldr, Windows::Storage::StorageFolder^ folder, Windows::Storage::Streams::IRandomAccessStream^ stream);

	private:
		Platform::String^ m_link;
		Windows::ApplicationModel::Resources::ResourceLoader^ m_resLdr;
		Windows::Storage::StorageFolder^ m_folder;
		Windows::Storage::Streams::IRandomAccessStream^ m_stream;
		uint64_t m_totalBytes;
		bool m_Closable, m_InitialPhase;

		Platform::String^ FormatProgress();

		void OnPrimaryButtonClick(Windows::UI::Xaml::Controls::ContentDialog^ sender, Windows::UI::Xaml::Controls::ContentDialogButtonClickEventArgs^ args);
		void OnClosing(Windows::UI::Xaml::Controls::ContentDialog^ sender, Windows::UI::Xaml::Controls::ContentDialogClosingEventArgs^ args);
		void OnOpened(Windows::UI::Xaml::Controls::ContentDialog^ sender, Windows::UI::Xaml::Controls::ContentDialogOpenedEventArgs^ args);
	};
}
