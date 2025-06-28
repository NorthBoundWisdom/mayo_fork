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
IWidgetOccView::Creator &getWidgetOccViewCreator()
{
    static IWidgetOccView::Creator fn;
    return fn;
}

void IWidgetOccView::setCreator(IWidgetOccView::Creator fn)
{
    getWidgetOccViewCreator() = std::move(fn);
}

IWidgetOccView *IWidgetOccView::create(const OccHandle<V3d_View> &view, QWidget *parent)
{
    const auto &fn = getWidgetOccViewCreator();
    if (!fn)
    {
        Message::SendFail() << "IWidgetOccView::create() called without a creator function";
        return nullptr;
    }
    return fn(view, parent);
}
} // namespace Mayo
