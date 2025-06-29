/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include <QOpenGLWidget>
#include <QtWidgets/QWidget>

#include <Graphic3d_GraphicDriver.hxx>
#include <Standard_Version.hxx>
#include <V3d_View.hxx>

#include "widget_occ_view_i.h"

namespace Mayo
{

// Integration of OpenCascade 7.6 with QOpenGLWidget allows widgets with
// translucid background to be correctly displayed over V3d_View
// QOpenGLWidgetOccView implementation is based on
// https://github.com/gkv311/occt-samples-qopenglwidget
class QOpenGLWidgetOccView : public QOpenGLWidget, public IWidgetOccView
{
public:
    ~QOpenGLWidgetOccView() override;

    void redraw() override;
    QWidget *widget() override
    {
        return this;
    }
    bool supportsWidgetOpacity() const override
    {
        return true;
    }

    static QOpenGLWidgetOccView *create(const OccHandle<V3d_View> &view, QWidget *parent);
    static OccHandle<Graphic3d_GraphicDriver> createCompatibleGraphicsDriver();

private:
    QOpenGLWidgetOccView(const OccHandle<V3d_View> &view, QWidget *parent = nullptr);

protected:
    void initializeGL() override;
    void paintGL() override;
};
} // namespace Mayo
