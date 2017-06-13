//
//  SPWebViewController.h
//  SDLPal
//
//  Created by palxex on 2017/6/13.
//  Copyright © 2017年 SDLPAL team. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "SettingsTableViewController.h"

@interface SPWebViewController : UIViewController

@property (weak, nonatomic) id<WebCaptureProtocol> delegate;

@property (assign, nonatomic) NSString *url;
@property (assign, nonatomic) NSString *signature;

@end
