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

@implementation SettingsTableViewController {
    NSArray *AudioSampleRates;
    NSArray *AudioBufferSizes;
    NSArray *OPLSampleRates;
    NSArray *CDFormats;
    NSArray *MusicFormats;
    NSArray *OPLFormats;
    NSArray *LogLevels;
    NSMutableArray *AvailFiles;
    
    AbstractActionSheetPicker *picker;
    
    IBOutlet UIView *transitionView;
    
    IBOutlet UILabel *lblResourceStatus;
    
    IBOutlet UILabel *lblLanguageFile;
    IBOutlet UILabel *lblFontFile;
    
    IBOutlet UISwitch *toggleTouchScreenOverlay;
    IBOutlet UISwitch *toggleKeepAspect;
    
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
    
    UILabel* tlabel=[[UILabel alloc] initWithFrame:CGRectMake(0,0, 300, 40)];
    tlabel.text=[NSString stringWithUTF8String:PAL_GIT_REVISION];
    tlabel.backgroundColor =[UIColor clearColor];
    tlabel.adjustsFontSizeToFitWidth=YES;
    self.navigationItem.titleView=tlabel;
    
    AudioSampleRates = @[ @"11025", @"22050", @"44100", @"49716" ];
    AudioBufferSizes = @[ @"512", @"1024", @"2048", @"4096", @"8192" ];
    OPLSampleRates = @[ @"12429", @"24858", @"44100", @"49716" ];
    CDFormats = @[ @"MP3", @"OGG" ];
    MusicFormats = @[ @"MIDI", @"RIX", @"MP3", @"OGG" ];
    OPLFormats = @[ @"DOSBOX", @"MAME", @"DOSBOXNEW" ];
    LogLevels = @[ @"VERBOSE", @"DEBUG", @"INFO", @"WARNING", @"ERROR", @"FATAL" ];
    
    AvailFiles = [NSMutableArray new];
    NSArray *builtinList = @[ @"wor16.fon", @"wor16.asc", @"m.msg"];
    NSArray *builtinExtensionList = @[@"exe",@"drv",@"dll",@"rpg",@"mkf",@"avi",@"dat",@"cfg",@"ini"];
    for( NSString *filename in [[NSFileManager defaultManager] contentsOfDirectoryAtPath:[NSString stringWithUTF8String:UTIL_BasePath()] error:nil] ) {
        if( ![self includedInList:builtinExtensionList name:filename.pathExtension] &&
            ![self includedInList:builtinList name:filename] ) {
            [AvailFiles addObject:filename];
        }
    }
    
    [self readConfigs];
    
    UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(dismissKeyboard)];
    tap.cancelsTouchesInView = NO;
    [self.view addGestureRecognizer:tap];
}
-(void)dismissKeyboard
{
    [self.view endEditing:YES];
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
            rows = 2;
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

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [self.tableView cellForRowAtIndexPath:indexPath];
    if( indexPath.section == 1 && indexPath.row == 0 ) { //language file
        [self showPickerWithTitle:nil toLabel:lblLanguageFile inArray:AvailFiles origin:cell allowEmpty:YES];
    }else if( indexPath.section == 1 && indexPath.row == 1 ) { //font file
        [self showPickerWithTitle:nil toLabel:lblFontFile inArray:AvailFiles origin:cell allowEmpty:YES];
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

- (IBAction)btnConfirmClicked:(id)sender {
    [UIView transitionFromView:[self.navigationController view]
                        toView:transitionView
                      duration:0.65f
                       options:UIViewAnimationOptionTransitionCurlDown
                    completion:^(BOOL finished){
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

@end
