/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include <QtCore/QtGlobal>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <QtGui/QResizeEvent>

#include <Aspect_DisplayConnection.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <OpenGl_Context.hxx>
#include <OpenGl_FrameBuffer.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Standard_Version.hxx>

#include "base/occ_handle.h"
#include "graphics/graphics_utils.h"

#include "widget_occ_view.h"
#include "widget_occ_view_window.h"

namespace Mayo
{

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

OccHandle<Graphic3d_GraphicDriver> QWidgetOccView::createCompatibleGraphicsDriver()
{
    return new OpenGl_GraphicDriver(GraphicsUtils::AspectDisplayConnection_create());
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
