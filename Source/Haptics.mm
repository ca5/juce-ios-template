#include "Haptics.h"

#if JUCE_IOS
#import <UIKit/UIKit.h>
#elif JUCE_MAC
#import <AppKit/AppKit.h>
#endif

namespace Haptics
{
    void trigger (Style style)
    {
       #if JUCE_IOS
        if (@available(iOS 10.0, *))
        {
            UIImpactFeedbackStyle impactStyle = UIImpactFeedbackStyleLight;
            switch (style)
            {
                case Style::light:  impactStyle = UIImpactFeedbackStyleLight; break;
                case Style::medium: impactStyle = UIImpactFeedbackStyleMedium; break;
                case Style::heavy:  impactStyle = UIImpactFeedbackStyleHeavy; break;
                case Style::rigid:
                    if (@available(iOS 13.0, *)) impactStyle = UIImpactFeedbackStyleRigid;
                    else impactStyle = UIImpactFeedbackStyleMedium;
                    break;
                case Style::soft:
                    if (@available(iOS 13.0, *)) impactStyle = UIImpactFeedbackStyleSoft;
                    else impactStyle = UIImpactFeedbackStyleLight;
                    break;
            }

            dispatch_async (dispatch_get_main_queue(), ^{
                UIImpactFeedbackGenerator* generator = [[UIImpactFeedbackGenerator alloc] initWithStyle:impactStyle];
                [generator prepare];
                [generator impactOccurred];
            });
        }
       #elif JUCE_MAC
        if (@available(macOS 10.11, *))
        {
            NSHapticFeedbackPattern pattern = NSHapticFeedbackPatternGeneric;
            switch (style)
            {
                case Style::light:
                case Style::soft:
                    pattern = NSHapticFeedbackPatternGeneric;
                    break;
                case Style::medium:
                case Style::rigid:
                    pattern = NSHapticFeedbackPatternAlignment;
                    break;
                case Style::heavy:
                    pattern = NSHapticFeedbackPatternLevelChange;
                    break;
            }

            dispatch_async (dispatch_get_main_queue(), ^{
                [[NSHapticFeedbackManager defaultPerformer] performFeedbackPattern:pattern
                                                                   performanceTime:NSHapticFeedbackPerformanceTimeNow];
            });
        }
       #endif
    }

    void triggerFromString (const juce::String& styleName)
    {
        if (styleName == "medium")
            trigger (Style::medium);
        else if (styleName == "heavy")
            trigger (Style::heavy);
        else if (styleName == "rigid")
            trigger (Style::rigid);
        else if (styleName == "soft")
            trigger (Style::soft);
        else
            trigger (Style::light);
    }
}
