//
//  Config.h
//  SDLPal
//
//  Created by Frank on 2018/11/19.
//  Copyright © 2018 SDLPAL team. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface Config : NSObject

/** Landscape switch setting */
@property (nonatomic) BOOL isLandscape;


/** @return Singleton for Config */
+ (instancetype)defaultConfig;

@end

NS_ASSUME_NONNULL_END
