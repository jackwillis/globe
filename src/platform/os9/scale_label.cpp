// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "scale_label.hpp"
#include <Fonts.h>
#include <Quickdraw.h>
#include <algorithm>
#include <cstring>
namespace globe {
void ScaleLabel::sync(const Camera& camera, int width) {
    char text[16];
    const int length = scale_label_text(camera, width, text, sizeof text);
    const short size = width >= 360 ? 12 : 9; // Monaco's two installed sizes
    if (length == 0) {
        ready_ = false;
        text_[0] = 0;
        return;
    }
    if (ready_ && size == size_ && std::strcmp(text, text_) == 0)
        return;
    std::strcpy(text_, text);
    size_ = size;
    ready_ = false;
    short family = 0;
    GetFNum("\pMonaco", &family);
    if (family == 0) // not installed: the lookup gives the system font's number
        return;
    GrafPtr saved;
    GetPort(&saved);
    GrafPort port;
    OpenPort(&port);
    BitMap map;
    map.baseAddr = reinterpret_cast<Ptr>(bits_);
    map.rowBytes = kStride;
    TextFont(family);
    TextSize(size);
    TextFace(normal);
    FontInfo info;
    GetFontInfo(&info);
    unsigned char label_text[17];
    label_text[0] = static_cast<unsigned char>(length);
    std::memcpy(label_text + 1, text, static_cast<std::size_t>(length));
    const int height = info.ascent + info.descent;
    const int label_width = StringWidth(label_text);
    if (height > 0 && height <= LabelBitmap::kMaxHeight && label_width > 0 &&
        label_width <= kMaxWidth) {
        SetRect(&map.bounds, 0, 0, static_cast<short>(kMaxWidth),
                static_cast<short>(LabelBitmap::kMaxHeight));
        SetPortBits(&map);
        RectRgn(port.clipRgn, &map.bounds);
        EraseRect(&map.bounds);
        MoveTo(0, info.ascent);
        DrawString(label_text);
        bitmap_.width = label_width;
        bitmap_.height = height;
        bitmap_.stride = kStride;
        bitmap_.bits = bits_;
        ready_ = true;
    }
    ClosePort(&port);
    SetPort(saved);
}
} // namespace globe
