/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include <AIS_InteractiveObject.hxx>
#include <SelectMgr_EntityOwner.hxx>

#include "base/occ_handle.h"


namespace Mayo
{
using GraphicsOwnerPtr = OccHandle<SelectMgr_EntityOwner>;
using GraphicsObjectPtr = OccHandle<AIS_InteractiveObject>;

} // namespace Mayo
