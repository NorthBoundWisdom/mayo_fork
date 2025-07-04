/****************************************************************************
** Copyright (c) 2023, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include <AIS_PointCloud.hxx>

#include "base/caf_utils.h"
#include "base/label_data.h"
#include "base/point_cloud_data.h"

#include "graphics_object_driver_point_cloud.h"


namespace Mayo
{

GraphicsPointCloudObjectDriver::GraphicsPointCloudObjectDriver()
{
}

GraphicsPointCloudObjectDriver::Support
GraphicsPointCloudObjectDriver::supportStatus(const TDF_Label &label) const
{
    return pointCloudSupportStatus(label);
}

GraphicsObjectPtr GraphicsPointCloudObjectDriver::createObject(const TDF_Label &label) const
{
    if (findLabelDataFlags(label) & LabelData_HasPointCloudData)
    {
        auto attrPointCloudData = CafUtils::findAttribute<PointCloudData>(label);
        auto object = new AIS_PointCloud;
        object->SetPoints(attrPointCloudData->points());
        object->SetOwner(this);
        return object;
    }

    return {};
}

void GraphicsPointCloudObjectDriver::applyDisplayMode(GraphicsObjectPtr object,
                                                      Enumeration::Value /*mode*/) const
{
    this->throwIf_differentDriver(object);
}

Enumeration::Value
GraphicsPointCloudObjectDriver::currentDisplayMode(const GraphicsObjectPtr &object) const
{
    this->throwIf_differentDriver(object);
    return object->DisplayMode();
}

std::unique_ptr<PropertyGroupSignals>
GraphicsPointCloudObjectDriver::properties(Span<const GraphicsObjectPtr> spanObject) const
{
    this->throwIf_differentDriver(spanObject);
    return {};
}

GraphicsPointCloudObjectDriver::Support
GraphicsPointCloudObjectDriver::pointCloudSupportStatus(const TDF_Label &label)
{
    const LabelDataFlags flags = findLabelDataFlags(label);
    if (flags & LabelData_HasPointCloudData)
        return GraphicsObjectDriver::Support::Complete;
    else
        return GraphicsObjectDriver::Support::None;
}

} // namespace Mayo
