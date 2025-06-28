/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include <QtCore/QtGlobal>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <functional>

#include <QtGui/QResizeEvent>

#include <Aspect_DisplayConnection.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <OpenGl_Context.hxx>
#include <OpenGl_FrameBuffer.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Standard_Version.hxx>

#include "base/occ_handle.h"
#include "graphics/graphics_utils.h"

#include "occt_window.h"
#include "widget_occ_view.h"

namespace Mayo
{

// namespace
// {
// class QtOccFrameBuffer : public OpenGl_FrameBuffer
// {
//     DEFINE_STANDARD_RTTI_INLINE(QtOccFrameBuffer, OpenGl_FrameBuffer)

// public:
//     QtOccFrameBuffer()
//     {
//     }

//     void BindBuffer(const OccHandle<OpenGl_Context> &ctx) override
//     {
//         OpenGl_FrameBuffer::BindBuffer(ctx);
//         // NOTE: commenting the line just below makes the FBO to work on some
//         // configs(eg VM Ubuntu 18.04)
//         ctx->SetFrameBufferSRGB(true, false);
//     }

//     void BindDrawBuffer(const OccHandle<OpenGl_Context> &ctx) override
//     {
//         OpenGl_FrameBuffer::BindDrawBuffer(ctx);
//         ctx->SetFrameBufferSRGB(true, false);
//     }

//     void BindReadBuffer(const OccHandle<OpenGl_Context> &ctx) override
//     {
//         OpenGl_FrameBuffer::BindReadBuffer(ctx);
//     }
// };

// void QOpenGLWidgetOccView_createOpenGlContext(
//     std::function<void(Aspect_RenderingContext)> fnCallback)
// {
//     auto glCtx = makeOccHandle<OpenGl_Context>();
//     constexpr bool isCoreProfile = false;
//     if (!glCtx->Init(isCoreProfile))
//     {
//         Message::SendFail() << "Error: OpenGl_Context is unable to wrap OpenGL context";
//         return;
//     }

//     if (fnCallback)
//         fnCallback(glCtx->RenderingContext());
// }

// OccHandle<Graphic3d_GraphicDriver> QOpenGLWidgetOccView_createCompatibleGraphicsDriver()
// {
//     auto gfxDriver = new OpenGl_GraphicDriver(GraphicsUtils::AspectDisplayConnection_create(),
//                                               false /*dontInit*/);
//     // Let QOpenGLWidget manage buffer swap
//     gfxDriver->ChangeOptions().buffersNoSwap = true;
//     // Don't write into alpha channel
//     gfxDriver->ChangeOptions().buffersOpaqueAlpha = true;
//     // Offscreen FBOs should be always used
//     gfxDriver->ChangeOptions().useSystemBuffer = false;

//     return gfxDriver;
// }

// bool QOpenGLWidgetOccView_wrapFrameBuffer(const OccHandle<Graphic3d_GraphicDriver> &gfxDriver)
// {
//     // Wrap FBO created by QOpenGLWidget
//     auto driver = OccHandle<OpenGl_GraphicDriver>::DownCast(gfxDriver);
//     if (!driver)
//         return false;

//     const OccHandle<OpenGl_Context> &glCtx = driver->GetSharedContext();
//     OccHandle<OpenGl_FrameBuffer> defaultFbo = glCtx->DefaultFrameBuffer();
//     if (!defaultFbo)
//     {
//         // defaultFbo = new OpenGl_FrameBuffer;
//         defaultFbo = new QtOccFrameBuffer;
//         glCtx->SetDefaultFrameBuffer(defaultFbo);
//     }

//     if (!defaultFbo->InitWrapper(glCtx))
//     {
//         defaultFbo.Nullify();
//         Message::SendFail() << "Default FBO wrapper creation failed";
//         return false;
//     }

//     return true;
// }

// Graphic3d_Vec2i QOpenGLWidgetOccView_getDefaultframeBufferViewportSize(
//     const OccHandle<Graphic3d_GraphicDriver> &gfxDriver)
// {
//     auto driver = OccHandle<OpenGl_GraphicDriver>::DownCast(gfxDriver);
//     return driver->GetSharedContext()->DefaultFrameBuffer()->GetVPSize();
// }

OccHandle<Graphic3d_GraphicDriver> QWidgetOccView_createCompatibleGraphicsDriver()
{
    return new OpenGl_GraphicDriver(GraphicsUtils::AspectDisplayConnection_create());
}

// OccHandle<Aspect_NeutralWindow> createNativeWindow([[maybe_unused]] QWidget *widget)
// {
//     auto window = new Aspect_NeutralWindow;
//     // On non-Windows systems Aspect_Drawable is aliased to 'unsigned long' so
//     // can't init with nullptr
//     Aspect_Drawable nativeWin = 0;
// #ifdef Q_OS_WIN
//     HDC wglDevCtx = wglGetCurrentDC();
//     HWND wglWin = WindowFromDC(wglDevCtx);
//     nativeWin = (Aspect_Drawable)wglWin;
// #else
//     nativeWin = (Aspect_Drawable)widget->winId();
// #endif
//     window->SetNativeHandle(nativeWin);
//     return window;
// }
// } // namespace

QWidgetOccView::QWidgetOccView(const OccHandle<V3d_View> &view, QWidget *parent)
    : QWidget(parent)
    , IWidgetOccView(view)
{
    std::cout << "QWidgetOccView::QWidgetOccView()" << std::endl;
    this->setMouseTracking(true);
    this->setBackgroundRole(QPalette::NoRole);

    // Avoid Qt background clears to improve resizing speed
    this->setAutoFillBackground(false);
    this->setAttribute(Qt::WA_NoSystemBackground);
    this->setAttribute(Qt::WA_PaintOnScreen);
}

// Defined in widget_occ_view.cpp
OccHandle<Graphic3d_GraphicDriver> QWidgetOccView::createCompatibleGraphicsDriver()
{
    return QWidgetOccView_createCompatibleGraphicsDriver();
}

void QWidgetOccView::redraw()
{
    this->v3dView()->Redraw();
}

QWidgetOccView *QWidgetOccView::create(const OccHandle<V3d_View> &view, QWidget *parent)
{
    return new QWidgetOccView(view, parent);
}

void QWidgetOccView::showEvent(QShowEvent *)
{
    if (this->v3dView()->Window().IsNull())
    {
        auto hWnd = makeOccHandle<OcctWindow>(this);
        this->v3dView()->SetWindow(hWnd);
        if (!hWnd->IsMapped())
            hWnd->Map();

        this->v3dView()->MustBeResized();
    }
}

void QWidgetOccView::paintEvent(QPaintEvent *)
{
    this->v3dView()->Redraw();
}

void QWidgetOccView::resizeEvent(QResizeEvent *event)
{
    if (!event->spontaneous()) // Workaround for infinite window shrink on Ubuntu
        this->v3dView()->MustBeResized();
}

} // namespace Mayo
