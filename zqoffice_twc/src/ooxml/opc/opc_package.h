// =============================================================================
// src/ooxml/opc/opc_package.h
//
// OpcPackage：OOXML 关系包读取器
//
//   OOXML 文件（xlsx/pptx/docx）本质是 ZIP 压缩包，遵循 OPC（Open Packaging
//   Conventions，ECMA-376 第 II 部分）规范：
//     - 每个 part 是 ZIP 中的一个文件（如 xl/workbook.xml）
//     - 每个 part 可有关系（_rels/*.rels）
//     - 顶层关系文件为 _rels/.rels，指向文档入口
//
//   关系文件结构：
//     <Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
//       <Relationship Id="rId1"
//                      Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
//                      Target="xl/workbook.xml"/>
//     </Relationships>
//
//   常见关系类型（Type）：
//     .../officeDocument          - 文档入口（workbook.xml/presentation.xml/document.xml）
//     .../worksheet               - 工作表（xlsx）
//     .../sharedStrings           - 共享字符串表
//     .../styles                  - 样式表
//     .../theme                   - 主题
//     .../slide                   - 幻灯片（pptx）
//     .../slideMaster             - 幻灯片母版
//     .../slideLayout             - 幻灯片版式
//     .../image                   - 图片
//     .../thumbnail               - 缩略图
//
//   OpcPackage 提供：
//     - openFromMemory / openFromFile：加载 ZIP 容器
//     - getPart：按路径获取 part 字节
//     - getRelationships：获取指定 part 的关系（_rels/<partname>.rels）
//     - getRootRelationships：获取顶层关系（_rels/.rels）
//     - resolveByType：按关系类型查找 part 路径
//     - parts：列出所有 part 路径
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../zip_reader.h"

namespace zq {
namespace office {
namespace ooxml {
namespace opc {

/// 关系项（<Relationship>）
struct Relationship {
    std::string id;          // Id 属性（如 "rId1"）
    std::string type;        // Type 属性（完整 URI）
    std::string target;      // Target 属性（相对路径，如 "xl/workbook.xml"）
    std::string targetMode;  // TargetMode 属性（"External" 或空）

    /// 是否为外部关系
    bool isExternal() const { return targetMode == "External"; }

    /// 获取 Type 的最后一段（如 "officeDocument" / "worksheet" / "slide"）
    std::string typeShortName() const;
};

/// OPC 关系包
class OpcPackage {
public:
    OpcPackage() = default;
    ~OpcPackage() = default;

    OpcPackage(const OpcPackage&) = delete;
    OpcPackage& operator=(const OpcPackage&) = delete;

    // -----------------------------------------------------------------------
    // 打开 / 关闭
    // -----------------------------------------------------------------------

    /// 从内存数据加载（OOXML ZIP 字节流）
    bool openFromMemory(const std::vector<unsigned char>& data);

    /// 从内存数据加载（指针 + 大小）
    bool openFromMemory(const unsigned char* data, std::size_t size);

    /// 从文件加载
    bool openFromFile(const std::string& filePath);

    /// 关闭释放资源
    void close();

    /// 是否已打开
    bool isOpen() const;

    // -----------------------------------------------------------------------
    // Part 访问
    // -----------------------------------------------------------------------

    /// 获取指定路径 part 的字节内容
    /// @param partPath  part 路径（如 "xl/workbook.xml" 或 "/xl/workbook.xml"）
    /// @return 字节内容（失败返回空 vector）
    std::vector<unsigned char> getPart(const std::string& partPath) const;

    /// 获取指定路径 part 的文本内容（UTF-8）
    std::string getPartText(const std::string& partPath) const;

    /// 是否包含指定 part
    bool hasPart(const std::string& partPath) const;

    /// 列出所有 part 路径
    std::vector<std::string> parts() const;

    // -----------------------------------------------------------------------
    // 关系访问
    // -----------------------------------------------------------------------

    /// 获取顶层关系（_rels/.rels）
    std::vector<Relationship> getRootRelationships() const;

    /// 获取指定 part 的关系
    /// @param partPath  part 路径（如 "xl/workbook.xml"）
    /// @return 关系列表（若该 part 无关系文件，返回空）
    std::vector<Relationship> getRelationships(
        const std::string& partPath) const;

    /// 按关系类型查找 part 路径（在顶层关系中）
    /// @param relTypeShort  关系类型短名（如 "officeDocument" / "thumbnail"）
    /// @return 第一个匹配的 part 路径，未找到返回空字符串
    std::string resolveByType(const std::string& relTypeShort) const;

    /// 按 Id 查找 part 路径（在顶层关系中）
    /// @param relId  关系 Id（如 "rId1"）
    /// @return part 路径，未找到返回空字符串
    std::string resolveById(const std::string& relId) const;

    // -----------------------------------------------------------------------
    // 错误信息
    // -----------------------------------------------------------------------

    const std::string& error() const { return error_; }

private:
    // -----------------------------------------------------------------------
    // 内部工具
    // -----------------------------------------------------------------------

    /// 解析关系文件 XML，提取 Relationship 列表
    /// @param relsXml  关系文件 XML 内容
    /// @return 关系列表
    std::vector<Relationship> parseRelationships_(
        const std::string& relsXml) const;

    /// 计算 part 的关系文件路径
    /// 例如 partPath="xl/workbook.xml"，关系文件为 "xl/_rels/workbook.xml.rels"
    std::string relsPathForPart_(const std::string& partPath) const;

    /// 规范化 part 路径（去除前导 /，去除 ./ 前缀）
    static std::string normalizePath_(const std::string& path);

    /// 根据关系 base 路径 + target 解析最终 part 路径
    /// 例如 base="xl/workbook.xml"，target="worksheets/sheet1.xml"
    ///   → "xl/worksheets/sheet1.xml"
    static std::string resolveTarget_(const std::string& base,
                                        const std::string& target);

    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    ZipReader zip_;
    mutable std::string error_;
};

} // namespace opc
} // namespace ooxml
} // namespace office
} // namespace zq
