/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../src/SDL_internal.h"

#if SDL_VIDEO_DRIVER_UIKIT

#include "../src/video/SDL_sysvideo.h"
#include "sdl_compat.h"

#import "SDLPal_AppDelegate.h"

#ifdef main
#undef main
#endif

#include "palcfg.h"

static int forward_argc;
static char **forward_argv;
static int exit_status;

int sdlpal_main(int argc, char **argv)
{
    int i;

    /* store arguments */
    forward_argc = argc;
    forward_argv = (char **)malloc((argc+1) * sizeof(char *));
    for (i = 0; i < argc; i++) {
        forward_argv[i] = malloc( (strlen(argv[i])+1) * sizeof(char));
        strcpy(forward_argv[i], argv[i]);
    }
    forward_argv[i] = NULL;

    /* Give over control to run loop, SDLPalAppDelegate will handle most things from here */
    @autoreleasepool {
        UIApplicationMain(argc, argv, nil, [SDLPalAppDelegate getAppDelegateClassName]);
    }

    /* free the memory we used to hold copies of argc and argv */
    for (i = 0; i < forward_argc; i++) {
        free(forward_argv[i]);
    }
    free(forward_argv);

    return exit_status;
}

@interface SDLPalAppDelegate ()
/** in settings or in game? */
@property (nonatomic) BOOL isInGame;

@end

@implementation SDLPalAppDelegate

/* convenience method */
+ (id)sharedAppDelegate
{
    /* the delegate is set in UIApplicationMain(), which is guaranteed to be
     * called before this method */
    return [UIApplication sharedApplication].delegate;
}

+ (NSString *)getAppDelegateClassName
{
    /* subclassing notice: when you subclass this appdelegate, make sure to add
     * a category to override this method and return the actual name of the
     * delegate */
    return @"SDLPalAppDelegate";
}

- (void)postFinishLaunch
{
    /* run the user's application, passing argc and argv */
    SDL_SetiOSEventPump(SDL_TRUE);
    exit_status = SDL_main(forward_argc, forward_argv);
    SDL_SetiOSEventPump(SDL_FALSE);

    /* exit, passing the return status from the user's application */
    /* We don't actually exit to support applications that do setup in their
     * main function and then allow the Cocoa event loop to run. */
    /* exit(exit_status); */
    [self restart];
}

#undef SDL_IPHONE_LAUNCHSCREEN

- (void)launchGame {
    self.isInGame = YES;
    
    SDL_SetMainReady();
    [self performSelector:@selector(postFinishLaunch) withObject:nil afterDelay:0.0];
    [self.window setBackgroundColor:[UIColor blackColor]];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    // iOS Files app will not show other app with no file inside documents. Create placeholder if needed.
    NSFileManager *fileMgr = [NSFileManager defaultManager];
    NSString *docPath = [NSString stringWithUTF8String:UTIL_BasePath()];
    NSString *placeholderPath =[NSString stringWithFormat:@"%@/placeholder", docPath];
    NSError *err = nil;
    NSArray *contents = [fileMgr contentsOfDirectoryAtPath:docPath error:&err];
    if( contents == nil || contents.count == 0 )
        [fileMgr createFileAtPath:placeholderPath contents:nil attributes:nil];
    else
        if( [fileMgr fileExistsAtPath:placeholderPath] )
            [fileMgr removeItemAtPath:placeholderPath error:&err];

    [self restart];
    return YES;
}
- (void)restart {
    PAL_LoadConfig(YES);
    const char *cachePath = UTIL_CachePath();
    if( getppid() != 1)
        NSLog(@"cache path:%s",cachePath);
    BOOL crashed = [[NSFileManager defaultManager] fileExistsAtPath:[NSString stringWithFormat:@"%s/running", cachePath]];
    if( gConfig.fLaunchSetting || crashed ) {
        self.isInGame = NO;
        
        self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
        UIStoryboard *sb = [UIStoryboard storyboardWithName:@"Settings" bundle:nil];
        UIViewController *vc = [sb instantiateInitialViewController];
        self.window.rootViewController = vc;
        [self.window makeKeyAndVisible];
    }else{
        [self launchGame];
    }
}

#if !TARGET_OS_TV
- (UIInterfaceOrientationMask)application:(UIApplication *)application supportedInterfaceOrientationsForWindow:(UIWindow *)window {
    // if in game.only support landscape
    return self.isInGame ? UIInterfaceOrientationMaskLandscape : UIInterfaceOrientationMaskAll;
}
#endif

@end

#endif /* SDL_VIDEO_DRIVER_UIKIT */

/* vi: set ts=4 sw=4 expandtab: */
