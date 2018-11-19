//
//  Config.m
//  SDLPal
//
//  Created by Frank on 2018/11/19.
//  Copyright © 2018 SDLPAL team. All rights reserved.
//

#import "Config.h"

static id _defaultConfig = nil;

static NSString *const LandscapeSetting = @"Landscape_Setting";

@implementation Config

@synthesize isLandscape = _isLandscape;

- (void)setIsLandscape:(BOOL)isLandscape {
    _isLandscape = isLandscape;
    
    [NSUserDefaults.standardUserDefaults setBool:isLandscape forKey:LandscapeSetting];
    
    // should refactor if setting is batch
    [NSUserDefaults.standardUserDefaults synchronize];
}

- (BOOL)isLandscape {
    if (!_isLandscape) {
        id isLandscape = [NSUserDefaults.standardUserDefaults objectForKey:LandscapeSetting];
        // default YES
        if (!isLandscape) {
            _isLandscape = YES;
            [self setIsLandscape:YES];
        // use value stored in Setting
        } else {
            _isLandscape = [isLandscape boolValue];
        }
    }
    return _isLandscape;
}

// ========== Singleton ====================

+ (instancetype)defaultConfig {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _defaultConfig = [Config new];
    });
    return _defaultConfig;
}

@end
