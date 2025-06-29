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
// Fallback using traditional QWidget wrapper, no translucid background support
class QWidgetOccView : public QWidget, public IWidgetOccView
{
public:
    void redraw() override;
    QWidget *widget() override
    {
        return this;
    }
    bool supportsWidgetOpacity() const override
    {
        return false;
    }

    static QWidgetOccView *create(const OccHandle<V3d_View> &view, QWidget *parent);
    static OccHandle<Graphic3d_GraphicDriver> createCompatibleGraphicsDriver();

private:
    QWidgetOccView(const OccHandle<V3d_View> &view, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    QPaintEngine *paintEngine() const override
    {
        return nullptr;
    }
};

} // namespace Mayo
