/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include <functional>

#include <QOpenGLWidget>
#include <QtWidgets/QWidget>

#include <Graphic3d_GraphicDriver.hxx>
#include <Standard_Version.hxx>
#include <V3d_View.hxx>

#include "base/occ_handle.h"

namespace Mayo
{

// Base interface for bridging Qt and OpenCascade 3D view
// IWidgetOccView does not handle input devices interaction like keyboard and
// mouse
class IWidgetOccView
{
public:
    const OccHandle<V3d_View> &v3dView() const
    {
        return m_view;
    }

    virtual void redraw() = 0;
    virtual QWidget *widget() = 0;
    virtual bool supportsWidgetOpacity() const = 0;

    using Creator = std::function<IWidgetOccView *(const OccHandle<V3d_View> &, QWidget *)>;
    static void setCreator(Creator fn);
    static IWidgetOccView *create(const OccHandle<V3d_View> &view, QWidget *parent = nullptr);

protected:
    IWidgetOccView(const OccHandle<V3d_View> &view)
        : m_view(view)
    {
    }

private:
    OccHandle<V3d_View> m_view;
};
} // namespace Mayo
