#!/usr/bin/env python3
"""
TTF/OTF → C header font generator for Arduino (Adafruit_GFX-friendly)

Generates a compact monochrome bitmap font header that you can include in
your sketch and render on an Adafruit_GFX display. It supports UTF‑8 text by
embedding a sparse codepoint→glyph map and a small UTF‑8 decoder in the header.

Why this tool?
- Adafruit_GFX built-in converter is great but expects contiguous codepoint
  ranges which is impractical for Vietnamese. This tool lets you include only
  the glyphs you need (from a sample text or charset) and keeps memory small.

Requirements:
  pip install freetype-py

Usage examples:
  # Generate a font from a TTF for glyphs used in a sample text file
  python tools/font_to_header.py \
    --ttf path/to/YourFont.ttf --size 16 \
    --text assets/sample_vi_text.txt \
    --name ViFont16 --out include/ViFont16.h

  # Generate from an explicit charset string
  python tools/font_to_header.py --ttf path.ttf --size 16 \
    --charset "0123456789:%-/ ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzÀÁÂ..." \
    --name ClockFont16 --out include/ClockFont16.h

Notes:
- 'size' is the pixel height target set through FreeType. Actual glyph sizes
   can vary by font metrics.
- The output header includes helper functions to draw UTF‑8 strings with an
   Adafruit_GFX display: drawTextUTF8(display, x, y, text [, color]).
"""

from __future__ import annotations
import argparse
import os
from typing import List, Tuple, Dict, Set

import freetype


def parse_args() -> argparse.Namespace:
    ap = argparse.ArgumentParser()
    ap.add_argument('--ttf', required=True, help='Path to TTF/OTF font file')
    ap.add_argument('--size', type=int, required=True, help='Pixel height target')
    group = ap.add_mutually_exclusive_group(required=True)
    group.add_argument('--text', help='Path to UTF-8 text file to collect codepoints')
    group.add_argument('--charset', help='Literal UTF-8 characters to include')
    group.add_argument('--range', help='Inclusive codepoint range like 32-126')
    ap.add_argument('--name', required=True, help='C symbol prefix (e.g., MyFont16)')
    ap.add_argument('--out', required=True, help='Output header path (.h)')
    ap.add_argument('--threshold', type=int, default=128, help='Grayscale threshold 0..255 to set bits (default 128)')
    return ap.parse_args()


def collect_codepoints(args: argparse.Namespace) -> List[int]:
    cps: Set[int] = set()
    if args.text:
        with open(args.text, 'r', encoding='utf-8') as f:
            s = f.read()
        for ch in s:
            cps.add(ord(ch))
    elif args.charset:
        for ch in args.charset:
            cps.add(ord(ch))
    else:
        lo, hi = args.range.split('-')
        cps = set(range(int(lo), int(hi) + 1))
    # Always include fallback '?' (0x3F) for missing glyphs
    cps.add(0x3F)
    # Drop control chars except newline and space
    cps.discard(0x0D)
    # Keep space
    return sorted(cps)


def pack_bitmap(rows: List[List[int]], threshold: int) -> Tuple[bytearray, int, int]:
    h = len(rows)
    w = len(rows[0]) if h else 0
    out = bytearray()
    for y in range(h):
        byte = 0
        bitpos = 7
        for x in range(w):
            v = rows[y][x]
            bit = 1 if v >= threshold else 0
            byte |= (bit << bitpos)
            bitpos -= 1
            if bitpos < 0:
                out.append(byte)
                byte = 0
                bitpos = 7
        if bitpos != 7:
            out.append(byte)
    return out, w, h


def render_font(ttf_path: str, px_size: int, codepoints: List[int], threshold: int):
    face = freetype.Face(ttf_path)
    face.set_pixel_sizes(0, px_size)

    # freetype-py exposes size metrics directly on face.size
    sm = face.size
    ascent = sm.ascender // 64
    descent = -(sm.descender // 64)
    y_advance = (sm.height // 64) or (ascent + descent)

    bitmaps = bytearray()
    glyphs = []  # (offset, w, h, xAdvance, xOffset, yOffset)

    for cp in codepoints:
        face.load_char(cp, freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_NORMAL)
        glyph = face.glyph
        bm = glyph.bitmap
        w, h = bm.width, bm.rows
        # Convert FT grayscale buffer (0..255) row-major to 1-bit packed MSB-first
        rows: List[List[int]] = []
        if h > 0 and w > 0:
            buf = bm.buffer
            for y in range(h):
                row = list(buf[y * bm.pitch : y * bm.pitch + w])
                rows.append(row)
            packed, _, _ = pack_bitmap(rows, threshold)
        else:
            packed = bytearray()

        offset = len(bitmaps)
        bitmaps.extend(packed)

        xAdvance = glyph.advance.x // 64
        xOffset = glyph.bitmap_left
        yOffset = -glyph.bitmap_top  # distance from baseline to top (negative in GFXfont convention)

        glyphs.append((offset, w, h, xAdvance, xOffset, yOffset))

    return {
        'ascent': ascent,
        'descent': descent,
        'y_advance': y_advance,
        'bitmaps': bitmaps,
        'glyphs': glyphs,
    }


HEADER_TEMPLATE = """
#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <pgmspace.h>

// Font: {font_name}
// Source: {ttf_basename}, size={size}px, glyphs={num_glyphs}
// Generated by tools/font_to_header.py

namespace {font_name} {{

static const uint8_t PROGMEM Bitmaps[] = {{
{bitmap_bytes}
}};

typedef struct __attribute__((packed)) {{
  uint16_t offset;
  uint8_t  w;
  uint8_t  h;
  int8_t   xAdvance;
  int8_t   xOffset;
  int8_t   yOffset;
}} Glyph;

static const Glyph PROGMEM Glyphs[] = {{
{glyph_entries}
}};

// UTF-32 codepoints for glyph indices 0..N-1
static const uint32_t PROGMEM Codepoints[] = {{
{codepoint_entries}
}};

static const uint8_t  YAdvance = {y_advance};
static const int8_t   Ascent   = {ascent};
static const int8_t   Descent  = {descent};

// Linear search; for bigger sets, generate a binary search.
static int16_t findGlyph(uint32_t cp) {{
  uint16_t n = sizeof(Codepoints)/sizeof(Codepoints[0]);
  for (uint16_t i=0;i<n;i++) {{
    if (cp == pgm_read_dword(&Codepoints[i])) return (int16_t)i;
  }}
  return -1;
}}

// Decode next UTF-8 codepoint; returns codepoint, advances index by reference.
static uint32_t decodeUTF8(const char* s, int &i) {{
  uint8_t c = (uint8_t)s[i++];
  if (c < 0x80) return c;
  if ((c & 0xE0) == 0xC0) {{
    uint32_t c2 = (uint8_t)s[i++];
    return ((c & 0x1F) << 6) | (c2 & 0x3F);
  }}
  if ((c & 0xF0) == 0xE0) {{
    uint32_t c2 = (uint8_t)s[i++];
    uint32_t c3 = (uint8_t)s[i++];
    return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
  }}
  if ((c & 0xF8) == 0xF0) {{
    uint32_t c2 = (uint8_t)s[i++];
    uint32_t c3 = (uint8_t)s[i++];
    uint32_t c4 = (uint8_t)s[i++];
    return ((c & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
  }}
  return 0x3F; // fallback '?'
}}

// Draw a UTF-8 string at baseline (x,y)
static void drawTextUTF8(Adafruit_GFX &d, int16_t x, int16_t y, const char* utf8, uint16_t color=1) {{
  for (int i=0; utf8[i];) {{
    uint32_t cp = decodeUTF8(utf8, i);
    int16_t gi = findGlyph(cp);
    if (gi < 0) gi = findGlyph(0x3F); // '?'
    uint16_t off = pgm_read_word(&Glyphs[gi].offset);
    uint8_t  gw  = pgm_read_byte(&Glyphs[gi].w);
    uint8_t  gh  = pgm_read_byte(&Glyphs[gi].h);
    int8_t   xa  = pgm_read_byte(&Glyphs[gi].xAdvance);
    int8_t   xo  = pgm_read_byte(&Glyphs[gi].xOffset);
    int8_t   yo  = pgm_read_byte(&Glyphs[gi].yOffset);
    if (gw && gh) {{
      d.drawBitmap(x + xo, y + yo, Bitmaps + off, gw, gh, color);
    }}
    x += xa;
  }}
}}

}} // namespace {font_name}
"""


def to_c_hex_list(data: bytes, width: int = 12) -> str:
    out = []
    line = []
    for i, b in enumerate(data):
        line.append(f"0x{b:02X}")
        if len(line) >= width:
            out.append(', '.join(line))
            line = []
    if line:
        out.append(', '.join(line))
    return ',\n'.join(out)


def main():
    args = parse_args()
    cps = collect_codepoints(args)
    font = render_font(args.ttf, args.size, cps, args.threshold)

    glyph_entries = []
    for g in font['glyphs']:
        glyph_entries.append(f"  {{ {g[0]}, {g[1]}, {g[2]}, {g[3]}, {g[4]}, {g[5]} }}")

    codepoint_entries = []
    for cp in cps:
        codepoint_entries.append(f"  0x{cp:04X}")

    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    with open(args.out, 'w', encoding='utf-8') as f:
        f.write(HEADER_TEMPLATE.format(
            font_name=args.name,
            ttf_basename=os.path.basename(args.ttf),
            size=args.size,
            num_glyphs=len(cps),
            bitmap_bytes=to_c_hex_list(font['bitmaps']),
            glyph_entries=',\n'.join(glyph_entries),
            codepoint_entries=',\n'.join(codepoint_entries),
            y_advance=font['y_advance'],
            ascent=font['ascent'],
            descent=font['descent'],
        ))

    print(f"Wrote {args.out} with {len(cps)} glyphs; yAdvance={font['y_advance']}")


if __name__ == '__main__':
    main()


