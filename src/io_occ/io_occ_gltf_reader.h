/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include <RWGltf_CafReader.hxx>

#include "io_occ_base_mesh.h"

namespace Mayo::IO
{

// OpenCascade-based reader for glTF format
// Requires OpenCascade >= v7.4.0
class OccGltfReader : public OccBaseMeshReader
{
public:
    OccGltfReader();

    static std::unique_ptr<PropertyGroup> createProperties(PropertyGroup *parentGroup);
    void applyProperties(const PropertyGroup *params) override;

    // Parameters

    struct Parameters : public OccBaseMeshReader::Parameters
    {
        bool skipEmptyNodes = true;
        bool useMeshNameAsFallback = true;
    };
    OccGltfReader::Parameters &parameters() override
    {
        return m_params;
    }
    const OccGltfReader::Parameters &constParameters() const override
    {
        return m_params;
    }

protected:
    void applyParameters() override;

private:
    class Properties;
    Parameters m_params;
    RWGltf_CafReader m_reader;
};

} // namespace Mayo::IO
