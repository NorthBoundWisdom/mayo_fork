/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include <Message_ProgressIndicator.hxx>

#include "tkernel_utils.h"

namespace Mayo
{

class TaskProgress;

// Provides implementation of OpenCascade-based progress indicator around
// Mayo::TaskProgress
class OccProgressIndicator : public Message_ProgressIndicator
{
public:
    OccProgressIndicator(TaskProgress *progress);

    bool UserBreak() override;

    void Show(const Message_ProgressScope &theScope, const bool isForce) override;

private:
    TaskProgress *m_progress = nullptr;
    const char *m_lastStepName = nullptr;
    int m_lastProgress = -1;
};

} // namespace Mayo
