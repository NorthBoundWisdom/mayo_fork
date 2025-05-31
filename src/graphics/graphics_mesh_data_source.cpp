/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "graphics_mesh_data_source.h"

#include <Precision.hxx>
#include <Standard_Type.hxx>
#include <TColStd_DataMapOfIntegerInteger.hxx>
#include <TColStd_DataMapOfIntegerReal.hxx>
#include <TColgp_SequenceOfXYZ.hxx>

#include "base/mesh_utils.h"

namespace Mayo
{

/**
 * @brief 构造函数，根据给定的多边形三角剖分网格创建数据源
 *
 * 该构造函数负责初始化网格数据源，处理以下步骤：
 * 1. 提取所有网格节点的坐标信息并存储
 * 2. 计算每个三角形面元的法向量
 * 3. 建立面元与节点间的拓扑关系
 *
 * 该数据源为MeshVS可视化提供基础数据支持，用于3D网格模型的显示和操作。
 *
 * @param mesh OpenCascade三角剖分网格对象
 */
GraphicsMeshDataSource::GraphicsMeshDataSource(const OccHandle<Poly_Triangulation> &mesh)
    : m_mesh(mesh)
{
    if (!m_mesh.IsNull())
    {
        // 获取网格节点数量
        const int lenCoords = m_mesh->NbNodes();
        // 创建存储节点坐标的二维数组（行：节点ID，列：XYZ坐标）
        m_nodeCoords = new TColStd_HArray2OfReal(1, lenCoords, 1, 3);

        // 遍历所有节点，提取并存储其坐标
        for (int i = 1; i <= lenCoords; ++i)
        {
            m_nodes.Add(i);                           // 将节点ID添加到节点集合中
            const gp_XYZ xyz = m_mesh->Node(i).XYZ(); // 获取节点坐标
            // 存储节点的X、Y、Z坐标
            m_nodeCoords->SetValue(i, 1, xyz.X());
            m_nodeCoords->SetValue(i, 2, xyz.Y());
            m_nodeCoords->SetValue(i, 3, xyz.Z());
        }

        // 获取三角形面元数组
        const Poly_Array1OfTriangle &aSeq = MeshUtils::triangles(m_mesh);
        const int lenTriangles = aSeq.Length(); // 获取三角形数量
        // 创建存储面元法向量的二维数组
        m_elemNormals = new TColStd_HArray2OfReal(1, lenTriangles, 1, 3);
        // 创建存储面元节点索引的二维数组
        m_elemNodes = new TColStd_HArray2OfInteger(1, lenTriangles, 1, 3);

        // 遍历所有三角形面元
        for (int i = 1; i <= lenTriangles; ++i)
        {
            m_elements.Add(i);                   // 将面元ID添加到面元集合中
            const Poly_Triangle &aTri = aSeq(i); // 获取当前三角形

            int V[3];                   // 存储三角形的三个顶点索引
            aTri.Get(V[0], V[1], V[2]); // 获取三角形的三个顶点索引

            // 计算三角形的两条边向量
            const gp_Vec aV1(m_mesh->Node(V[0]), m_mesh->Node(V[1]));
            const gp_Vec aV2(m_mesh->Node(V[1]), m_mesh->Node(V[2]));

            // 计算三角形法向量（两条边的叉积）
            gp_Vec aN = aV1.Crossed(aV2);
            // 如果法向量长度足够大，则归一化
            if (aN.SquareMagnitude() > Precision::SquareConfusion())
                aN.Normalize();
            else
                aN.SetCoord(0.0, 0.0, 0.0); // 否则设置为零向量

            // 存储面元的节点索引
            for (int j = 0; j < 3; ++j)
                m_elemNodes->SetValue(i, j + 1, V[j]);

            // 存储面元的法向量分量
            m_elemNormals->SetValue(i, 1, aN.X());
            m_elemNormals->SetValue(i, 2, aN.Y());
            m_elemNormals->SetValue(i, 3, aN.Z());
        }
    }
}

/**
 * @brief 获取指定实体（节点或面元）的几何信息
 *
 * 该方法根据ID获取网格中节点或面元的几何信息：
 * - 对于面元(IsElement=true)：返回构成该面元的三个节点坐标
 * - 对于节点(IsElement=false)：返回该节点的坐标
 *
 * 该方法是MeshVS可视化系统获取网格几何数据的核心接口，用于绘制网格的节点和面元。
 *
 * @param ID 实体ID（节点ID或面元ID）
 * @param IsElement 指示ID是否为面元ID，true表示面元，false表示节点
 * @param Coords 输出参数，用于存储坐标值
 * @param NbNodes 输出参数，返回节点数量
 * @param Type 输出参数，返回实体类型
 * @return bool 操作是否成功
 */
bool GraphicsMeshDataSource::GetGeom(const int ID, const bool IsElement,
                                     TColStd_Array1OfReal &Coords, int &NbNodes,
                                     MeshVS_EntityType &Type) const
{
    if (m_mesh.IsNull())
        return false; // 如果网格为空，返回失败

    if (IsElement) // 如果请求的是面元几何信息
    {
        if (ID >= 1 && ID <= m_elements.Extent()) // 检查ID是否有效
        {
            Type = MeshVS_ET_Face; // 设置实体类型为面
            NbNodes = 3;           // 三角形有3个节点

            // 遍历三角形的三个节点，提取它们的坐标
            for (int i = 1, k = 1; i <= 3; ++i)
            {
                const int IdxNode = m_elemNodes->Value(ID, i); // 获取节点索引
                // 提取节点的XYZ坐标并存储到Coords数组
                for (int j = 1; j <= 3; ++j, ++k)
                    Coords(k) = m_nodeCoords->Value(IdxNode, j);
            }

            return true; // 操作成功
        }

        return false; // ID无效，返回失败
    }
    else // 如果请求的是节点几何信息
    {
        if (ID >= 1 && ID <= m_nodes.Extent()) // 检查ID是否有效
        {
            Type = MeshVS_ET_Node; // 设置实体类型为节点
            NbNodes = 1;           // 单个节点

            // 提取节点的XYZ坐标
            Coords(1) = m_nodeCoords->Value(ID, 1); // X坐标
            Coords(2) = m_nodeCoords->Value(ID, 2); // Y坐标
            Coords(3) = m_nodeCoords->Value(ID, 3); // Z坐标
            return true;                            // 操作成功
        }

        return false; // ID无效，返回失败
    }
}

/**
 * @brief 获取指定实体的几何类型
 *
 * 该方法返回指定ID对应实体的几何类型。这是MeshVS可视化系统用来区分
 * 不同类型几何实体（如节点、面、边等）的基本方法。
 *
 * @param [in] ID 实体ID（未使用）
 * @param [in] IsElement 指示实体是否为面元
 * @param [out] Type 输出参数，返回实体类型
 * @return bool 总是返回true
 */
bool GraphicsMeshDataSource::GetGeomType(const int, const bool IsElement,
                                         MeshVS_EntityType &Type) const
{
    // 根据IsElement参数确定实体类型：
    // 如果IsElement为true，则为面元(Face)；否则为节点(Node)
    Type = IsElement ? MeshVS_ET_Face : MeshVS_ET_Node;
    return true; // 始终返回成功
}

/**
 * @brief 获取指定面元的节点ID列表
 *
 * 该方法返回指定三角形面元的三个节点ID。这是MeshVS可视化系统用来
 * 获取面元拓扑信息的重要方法，用于面元选择、高亮等操作。
 *
 * @param [in] ID 面元ID
 * @param [out] theNodeIDs 输出参数，用于存储节点ID
 * @param [out] theNbNodes 输出参数，节点数量（未使用）
 * @return bool 操作是否成功
 */
bool GraphicsMeshDataSource::GetNodesByElement(const int ID, TColStd_Array1OfInteger &theNodeIDs,
                                               int & /*theNbNodes*/) const
{
    if (m_mesh.IsNull())
        return false; // 如果网格为空，返回失败

    // 检查ID是否有效，以及输出数组是否足够大
    if (ID >= 1 && ID <= m_elements.Extent() && theNodeIDs.Length() >= 3)
    {
        const int aLow = theNodeIDs.Lower(); // 获取数组的下界索引
        // 将面元的三个节点ID存储到输出数组
        theNodeIDs(aLow) = m_elemNodes->Value(ID, 1);     // 第一个节点
        theNodeIDs(aLow + 1) = m_elemNodes->Value(ID, 2); // 第二个节点
        theNodeIDs(aLow + 2) = m_elemNodes->Value(ID, 3); // 第三个节点
        return true;                                      // 操作成功
    }

    return false; // 参数无效，返回失败
}

/**
 * @brief 获取指定面元的法向量
 *
 * 该方法返回指定三角形面元的单位法向量。法向量对于正确渲染网格表面
 * （如光照计算）至关重要。
 *
 * @param [in] Id 面元ID
 * @param [in] Max 最大分量数（必须至少为3）
 * @param [out] nx 输出参数，法向量X分量
 * @param [out] ny 输出参数，法向量Y分量
 * @param [out] nz 输出参数，法向量Z分量
 * @return bool 操作是否成功
 */
bool GraphicsMeshDataSource::GetNormal(const int Id, const int Max, double &nx, double &ny,
                                       double &nz) const
{
    if (m_mesh.IsNull())
        return false; // 如果网格为空，返回失败

    // 检查ID和Max参数是否有效
    if (Id >= 1 && Id <= m_elements.Extent() && Max >= 3)
    {
        // 获取存储的法向量分量
        nx = m_elemNormals->Value(Id, 1); // X分量
        ny = m_elemNormals->Value(Id, 2); // Y分量
        nz = m_elemNormals->Value(Id, 3); // Z分量
        return true;                      // 操作成功
    }

    return false; // 参数无效，返回失败
}

} // namespace Mayo
