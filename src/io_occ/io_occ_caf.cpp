/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "io_occ_caf.h"

#include <gsl/util>

#include <IGESCAFControl_Reader.hxx>
#include <IGESCAFControl_Writer.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <Transfer_TransientProcess.hxx>

#include "base/document.h"
#include "base/global.h"
#include "base/io_system.h"
#include "base/occ_progress_indicator.h"
#include "base/task_progress.h"
#include "base/tkernel_utils.h"

namespace Mayo::IO::Private
{

namespace
{

// NOTE
// Maybe STEP/IGES CAF ReadFile() can be run concurrently(they should)
// But concurrent calls to Transfer() to the same target Document must be
// serialized

template <typename CafReaderType>
bool cafGenericReadFile(CafReaderType &reader, const FilePath &filepath,
                        TaskProgress * /*progress*/)
{
    // readFile_prepare(reader);
    const IFSelect_ReturnStatus error = reader.ReadFile(filepath.u8string().c_str());
    return error == IFSelect_RetDone;
}

template <typename CafReaderType>
TDF_LabelSequence cafGenericReadTransfer(CafReaderType &reader, DocumentPtr doc,
                                         TaskProgress *progress)
{
    auto indicator = makeOccHandle<OccProgressIndicator>(progress);
    const TDF_LabelSequence seqMark = doc->xcaf().topLevelFreeShapes();
    OccHandle<TDocStd_Document> stdDoc = doc;
    const bool okTransfer = reader.Transfer(stdDoc, indicator->Start());

    MAYO_UNUSED(okTransfer);
    return doc->xcaf().diffTopLevelFreeShapes(seqMark);
}

template <typename CafWriterType>
bool cafGenericWriteTransfer(CafWriterType &writer, Span<const ApplicationItem> appItems,
                             TaskProgress *progress)
{
    auto indicator = makeOccHandle<OccProgressIndicator>(progress);

    bool okTransfer = true;
    System::visitUniqueItems(appItems,
                             [&](const ApplicationItem &item)
                             {
                                 if (!okTransfer)
                                     return; // Skip if already in error state

                                 bool okItemTransfer = false;
                                 if (item.isDocument())
                                     okItemTransfer = writer.Transfer(item.document());
                                 else if (item.isDocumentTreeNode())
                                     okItemTransfer =
                                         writer.Transfer(item.documentTreeNode().label());

                                 if (!okItemTransfer)
                                     okTransfer = false;
                             });

    return okTransfer;
}

} // namespace

std::mutex &cafGlobalMutex()
{
    static std::mutex mutex;
    return mutex;
}

OccHandle<XSControl_WorkSession> cafWorkSession(const STEPCAFControl_Reader &reader)
{
    return reader.Reader().WS();
}

OccHandle<XSControl_WorkSession> cafWorkSession(const IGESCAFControl_Reader &reader)
{
    return reader.WS();
}

bool cafReadFile(IGESCAFControl_Reader &reader, const FilePath &filepath, TaskProgress *progress)
{
    return cafGenericReadFile(reader, filepath, progress);
}

bool cafReadFile(STEPCAFControl_Reader &reader, const FilePath &filepath, TaskProgress *progress)
{
    return cafGenericReadFile(reader, filepath, progress);
}

TDF_LabelSequence cafTransfer(IGESCAFControl_Reader &reader, DocumentPtr doc,
                              TaskProgress *progress)
{
    return cafGenericReadTransfer(reader, doc, progress);
}

TDF_LabelSequence cafTransfer(STEPCAFControl_Reader &reader, DocumentPtr doc,
                              TaskProgress *progress)
{
    return cafGenericReadTransfer(reader, doc, progress);
}

OccHandle<Transfer_FinderProcess> cafFinderProcess(const IGESCAFControl_Writer &writer)
{
    return writer.TransferProcess();
}

OccHandle<Transfer_FinderProcess> cafFinderProcess(const STEPCAFControl_Writer &writer)
{
    return writer.Writer().WS()->TransferWriter()->FinderProcess();
}

bool cafTransfer(IGESCAFControl_Writer &writer, Span<const ApplicationItem> appItems,
                 TaskProgress *progress)
{
    return cafGenericWriteTransfer(writer, appItems, progress);
}

bool cafTransfer(STEPCAFControl_Writer &writer, Span<const ApplicationItem> appItems,
                 TaskProgress *progress)
{
    return cafGenericWriteTransfer(writer, appItems, progress);
}

} // namespace Mayo::IO::Private
