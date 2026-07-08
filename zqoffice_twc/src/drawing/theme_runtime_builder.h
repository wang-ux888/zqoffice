// =============================================================================
// src/drawing/theme_runtime_builder.h
//
// ThemeRuntimeBuilder 类：运行时主题构建器
//
//   从 OOXML 主题 XML 或二进制 OfficeArtTheme 记录构建 Theme 对象。
//
//   BuildFromOOXML：解析 ppt/theme/theme1.xml 内容
//     - a:clrScheme：12 个颜色槽位
//     - a:fontScheme：majorFont/minorFont
//     - a:fmtScheme：fillStyleLst/lnStyleLst/effectStyleLst/bgFillStyleLst
//
//   BuildFromBinary：解析旧版 .ppt/.xls 中的 OfficeArtTheme 记录
//     [MS-ODRAW] section 2.5.4 OfficeArtHTTPHeader
//     旧版格式颜色方案位于 RecordType::kColorMRU 中
// =============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "drawing/theme.h"

namespace zq {
namespace office {

// 前向声明（避免暴露 ooxml::XmlNode 头文件）
namespace ooxml {
class XmlNode;
}

namespace drawing {

/// 主题运行时构建器
///
class ThemeRuntimeBuilder {
public:
    ThemeRuntimeBuilder();
    ~ThemeRuntimeBuilder();

    // -----------------------------------------------------------------------
    // 构建接口
    // -----------------------------------------------------------------------

    /// 从 OOXML 主题 XML 构建
    /// @param themeXml  ppt/theme/theme1.xml 的 XML 内容
    /// @return 构建成功返回 true
    bool buildFromOOXML(const std::string& themeXml);

    /// 从二进制 OfficeArtTheme 记录构建（旧版 .ppt/.xls）
    /// @param data  二进制数据指针
    /// @param size  数据字节数
    /// @return 构建成功返回 true
    bool buildFromBinary(const unsigned char* data, std::size_t size);

    // -----------------------------------------------------------------------
    // 访问结果
    // -----------------------------------------------------------------------

    /// 获取构建的主题对象
    const Theme& theme() const { return theme_; }
    Theme& theme() { return theme_; }

    /// 错误信息
    const std::string& error() const { return error_; }

    /// 是否构建成功
    bool isSuccess() const { return success_; }

private:
    Theme theme_;
    std::string error_;
    bool success_ = false;

    // -----------------------------------------------------------------------
    // OOXML 解析辅助
    // -----------------------------------------------------------------------

    /// 解析 a:theme 根元素
    bool parseThemeRoot_(const ooxml::XmlNode& root);

    /// 解析 a:clrScheme（颜色方案）
    bool parseColorScheme_(const ooxml::XmlNode& clrSchemeNode);

    /// 解析 a:fontScheme（字体方案）
    bool parseFontScheme_(const ooxml::XmlNode& fontSchemeNode);

    /// 解析 a:fmtScheme（格式方案）
    bool parseFmtScheme_(const ooxml::XmlNode& fmtSchemeNode);

    /// 从颜色节点提取 RGB 值
    /// 支持 a:srgbClr / a:sysClr / a:schemeClr / a:prstClr
    std::string extractColorRgb_(const ooxml::XmlNode& colorParent);

    /// 提取节点 localName（去掉命名空间前缀）
    static std::string localName_(const std::string& fullName);

    // -----------------------------------------------------------------------
    // 二进制解析辅助
    // -----------------------------------------------------------------------

    /// 解析 OfficeArtTheme 二进制记录
    bool parseBinaryTheme_(const unsigned char* data, std::size_t size);

    /// 从二进制颜色方案记录解析颜色
    bool parseBinaryColorScheme_(const unsigned char* data, std::size_t size);
};

} // namespace drawing
} // namespace office
} // namespace zq
