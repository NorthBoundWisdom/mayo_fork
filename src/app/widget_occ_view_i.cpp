/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/
#include "widget_occ_view_i.h"

#include "base/occ_handle.h"

namespace Mayo
{
inline IWidgetOccView::Creator &getWidgetOccViewCreator()
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
