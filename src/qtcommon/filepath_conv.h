/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include <QtCore/QFileInfo>
#include <QtCore/QString>

#include "base/filepath_conv.h"

namespace Mayo
{

// Returns a FilePath object constructed from input
inline FilePath filepathFrom(const QByteArray &bytes)
{
    return filepathFrom(std::string_view{bytes.constData()});
}
inline FilePath filepathFrom(const QString &str)
{
    return reinterpret_cast<const char16_t *>(str.utf16());
}
inline FilePath filepathFrom(const QFileInfo &fi)
{
    return filepathFrom(fi.filePath());
}

// FilePath -> QByteArray
template <>
struct FilePathConv<QByteArray>
{
    static auto to(const FilePath &fp)
    {
        return QByteArray::fromStdString(fp.string());
    }
};

// FilePath -> QString
template <>
struct FilePathConv<QString>
{
    static auto to(const FilePath &fp)
    {
        return QString::fromStdString(fp.string());
    }
};

// FilePath -> QFileInfo
template <>
struct FilePathConv<QFileInfo>
{
    static auto to(const FilePath &fp)
    {
        return QFileInfo(FilePathConv<QString>::to(fp));
    }
};

} // namespace Mayo
