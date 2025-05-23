/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include <Aspect_HatchStyle.hxx>
#include <Graphic3d_NameOfMaterial.hxx>

#include "property_enumeration.h"

namespace Mayo
{

class OcctEnums
{
public:
    static const Enumeration &Graphic3d_NameOfMaterial();
    static const Enumeration &Aspect_HatchStyle();
};

} // namespace Mayo
