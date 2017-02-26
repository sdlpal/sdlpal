//
// MainPage.xaml.cpp
// MainPage 类的实现。
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "StringHelper.h"
#include "AsyncHelper.h"
#include "../../src/global.h"
#include "../../src/palcfg.h"

using namespace SDLPal;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

MainPage::MainPage()
{
	InitializeComponent();
	LoadControlContents();

	m_resLdr = Windows::ApplicationModel::Resources::ResourceLoader::GetForCurrentView();
	auto app = static_cast<App^>(Application::Current);
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
	app->Page = this;
#endif
	if (app->LastCrashed)
		(ref new Windows::UI::Popups::MessageDialog(m_resLdr->GetString("MBCrashContent")))->ShowAsync();
}

void SDLPal::MainPage::LoadControlContents()
{
	if (gConfig.pszGamePath) tbGamePath->Text = ConvertString(gConfig.pszGamePath);
	if (gConfig.pszMsgFile) tbMsgFile->Text = ConvertString(gConfig.pszMsgFile);

	tsLanguage->IsOn = (gConfig.uCodePage == CP_GBK);
	tsUseEmbedFont->IsOn = (gConfig.fUseEmbeddedFonts == TRUE);
	tsKeepAspect->IsOn = (gConfig.fKeepAspectRatio == TRUE);
	tsStereo->IsOn = (gConfig.iAudioChannels == 2);
	tsSurroundOPL->IsOn = (gConfig.fUseSurroundOPL == TRUE);
	tsTouchOverlay->IsOn = (gConfig.fUseTouchOverlay == TRUE);

	slMusicVolume->Value = gConfig.iMusicVolume;
	slSoundVolume->Value = gConfig.iSoundVolume;
	slQuality->Value = gConfig.iResampleQuality;

	cbCD->SelectedIndex = (gConfig.eCDType == MUSIC_MP3) ? 0 : 1;
	cbBGM->SelectedIndex = (gConfig.eMusicType >= MUSIC_RIX && gConfig.eMusicType <= MUSIC_OGG) ? (gConfig.eMusicType - MUSIC_RIX) : 0;
	cbOPL->SelectedIndex = (int)gConfig.eOPLType;

	if (gConfig.iSampleRate <= 11025)
		cbSampleRate->SelectedIndex = 0;
	else if (gConfig.iSampleRate <= 22050)
		cbSampleRate->SelectedIndex = 1;
	else
		cbSampleRate->SelectedIndex = 2;

	auto wValue = gConfig.wAudioBufferSize >> 10;
	unsigned int index = 0;
	while (wValue) { index++; wValue >>= 1; }
	if (index >= cbAudioBuffer->Items->Size)
		cbAudioBuffer->SelectedIndex = cbAudioBuffer->Items->Size - 1;
	else
		cbAudioBuffer->SelectedIndex = index;

	if (gConfig.iOPLSampleRate <= 12429)
		cbOPLSR->SelectedIndex = 0;
	else if (gConfig.iSampleRate <= 24858)
		cbOPLSR->SelectedIndex = 1;
	else
		cbOPLSR->SelectedIndex = 2;
}

void SDLPal::MainPage::SaveControlContents()
{
	std::wstring path;

	if (gConfig.pszGamePath) free(gConfig.pszGamePath);
	path.assign(tbGamePath->Text->Data());
	if (path.back() != '\\') path.append(L"\\");
	gConfig.pszGamePath = strdup(ConvertString(path).c_str());

	if (gConfig.pszMsgFile) { free(gConfig.pszMsgFile); gConfig.pszMsgFile = NULL; }
	gConfig.pszMsgFile = (tbMsgFile->Text->Length() > 0) ? strdup(ConvertString(tbMsgFile->Text).c_str()) : nullptr;

	gConfig.fUseEmbeddedFonts = tsUseEmbedFont->IsOn ? TRUE : FALSE;
	gConfig.fKeepAspectRatio = tsKeepAspect->IsOn ? TRUE : FALSE;
	gConfig.iAudioChannels = tsStereo->IsOn ? 2 : 1;
	gConfig.fUseSurroundOPL = tsSurroundOPL->IsOn ? TRUE : FALSE;
	gConfig.fUseTouchOverlay = tsTouchOverlay->IsOn ? TRUE : FALSE;

	gConfig.iMusicVolume = (int)slMusicVolume->Value;
	gConfig.iSoundVolume = (int)slSoundVolume->Value;
	gConfig.iResampleQuality = (int)slQuality->Value;
	gConfig.uCodePage = tsLanguage->IsOn ? CP_GBK : CP_BIG5;

	gConfig.eCDType = (MUSICTYPE)(MUSIC_MP3 + cbCD->SelectedIndex);
	gConfig.eMusicType = (MUSICTYPE)(MUSIC_RIX + cbBGM->SelectedIndex);
	gConfig.eOPLType = (OPLTYPE)cbOPL->SelectedIndex;

	gConfig.iSampleRate = wcstoul(static_cast<Platform::String^>(static_cast<ComboBoxItem^>(cbSampleRate->SelectedItem)->Content)->Data(), nullptr, 10);
	gConfig.iOPLSampleRate = wcstoul(static_cast<Platform::String^>(static_cast<ComboBoxItem^>(cbOPLSR->SelectedItem)->Content)->Data(), nullptr, 10);
	gConfig.wAudioBufferSize = wcstoul(static_cast<Platform::String^>(static_cast<ComboBoxItem^>(cbAudioBuffer->SelectedItem)->Content)->Data(), nullptr, 10);
}

void SDLPal::MainPage::cbBGM_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
	auto visibility = (cbBGM->SelectedIndex == 0) ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
	cbOPL->Visibility = visibility;
	cbOPLSR->Visibility = visibility;
	tsSurroundOPL->Visibility = visibility;
}


void SDLPal::MainPage::btnReset_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	PAL_LoadConfig(FALSE);
	LoadControlContents();
}

void SDLPal::MainPage::btnFinish_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (tbGamePath->Text->Length() > 0)
	{
		auto mru_list = Windows::Storage::AccessCache::StorageApplicationPermissions::MostRecentlyUsedList;
		if (tbGamePath->Tag) mru_list->Add(safe_cast<Windows::Storage::StorageFolder^>(tbGamePath->Tag), tbGamePath->Text);
		if (tbMsgFile->Tag) mru_list->Add(safe_cast<Windows::Storage::StorageFile^>(tbMsgFile->Tag), tbMsgFile->Text);

		SaveControlContents();
		gConfig.fLaunchSetting = FALSE;
		PAL_SaveConfig();

		auto dlg = ref new Windows::UI::Popups::MessageDialog(m_resLdr->GetString("MBExitContent"));
		dlg->Title = m_resLdr->GetString("MBExitTitle");
		concurrency::create_task(dlg->ShowAsync()).then([] (Windows::UI::Popups::IUICommand^ command) {
			Application::Current->Exit();
		});
	}
	else
	{
		(ref new Windows::UI::Popups::MessageDialog(m_resLdr->GetString("MBEmptyContent")))->ShowAsync();
	}
}

void SDLPal::MainPage::btnClearFile_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	tbMsgFile->Text = "";
	tbMsgFile->Tag = nullptr;
}

void SDLPal::MainPage::SetPath(Windows::Storage::StorageFolder^ folder)
{
	if (folder)
	{
		tbGamePath->Text = folder->Path;
		tbGamePath->Tag = folder;
	}
}

void SDLPal::MainPage::SetFile(Windows::Storage::StorageFile^ file)
{
	if (file)
	{
		tbMsgFile->Text = file->Path;
		tbMsgFile->Tag = file;
	}
}

void SDLPal::MainPage::btnBrowse_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto picker = ref new Windows::Storage::Pickers::FolderPicker();
	picker->FileTypeFilter->Append("*");
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
	picker->PickFolderAndContinue();
#else
	concurrency::create_task(picker->PickSingleFolderAsync()).then([this](Windows::Storage::StorageFolder^ folder) { SetPath(folder); });
#endif
}

void SDLPal::MainPage::btnBrowseFile_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto picker = ref new Windows::Storage::Pickers::FileOpenPicker();
	picker->FileTypeFilter->Append("*");
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
	picker->PickSingleFileAndContinue();
#else
	concurrency::create_task(picker->PickSingleFileAsync()).then([this](Windows::Storage::StorageFile^ file) { SetFile(file); });
#endif
}

void SDLPal::MainPage::Page_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
#if NTDDI_VERSION >= NTDDI_WIN10
	if (!Windows::Foundation::Metadata::ApiInformation::IsTypePresent("Windows.UI.ViewManagement.StatusBar")) return;
#endif
#if NTDDI_VERSION >= NTDDI_WIN10 || WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
	auto statusBar = Windows::UI::ViewManagement::StatusBar::GetForCurrentView();
	concurrency::create_task(statusBar->ShowAsync()).then([statusBar]() { statusBar->BackgroundOpacity = 1.0; });
#endif
}
