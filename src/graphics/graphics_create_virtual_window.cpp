/****************************************************************************
+** Copyright (c) 2022, Fougue Ltd. <http://www.fougue.pro>
+** All rights reserved.
+** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
+****************************************************************************/

// --
// NOTE
// This file isolates inclusion of <Aspect_DisplayConnection.hxx> which is
// problematic on X11/Linux <X.h> #defines constants like "None" which causes
// name clash with GuiDocument::ViewTrihedronMode::None
// --

#include "base/global.h"

#ifdef MAYO_OS_WINDOWS
#include <windows.h>
#endif

#include <Aspect_DisplayConnection.hxx>
#include <Graphic3d_GraphicDriver.hxx>

#include "base/occ_handle.h"
#if defined(MAYO_OS_WINDOWS)
#include <WNT_WClass.hxx>
#include <WNT_Window.hxx>
#elif defined(MAYO_OS_MAC)
#include <Cocoa_Window.hxx>
#elif defined(MAYO_OS_ANDROID)
#include <Aspect_NeutralWindow.hxx>
#else
#include <Xw_Window.hxx>
#endif

namespace Mayo
{
/**
 * @brief 创建一个虚拟窗口用于离屏渲染
 *
 * 该函数根据不同操作系统创建相应的虚拟窗口。虚拟窗口不会被实际显示出来，
 * 但可以用于OpenCascade图形渲染管线中，适用于图像离屏生成、3D模型预览等场景。
 *
 * @param gfxDriver 图形驱动程序句柄，在非Windows平台上用于获取显示连接
 * @param wndWidth 窗口宽度
 * @param wndHeight 窗口高度
 * @return OccHandle<Aspect_Window> 返回创建的虚拟窗口句柄
 */
OccHandle<Aspect_Window>
graphicsCreateVirtualWindow([[maybe_unused]] const OccHandle<Graphic3d_GraphicDriver> &gfxDriver,
                            int wndWidth, int wndHeight)
{
#if defined(MAYO_OS_WINDOWS)
    // Create a "virtual" WNT window being a pure WNT window redefined to be never
    // shown
    static OccHandle<WNT_WClass> wClass;
    if (wClass.IsNull())
    {
        auto cursor = LoadCursor(NULL, IDC_ARROW);
        wClass = new WNT_WClass("GW3D_Class", nullptr, CS_VREDRAW | CS_HREDRAW, 0, 0, cursor);
    }

    auto wnd = new WNT_Window("", wClass, WS_POPUP, 0, 0, wndWidth, wndHeight, Quantity_NOC_BLACK);
#elif defined(MAYO_OS_MAC)
    auto wnd = new Cocoa_Window("", 0, 0, wndWidth, wndHeight);
#elif defined(MAYO_OS_ANDROID)
    auto wnd = new Aspect_NeutralWindow;
    wnd->SetSize(wndWidth, wndHeight);
#else
    auto displayConn = gfxDriver->GetDisplayConnection();
    auto wnd = new Xw_Window(displayConn, "", 0, 0, wndWidth, wndHeight);
#endif

    wnd->SetVirtual(true);
    return wnd;
}

} // namespace Mayo
