//
//  SettingsViewController.h
//  SDLPal
//
//  Created by palxex on 2017/5/18.
//  Copyright © 2017年 SDLPAL team. All rights reserved.
//

#ifndef SettingsViewController_h
#define SettingsViewController_h

#import <UIKit/UIKit.h>

@protocol WebCaptureProtocol

- (void)capturedURL:(NSURL *)url;

@end

@interface SettingsTableViewController : UITableViewController<WebCaptureProtocol> {
    
}
@end


#endif /* SettingsViewController_h */
