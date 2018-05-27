//
// MainPage.xaml.cpp
// MainPage 类的实现。
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "StringHelper.h"
#include "AsyncHelper.h"
#include "../../global.h"
#include "../../palcfg.h"
#include "../../generated.h"

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

static Platform::String^ msg_file_exts[] = { ".msg" };
static Platform::String^ font_file_exts[] = { ".bdf" };
static Platform::String^ log_file_exts[] = { ".log" };
static Platform::String^ shader_file_exts[] = { ".glsl" };

MainPage^ MainPage::Current = nullptr;

MainPage::MainPage()
{
	InitializeComponent();

	Current = this;

	m_controls = ref new Platform::Collections::Map<Platform::String^, ButtonAttribute^>();
	m_controls->Insert(btnBrowseMsgFile->Name, ref new ButtonAttribute(tbMsgFile, ref new Platform::Array<Platform::String^>(msg_file_exts, sizeof(msg_file_exts) / sizeof(msg_file_exts[0]))));
	m_controls->Insert(btnBrowseFontFile->Name, ref new ButtonAttribute(tbFontFile, ref new Platform::Array<Platform::String^>(font_file_exts, sizeof(font_file_exts) / sizeof(font_file_exts[0]))));
	m_controls->Insert(btnBrowseLogFile->Name, ref new ButtonAttribute(tbLogFile, ref new Platform::Array<Platform::String^>(log_file_exts, sizeof(log_file_exts) / sizeof(log_file_exts[0]))));
	m_controls->Insert(btnBrowseShaderFile->Name, ref new ButtonAttribute(tbShaderFile, ref new Platform::Array<Platform::String^>(shader_file_exts, sizeof(shader_file_exts) / sizeof(shader_file_exts[0]))));
	m_controls->Insert(cbUseMsgFile->Name, ref new ButtonAttribute(gridMsgFile, nullptr));
	m_controls->Insert(cbUseFontFile->Name, ref new ButtonAttribute(gridFontFile, nullptr));
	m_controls->Insert(cbUseLogFile->Name, ref new ButtonAttribute(gridLogFile, nullptr));
	m_controls->Insert(cbEnableGLSL->Name, ref new ButtonAttribute(gridGLSL, nullptr));

	m_acl[PALCFG_GAMEPATH] = ref new AccessListEntry(tbGamePath, nullptr, ConvertString(PAL_ConfigName(PALCFG_GAMEPATH)));
	m_acl[PALCFG_SAVEPATH] = ref new AccessListEntry(tbGamePath, nullptr, ConvertString(PAL_ConfigName(PALCFG_SAVEPATH)));
	m_acl[PALCFG_MESSAGEFILE] = ref new AccessListEntry(tbMsgFile, cbUseMsgFile, ConvertString(PAL_ConfigName(PALCFG_MESSAGEFILE)));
	m_acl[PALCFG_FONTFILE] = ref new AccessListEntry(tbFontFile, cbUseFontFile, ConvertString(PAL_ConfigName(PALCFG_FONTFILE)));
	m_acl[PALCFG_LOGFILE] = ref new AccessListEntry(tbLogFile, cbUseLogFile, ConvertString(PAL_ConfigName(PALCFG_LOGFILE)));
	m_acl[PALCFG_SHADER] = ref new AccessListEntry(tbShaderFile, cbEnableGLSL, ConvertString(PAL_ConfigName(PALCFG_SHADER)));

	tbGitRevision->Text = "  " PAL_GIT_REVISION;

	LoadControlContents(false);

	m_resLdr = Windows::ApplicationModel::Resources::ResourceLoader::GetForCurrentView();
	if (static_cast<App^>(Application::Current)->LastCrashed)
	{
		(ref new Windows::UI::Popups::MessageDialog(m_resLdr->GetString("MBCrashContent")))->ShowAsync();
	}
}

void SDLPal::MainPage::LoadControlContents(bool loadDefault)
{
	for (auto i = m_acl.begin(); i != m_acl.end(); i++)
	{
		auto item = i->second;
		item->text->Text = "";
		item->text->Tag = nullptr;
		if (item->check)
		{
			item->check->IsChecked = false;
			m_controls->Lookup(item->check->Name)->Object->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		}
	}

	if (!loadDefault)
	{
		// Always load folder/files from FutureAccessList
		std::list<Platform::String^> invalid_tokens;
		auto fal = Windows::Storage::AccessCache::StorageApplicationPermissions::FutureAccessList;
		for each (auto entry in fal->Entries)
		{
			auto& ace = m_acl[PAL_ConfigIndex(ConvertString(entry.Token).c_str())];
			ace->text->Tag = AWait(fal->GetItemAsync(entry.Token), g_eventHandle);
			if (ace->text->Tag)
				ace->text->Text = entry.Metadata;
			else
				invalid_tokens.push_back(entry.Token);
			if (ace->check)
			{
				auto grid = m_controls->Lookup(ace->check->Name)->Object;
				ace->check->IsChecked = (ace->text->Tag != nullptr);
				grid->Visibility = ace->check->IsChecked->Value ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
			}
		}
		for (auto i = invalid_tokens.begin(); i != invalid_tokens.end(); fal->Remove(*i++));
	}

	tsKeepAspect->IsOn = (gConfig.fKeepAspectRatio == TRUE);
	tsStereo->IsOn = (gConfig.iAudioChannels == 2);
	tsSurroundOPL->IsOn = (gConfig.fUseSurroundOPL == TRUE);
	tsTouchOverlay->IsOn = (gConfig.fUseTouchOverlay == TRUE);
	tsEnableAVI->IsOn = (gConfig.fEnableAviPlay == TRUE);
	tsEnableHDR->IsOn = (gConfig.fEnableHDR == TRUE);

	slMusicVolume->Value = gConfig.iMusicVolume;
	slSoundVolume->Value = gConfig.iSoundVolume;
	slQuality->Value = gConfig.iResampleQuality;
	cbLogLevel->SelectedIndex = (int)gConfig.iLogLevel;

	cbCD->SelectedIndex = (gConfig.eCDType == MUSIC_MP3) ? 0 : 1;
	cbBGM->SelectedIndex = (gConfig.eMusicType <= MUSIC_OGG) ? gConfig.eMusicType : MUSIC_RIX;
	cbOPLCore->SelectedIndex = (int)gConfig.eOPLCore;
	cbOPLChip->SelectedIndex = (int)gConfig.eOPLChip;
	cbEnableGLSL->IsChecked = gConfig.fEnableGLSL == TRUE;

	tbShaderFile->Text = ConvertString(gConfig.pszShader);
	tbWindowWidth->Text = ConvertString(std::to_string(gConfig.dwScreenWidth));
	tbWindowHeight->Text = ConvertString(std::to_string(gConfig.dwScreenHeight));
	tbTextureWidth->Text = ConvertString(std::to_string(gConfig.dwTextureWidth));
	tbTextureHeight->Text = ConvertString(std::to_string(gConfig.dwTextureHeight));

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
	// All folders/files are not stored in config file, as they are store in FutureAcessList
	if (gConfig.pszGamePath) { free(gConfig.pszGamePath); gConfig.pszGamePath = nullptr; }
	if (gConfig.pszMsgFile) { free(gConfig.pszMsgFile); gConfig.pszMsgFile = nullptr; }
	if (gConfig.pszFontFile) { free(gConfig.pszFontFile); gConfig.pszFontFile = nullptr; }
	if (gConfig.pszLogFile) { free(gConfig.pszLogFile); gConfig.pszLogFile = nullptr; }

	gConfig.fKeepAspectRatio = tsKeepAspect->IsOn ? TRUE : FALSE;
	gConfig.iAudioChannels = tsStereo->IsOn ? 2 : 1;
	gConfig.fUseSurroundOPL = tsSurroundOPL->IsOn ? TRUE : FALSE;
	gConfig.fUseTouchOverlay = tsTouchOverlay->IsOn ? TRUE : FALSE;
	gConfig.fEnableAviPlay = tsEnableAVI->IsOn ? TRUE : FALSE;
	gConfig.fEnableGLSL = cbEnableGLSL->IsChecked->Value ? TRUE : FALSE;
	gConfig.fEnableHDR = tsEnableHDR->IsOn ? TRUE : FALSE;

	gConfig.iMusicVolume = (int)slMusicVolume->Value;
	gConfig.iSoundVolume = (int)slSoundVolume->Value;
	gConfig.iResampleQuality = (int)slQuality->Value;
	gConfig.iLogLevel = (LOGLEVEL)cbLogLevel->SelectedIndex;
	gConfig.dwScreenWidth = _wtoi(tbWindowWidth->Text->Data());
	gConfig.dwScreenHeight = _wtoi(tbWindowHeight->Text->Data());
	gConfig.dwTextureWidth = _wtoi(tbTextureWidth->Text->Data());
	gConfig.dwTextureHeight = _wtoi(tbTextureHeight->Text->Data());

	gConfig.eCDType = (MUSICTYPE)(MUSIC_MP3 + cbCD->SelectedIndex);
	gConfig.eMusicType = (MUSICTYPE)cbBGM->SelectedIndex;
	gConfig.eOPLCore = (OPLCORE_TYPE)cbOPLCore->SelectedIndex;
	gConfig.eOPLChip = gConfig.eOPLCore == OPLCORE_NUKED ? OPLCHIP_OPL3 : (OPLCHIP_TYPE)cbOPLChip->SelectedIndex;

	gConfig.iSampleRate = wcstoul(static_cast<Platform::String^>(static_cast<ComboBoxItem^>(cbSampleRate->SelectedItem)->Content)->Data(), nullptr, 10);
	gConfig.iOPLSampleRate = wcstoul(static_cast<Platform::String^>(static_cast<ComboBoxItem^>(cbOPLSR->SelectedItem)->Content)->Data(), nullptr, 10);
	gConfig.wAudioBufferSize = wcstoul(static_cast<Platform::String^>(static_cast<ComboBoxItem^>(cbAudioBuffer->SelectedItem)->Content)->Data(), nullptr, 10);
}

void SDLPal::MainPage::cbBGM_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
	auto visibility = (cbBGM->SelectedIndex == MUSIC_RIX) ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
	cbOPLCore->Visibility = visibility;
	cbOPLChip->Visibility = visibility;
	cbOPLSR->Visibility = visibility;
	tsSurroundOPL->Visibility = visibility;
}

void SDLPal::MainPage::btnDefault_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	PAL_LoadConfig(FALSE);
	LoadControlContents(true);
}

void SDLPal::MainPage::btnReset_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	PAL_LoadConfig(TRUE);
	LoadControlContents(false);
}

void SDLPal::MainPage::btnFinish_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (tbGamePath->Text->Length() > 0)
	{
		auto fal = Windows::Storage::AccessCache::StorageApplicationPermissions::FutureAccessList;
		for (auto i = m_acl.begin(); i != m_acl.end(); i++)
		{
			auto item = i->second;
			auto check = item->check ? item->check->IsChecked->Value : true;
			if (check && item->text->Tag)
				fal->AddOrReplace(item->token, safe_cast<Windows::Storage::IStorageItem^>(item->text->Tag), item->text->Text);
			else if (fal->ContainsItem(item->token))
				fal->Remove(item->token);
		}

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

void SDLPal::MainPage::SetFile(Windows::UI::Xaml::Controls::TextBox^ target, Windows::Storage::StorageFile^ file)
{
	if (target && file)
	{
		target->Text = file->Path;
		target->Tag = file;
	}
}

void SDLPal::MainPage::btnBrowseFolder_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto picker = ref new Windows::Storage::Pickers::FolderPicker();
	picker->FileTypeFilter->Append("*");
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
	picker->PickFolderAndContinue();
#else
	concurrency::create_task(picker->PickSingleFolderAsync()).then([this](Windows::Storage::StorageFolder^ folder) { SetPath(folder); });
#endif
}

void SDLPal::MainPage::btnBrowseFileOpen_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto button = static_cast<Windows::UI::Xaml::Controls::Button^>(sender);
	auto target = m_controls->Lookup(button->Name);
	auto picker = ref new Windows::Storage::Pickers::FileOpenPicker();
	picker->FileTypeFilter->ReplaceAll(target->Filter);
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
	picker->ContinuationData->Insert("Target", button->Name);
	picker->PickSingleFileAndContinue();
#else
	concurrency::create_task(picker->PickSingleFileAsync()).then(
		[this, target](Windows::Storage::StorageFile^ file) {
			SetFile(static_cast<Windows::UI::Xaml::Controls::TextBox^>(target->Object), file);
		}
	);
#endif
}

void SDLPal::MainPage::btnBrowseFileSave_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto button = static_cast<Windows::UI::Xaml::Controls::Button^>(sender);
	auto target = m_controls->Lookup(button->Name);
	auto picker = ref new Windows::Storage::Pickers::FileSavePicker();
	picker->FileTypeChoices->Insert(m_resLdr->GetString("LogFileType"), ref new Platform::Collections::Vector<Platform::String^>(target->Filter));
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
	picker->ContinuationData->Insert("Target", button->Name);
	picker->PickSaveFileAndContinue();
#else
	concurrency::create_task(picker->PickSaveFileAsync()).then(
		[this, target](Windows::Storage::StorageFile^ file) {
		SetFile(static_cast<Windows::UI::Xaml::Controls::TextBox^>(target->Object), file);
	}
	);
#endif
}

void SDLPal::MainPage::cbUseFile_CheckChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto checker = static_cast<Windows::UI::Xaml::Controls::CheckBox^>(sender);
	auto attr = m_controls->Lookup(checker->Name);
	attr->Object->Visibility = checker->IsChecked->Value ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
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

void SDLPal::MainPage::cbEnableGLSL_CheckChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto checker = static_cast<Windows::UI::Xaml::Controls::CheckBox^>(sender);
	auto attr = m_controls->Lookup(checker->Name);
	attr->Object->Visibility = checker->IsChecked->Value ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
}
