//
// MainPage.xaml.cpp
// MainPage 类的实现。
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "../SDLPal.Common/StringHelper.h"
#include "../SDLPal.Common/AsyncHelper.h"
#include "../../global.h"

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
	static_cast<App^>(Application::Current)->SetMainPage(this);
	LoadControlContents();

	Windows::UI::Xaml::Controls::Button^ buttons[] = { btnExport1, btnExport2, btnExport3, btnExport4, btnExport5 };
	for (int i = 0; i < 5; i++) m_buttons[i] = buttons[i];
	CheckSaveSlots();
}

void SDLPal::MainPage::SetPath(Windows::Storage::StorageFolder^ folder)
{
	tbGamePath->Text = folder->Path;
	tbGamePath->Tag = folder;
}

void SDLPal::MainPage::Export(Windows::Storage::StorageFile^ file, Platform::String^ slot)
{
	auto src = AWait(Windows::Storage::ApplicationData::Current->LocalFolder->GetFileAsync(slot), g_eventHandle);
	try
	{
		AWait(src->CopyAndReplaceAsync(file), g_eventHandle);
	}
	catch (Platform::Exception^ e)
	{
		(ref new Windows::UI::Popups::MessageDialog(e->Message))->ShowAsync();
	}
}

void SDLPal::MainPage::Import(Windows::Storage::StorageFile^ file, Platform::String^ slot)
{
	try
	{
		AWait(file->CopyAsync(Windows::Storage::ApplicationData::Current->LocalFolder, slot, Windows::Storage::NameCollisionOption::ReplaceExisting), g_eventHandle);
		m_buttons[_wtoi(slot->Data()) - 1]->IsEnabled = true;
	}
	catch (Platform::Exception^ e)
	{
		(ref new Windows::UI::Popups::MessageDialog(e->Message))->ShowAsync();
	}
}

void SDLPal::MainPage::LoadControlContents()
{
	if (gConfig.pszGamePath)
	{
		tbGamePath->Text = ConvertString(gConfig.pszGamePath);
		try { tbGamePath->Tag = AWait(Windows::Storage::StorageFolder::GetFolderFromPathAsync(tbGamePath->Text), g_eventHandle); }
		catch (Platform::Exception^) {}
	}
	else
	{
		tbGamePath->Tag = Windows::Storage::ApplicationData::Current->LocalFolder;
		tbGamePath->Text = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
	}

	if (gConfig.pszMsgFile) tbMsgFile->Text = ConvertString(gConfig.pszMsgFile);

	tsLanguage->IsOn = (gConfig.uCodePage == CP_GBK);
	tsIsDOS->IsOn = (gConfig.fIsWIN95 == FALSE);
	tsUseEmbedFont->IsOn = (gConfig.fUseEmbeddedFonts == TRUE);
	tsKeepAspect->IsOn = (gConfig.fKeepAspectRatio == TRUE);
	tsStereo->IsOn = (gConfig.iAudioChannels == 2);
	tsSurroundOPL->IsOn = (gConfig.fUseSurroundOPL == TRUE);

	slVolume->Value = gConfig.iVolume;
	slQuality->Value = gConfig.iResampleQuality;

	cbCD->SelectedIndex = (gConfig.eCDType == MUSIC_MP3) ? 0 : 1;
	if (gConfig.eMusicType == MUSIC_OGG)
		cbBGM->SelectedIndex = 2;
	else if (gConfig.eMusicType == MUSIC_MP3)
		cbBGM->SelectedIndex = 1;
	else
		cbBGM->SelectedIndex = 0;
	cbOPL->SelectedIndex = (int)gConfig.eOPLType;

	if (gConfig.iSampleRate <= 11025)
		cbSampleRate->SelectedIndex = 0;
	else if (gConfig.iSampleRate <= 22050)
		cbSampleRate->SelectedIndex = 1;
	else
		cbSampleRate->SelectedIndex = 2;

	if (gConfig.wAudioBufferSize <= 512)
		cbAudioBuffer->SelectedIndex = 0;
	else if (gConfig.wAudioBufferSize == 1024)
		cbAudioBuffer->SelectedIndex = 1;
	else if (gConfig.wAudioBufferSize == 2048)
		cbAudioBuffer->SelectedIndex = 2;
	else
		cbAudioBuffer->SelectedIndex = 3;

	if (gConfig.iOPLSampleRate <= 12429)
		cbOPLSR->SelectedIndex = 0;
	else if (gConfig.iSampleRate <= 24858)
		cbOPLSR->SelectedIndex = 1;
	else
		cbOPLSR->SelectedIndex = 2;

	tsUseEmbedFont->Visibility = tsIsDOS->IsOn ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
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

	gConfig.fIsWIN95 = tsIsDOS->IsOn ? FALSE : TRUE;
	gConfig.fUseEmbeddedFonts = tsUseEmbedFont->IsOn ? TRUE : FALSE;
	gConfig.fKeepAspectRatio = tsKeepAspect->IsOn ? TRUE : FALSE;
	gConfig.iAudioChannels = tsStereo->IsOn ? 2 : 1;
	gConfig.fUseSurroundOPL = tsSurroundOPL->IsOn ? TRUE : FALSE;

	gConfig.iVolume = (int)slVolume->Value;
	gConfig.iResampleQuality = (int)slQuality->Value;
	gConfig.uCodePage = tsLanguage->IsOn ? CP_GBK : CP_BIG5;

	gConfig.eCDType = (MUSICTYPE)(MUSIC_MP3 + cbCD->SelectedIndex);
	gConfig.eMusicType = (cbBGM->SelectedIndex >= 1) ? (MUSICTYPE)(MUSIC_MP3 + cbBGM->SelectedIndex) : MUSIC_RIX;
	gConfig.eOPLType = (OPLTYPE)cbOPL->SelectedIndex;

	gConfig.iSampleRate = wcstoul(static_cast<Platform::String^>(static_cast<ComboBoxItem^>(cbSampleRate->SelectedItem)->Content)->Data(), nullptr, 10);
	gConfig.iOPLSampleRate = wcstoul(static_cast<Platform::String^>(static_cast<ComboBoxItem^>(cbOPLSR->SelectedItem)->Content)->Data(), nullptr, 10);
	gConfig.wAudioBufferSize = wcstoul(static_cast<Platform::String^>(static_cast<ComboBoxItem^>(cbAudioBuffer->SelectedItem)->Content)->Data(), nullptr, 10);
}

void SDLPal::MainPage::CheckSaveSlots()
{
	static Platform::String^ slots[] = { "1.rpg", "2.rpg", "3.rpg", "4.rpg", "5.rpg" };
	auto folder = Windows::Storage::ApplicationData::Current->LocalFolder;
	for (int i = 0; i < 5; i++)
	{
		try
		{
			AWait(folder->GetFileAsync(slots[i]), g_eventHandle);
			m_buttons[i]->IsEnabled = true;
		}
		catch (Platform::Exception^)
		{
			m_buttons[i]->IsEnabled = false;
		}
	}
}

void SDLPal::MainPage::btnBrowse_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto picker = ref new Windows::Storage::Pickers::FolderPicker();
	picker->PickFolderAndContinue();
}

void SDLPal::MainPage::btnDefault_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	tbGamePath->Tag = Windows::Storage::ApplicationData::Current->LocalFolder;
	tbGamePath->Text = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
}

void SDLPal::MainPage::tsIsDOS_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (tsIsDOS->IsOn)
	{
		tsLanguage->IsOn = false;
		tsLanguage->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		tsUseEmbedFont->Visibility = Windows::UI::Xaml::Visibility::Visible;
	}
	else
	{
		tsLanguage->IsOn = (gConfig.uCodePage == CP_GBK);
		tsLanguage->Visibility = Windows::UI::Xaml::Visibility::Visible;
		tsUseEmbedFont->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	}
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
	SaveControlContents();
	auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
	auto handler = ref new Windows::UI::Popups::UICommandInvokedHandler(this, &MainPage::CloseUICommandHandler);
	auto dlg = ref new Windows::UI::Popups::MessageDialog(loader->GetString("MBContent"));
	dlg->Commands->Append(ref new Windows::UI::Popups::UICommand(loader->GetString("MBYes"), handler, safe_cast<Platform::Object^>(0)));
	dlg->Commands->Append(ref new Windows::UI::Popups::UICommand(loader->GetString("MBNo"), handler, safe_cast<Platform::Object^>(1)));
	dlg->ShowAsync();
}

void SDLPal::MainPage::btnImport_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
	auto picker = ref new Windows::Storage::Pickers::FileOpenPicker();
	picker->SettingsIdentifier = safe_cast<Platform::String^>(static_cast<Windows::UI::Xaml::Controls::Button^>(sender)->Tag);
	picker->ContinuationData->Insert("Slot", picker->SettingsIdentifier);
	picker->FileTypeFilter->Append(".rpg");
	picker->PickSingleFileAndContinue();
}


void SDLPal::MainPage::btnExport_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
	auto picker = ref new Windows::Storage::Pickers::FileSavePicker();
	picker->FileTypeChoices->Insert(loader->GetString("SaveSlotType"), ref new Platform::Collections::Vector<Platform::String^>(1, { ".rpg" }));
	picker->SettingsIdentifier = safe_cast<Platform::String^>(static_cast<Windows::UI::Xaml::Controls::Button^>(sender)->Tag);
	picker->SuggestedFileName = picker->SettingsIdentifier;
	picker->DefaultFileExtension = ".rpg";
	picker->ContinuationData->Insert("Slot", picker->SettingsIdentifier);
	picker->PickSaveFileAndContinue();
}

void SDLPal::MainPage::CloseUICommandHandler(Windows::UI::Popups::IUICommand^ command)
{
	gConfig.fLaunchSetting = ((int)command->Id == 0);
	PAL_SaveConfig();
	for (int i = 0; i < 5; i++) m_buttons[i] = nullptr;
	Application::Current->Exit();
}
