// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IRenderer.h"

class glArchivItem_Bitmap;

class OpenGLRenderer : public IRenderer
{
public:
    bool initOpenGL(OpenGL_Loader_Proc) override;
    void synchronize() override;
    void Draw3DBorder(const Rect& rect, bool elevated, glArchivItem_Bitmap& texture) override;
    void Draw3DContent(const Rect& rect, bool elevated, glArchivItem_Bitmap& texture, bool illuminated,
                       unsigned color) override;
    void DrawRect(const Rect& rect, unsigned color) override;
    void DrawLine(DrawPoint pt1, DrawPoint pt2, unsigned width, unsigned color) override;
};
