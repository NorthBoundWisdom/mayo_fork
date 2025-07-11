/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include <vector>

#include <QtWidgets/QWidget>

#include <Bnd_Box.hxx>
#include <Graphic3d_ClipPlane.hxx>
#include <Graphic3d_TextureMap.hxx>

#include "base/occ_handle.h"
#include "graphics/graphics_scene.h"


class QAbstractButton;
class QAbstractSlider;
class QCheckBox;
class QDoubleSpinBox;

namespace Mayo
{

// Widget panel dedicated to clip planes in 3D view
class WidgetClipPlanes : public QWidget
{
    Q_OBJECT

public:
    WidgetClipPlanes(GraphicsScene *scene, OccHandle<V3d_View> view, QWidget *parent = nullptr);
    ~WidgetClipPlanes() override;

    void setRanges(const Bnd_Box &box);
    void setClippingOn(bool on);

private:
    struct UiClipPlane
    {
        QCheckBox *check_On;
        QWidget *widget_Control;

        UiClipPlane(QCheckBox *checkOn, QWidget *widgetControl);
        QDoubleSpinBox *posSpin() const;
        QAbstractSlider *posSlider() const;
        QAbstractButton *inverseBtn() const;
        QDoubleSpinBox *customXDirSpin() const;
        QDoubleSpinBox *customYDirSpin() const;
        QDoubleSpinBox *customZDirSpin() const;
        double spinValueToSliderValue(double val) const;
        double sliderValueToSpinValue(double val) const;
    };

    struct ClipPlaneData
    {
        OccHandle<Graphic3d_ClipPlane> graphics;
        UiClipPlane ui;
    };

    using Range = std::pair<double, double>;

    void connectUi(ClipPlaneData *data);

    void setPlaneOn(const OccHandle<Graphic3d_ClipPlane> &plane, bool on);
    void setPlaneRange(ClipPlaneData *data, const Range &range);

    void createPlaneCappingTexture();

    class Ui_WidgetClipPlanes *m_ui;

    GraphicsScene *m_scene = nullptr;
    OccHandle<V3d_View> m_view;

    std::vector<ClipPlaneData> m_vecClipPlaneData;
    Bnd_Box m_bndBox;
    OccHandle<Graphic3d_TextureMap> m_textureCapping;
};

} // namespace Mayo
