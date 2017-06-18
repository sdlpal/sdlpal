//
// MainPage.xaml.cpp
// MainPage 类的实现。
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "DownloadDialog.xaml.h"
#include "StringHelper.h"
#include "AsyncHelper.h"
#include "global.h"
#include "palcfg.h"
#include "util.h"
#include "generated.h"

using namespace SDLPal;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::Storage::AccessCache;
using namespace Windows::UI::Core;
using namespace Windows::UI::Popups;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

MainPage^ MainPage::Current = nullptr;

MainPage::MainPage()
	: m_dlg(nullptr)
{
	InitializeComponent();

	Current = this;

	m_controls = ref new Map<String^, FrameworkElement^>();
	m_controls->Insert(btnBrowseMsgFile->Name, tbMsgFile);
	m_controls->Insert(btnBrowseFontFile->Name, tbFontFile);
	m_controls->Insert(btnBrowseLogFile->Name, tbLogFile);
	m_controls->Insert(cbUseMsgFile->Name, gridMsgFile);
	m_controls->Insert(cbUseFontFile->Name, gridFontFile);
	m_controls->Insert(cbUseLogFile->Name, gridLogFile);

	m_acl[PALCFG_GAMEPATH] = ref new AccessListEntry(tbGamePath, nullptr, ConvertString(PAL_ConfigName(PALCFG_GAMEPATH)));
	m_acl[PALCFG_SAVEPATH] = ref new AccessListEntry(tbGamePath, nullptr, ConvertString(PAL_ConfigName(PALCFG_SAVEPATH)));
	m_acl[PALCFG_MESSAGEFILE] = ref new AccessListEntry(tbMsgFile, cbUseMsgFile, ConvertString(PAL_ConfigName(PALCFG_MESSAGEFILE)));
	m_acl[PALCFG_FONTFILE] = ref new AccessListEntry(tbFontFile, cbUseFontFile, ConvertString(PAL_ConfigName(PALCFG_FONTFILE)));
	m_acl[PALCFG_LOGFILE] = ref new AccessListEntry(tbLogFile, cbUseLogFile, ConvertString(PAL_ConfigName(PALCFG_LOGFILE)));

	tbMsgFile->Tag = ConvertString(PAL_ConfigName(PALCFG_MESSAGEFILE));
	tbFontFile->Tag = ConvertString(PAL_ConfigName(PALCFG_MESSAGEFILE));
	tbLogFile->Tag = ConvertString(PAL_ConfigName(PALCFG_MESSAGEFILE));

	tbGitRevision->Text = "  " PAL_GIT_REVISION;

	LoadControlContents(false);

	m_resLdr = Windows::ApplicationModel::Resources::ResourceLoader::GetForCurrentView();
	if (static_cast<App^>(Application::Current)->LastCrashed)
	{
		(ref new MessageDialog(m_resLdr->GetString("MBCrashContent")))->ShowAsync();
	}

	try
	{
		delete AWait(ApplicationData::Current->LocalFolder->GetFileAsync("sdlpal.cfg"), g_eventHandle);
	}
	catch (Exception^)
	{
		(ref new MessageDialog(m_resLdr->GetString("MBStartupMessage"), m_resLdr->GetString("MBStartupTitle")))->ShowAsync();
	}
}

void SDLPal::MainPage::LoadControlContents(bool loadDefault)
{
	for (auto i = m_acl.begin(); i != m_acl.end(); i++)
	{
		auto item = i->second;
		item->text->Text = "";
		if (item->check)
		{
			item->check->IsChecked = false;
			m_controls->Lookup(item->check->Name)->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		}
	}

	// Clear MRU list
	StorageApplicationPermissions::MostRecentlyUsedList->Clear();

	if (!loadDefault)
	{
		// Always load folder/files from FutureAccessList
		std::list<Platform::String^> invalid_tokens;
		auto fal = StorageApplicationPermissions::FutureAccessList;
		for each (auto entry in fal->Entries)
		{
			try
			{
				auto item = AWait(fal->GetItemAsync(entry.Token), g_eventHandle);
				auto& ace = m_acl[PAL_ConfigIndex(ConvertString(entry.Token).c_str())];
				StorageApplicationPermissions::MostRecentlyUsedList->AddOrReplace(entry.Token, item, entry.Metadata);
				ace->text->Text = entry.Metadata;
				if (ace->check)
				{
					auto grid = m_controls->Lookup(ace->check->Name);
					ace->check->IsChecked = (item != nullptr);
					grid->Visibility = ace->check->IsChecked->Value ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
				}
				else
				{
					ace->text->Tag = item;
				}
			}
			catch (Exception ^)
			{
				invalid_tokens.push_back(entry.Token);
			}
		}
		for (auto i = invalid_tokens.begin(); i != invalid_tokens.end(); fal->Remove(*i++));
	}

	tsKeepAspect->IsOn = (gConfig.fKeepAspectRatio == TRUE);
	tsStereo->IsOn = (gConfig.iAudioChannels == 2);
	tsSurroundOPL->IsOn = (gConfig.fUseSurroundOPL == TRUE);
	tsTouchOverlay->IsOn = (gConfig.fUseTouchOverlay == TRUE);
	tsEnableAVI->IsOn = (gConfig.fEnableAviPlay == TRUE);

	slMusicVolume->Value = gConfig.iMusicVolume;
	slSoundVolume->Value = gConfig.iSoundVolume;
	slQuality->Value = gConfig.iResampleQuality;
	cbLogLevel->SelectedIndex = (int)gConfig.iLogLevel;

	cbCD->SelectedIndex = (gConfig.eCDType == MUSIC_MP3) ? 0 : 1;
	cbBGM->SelectedIndex = (gConfig.eMusicType <= MUSIC_OGG) ? gConfig.eMusicType : MUSIC_RIX;
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

	gConfig.iMusicVolume = (int)slMusicVolume->Value;
	gConfig.iSoundVolume = (int)slSoundVolume->Value;
	gConfig.iResampleQuality = (int)slQuality->Value;
	gConfig.iLogLevel = (LOGLEVEL)cbLogLevel->SelectedIndex;

	gConfig.eCDType = (MUSICTYPE)(MUSIC_MP3 + cbCD->SelectedIndex);
	gConfig.eMusicType = (MUSICTYPE)cbBGM->SelectedIndex;
	gConfig.eOPLType = (OPLTYPE)cbOPL->SelectedIndex;

	gConfig.iSampleRate = wcstoul(static_cast<Platform::String^>(static_cast<ComboBoxItem^>(cbSampleRate->SelectedItem)->Content)->Data(), nullptr, 10);
	gConfig.iOPLSampleRate = wcstoul(static_cast<Platform::String^>(static_cast<ComboBoxItem^>(cbOPLSR->SelectedItem)->Content)->Data(), nullptr, 10);
	gConfig.wAudioBufferSize = wcstoul(static_cast<Platform::String^>(static_cast<ComboBoxItem^>(cbAudioBuffer->SelectedItem)->Content)->Data(), nullptr, 10);
}

void SDLPal::MainPage::CheckResourceFolder()
{
	if (tbGamePath->Text->Length() > 0 && PAL_MISSING_REQUIRED(UTIL_CheckResourceFiles(ConvertString(tbGamePath->Text).c_str(), ConvertString(tbMsgFile->Text).c_str())))
	{
		auto msgbox = ref new MessageDialog(m_resLdr->GetString("MBDownloadRequiredText"), m_resLdr->GetString("MBDownloadRequiredTitle"));
		msgbox->Commands->Append(ref new UICommand(m_resLdr->GetString("MBButtonYes"), nullptr, 1));
		msgbox->Commands->Append(ref new UICommand(m_resLdr->GetString("MBButtonNo"), nullptr, nullptr));
		msgbox->DefaultCommandIndex = 0;
		msgbox->CancelCommandIndex = 1;
		concurrency::create_task(msgbox->ShowAsync()).then([this](IUICommand^ command)->IAsyncOperation<IUICommand^>^ {
			if (command->Id == nullptr)
			{
				return (ref new MessageDialog(m_resLdr->GetString("MBFolderManually")))->ShowAsync();
			}

			auto msgbox = ref new MessageDialog(m_resLdr->GetString("MBDownloadOption"), m_resLdr->GetString("MBDownloadTitle"));
			msgbox->Commands->Append(ref new UICommand(m_resLdr->GetString("MBButtonFromURL"), nullptr, 1));
			msgbox->Commands->Append(ref new UICommand(m_resLdr->GetString("MBButtonFromBaiyou"), nullptr, -1));
			msgbox->DefaultCommandIndex = 0;
			msgbox->CancelCommandIndex = 1;
			return msgbox->ShowAsync();
		}).then([this](IUICommand^ command)->IAsyncOperation<IUICommand^>^ {
			if (command && command->Id && (int)command->Id == -1)
			{
				auto msgbox = ref new MessageDialog(m_resLdr->GetString("MBDownloadMessage"), m_resLdr->GetString("MBDownloadTitle"));
				msgbox->Commands->Append(ref new UICommand(m_resLdr->GetString("MBButtonOK"), nullptr, -1));
				return msgbox->ShowAsync();
			}
			else
			{
				return concurrency::create_async([command]()->IUICommand^ { return command; });
			}
		}).then([this](IUICommand^ command) {
			if (!command || !command->Id)
			{
				ClearResourceFolder();
				return;
			}

			auto folder = dynamic_cast<StorageFolder^>(tbGamePath->Tag);
			try
			{
				auto file = AWait(folder->CreateFileAsync("pal98.zip", CreationCollisionOption::ReplaceExisting), g_eventHandle);
				auto stream = AWait(file->OpenAsync(FileAccessMode::ReadWrite), g_eventHandle);
				bool from_url = ((int)command->Id == 1);
				concurrency::create_task(this->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, folder, file, stream, from_url]() {
					m_dlg = ref new DownloadDialog(m_resLdr, folder, stream, ActualWidth, ActualHeight, from_url);
				}))).then([this]()->IAsyncOperation<ContentDialogResult>^ {
					return m_dlg->ShowAsync();
				}).then([this, file, stream](ContentDialogResult result) {
					delete stream;
					try { AWait(file->DeleteAsync(), g_eventHandle); }
					catch (Exception^) {}
					delete file;
					if (m_dlg->Result != ContentDialogResult::None)
					{
						ClearResourceFolder();
					}
				});
			}
			catch (Exception^ e)
			{
				(ref new MessageDialog(String::Concat(m_resLdr->GetString("MBDownloadError"), e)))->ShowAsync();
				ClearResourceFolder();
			}
		});
	}
}

void SDLPal::MainPage::ClearResourceFolder()
{
	tbGamePath->Text = "";
	tbGamePath->Tag = nullptr;
}

void SDLPal::MainPage::cbBGM_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
	auto visibility = (cbBGM->SelectedIndex == MUSIC_RIX) ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
	cbOPL->Visibility = visibility;
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
		if (PAL_MISSING_REQUIRED(UTIL_CheckResourceFiles(ConvertString(tbGamePath->Text).c_str(), ConvertString(tbMsgFile->Text).c_str())))
		{
			auto msg = std::wstring(m_resLdr->GetString("MBRequired")->Data());
			msg.replace(msg.find(L"{0}", 0), 3, tbGamePath->Text->Data());
			(ref new MessageDialog(ref new Platform::String(msg.c_str())))->ShowAsync();
			tbGamePath->Focus(Windows::UI::Xaml::FocusState::Programmatic);
			return;
		}

		auto fal = StorageApplicationPermissions::FutureAccessList;
		auto mru = StorageApplicationPermissions::MostRecentlyUsedList;
		fal->Clear();
		for (auto i = m_acl.begin(); i != m_acl.end(); i++)
		{
			auto entry = i->second;
			if (mru->ContainsItem(entry->token))
			{
				auto item = AWait(mru->GetItemAsync(entry->token), g_eventHandle);
				if ((!entry->check || entry->check->IsChecked->Value) && item)
				{
					fal->AddOrReplace(entry->token, item, entry->text->Text);
				}
			}
		}
		mru->Clear();

		SaveControlContents();
		gConfig.fLaunchSetting = FALSE;
		PAL_SaveConfig();

		concurrency::create_task((ref new MessageDialog(m_resLdr->GetString("MBExitContent"), m_resLdr->GetString("MBExitTitle")))->ShowAsync()).then([] (IUICommand^ command) {
			Application::Current->Exit();
		});
	}
	else
	{
		(ref new MessageDialog(m_resLdr->GetString("MBEmptyContent")))->ShowAsync();
	}
}

void SDLPal::MainPage::btnClearFile_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	tbMsgFile->Text = "";
}

void SDLPal::MainPage::SetPath(StorageFolder^ folder)
{
	if (folder)
	{
		tbGamePath->Text = folder->Path;
		tbGamePath->Tag = folder;
		StorageApplicationPermissions::MostRecentlyUsedList->AddOrReplace(m_acl[PALCFG_GAMEPATH]->token, folder, folder->Path);
		StorageApplicationPermissions::MostRecentlyUsedList->AddOrReplace(m_acl[PALCFG_SAVEPATH]->token, folder, folder->Path);
		CheckResourceFolder();
	}
}

void SDLPal::MainPage::SetFile(Windows::UI::Xaml::Controls::TextBox^ target, StorageFile^ file)
{
	if (target && file)
	{
		target->Text = file->Path;
		StorageApplicationPermissions::MostRecentlyUsedList->AddOrReplace(static_cast<String^>(target->Tag), file, file->Path);
	}
}

void SDLPal::MainPage::btnBrowseFolder_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto picker = ref new Pickers::FolderPicker();
	picker->FileTypeFilter->Append("*");
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
	picker->PickFolderAndContinue();
#else
	concurrency::create_task(picker->PickSingleFolderAsync()).then([this](StorageFolder^ folder) { SetPath(folder); });
#endif
}

void SDLPal::MainPage::btnBrowseFileOpen_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto button = static_cast<Windows::UI::Xaml::Controls::Button^>(sender);
	auto target = m_controls->Lookup(button->Name);
	auto picker = ref new Pickers::FileOpenPicker();
	picker->FileTypeFilter->Append("*");
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
	picker->ContinuationData->Insert("Target", button->Name);
	picker->PickSingleFileAndContinue();
#else
	concurrency::create_task(picker->PickSingleFileAsync()).then([this, target](StorageFile^ file) { SetFile(static_cast<TextBox^>(target), file); });
#endif
}

void SDLPal::MainPage::btnBrowseFileSave_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto button = static_cast<Windows::UI::Xaml::Controls::Button^>(sender);
	auto target = m_controls->Lookup(button->Name);
	auto picker = ref new Pickers::FileSavePicker();
	picker->FileTypeChoices->Insert(m_resLdr->GetString("LogFileType"), ref new Vector<String^>(1, ref new String(L".log")));
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
	picker->ContinuationData->Insert("Target", button->Name);
	picker->PickSaveFileAndContinue();
#else
	concurrency::create_task(picker->PickSaveFileAsync()).then([this, target](StorageFile^ file) { SetFile(static_cast<TextBox^>(target), file); });
#endif
}

void SDLPal::MainPage::cbUseFile_CheckChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto checker = static_cast<Windows::UI::Xaml::Controls::CheckBox^>(sender);
	m_controls->Lookup(checker->Name)->Visibility = checker->IsChecked->Value ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
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
	CheckResourceFolder();
}


void SDLPal::MainPage::OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
{
	if (m_dlg)
	{
		m_dlg->MaxWidth = e->NewSize.Width;
		if (m_dlg->MaxHeight == e->PreviousSize.Height)
			m_dlg->MaxHeight = e->NewSize.Height;
		m_dlg->UpdateLayout();
	}
}
