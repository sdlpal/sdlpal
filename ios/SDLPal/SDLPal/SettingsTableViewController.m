//
//  Settings.m
//  SDLPal
//
//  Created by palxex on 2017/5/18.
//  Copyright © 2017年 SDLPAL team. All rights reserved.
//

#import "SettingsTableViewController.h"
#import "SPWebViewController.h"

#import "SDLPal_AppDelegate.h"
#import "ActionSheetStringPicker.h"
#import "AFNetworking.h"
#import "SSZipArchive.h"

#include "palcfg.h"

#define UIKitLocalizedString(key) [[NSBundle bundleWithIdentifier:@"com.apple.UIKit"] localizedStringForKey:key value:@"" table:nil]

@implementation SettingsTableViewController {
    NSArray *AudioSampleRates;
    NSArray *AudioBufferSizes;
    NSArray *OPLSampleRates;
    NSArray *CDFormats;
    NSArray *MusicFormats;
    NSArray *OPLFormats;
    NSArray *LogLevels;
    NSArray *allFiles;
    NSMutableArray *AvailFiles;
    BOOL checkAllFilesIncluded;
    NSString *resourceStatus;
    
    NSArray *officialLinks;
    NSInteger linkSelected;
    
    UIAlertController *customURLAlert;
    
    IBOutlet UIView *transitionView;
    
    IBOutlet UILabel *lblResourceStatus;
    
    IBOutlet UILabel *lblLanguageFile;
    IBOutlet UILabel *lblFontFile;
    
    IBOutlet UISwitch *toggleTouchScreenOverlay;
    IBOutlet UISwitch *toggleKeepAspect;
    IBOutlet UISwitch *toggleSmoothScaling;
    
    IBOutlet UILabel *lblMusicType;
    IBOutlet UILabel *lblOPLType;
    IBOutlet UILabel *lblOPLRate;
    IBOutlet UILabel *lblCDAudioSource;
    IBOutlet UISwitch *toggleStereo;
    IBOutlet UISwitch *toggleSurroundOPL;
    IBOutlet UILabel *lblResampleRate;
    IBOutlet UILabel *lblAudioBufferSize;
    IBOutlet UISlider *sliderResampleQuality;
    IBOutlet UISlider *sliderMusicVolume;
    IBOutlet UISlider *sliderSFXVolume;
    
    IBOutlet UILabel *lblLogLevel;
    IBOutlet UITextField *textLogFile;
}

- (BOOL)includedInList:(NSArray*)array name:(NSString *)filename {
    for( NSString *item in array ) {
        if( [filename caseInsensitiveCompare:item] == NSOrderedSame )
            return YES;
    }
    return NO;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    [transitionView setFrame:self.view.frame];
    
    UILabel* tlabel=[[UILabel alloc] initWithFrame:CGRectMake(0,0, 300, 40)];
    tlabel.text=[NSString stringWithUTF8String:PAL_GIT_REVISION];
    tlabel.backgroundColor =[UIColor clearColor];
    tlabel.adjustsFontSizeToFitWidth=YES;
    self.navigationItem.titleView=tlabel;
    
    AudioSampleRates = @[ @"11025", @"22050", @"44100", @"49716" ];
    AudioBufferSizes = @[ @"512", @"1024", @"2048", @"4096", @"8192" ];
    OPLSampleRates = @[ @"12429", @"24858", @"49716", @"11025", @"22050", @"44100" ];
    CDFormats = @[ @"MP3", @"OGG" ];
    MusicFormats = @[ @"MIDI", @"RIX", @"MP3", @"OGG" ];
    OPLFormats = @[ @"DOSBOX", @"MAME", @"DOSBOXNEW" ];
    LogLevels = @[ @"VERBOSE", @"DEBUG", @"INFO", @"WARNING", @"ERROR", @"FATAL" ];
    
    officialLinks = @[ @[@"BAIYOU",@"http://pal5q.baiyou100.com/pal5/download/98xjrq.html",         @"Pal98rqp.zip"],
//                       @[@"ROO GAMES",@"http://gamelib.roogames.com/WDetail/WDJDetail?gameid=10256",   @"NEED client to parse & handle, ignore"],
//                       @[@"CUBE GAME",@"http://ku.cubejoy.com/GameDetail/Detail/1000038",              @"NEED client to parse & handle, ignore"],
                       ];
    
    UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(dismissKeyboard)];
    tap.cancelsTouchesInView = NO;
    [self.view addGestureRecognizer:tap];
    
    UIRefreshControl *refreshController = [[UIRefreshControl alloc] init];
    [refreshController addTarget:self action:@selector(handleRefresh:) forControlEvents:UIControlEventValueChanged];
    [self.tableView addSubview:refreshController];

    [self recheckSharingFolder];
}
-(void)dismissKeyboard
{
    [self.view endEditing:YES];
}
-(void)handleRefresh : (id)sender
{
    UIRefreshControl *refreshController = sender;
    [self recheckSharingFolder];
    [refreshController endRefreshing];
}

- (void)recheckSharingFolder {
    AvailFiles = [NSMutableArray new];
    NSArray *builtinList = @[ @"wor16.fon", @"wor16.asc", @"m.msg"];
    NSArray *builtinExtensionList = @[@"exe",@"drv",@"dll",@"rpg",@"mkf",@"avi",@"dat",@"cfg",@"ini"];
    allFiles = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:[NSString stringWithUTF8String:UTIL_BasePath()] error:nil];
    for( NSString *filename in allFiles ) {
        if( ![self includedInList:builtinExtensionList name:filename.pathExtension] &&
           ![self includedInList:builtinList name:filename] ) {
            [AvailFiles addObject:filename];
        }
    }
    checkAllFilesIncluded = YES;
    for( NSString *checkFile in @[@"abc.mkf", @"ball.mkf", @"data.mkf", @"f.mkf", @"fbp.mkf", @"fire.mkf", @"gop.mkf", @"m.msg", @"map.mkf", @"mgo.mkf", @"rgm.mkf", @"rng.mkf", @"sss.mkf", @"word.dat"] ) {
        if( ![self includedInList:allFiles name:checkFile] ) {
            checkAllFilesIncluded = NO;
            break;
        }
    }
    if(!resourceStatus) resourceStatus = lblResourceStatus.text;
    lblResourceStatus.text  = [NSString stringWithFormat:@"%@%@", resourceStatus, checkAllFilesIncluded ? @"✅" : @"❌" ];

    [self readConfigs];
}

typedef void(^SelectedBlock)(NSString *selected);

- (void)showPickerWithTitle:(NSString *)title toLabel:(UILabel*)label inArray:(NSArray*)array {
    [self showPickerWithTitle:title toLabel:label inArray:array origin:self.navigationController.navigationBar allowEmpty:NO];
}
- (void)showPickerWithTitle:(NSString *)title toLabel:(UILabel*)label inArray:(NSArray*)array origin:(UIView*)origin {
    [self showPickerWithTitle:title toLabel:label inArray:array origin:origin allowEmpty:NO];
}
- (void)showPickerWithTitle:(NSString *)title toLabel:(UILabel*)label inArray:(NSArray*)array origin:(UIView*)origin allowEmpty:(BOOL)allowEmpty {
    [self showPickerWithTitle:title toLabel:label inArray:array origin:origin allowEmpty:allowEmpty doneBlock:nil];
}
- (void)showPickerWithTitle:(NSString *)title toLabel:(UILabel*)label inArray:(NSArray*)array origin:(UIView*)origin allowEmpty:(BOOL)allowEmpty doneBlock:(SelectedBlock)doneBlock {
    array = allowEmpty ? [array arrayByAddingObject:@""] : array;
    [ActionSheetStringPicker showPickerWithTitle:nil
                                            rows:array
                                initialSelection:[array containsObject:label.text] ? [array indexOfObject:label.text] : 0
                                       doneBlock:^(ActionSheetStringPicker *picker, NSInteger selectedIndex, id selectedValue) {
                                           label.text = array[selectedIndex];
                                           if(doneBlock) doneBlock(label.text);
                                       }
                                     cancelBlock:nil
                                          origin:origin];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    //need manually sync with storyboard...
    int rows = 1;
    switch(section) {
        case 0:
            rows = 1;
            break;
        case 1:
            rows = 2;
            break;
        case 2:
            rows = 3;
            break;
        case 3:
            rows = [lblMusicType.text isEqualToString:@"RIX"] ? 11 : 4;
            break;
        case 4:
            rows = 2;
            break;
        default:
            break;
    }
    return rows;
}

- (void)capturedURL:(NSURL *)url{
    UIAlertController  *alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Copyright Claim",nil)
                                                                    message:[NSString stringWithFormat:NSLocalizedString(@"This App is about to download the free & publicly available game resource from %@ to the iTunes File Sharing folder.\nPlease notice that all the copyright of the downloaded game resource belongs to its creator, Softstar, Inc. This App provides the download function here only for convenient purpose, and Softstar, Inc. may remove the download links at any time. Furthermore, you should take full responsibility for using the downloaded game resource which is completely irrelevant to the developers of this App.\nYou have to agree the above declaration before you click 'ok' to start the downloading process. Otherwise, please click 'cancel' to return.",nil),url.host]
                                                             preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction *ensureAction = [UIAlertAction actionWithTitle:UIKitLocalizedString(@"OK") style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        [self downloadURL:url];
    }];
    [alert addAction:ensureAction];
    UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:UIKitLocalizedString(@"Cancel") style:UIAlertActionStyleCancel handler:nil];
    [alert addAction:cancelAction];
    [self presentViewController:alert animated:YES completion:nil];
}

- (void)downloadURL:(NSURL *)url{
    UIAlertController  *alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Downloading",nil)
                                                                    message:@"\n"
                                                             preferredStyle:UIAlertControllerStyleAlert];
    __block NSURLSessionDownloadTask *downloadTask;
    UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:UIKitLocalizedString(@"Cancel") style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        if(downloadTask) {
            [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible:NO];
            [downloadTask cancel];
        }
    }];
    [alert addAction:cancelAction];
    UIProgressView *progressView = [[UIProgressView alloc] initWithFrame:CGRectMake(10, 60, 250, 2.0)];
    [progressView setProgress:0];
    [alert.view addSubview:progressView];
    [self presentViewController:alert animated:YES completion:nil];
    AFHTTPSessionManager *manager = [AFHTTPSessionManager manager];
    NSURLRequest *request = [NSURLRequest requestWithURL:url];
    downloadTask = [manager downloadTaskWithRequest:request
                                           progress:^(NSProgress * _Nonnull downloadProgress) {
                                               progressView.progress = downloadProgress.fractionCompleted;
                                           }
                                        destination:^NSURL *(NSURL *targetPath, NSURLResponse *response) {
                                            return [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:[response suggestedFilename]]];
                                        }
                                  completionHandler:^(NSURLResponse *response, NSURL *filePath, NSError *error) {
                                      downloadTask = nil;
                                      [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible:NO];
                                      if( error ) {
                                          [alert dismissViewControllerAnimated:YES completion:nil];
                                          UIAlertController  *alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Error",nil)
                                                                                                          message:[error localizedDescription]
                                                                                                   preferredStyle:UIAlertControllerStyleAlert];
                                          UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:UIKitLocalizedString(@"OK") style:UIAlertActionStyleDefault handler:nil];
                                          [alert addAction:cancelAction];
                                          [self presentViewController:alert animated:YES completion:nil];
                                      }else{
                                          alert.title = @"Extracting";
                                          progressView.progress = 0.0f;
                                          [SSZipArchive unzipFileAtPath:filePath.path
                                                          toDestination:[NSString stringWithUTF8String:UTIL_BasePath()]
                                                              overwrite:YES
                                                               password:nil
                                                        progressHandler:^(NSString * _Nonnull entry, unz_file_info zipInfo, long entryNumber, long total) {
                                                            progressView.progress = (float)entryNumber/total;
                                                        }
                                                      completionHandler:^(NSString * _Nonnull path, BOOL succeeded, NSError * _Nonnull error) {
                                                          [alert dismissViewControllerAnimated:YES completion:nil];
                                                          if( error ) {
                                                              UIAlertController  *alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Error",nil)
                                                                                                                              message:[error localizedDescription]
                                                                                                                       preferredStyle:UIAlertControllerStyleAlert];
                                                              UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:UIKitLocalizedString(@"OK") style:UIAlertActionStyleDefault handler:nil];
                                                              [alert addAction:cancelAction];
                                                              [self presentViewController:alert animated:YES completion:nil];
                                                          }else{
                                                              [self recheckSharingFolder];
                                                          }
                                                      }];
                                      }
                                  }];
    [downloadTask resume];
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible:YES];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [self.tableView cellForRowAtIndexPath:indexPath];
    if( indexPath.section == 1 && indexPath.row == 0 ) { //language file
        [self showPickerWithTitle:nil toLabel:lblLanguageFile inArray:AvailFiles origin:cell allowEmpty:YES];
    }else if( indexPath.section == 1 && indexPath.row == 1 ) { //font file
        [self showPickerWithTitle:nil toLabel:lblFontFile inArray:AvailFiles origin:cell allowEmpty:YES];
    }else if( indexPath.section == 2 && indexPath.row == 0 ) { //touch overlay
        toggleTouchScreenOverlay.on = !toggleTouchScreenOverlay.isOn;
    }else if( indexPath.section == 2 && indexPath.row == 1 ) { //keep aspect
        toggleKeepAspect.on = !toggleKeepAspect.isOn;
    }else if( indexPath.section == 2 && indexPath.row == 2 ) { //smooth scaling
        toggleSmoothScaling.on = !toggleSmoothScaling.isOn;
    }else if( indexPath.section == 3 && indexPath.row == 0 ) { //BGM
        [self showPickerWithTitle:nil toLabel:lblMusicType inArray:MusicFormats origin:cell allowEmpty:NO doneBlock:^(NSString *selected) {
            [self.tableView reloadData];
        }];
    }else if( indexPath.section == 3 && indexPath.row == 4 ) { //OPL Type
        [self showPickerWithTitle:nil toLabel:lblOPLType inArray:OPLFormats origin:cell];
    }else if( indexPath.section == 3 && indexPath.row == 5 ) { //OPL Rate
        [self showPickerWithTitle:nil toLabel:lblOPLRate inArray:OPLSampleRates origin:cell];
    }else if( indexPath.section == 3 && indexPath.row == 1 ) { //CD Source
        [self showPickerWithTitle:nil toLabel:lblCDAudioSource inArray:CDFormats origin:cell];
    }else if( indexPath.section == 3 && indexPath.row == 8 ) { //Stereo
        toggleStereo.on = !toggleStereo.isOn;
    }else if( indexPath.section == 3 && indexPath.row == 9 ) { //Surround
        toggleSurroundOPL.on = !toggleSurroundOPL.isOn;
    }else if( indexPath.section == 3 && indexPath.row == 6 ) { //SampleRate
        [self showPickerWithTitle:nil toLabel:lblResampleRate inArray:AudioSampleRates origin:cell];
    }else if( indexPath.section == 3 && indexPath.row == 7 ) { //Buffer size
        [self showPickerWithTitle:nil toLabel:lblAudioBufferSize inArray:AudioBufferSizes origin:cell];
    }else if( indexPath.section == 4 && indexPath.row == 0 ) { //Log Level
        [self showPickerWithTitle:nil toLabel:lblLogLevel inArray:LogLevels origin:cell];
    }
}

- (IBAction)btnDefaultClicked:(id)sender {
    PAL_LoadConfig(NO);
    [self readConfigs];
}

- (void)customURLDidChanged:(id)textField {
    NSURL *url = [NSURL URLWithString:customURLAlert.textFields[0].text];
    customURLAlert.actions[0].enabled = [NSURLConnection canHandleRequest:[NSURLRequest requestWithURL:url]] && url.host;
}

- (IBAction)btnConfirmClicked:(id)sender {
    if(!checkAllFilesIncluded){
        __weak id weakSelf = self;
        UIAlertController  *alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Cannot find data file in the iTunes File Sharing directory",nil)
                                                                        message:NSLocalizedString(@"NOTE: For copyright reasons data files required to run the game are NOT included. You can obtain them by visiting official links, or copy them via iTunes File Sharing. Which do you prefer?",nil)
                                                                 preferredStyle:UIAlertControllerStyleAlert];
        for( NSArray *linkInfo in officialLinks ) {
            UIAlertAction *selectAction = [UIAlertAction actionWithTitle:NSLocalizedString(linkInfo[0],nil) style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
                linkSelected = [officialLinks indexOfObject:linkInfo];
                [self performSegueWithIdentifier:@"showWebView" sender:self];
            }];
            [alert addAction:selectAction];
        }
        UIAlertAction *customAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Custom URL",nil) style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
            customURLAlert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Custom URL",nil)
                                                                            message:@""
                                                                     preferredStyle:UIAlertControllerStyleAlert];
            [customURLAlert addTextFieldWithConfigurationHandler:^(UITextField * _Nonnull textField) {
                textField.placeholder = NSLocalizedString(@"Custom URL",nil);
                [textField addTarget:weakSelf action:@selector(customURLDidChanged:) forControlEvents:UIControlEventEditingChanged];
            }];
            UIAlertAction *ensureAction = [UIAlertAction actionWithTitle:UIKitLocalizedString(@"OK") style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
                NSURL *url = [NSURL URLWithString:customURLAlert.textFields[0].text];
                [weakSelf downloadURL:url];
            }];
            ensureAction.enabled = NO;
            [customURLAlert addAction:ensureAction];
            UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:UIKitLocalizedString(@"Cancel") style:UIAlertActionStyleCancel handler:nil];
            [customURLAlert addAction:cancelAction];
            [self presentViewController:customURLAlert animated:YES completion:nil];
        }];
        [alert addAction:customAction];
        UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:UIKitLocalizedString(@"Cancel") style:UIAlertActionStyleCancel handler:nil];
        [alert addAction:cancelAction];
        [self presentViewController:alert animated:YES completion:nil];
        return;
    }
    [UIView animateWithDuration:0.65
                          delay:0.0
         usingSpringWithDamping:1.0
          initialSpringVelocity:0.0
                        options:UIViewAnimationOptionCurveEaseInOut
                     animations:^{
                         CGPoint point = self.navigationController.view.center;
                         point.y += self.navigationController.view.frame.size.height;
                         [self.navigationController.view setCenter:point];
                     } completion:^(BOOL finished) {
                         [self saveConfigs];
                         gConfig.fLaunchSetting = NO;
                         PAL_SaveConfig();
                         [[SDLPalAppDelegate sharedAppDelegate] launchGame];
                     }];
}

- (void)readConfigs {
    gConfig.fFullScreen = YES; //iOS specific; need this to make sure statusbar hidden in game completely
    
    lblLanguageFile.text    = [NSString stringWithUTF8String:gConfig.pszMsgFile  ? gConfig.pszMsgFile  : ""];
    lblFontFile.text        = [NSString stringWithUTF8String:gConfig.pszFontFile ? gConfig.pszFontFile : ""];
    textLogFile.text        = [NSString stringWithUTF8String:gConfig.pszLogFile  ? gConfig.pszLogFile  : ""];
    
    toggleStereo.on         = gConfig.iAudioChannels == 2;
    toggleSurroundOPL.on    = gConfig.fUseSurroundOPL;
    
    toggleTouchScreenOverlay.on = gConfig.fUseTouchOverlay;
    toggleKeepAspect.on         = gConfig.fKeepAspectRatio;
    toggleSmoothScaling.on      = gConfig.pszScaleQuality ? strncmp(gConfig.pszScaleQuality, "0", sizeof(char)) != 0 : NO;
    
    lblMusicType.text       = MusicFormats[gConfig.eMusicType];
    lblOPLType.text         = OPLFormats[gConfig.eOPLType];
    lblOPLRate.text         = [NSString stringWithFormat:@"%d",gConfig.iOPLSampleRate];
    lblCDAudioSource.text   = CDFormats[gConfig.eCDType-MUSIC_OGG];
    lblResampleRate.text    = [NSString stringWithFormat:@"%d",gConfig.iSampleRate];
    lblAudioBufferSize.text = [NSString stringWithFormat:@"%d",gConfig.wAudioBufferSize];
    
    sliderMusicVolume.value     = gConfig.iMusicVolume;
    sliderSFXVolume.value       = gConfig.iSoundVolume;
    sliderResampleQuality.value = gConfig.iResampleQuality;
    
    
    lblLogLevel.text        = LogLevels[gConfig.iLogLevel];
    
    [self.tableView reloadData];
}

- (void)saveConfigs {
    gConfig.pszMsgFile  = [lblLanguageFile.text length] == 0 ? NULL : strdup([[lblLanguageFile  text] UTF8String]);
    gConfig.pszFontFile = [lblLanguageFile.text length] == 0 ? NULL : strdup([[lblFontFile      text] UTF8String]);
    gConfig.pszLogFile  = [lblLanguageFile.text length] == 0 ? NULL : strdup([[textLogFile      text] UTF8String]);
    
    gConfig.iAudioChannels  = toggleStereo.isOn ? 2 : 1;
    gConfig.fUseSurroundOPL = toggleSurroundOPL.isOn;
    
    gConfig.fKeepAspectRatio = toggleKeepAspect.isOn;
    gConfig.fUseTouchOverlay = toggleTouchScreenOverlay.isOn;
    gConfig.pszScaleQuality  = strdup(toggleSmoothScaling.on ? "1" : "0");
   
    gConfig.eMusicType  = (MUSICTYPE)[MusicFormats indexOfObject:lblMusicType.text];
    gConfig.eOPLType    = (OPLTYPE  )[OPLFormats   indexOfObject:lblOPLType.text];
    gConfig.iOPLSampleRate = [lblOPLRate.text intValue];
    gConfig.eCDType     = (MUSICTYPE)[CDFormats indexOfObject:lblCDAudioSource.text]+MUSIC_OGG;
    gConfig.iSampleRate = [lblResampleRate.text intValue];
    gConfig.wAudioBufferSize = [lblAudioBufferSize.text intValue];
    
    gConfig.iMusicVolume = sliderMusicVolume.value;
    gConfig.iSoundVolume = sliderSFXVolume.value;
    gConfig.iResampleQuality = sliderResampleQuality.value;
    
    gConfig.iLogLevel = (LOGLEVEL)[LogLevels indexOfObject:lblLogLevel.text];
}
 #pragma mark - Navigation
 
// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    if( [segue.identifier isEqualToString:@"showWebView"] ) {
        SPWebViewController *webView = segue.destinationViewController;
        webView.delegate = self;
        webView.url = officialLinks[linkSelected][1];
        webView.signature = officialLinks[linkSelected][2];
    }
}

@end
