// =============================================================================
// src/numfmt/date_util.h
//
// zq::office::numfmt::DateUtil 类（全 static）
//   - static DateTime ConvertDateTime(double serial)
//   - static bool isADateFormat(int formatIndex, const std::string& formatStr)
//   - static bool isInternalDateFormat(int formatIndex)
//   - private static const std::regex date_ptrn1 ~ date_ptrn5
//
// 参考规范：
//   - [MS-OE376] 2.2.1: Excel 内置日期格式索引
//   - ECMA-376 Part 1 §18.8.30: numFmt (number format)
//   - Excel 1900 日期系统（serial 1 = 1900-01-01，含 1900 闰年 bug）
// =============================================================================
#pragma once

#include <regex>
#include <string>

#include "numfmt/date_time.h"

namespace zq {
namespace office {
namespace numfmt {

/// 日期/数字格式辅助工具（全 static）
class DateUtil {
public:
    /// Excel 序列号 → DateTime
    /// @param serial  Excel 1900 日期系统的序列号（1 = 1900-01-01）
    /// @return 对应的 DateTime
    /// @note 自动处理 1900 闰年 bug（serial 60 = 不存在的 1900-02-29）
    static DateTime ConvertDateTime(double serial);

    /// 判断格式串是否为日期/时间格式
    /// @param formatIndex  内置格式索引（0 表示自定义格式，仅看 formatStr）
    /// @param formatStr    格式串（如 "yyyy/mm/dd"、"h:mm:ss"）
    /// @return true 表示是日期/时间格式
    static bool isADateFormat(int formatIndex, const std::string& formatStr);

    /// 判断内置格式索引是否为日期/时间格式
    /// @param formatIndex  内置格式索引（14-22, 27-36, 45-47, 50-58 等）
    /// @return true 表示是日期/时间格式
    static bool isInternalDateFormat(int formatIndex);

    // 禁止构造（全 static 工具类）
    DateUtil() = delete;
    DateUtil(const DateUtil&) = delete;
    DateUtil& operator=(const DateUtil&) = delete;

private:
    /// 日期格式正则 1：年-月-日 顺序（如 yyyy/mm/dd、yyyy-mm-dd、yyyy年m月d日）
    static const std::regex date_ptrn1;
    /// 日期格式正则 2：月-日-年 顺序（如 mm/dd/yyyy、dd-mm-yyyy）
    static const std::regex date_ptrn2;
    /// 日期格式正则 3a：月-日（无年，如 mm/dd、m月d日）
    static const std::regex date_ptrn3a;
    /// 日期格式正则 3b：日-月（无年，如 dd/mm、d日m月）
    static const std::regex date_ptrn3b;
    /// 日期格式正则 4：仅日（如 dd、d）
    static const std::regex date_ptrn4;
    /// 日期格式正则 5：时间格式（如 h:mm:ss、hh:mm）
    static const std::regex date_ptrn5;

    /// 清理格式串：移除引号内容、转义字符、颜色段、条件段
    /// 使 regex 匹配更准确
    static std::string cleanFormatString(const std::string& fmt);
};

}  // namespace numfmt
}  // namespace office
}  // namespace zq
