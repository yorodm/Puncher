
#include <TargetConditionals.h>
#if TARGET_OS_IOS == 1 || TARGET_OS_VISION == 1
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

#define IPLUG_AUVIEWCONTROLLER IPlugAUViewController_vPuncher
#define IPLUG_AUAUDIOUNIT IPlugAUAudioUnit_vPuncher
#import <PuncherAU/IPlugAUViewController.h>
#import <PuncherAU/IPlugAUAudioUnit.h>

//! Project version number for PuncherAU.
FOUNDATION_EXPORT double PuncherAUVersionNumber;

//! Project version string for PuncherAU.
FOUNDATION_EXPORT const unsigned char PuncherAUVersionString[];

@class IPlugAUViewController_vPuncher;
