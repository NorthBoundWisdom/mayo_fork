/****************************************************************************
** Copyright (c) 2024, Fougue Ltd. <https://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include <string_view>

#include "base/text_id.h"

namespace Mayo
{

// Function called by the Application i18n system, see
// Application::addTranslator()
std::string_view qtAppTranslate(const TextId &text, int n);

} // namespace Mayo
