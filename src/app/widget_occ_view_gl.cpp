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
#include "widget_occ_view_gl.h"

namespace Mayo
{

namespace
{
class QtOccFrameBuffer : public OpenGl_FrameBuffer
{
    DEFINE_STANDARD_RTTI_INLINE(QtOccFrameBuffer, OpenGl_FrameBuffer)

public:
    QtOccFrameBuffer()
    {
    }

    void BindBuffer(const OccHandle<OpenGl_Context> &ctx) override
    {
        OpenGl_FrameBuffer::BindBuffer(ctx);
        // NOTE: commenting the line just below makes the FBO to work on some
        // configs(eg VM Ubuntu 18.04)
        ctx->SetFrameBufferSRGB(true, false);
    }

    void BindDrawBuffer(const OccHandle<OpenGl_Context> &ctx) override
    {
        OpenGl_FrameBuffer::BindDrawBuffer(ctx);
        ctx->SetFrameBufferSRGB(true, false);
    }

    void BindReadBuffer(const OccHandle<OpenGl_Context> &ctx) override
    {
        OpenGl_FrameBuffer::BindReadBuffer(ctx);
    }
};

void QOpenGLWidgetOccView_createOpenGlContext(
    std::function<void(Aspect_RenderingContext)> fnCallback)
{
    auto glCtx = makeOccHandle<OpenGl_Context>();
    constexpr bool isCoreProfile = false;
    if (!glCtx->Init(isCoreProfile))
    {
        Message::SendFail() << "Error: OpenGl_Context is unable to wrap OpenGL context";
        return;
    }

    if (fnCallback)
        fnCallback(glCtx->RenderingContext());
}

OccHandle<Graphic3d_GraphicDriver> QOpenGLWidgetOccView_createCompatibleGraphicsDriver()
{
    auto gfxDriver = new OpenGl_GraphicDriver(GraphicsUtils::AspectDisplayConnection_create(),
                                              false /*dontInit*/);
    // Let QOpenGLWidget manage buffer swap
    gfxDriver->ChangeOptions().buffersNoSwap = true;
    // Don't write into alpha channel
    gfxDriver->ChangeOptions().buffersOpaqueAlpha = true;
    // Offscreen FBOs should be always used
    gfxDriver->ChangeOptions().useSystemBuffer = false;

    return gfxDriver;
}

bool QOpenGLWidgetOccView_wrapFrameBuffer(const OccHandle<Graphic3d_GraphicDriver> &gfxDriver)
{
    // Wrap FBO created by QOpenGLWidget
    auto driver = OccHandle<OpenGl_GraphicDriver>::DownCast(gfxDriver);
    if (!driver)
        return false;

    const OccHandle<OpenGl_Context> &glCtx = driver->GetSharedContext();
    OccHandle<OpenGl_FrameBuffer> defaultFbo = glCtx->DefaultFrameBuffer();
    if (!defaultFbo)
    {
        // defaultFbo = new OpenGl_FrameBuffer;
        defaultFbo = new QtOccFrameBuffer;
        glCtx->SetDefaultFrameBuffer(defaultFbo);
    }

    if (!defaultFbo->InitWrapper(glCtx))
    {
        defaultFbo.Nullify();
        Message::SendFail() << "Default FBO wrapper creation failed";
        return false;
    }

    return true;
}

Graphic3d_Vec2i QOpenGLWidgetOccView_getDefaultframeBufferViewportSize(
    const OccHandle<Graphic3d_GraphicDriver> &gfxDriver)
{
    auto driver = OccHandle<OpenGl_GraphicDriver>::DownCast(gfxDriver);
    return driver->GetSharedContext()->DefaultFrameBuffer()->GetVPSize();
}

OccHandle<Graphic3d_GraphicDriver> QWidgetOccView_createCompatibleGraphicsDriver()
{
    return new OpenGl_GraphicDriver(GraphicsUtils::AspectDisplayConnection_create());
}

OccHandle<Aspect_NeutralWindow> createNativeWindow([[maybe_unused]] QWidget *widget)
{
    auto window = new Aspect_NeutralWindow;
    // On non-Windows systems Aspect_Drawable is aliased to 'unsigned long' so
    // can't init with nullptr
    Aspect_Drawable nativeWin = 0;
#ifdef Q_OS_WIN
    HDC wglDevCtx = wglGetCurrentDC();
    HWND wglWin = WindowFromDC(wglDevCtx);
    nativeWin = (Aspect_Drawable)wglWin;
#else
    nativeWin = (Aspect_Drawable)widget->winId();
#endif
    window->SetNativeHandle(nativeWin);
    return window;
}
} // namespace

QOpenGLWidgetOccView::QOpenGLWidgetOccView(const OccHandle<V3d_View> &view, QWidget *parent)
    : QOpenGLWidget(parent)
    , IWidgetOccView(view)
{
    std::cout << "QOpenGLWidgetOccView::QOpenGLWidgetOccView()" << std::endl;
    this->setMouseTracking(true);
    this->setBackgroundRole(QPalette::NoRole);

    this->setUpdatesEnabled(true);
    this->setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

    QSurfaceFormat glFormat;
    glFormat.setDepthBufferSize(24);
    glFormat.setStencilBufferSize(8);
    constexpr bool isCoreProfile = false;
    if (isCoreProfile)
        glFormat.setVersion(4, 5);

    glFormat.setProfile(isCoreProfile ? QSurfaceFormat::CoreProfile :
                                        QSurfaceFormat::CompatibilityProfile);

    this->setFormat(glFormat);
}

void QOpenGLWidgetOccView::redraw()
{
    this->update();
}

QOpenGLWidgetOccView *QOpenGLWidgetOccView::create(const OccHandle<V3d_View> &view, QWidget *parent)
{
    return new QOpenGLWidgetOccView(view, parent);
}

OccHandle<Graphic3d_GraphicDriver> QOpenGLWidgetOccView::createCompatibleGraphicsDriver()
{
    return QOpenGLWidgetOccView_createCompatibleGraphicsDriver();
}

void QOpenGLWidgetOccView::initializeGL()
{
    const QRect wrect = this->rect();
    const Graphic3d_Vec2i viewSize(wrect.right() - wrect.left(), wrect.bottom() - wrect.top());
    QOpenGLWidgetOccView_createOpenGlContext(
        [=](Aspect_RenderingContext context)
        {
            auto window = OccHandle<Aspect_NeutralWindow>::DownCast(this->v3dView()->Window());
            if (!window)
                window = createNativeWindow(this);

            window->SetSize(viewSize.x(), viewSize.y());
            this->v3dView()->SetWindow(window, context);
        });
}

void QOpenGLWidgetOccView::paintGL()
{
    if (!this->v3dView()->Window())
        return;

    const OccHandle<Graphic3d_GraphicDriver> &driver = this->v3dView()->Viewer()->Driver();
    if (!QOpenGLWidgetOccView_wrapFrameBuffer(driver))
        return;

    Graphic3d_Vec2i viewSizeOld;
    const Graphic3d_Vec2i viewSizeNew =
        QOpenGLWidgetOccView_getDefaultframeBufferViewportSize(driver);
    auto window = OccHandle<Aspect_NeutralWindow>::DownCast(this->v3dView()->Window());
    window->Size(viewSizeOld.x(), viewSizeOld.y());
    if (viewSizeNew != viewSizeOld)
    {
        window->SetSize(viewSizeNew.x(), viewSizeNew.y());
        this->v3dView()->MustBeResized();
        this->v3dView()->Invalidate();
    }

    // Redraw the viewer
    // this->v3dView()->InvalidateImmediate();
    this->v3dView()->Redraw();
}
} // namespace Mayo
