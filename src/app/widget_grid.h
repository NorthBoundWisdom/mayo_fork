/****************************************************************************
** Copyright (c) 2023, Fougue Ltd. <https://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include <QtWidgets/QWidget>

#include <V3d_View.hxx>

#include "graphics/graphics_scene.h"

class gp_Ax2;

namespace Mayo
{

// Widget panel dedicated to management of the grid in 3D view
class WidgetGrid : public QWidget
{
    Q_OBJECT

public:
    WidgetGrid(GraphicsScene *scene, OccHandle<V3d_View> view, QWidget *parent = nullptr);
    ~WidgetGrid() override;

signals:
    void sizeAdjustmentRequested();

private:
    enum class GridColorType
    {
        Base,
        Tenth
    };

    static Aspect_GridType toGridType(int comboBoxItemIndex);
    static Aspect_GridDrawMode toGridDrawMode(int comboBoxItemIndex);
    static const gp_Ax2 &toPlaneAxis(int comboBoxItemIndex);

    void activateGrid(bool on);
    void applyGridParams();
    void applyGridGraphicsParams();
    void chooseGridColor(GridColorType colorType);
    void enableGridColorTenth(bool on);

    class Ui_WidgetGrid *m_ui = nullptr;
    Quantity_Color m_gridColorTenth;
    GraphicsScene *m_scene = nullptr;
    OccHandle<V3d_View> m_view;
};

} // namespace Mayo
