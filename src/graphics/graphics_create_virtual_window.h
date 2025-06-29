/****************************************************************************
+** Copyright (c) 2022, Fougue Ltd. <http://www.fougue.pro>
+** All rights reserved.
+** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
+****************************************************************************/

#pragma once
#include <Graphic3d_GraphicDriver.hxx>

#include "base/occ_handle.h"

namespace Mayo
{
extern OccHandle<Aspect_Window>
graphicsCreateVirtualWindow(const OccHandle<Graphic3d_GraphicDriver> &gfxDriver, int wndWidth,
                            int wndHeight);

} // namespace Mayo
