#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "util.h"
#include "palcfg.h"

#include <syslog.h>

#ifndef PAL_NO_LAUNCH_UI

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_Hor_Value_Slider.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Choice.H>

#ifdef PAL_HAS_NATIVEMIDI
    #define MUSIC_MIN MUSIC_MIDI
#else
    #define MUSIC_MIN MUSIC_RIX
#endif

struct {
   Fl_Input* gamepath;
   Fl_Radio_Round_Button* cht;
   Fl_Radio_Round_Button* chs;
   Fl_Input* msgfile;
   Fl_Check_Button* font;
   Fl_Check_Button* touch;
   Fl_Check_Button* aspect;
   Fl_Check_Button* fullscreen;
   Fl_Choice* cd;
   Fl_Choice* bgm;
   Fl_Choice* opl;
   Fl_Int_Input* samplerate;
   Fl_Check_Button* stereo;
   Fl_Int_Input* oplrate;
   Fl_Check_Button* surround;
   Fl_Int_Input* buffer;
   Fl_Hor_Value_Slider* quality;
   Fl_Hor_Value_Slider* music;
   Fl_Hor_Value_Slider* sound;
} gWidgets;

struct {
   const char* title;
   const char* language;
   const char* display;
   const char* audio;
   const char* gamepath;
   const char* cht;
   const char* chs;
   const char* msgfile;
   const char* font;
   const char* touch;
   const char* aspect;
   const char* fullscreen;
   const char* cd;
   const char* bgm;
   const char* opl;
   const char* samplerate;
   const char* stereo;
   const char* oplrate;
   const char* surround;
   const char* buffer;
   const char* quality;
   const char* musvol;
   const char* sndvol;
   const char* exit;
   const char* launch;
   const char* def;
} gLabels[3] = {
   { "SDLPAL Launcher", "Game language", "Display", "Audio", "Game resource path:",
     "&Traditional Chinese", "&Simplified Chinese", "Message file:", "Use &embedded font",
     "Use touc&h overlay", "&Keep aspect ratio", "&Full screen", "&CD type:", "&BGM type:",
     "&OPL type:", "Sample rate:", "Ste&reo", "OPL rate:", "Surround O&PL", "Buffer:",
     "Quality:", "Music volume:", "Sound volume:", "E&xit", "&Launch game", "&Default" },
   { "SDLPAL 启动器", "游戏语言设置", "显示设置", "音频设置", "游戏资源目录：",
     "繁体中文(&T)", "简体中文(&S)", "语言文件：", "使用游戏资源内嵌字体(&E)",
     "启用触屏辅助(&H)", "保持纵横比(&K)", "全屏模式(&F)", "&CD 音源：", "&BGM 音源：",
     "&OPL 类型：", "采样率：", "立体声(&R)", "OPL 采样率：", "环绕声 O&PL", "缓冲区：",
     "质量：", "音乐音量：", "音效音量：", "退出(&X)", "启动游戏(&L)", "默认设置(&D)" },
   { "SDLPAL 啟動器", "遊戲語言設置", "顯示設定", "音訊設定", "遊戲資源目錄：",
     "繁體中文(&T)", "簡體中文(&S)", "語言檔：", "使用遊戲資源內嵌字體(&E)",
     "啟用觸屏輔助(&H)", "保持縱橫比(&K)", "全屏模式(&F)", "&CD 音源：", "&BGM 音源：",
     "&OPL 類型：", "取樣速率：", "立體聲(&R)", "OPL 取樣速率：", "環繞聲 O&PL", "緩衝區：",
     "品質：", "音樂音量：", "音效音量：", "退出(&X)", "啟動遊戲(&L)", "默認設定(&D)" },
};

void InitControls()
{
   char buffer[64];
   gWidgets.gamepath->value(gConfig.pszGamePath);
   gWidgets.cht->value(gConfig.uCodePage == CP_BIG5 ? 1 : 0);
   gWidgets.chs->value(gConfig.uCodePage == CP_GBK ? 1 : 0);
   gWidgets.msgfile->value(gConfig.pszMsgFile);
   gWidgets.font->value(gConfig.fUseEmbeddedFonts ? 1 : 0);
   gWidgets.touch->value(gConfig.fUseTouchOverlay ? 1 : 0);
#if SDL_VERSION_ATLEAST(2,0,0)
   gWidgets.aspect->value(gConfig.fKeepAspectRatio ? 1 : 0);
#endif
   gWidgets.fullscreen->value(gConfig.fFullScreen ? 1 : 0);
   gWidgets.cd->value(gConfig.eCDType - MUSIC_MP3);
   gWidgets.bgm->value(gConfig.eMusicType - MUSIC_MIN);
   gWidgets.stereo->value(gConfig.iAudioChannels == 2 ? 1 : 0);
   sprintf(buffer, "%d", gConfig.iSampleRate); gWidgets.samplerate->value(buffer);
   gWidgets.opl->value(gConfig.eOPLType);
   sprintf(buffer, "%d", gConfig.iOPLSampleRate); gWidgets.oplrate->value(buffer);
   gWidgets.surround->value(gConfig.fUseSurroundOPL ? 1 : 0);
   sprintf(buffer, "%d", gConfig.wAudioBufferSize); gWidgets.buffer->value(buffer);
   gWidgets.quality->value(gConfig.iResampleQuality);
   gWidgets.music->value(gConfig.iMusicVolume);
   gWidgets.sound->value(gConfig.iSoundVolume);
}

void SaveControls()
{
   free(gConfig.pszGamePath);
   gConfig.pszGamePath = strlen(gWidgets.gamepath->value()) ? strdup(gWidgets.gamepath->value()) : nullptr;
   free(gConfig.pszMsgFile);
   gConfig.pszMsgFile = strlen(gWidgets.msgfile->value()) ? strdup(gWidgets.msgfile->value()) : nullptr;
   gConfig.uCodePage = (gWidgets.cht->value() ? CP_BIG5 : CP_GBK);
   gConfig.fUseEmbeddedFonts = gWidgets.font->value();
   gConfig.fUseTouchOverlay = gWidgets.touch->value();
#if SDL_VERSION_ATLEAST(2,0,0)
   gConfig.fKeepAspectRatio = gWidgets.aspect->value();
#endif
   gConfig.fFullScreen = gWidgets.fullscreen->value();
   gConfig.eCDType = (MUSICTYPE)(gWidgets.cd->value() + MUSIC_MP3);
   gConfig.eMusicType = (MUSICTYPE)(gWidgets.bgm->value() + MUSIC_MIN);
   gConfig.iAudioChannels = gWidgets.stereo->value() ? 2 : 1;
   gConfig.iSampleRate = atoi(gWidgets.samplerate->value());
   gConfig.eOPLType = (OPLTYPE)gWidgets.opl->value();
   gConfig.iOPLSampleRate = atoi(gWidgets.oplrate->value());
   gConfig.fUseSurroundOPL = gWidgets.surround->value();
   gConfig.wAudioBufferSize = atoi(gWidgets.buffer->value());
   gConfig.iResampleQuality = (int)gWidgets.quality->value();
   gConfig.iMusicVolume = (int)gWidgets.music->value();
   gConfig.iSoundVolume = (int)gWidgets.sound->value();
   gConfig.fLaunchSetting = FALSE;
}

int GetLanguage()
{
   auto lang = getenv("LANG");
   if (!lang) return 0;
   if (strncasecmp(lang, "zh_", 3) == 0)
   {
      if (strncasecmp(lang + 3, "hans", 4) == 0 || strncasecmp(lang + 3, "CN", 2) == 0 || strncasecmp(lang + 3, "SG", 2) == 0)
         return 1;
      else
         return 2;
   }
   else
      return 0;
}

Fl_Window* InitWindow()
{
   int lang = GetLanguage();
   Fl_Window* window = new Fl_Window(640, 400, gLabels[lang].title);

   (gWidgets.gamepath = new Fl_Input(160, 9, 475, 22, gLabels[lang].gamepath))->value(gConfig.pszGamePath);

   (new Fl_Box(FL_BORDER_BOX, 5, 70, 310, 100, gLabels[lang].language))->align(FL_ALIGN_TOP);
   gWidgets.cht = new Fl_Radio_Round_Button(10, 80, 160, 20, gLabels[lang].cht);
   gWidgets.chs = new Fl_Radio_Round_Button(10, 110, 160, 20, gLabels[lang].chs);
   (gWidgets.msgfile = new Fl_Input(109, 139, 196, 22, gLabels[lang].msgfile))->value(gConfig.pszMsgFile);

   (new Fl_Box(FL_BORDER_BOX, 325, 70, 310, 100, gLabels[lang].display))->align(FL_ALIGN_TOP);
   gWidgets.font = new Fl_Check_Button(330, 80, 160, 20, gLabels[lang].font);
   gWidgets.touch = new Fl_Check_Button(330, 110, 160, 20, gLabels[lang].touch);
   gWidgets.aspect = new Fl_Check_Button(330, 140, 160, 20, gLabels[lang].aspect);
   gWidgets.fullscreen = new Fl_Check_Button(520, 140, 120, 20, gLabels[lang].fullscreen);

   (new Fl_Box(FL_BORDER_BOX, 5, 210, 630, 130, gLabels[lang].audio))->align(FL_ALIGN_TOP);
   (gWidgets.cd = new Fl_Choice(84, 219, lang ? 100 : 120, 22, gLabels[lang].cd))->add("MP3|OGG");
   (gWidgets.bgm = new Fl_Choice(285, 219, 60, 22, gLabels[lang].bgm))->add( va("%s%s",(PAL_HAS_NATIVEMIDI ? "MIDI|" : ""), "RIX|MP3|OGG") );
   gWidgets.stereo = new Fl_Check_Button(365, 220, 60, 20, gLabels[lang].stereo);
   gWidgets.samplerate = new Fl_Int_Input(570, 219, 60, 22, gLabels[lang].samplerate);
   (gWidgets.opl = new Fl_Choice(84, 249, lang ? 100 : 120, 22, gLabels[lang].opl))->add("DOSBOX|MAME|DOSBOXNEW");
   gWidgets.oplrate = new Fl_Int_Input(285, 249, 60, 22, gLabels[lang].oplrate);
   gWidgets.surround = new Fl_Check_Button(365, 250, 60, 20, gLabels[lang].surround);
   gWidgets.buffer = new Fl_Int_Input(570, 249, 60, 22, gLabels[lang].buffer);

   gWidgets.quality = new Fl_Hor_Value_Slider(72, 279, 180, 22, gLabels[lang].quality);
   gWidgets.quality->align(FL_ALIGN_LEFT);
   gWidgets.quality->bounds(0, 4);
   gWidgets.quality->precision(0);

   gWidgets.music = new Fl_Hor_Value_Slider(380, 279, 250, 22, gLabels[lang].musvol);
   gWidgets.music->align(FL_ALIGN_LEFT);
   gWidgets.music->bounds(0, 100);
   gWidgets.music->precision(0);

   gWidgets.sound = new Fl_Hor_Value_Slider(380, 309, 250, 22, gLabels[lang].sndvol);
   gWidgets.sound->align(FL_ALIGN_LEFT);
   gWidgets.sound->bounds(0, 100);
   gWidgets.sound->precision(0);

   (new Fl_Button(5, 370, 120, 24, gLabels[lang].exit))->callback([](Fl_Widget* ctrl, void* window) {
      if (ctrl->when() == FL_WHEN_RELEASE)
         static_cast<Fl_Window*>(window)->hide();
   }, window);

   (new Fl_Button(260, 370, 120, 24, gLabels[lang].launch))->callback([](Fl_Widget* ctrl, void* window) {
      if (ctrl->when() == FL_WHEN_RELEASE) {
         SaveControls();
         PAL_SaveConfig();
         static_cast<Fl_Window*>(window)->hide();
      }
   }, window);

   (new Fl_Button(515, 370, 120, 24, gLabels[lang].def))->callback([](Fl_Widget*) { PAL_LoadConfig(FALSE); InitControls(); });

   window->end();

   InitControls();

   return window;
}

#endif

BOOL
UTIL_GetScreenSize(
   DWORD *pdwScreenWidth,
   DWORD *pdwScreenHeight
)
{
   return pdwScreenWidth && pdwScreenHeight && *pdwScreenWidth && *pdwScreenHeight;
}

BOOL
UTIL_IsAbsolutePath(
   LPCSTR lpszFileName
)
{
   return lpszFileName && *lpszFileName == '/';
}

INT
UTIL_Platform_Init(
   int argc,
   char* argv[]
)
{
	openlog("sdlpal", LOG_PERROR | LOG_PID, LOG_USER);
	UTIL_LogSetOutput([](LOGLEVEL level, const char* str, const char*)->void {
		const static int priorities[] = {
			LOG_DEBUG,
			LOG_DEBUG,
			LOG_INFO,
			LOG_WARNING,
			LOG_ERR,
			LOG_EMERG
		};
		syslog(priorities[level], "%s", str);
	}, 1024, TRUE);

#if !defined(UNIT_TEST) && !defined(PAL_NO_LAUNCH_UI)
   if (gConfig.fLaunchSetting)
   {
      Fl_Window *window = InitWindow();
      window->show(argc, argv);
      Fl::run();
      Fl::flush();
      delete window;
   }
#else
   gConfig.fLaunchSetting = FALSE;
#endif
   return 0;
}

VOID
UTIL_Platform_Quit(
   VOID
)
{
#if defined(_DEBUG)
	closelog();
#endif
}
