// =============================================================================
// src/drawing/chart_convert.h
//
// ChartConvert 类：图表转换器
//
//   从 OOXML a:graphicFrame/a:graphic/a:graphicData/c:chart 转换为图表模型
// =============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "drawing/enums/theme_type.h"

namespace zq {
namespace office {

namespace ooxml {
class XmlNode;
}

namespace drawing {

class Theme;

/// 图表数据系列
struct ChartSeries {
    std::string title;          // 系列标题
    std::vector<double> values; // 系列数值
    bool valid = false;
};

/// 图表数据类别
struct ChartCategory {
    std::string label;          // 类别标签
    bool valid = false;
};

/// 图表类型
enum class ChartType {
    kUnknown = 0,
    kBar,           // 条形图
    kColumn,        // 柱状图
    kLine,          // 折线图
    kPie,           // 饼图
    kArea,          // 面积图
    kScatter,       // 散点图
    kDoughnut,      // 圆环图
    kRadar,         // 雷达图
    kSurface,       // 曲面图
    kBubble,        // 气泡图
};

/// 图表转换器
///
class ChartConvert {
public:
    ChartConvert();
    ~ChartConvert();

    // -----------------------------------------------------------------------
    // 位置与尺寸（EMU 单位）
    // -----------------------------------------------------------------------

    void SetTop(std::int64_t top) { top_ = top; }
    void SetLeft(std::int64_t left) { left_ = left; }
    void SetWidth(std::int64_t width) { width_ = width; }
    void SetHeight(std::int64_t height) { height_ = height; }

    std::int64_t top() const { return top_; }
    std::int64_t left() const { return left_; }
    std::int64_t width() const { return width_; }
    std::int64_t height() const { return height_; }

    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    /// 设置数值列表
    void SetValLst(const std::vector<double>& vals) { valLst_ = vals; }
    const std::vector<double>& valLst() const { return valLst_; }

    /// 设置主题节点
    void SetThemeNode(std::shared_ptr<Theme> theme) { theme_ = theme; }
    std::shared_ptr<Theme> themeNode() const { return theme_; }

    /// 添加系列
    void addSeries(const ChartSeries& s) { series_.push_back(s); }
    const std::vector<ChartSeries>& series() const { return series_; }

    /// 添加类别
    void addCategory(const ChartCategory& c) { categories_.push_back(c); }
    const std::vector<ChartCategory>& categories() const { return categories_; }

    /// 图表类型
    void setChartType(ChartType t) { chartType_ = t; }
    ChartType chartType() const { return chartType_; }

    // -----------------------------------------------------------------------
    // OOXML 转换
    // -----------------------------------------------------------------------

    /// 从 OOXML c:chart 元素转换
    /// @param chartXml  chartN.xml 内容
    /// @return 转换成功返回 true
    bool convertFromOOXML(const std::string& chartXml);

    // -----------------------------------------------------------------------
    // 状态
    // -----------------------------------------------------------------------

    bool isValid() const { return chartType_ != ChartType::kUnknown; }
    const std::string& error() const { return error_; }
    bool isSuccess() const { return success_; }

    void clear();

private:
    std::int64_t top_ = 0, left_ = 0, width_ = 0, height_ = 0;
    std::vector<double> valLst_;
    std::shared_ptr<Theme> theme_;
    std::vector<ChartSeries> series_;
    std::vector<ChartCategory> categories_;
    ChartType chartType_ = ChartType::kUnknown;
    std::string error_;
    bool success_ = false;

    /// 在图表类型元素（如 barChart）内部解析系列
    void parseSeriesInChartElement_(const ooxml::XmlNode& chartElementNode);

    static std::string localName_(const std::string& fullName);
};

} // namespace drawing
} // namespace office
} // namespace zq
