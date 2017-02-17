//
// MainPage.xaml.h
// MainPage 类的声明。
//

#pragma once

#include "MainPage.g.h"

namespace SDLPal
{
	/// <summary>
	/// 可用于自身或导航至 Frame 内部的空白页。
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();

		void SetPath(Windows::Storage::StorageFolder^ folder);
		void SetFile(Windows::Storage::StorageFile^ file);

	protected:
		void LoadControlContents();
		void SaveControlContents();

	private:
		Windows::ApplicationModel::Resources::ResourceLoader^ m_resLdr;

		void btnBrowse_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void cbBGM_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e);
		void btnReset_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void btnFinish_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void btnBrowseFile_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void btnClearFile_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Page_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
