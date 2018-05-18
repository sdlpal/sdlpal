//
// MainPage.xaml.h
// MainPage 类的声明。
//

#pragma once

#include "MainPage.g.h"
#include <map>
#include "../../palcfg.h"

#ifdef main
# undef main
#endif


namespace SDLPal
{
	ref struct ButtonAttribute sealed
	{
		property Windows::UI::Xaml::FrameworkElement^ Object;
		property Platform::Array<Platform::String^>^  Filter;

		ButtonAttribute(Windows::UI::Xaml::FrameworkElement^ o, const Platform::Array<Platform::String^>^ f)
		{
			Object = o;
			Filter = f;
		}
	};

	ref struct AccessListEntry sealed
	{
		property Windows::UI::Xaml::Controls::TextBox^  text;
		property Windows::UI::Xaml::Controls::CheckBox^ check;
		property Platform::String^                      token;

		AccessListEntry(Windows::UI::Xaml::Controls::TextBox^ t, Windows::UI::Xaml::Controls::CheckBox^ c, Platform::String^ s)
		{
			text = t;
			check = c;
			token = s;
		}
	};

	/// <summary>
	/// 可用于自身或导航至 Frame 内部的空白页。
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();

		void SetPath(Windows::Storage::StorageFolder^ folder);
		void SetFile(Windows::UI::Xaml::Controls::TextBox^ target, Windows::Storage::StorageFile^ file);

	protected:
		void LoadControlContents(bool loadDefault);
		void SaveControlContents();

	private:
		Platform::Collections::Map<Platform::String^, ButtonAttribute^>^ m_controls;
		Windows::ApplicationModel::Resources::ResourceLoader^ m_resLdr;
		std::map<PALCFG_ITEM, AccessListEntry^> m_acl;

		void btnBrowseFolder_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void cbBGM_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e);
		void btnDefault_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void btnReset_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void btnFinish_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void btnBrowseFileOpen_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void btnBrowseFileSave_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void btnClearFile_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Page_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void cbUseFile_CheckChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void cbEnableGLSL_CheckChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

	internal:
		static MainPage^ Current;
	};
}
