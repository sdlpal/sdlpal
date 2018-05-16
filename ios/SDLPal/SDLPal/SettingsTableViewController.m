//
//  Settings.m
//  SDLPal
//
//  Created by palxex on 2017/5/18.
//  Copyright © 2017年 SDLPAL team. All rights reserved.
//

#import "SettingsTableViewController.h"
#import "SDLPal_AppDelegate.h"
#import <ActionSheetPicker-3.0/ActionSheetStringPicker.h>

#include "palcfg.h"

#define UIKitLocalizedString(key) [[NSBundle bundleWithIdentifier:@"com.apple.UIKit"] localizedStringForKey:key value:@"" table:nil]

@implementation SettingsTableViewController {
    NSArray *AudioSampleRates;
    NSArray *AudioBufferSizes;
    NSArray *OPLSampleRates;
    NSArray *CDFormats;
    NSArray *MusicFormats;
    NSArray *OPLCores;
    NSArray *OPLChips;
    NSArray *LogLevels;
    
    NSArray *allFiles;
    NSMutableArray *AvailFiles;
    BOOL checkAllFilesIncluded;
    NSString *resourceStatus;
    
    AbstractActionSheetPicker *picker;
    
    IBOutlet UIView *transitionView;
    
    IBOutlet UILabel *lblResourceStatus;
    
    IBOutlet UILabel *lblLanguageFile;
    IBOutlet UILabel *lblFontFile;
    
    IBOutlet UISwitch *toggleTouchScreenOverlay;
    IBOutlet UISwitch *toggleKeepAspect;
    IBOutlet UISwitch *toggleSmoothScaling;
    
    IBOutlet UISwitch *toggleGLSL;
    IBOutlet UISwitch *toggleHDR;
    IBOutlet UILabel *lblShader;
    IBOutlet UITextField *textTextureWidth;
    IBOutlet UITextField *textTextureHeight;
    
    IBOutlet UILabel *lblMusicType;
    IBOutlet UILabel *lblOPLCore;
    IBOutlet UILabel *lblOPLChip;
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
    OPLCores = @[ @"MAME", @"DBFLT", @"DBINT", @"NUKED" ];
    OPLChips = @[ @"OPL2", @"OPL3" ];
    LogLevels = @[ @"VERBOSE", @"DEBUG", @"INFO", @"WARNING", @"ERROR", @"FATAL" ];
    
    UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(dismissKeyboard)];
    tap.cancelsTouchesInView = NO;
    [self.view addGestureRecognizer:tap];
    
    UIRefreshControl *refreshController = [[UIRefreshControl alloc] init];
    [refreshController addTarget:self action:@selector(handleRefresh:) forControlEvents:UIControlEventValueChanged];
    [self.tableView addSubview:refreshController];

    [self recheckSharingFolder];
    [self readConfigs];
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
    picker = [ActionSheetStringPicker showPickerWithTitle:nil
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
            rows = 4;
            break;
        case 3:
            rows = [toggleGLSL isOn] ? 3 : 1;
            break;
        case 4:
            rows = [lblMusicType.text isEqualToString:@"RIX"] ? 11 : 4;
            break;
        case 5:
            rows = 2;
            break;
        default:
            break;
    }
    return rows;
}
- (IBAction)GLSLToggled:(id)sender {
    [self.tableView reloadData];
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
    }else if( indexPath.section == 3 && indexPath.row == 0 ) { //Enable GLSL
        toggleGLSL.on = !toggleGLSL.isOn;
        [self.tableView reloadData];
    }else if( indexPath.section == 3 && indexPath.row == 2 ) { //Shader
        [self showPickerWithTitle:nil toLabel:lblShader inArray:AvailFiles origin:cell allowEmpty:YES];
    }else if( indexPath.section == 4 && indexPath.row == 0 ) { //BGM
        [self showPickerWithTitle:nil toLabel:lblMusicType inArray:MusicFormats origin:cell allowEmpty:NO doneBlock:^(NSString *selected) {
            [self.tableView reloadData];
        }];
    }else if( indexPath.section == 4 && indexPath.row == 1 ) { //CD Source
        [self showPickerWithTitle:nil toLabel:lblCDAudioSource inArray:CDFormats origin:cell];
    }else if( indexPath.section == 4 && indexPath.row == 4 ) { //OPL Core
        [self showPickerWithTitle:nil toLabel:lblOPLCore inArray:OPLCores origin:cell allowEmpty:NO doneBlock:^(NSString *selected) {
            if( [selected isEqualToString:@"NUKED"] )
                lblOPLChip.text = @"OPL3";
        }];
    }else if( indexPath.section == 4 && indexPath.row == 6 ) { //OPL Rate
        [self showPickerWithTitle:nil toLabel:lblOPLRate inArray:OPLSampleRates origin:cell];
    }else if( indexPath.section == 4 && indexPath.row == 7 ) { //SampleRate
        [self showPickerWithTitle:nil toLabel:lblResampleRate inArray:AudioSampleRates origin:cell];
    }else if( indexPath.section == 4 && indexPath.row == 8 ) { //Buffer size
        [self showPickerWithTitle:nil toLabel:lblAudioBufferSize inArray:AudioBufferSizes origin:cell];
    }else if( indexPath.section == 4 && indexPath.row == 9 ) { //Stereo
        toggleStereo.on = !toggleStereo.isOn;
    }else if( indexPath.section == 4 && indexPath.row == 10 ) { //Surround
        toggleSurroundOPL.on = !toggleSurroundOPL.isOn;
    }else if( indexPath.section == 5 && indexPath.row == 0 ) { //Log Level
        [self showPickerWithTitle:nil toLabel:lblLogLevel inArray:LogLevels origin:cell];
    }
}

- (IBAction)btnDefaultClicked:(id)sender {
    PAL_LoadConfig(NO);
    [self readConfigs];
}

- (IBAction)btnConfirmClicked:(id)sender {
    if(!checkAllFilesIncluded){
        UIAlertController  *alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Cannot find data file in the iTunes File Sharing directory",nil)
                                                                        message:NSLocalizedString(@"NOTE: For copyright reasons data files required to run the game are NOT included.",nil)
                                                                 preferredStyle:UIAlertControllerStyleAlert];
        UIAlertAction *cacelAction = [UIAlertAction actionWithTitle:UIKitLocalizedString(@"OK") style:UIAlertActionStyleDefault handler:nil];
        [alert addAction:cacelAction];
        [self presentViewController:alert animated:YES completion:nil];
        return;
    }
    if(toggleGLSL.isOn && [[lblShader text] isEqualToString:@""]){
        UIAlertController  *alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Shader must be specified on GLSL enabled",nil)
                                                                        message:nil
                                                                 preferredStyle:UIAlertControllerStyleAlert];
        UIAlertAction *cacelAction = [UIAlertAction actionWithTitle:UIKitLocalizedString(@"OK") style:UIAlertActionStyleDefault handler:nil];
        [alert addAction:cacelAction];
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
    toggleGLSL.on           = gConfig.fEnableGLSL;
    toggleHDR.on            = gConfig.fEnableHDR;
    
    toggleTouchScreenOverlay.on = gConfig.fUseTouchOverlay;
    toggleKeepAspect.on         = gConfig.fKeepAspectRatio;
    textTextureWidth.text       = [NSString stringWithFormat:@"%d",gConfig.dwTextureWidth];
    textTextureHeight.text       = [NSString stringWithFormat:@"%d",gConfig.dwTextureHeight];
    toggleSmoothScaling.on      = gConfig.pszScaleQuality ? strncmp(gConfig.pszScaleQuality, "0", sizeof(char)) != 0 : NO;
    
    lblMusicType.text       = MusicFormats[gConfig.eMusicType];
    lblOPLCore.text         = OPLCores[gConfig.eOPLCore];
    lblOPLChip.text         = OPLChips[gConfig.eOPLChip];
    lblOPLRate.text         = [NSString stringWithFormat:@"%d",gConfig.iOPLSampleRate];
    lblCDAudioSource.text   = CDFormats[gConfig.eCDType-MUSIC_OGG];
    lblResampleRate.text    = [NSString stringWithFormat:@"%d",gConfig.iSampleRate];
    lblAudioBufferSize.text = [NSString stringWithFormat:@"%d",gConfig.wAudioBufferSize];
    lblShader.text          = [NSString stringWithFormat:@"%s",gConfig.pszShader];
    
    sliderMusicVolume.value     = gConfig.iMusicVolume;
    sliderSFXVolume.value       = gConfig.iSoundVolume;
    sliderResampleQuality.value = gConfig.iResampleQuality;
    
    
    lblLogLevel.text        = LogLevels[gConfig.iLogLevel];
    
    [self.tableView reloadData];
}

- (void)saveConfigs {
    gConfig.pszMsgFile  = [lblLanguageFile.text length] == 0 ? NULL : strdup([[lblLanguageFile  text] UTF8String]);
    gConfig.pszFontFile = [lblFontFile.text length] == 0 ? NULL : strdup([[lblFontFile      text] UTF8String]);
    gConfig.pszLogFile  = [textLogFile.text length] == 0 ? NULL : strdup([[textLogFile      text] UTF8String]);
    
    gConfig.iAudioChannels  = toggleStereo.isOn ? 2 : 1;
    gConfig.fUseSurroundOPL = toggleSurroundOPL.isOn;
    
    gConfig.fKeepAspectRatio = toggleKeepAspect.isOn;
    gConfig.dwTextureWidth = [textTextureWidth.text intValue];
    gConfig.dwTextureHeight = [textTextureHeight.text intValue];
    gConfig.fUseTouchOverlay = toggleTouchScreenOverlay.isOn;
    gConfig.pszScaleQuality  = strdup(toggleSmoothScaling.on ? "1" : "0");
    
    gConfig.fEnableGLSL = toggleGLSL.isOn;
    gConfig.fEnableHDR = toggleHDR.isOn;
    gConfig.pszShader = [lblShader.text length] == 0 ? NULL :strdup([lblShader.text UTF8String]);
   
    gConfig.eMusicType  = (MUSICTYPE)[MusicFormats indexOfObject:lblMusicType.text];
    gConfig.eOPLCore    = (OPLCORE_TYPE)[OPLCores  indexOfObject:lblOPLCore.text];
    gConfig.eOPLChip    = gConfig.eOPLCore == OPLCORE_NUKED ? OPLCHIP_OPL3 : (OPLCHIP_TYPE)[OPLChips  indexOfObject:lblOPLChip.text];
    gConfig.iOPLSampleRate = [lblOPLRate.text intValue];
    gConfig.eCDType     = (MUSICTYPE)[CDFormats indexOfObject:lblCDAudioSource.text]+MUSIC_OGG;
    gConfig.iSampleRate = [lblResampleRate.text intValue];
    gConfig.wAudioBufferSize = [lblAudioBufferSize.text intValue];
    
    gConfig.iMusicVolume = sliderMusicVolume.value;
    gConfig.iSoundVolume = sliderSFXVolume.value;
    gConfig.iResampleQuality = sliderResampleQuality.value;
    
    gConfig.iLogLevel = (LOGLEVEL)[LogLevels indexOfObject:lblLogLevel.text];
}

@end
