// Toast Renderer with Emoji Support
// Uses ToastFont12 for text with emoji-to-ASCII fallback
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "ToastFont12.h"

// Ensure stdint types are available
#if !defined(__STDINT_H) && !defined(_STDINT_H_)
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long int32_t;
typedef short int16_t;
typedef signed char int8_t;
#endif

// Also ensure these specific types are available
#ifndef int8_t
typedef signed char int8_t;
#endif
#ifndef uint16_t
typedef unsigned short uint16_t;
#endif
#ifndef int16_t
typedef short int16_t;
#endif

namespace ToastRenderer {

// Emoji to ASCII fallback mapping
struct EmojiMap {
    const char* emoji;
    const char* ascii;
};

static const EmojiMap emojiMappings[] = {
    // Faces
    {"😊", ":)"},
    {"😀", ":D"},
    {"😃", ":D"},
    {"😄", ":D"},
    {"😁", ":D"},
    {"🙂", ":)"},
    {"😢", ":("},
    {"😭", ":(("},
    {"😠", ">:("},
    {"😡", ">:("},
    {"🤔", "?"},
    {"😴", "zzz"},
    {"😪", "zzz"},
    {"🤗", "<3"},
    {"😱", "!!"},
    {"😇", "O:)"},
    
    // Objects & Symbols
    {"💭", "..."},
    {"💤", "zzz"},
    {"🤖", "[AI]"},
    {"👂", "[ear]"},
    {"🎉", "*"},
    {"🎊", "*"},
    {"🔥", "!"},
    {"⚡", "!"},
    {"💡", "*"},
    {"🔋", "[bat]"},
    {"📱", "[phone]"},
    {"💻", "[pc]"},
    {"🎵", "♪"},
    {"🎶", "♫"},
    {"🛠", "[tool]"},
    {"⚙", "[set]"},
    {"🔧", "[fix]"},
    {"🌟", "*"},
    {"✨", "*"},
    {"🎈", "o"},
    {"🎁", "[gift]"},
    {"🔔", "[!]"},
    {"📢", "[!]"},
    {"📣", "[!]"},
    {"🌈", "~"},
    {"⭐", "*"},
    
    // Tech & Connectivity
    {"📶", "[wifi]"},
    {"🔌", "[pwr]"},
    {"💾", "[save]"},
    {"🖥", "[disp]"},
    {"⌨", "[key]"},
    {"🖱", "[mouse]"},
    
    // Status
    {"✅", "[ok]"},
    {"❌", "[x]"},
    {"⚠", "[!]"},
    {"🆘", "[help]"},
    {"🔴", "[red]"},
    {"🟢", "[grn]"},
    {"🔵", "[blu]"},
    
    // End marker
    {nullptr, nullptr}
};

// Convert emoji-containing string to ASCII-compatible version
inline String convertEmojiToAscii(const String& input) {
    String result = input;
    
    // Replace each emoji with ASCII equivalent
    for (int i = 0; emojiMappings[i].emoji != nullptr; i++) {
        result.replace(emojiMappings[i].emoji, emojiMappings[i].ascii);
    }
    
    return result;
}

// Draw toast text with emoji fallback
inline void drawToastText(Adafruit_GFX& display, int16_t x, int16_t y, const String& text, uint16_t color = SH110X_WHITE) {
    String convertedText = convertEmojiToAscii(text);
    ToastFont12::drawTextUTF8(display, x, y, convertedText.c_str(), color);
}

// Get text bounds for toast with emoji conversion
inline void getToastTextBounds(const String& text, int16_t& x1, int16_t& y1, uint16_t& w, uint16_t& h) {
    String convertedText = convertEmojiToAscii(text);
    
    // Calculate bounds manually since we can't call Adafruit_GFX::getTextBounds without display object
    w = 0;
    h = ToastFont12::YAdvance;
    
    for (int i = 0; convertedText[i]; ) {
        uint32_t cp = ToastFont12::decodeUTF8(convertedText.c_str(), i);
        int16_t gi = ToastFont12::findGlyph(cp);
        if (gi < 0) gi = ToastFont12::findGlyph(0x3F); // '?'
        
        int8_t xa = pgm_read_byte(&ToastFont12::Glyphs[gi].xAdvance);
        w += xa;
    }
    
    x1 = 0;
    y1 = -ToastFont12::Ascent;
}

} // namespace ToastRenderer
