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
		void Export(Windows::Storage::StorageFile^ file, Platform::String^ slot);
		void Import(Windows::Storage::StorageFile^ file, Platform::String^ slot);

	protected:
		void LoadControlContents();
		void SaveControlContents();
		void CheckSaveSlots();

	private:
		Windows::UI::Xaml::Controls::Button^ m_buttons[5];

		void CloseUICommandHandler(Windows::UI::Popups::IUICommand^ command);

		void btnBrowse_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void btnDefault_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void tsIsDOS_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void cbBGM_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e);
		void btnReset_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void btnFinish_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void btnImport_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void btnExport_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
