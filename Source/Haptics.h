#pragma once

#include <juce_core/juce_core.h>

namespace Haptics
{
    enum class Style
    {
        light,
        medium,
        heavy,
        rigid,
        soft
    };

    void trigger (Style style);
    void triggerFromString (const juce::String& styleName);
}
