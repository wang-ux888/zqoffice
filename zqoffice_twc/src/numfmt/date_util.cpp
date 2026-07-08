// =============================================================================
// src/numfmt/date_util.cpp
//
// DateUtil 类实现
//
// 参考规范：
//   - [MS-OE376] 2.2.1: Excel 内置日期格式索引
//   - Excel 1900 日期系统（serial 1 = 1900-01-01，含 1900 闰年 bug）
//   - Howard Hinnant 的 "days_from_civil" 算法（公历↔序数日双向转换）
// =============================================================================
#include "numfmt/date_util.h"

#include <cmath>
#include <cstring>

namespace zq {
namespace office {
namespace numfmt {

// ---------------------------------------------------------------------------
// 私有静态正则初始化
// 这些正则用于识别 Excel 格式串中的日期/时间标记：
//   y/Y = 年, m/M = 月（在年/日附近时）或分（在时/秒附近时）, d/D = 日
//   h/H = 时, s/S = 秒
// ---------------------------------------------------------------------------

// 正则 1：年-月-日 顺序（含分隔符或文字）
// 匹配：yyyy/mm/dd, yyyy-mm-dd, yyyy年m月d日, yy.mm.dd
// 逻辑：先出现 y+，后出现 m+，再出现 d+（中间可有任意非日期标记字符）
const std::regex DateUtil::date_ptrn1(
    "y+[^dmyhms]*m+[^dmyhms]*d+",
    std::regex::icase);

// 正则 2：月-日-年 或 日-月-年 顺序
// 匹配：mm/dd/yyyy, dd-mm-yyyy, m/d/yy
// 逻辑：先出现 m+，后出现 d+，再出现 y+（或先 d 后 m 后 y）
const std::regex DateUtil::date_ptrn2(
    "(m+[^dmyhms]*d+[^dmyhms]*y+|d+[^dmyhms]*m+[^dmyhms]*y+)",
    std::regex::icase);

// 正则 3a：月-日（无年）
// 匹配：mm/dd, m-d, m月d日
const std::regex DateUtil::date_ptrn3a(
    "m+[^dmyhms]*d+",
    std::regex::icase);

// 正则 3b：日-月（无年）
// 匹配：dd/mm, d-m, d日m月
const std::regex DateUtil::date_ptrn3b(
    "d+[^dmyhms]*m+",
    std::regex::icase);

// 正则 4：仅日（无月年）
// 匹配：dd, d
// 注意：需排除被 m 包围的情况（由 3a/3b 优先匹配）
const std::regex DateUtil::date_ptrn4(
    "d+",
    std::regex::icase);

// 正则 5：时间格式（时-分-秒 或 时-分）
// 匹配：h:mm:ss, hh:mm, h时m分s秒
const std::regex DateUtil::date_ptrn5(
    "h+[^dmyhms]*m+([^dmyhms]*s+)?",
    std::regex::icase);

// ---------------------------------------------------------------------------
// 公开静态方法
// ---------------------------------------------------------------------------

DateTime DateUtil::ConvertDateTime(double serial) {
    DateTime dt;

    // 处理负数或零：返回 epoch
    if (serial < 1.0) {
        // 序列号 < 1 表示只有时间（0 = 1900-01-01 00:00:00）
        // 时间部分仍需解析
    }

    // 提取整数部分（天数）和小数部分（时间分数）
    double intPart;
    double fraction = std::modf(serial, &intPart);
    int days = static_cast<int>(intPart);

    // 1900 闰年 bug 处理：
    // Excel 错误地认为 1900 是闰年，在 serial 60 插入了不存在的 1900-02-29
    // 真实日历中 1900-02-28 之后直接是 1900-03-01（无 Feb 29）
    // 转换规则：
    //   serial 1-59：真实天数 = serial - 1（1900-01-01 ~ 1900-02-28 正确）
    //   serial 60：伪日期，按 1900-02-28 处理（同 serial 59）
    //   serial >= 61：真实天数 = serial - 2（减去伪 Feb 29 那一天）
    int realDaysFromEpoch;
    if (days >= 61) {
        realDaysFromEpoch = days - 2;  // 补偿 1900 闰年 bug
    } else if (days == 60) {
        realDaysFromEpoch = 58;  // 伪 Feb 29 → Feb 28（同 serial 59）
    } else {
        realDaysFromEpoch = days - 1;
    }

    // 1900-01-01 距 1970-01-01 = -25567 天
    // （70 年 × 365 天 + 17 个闰年(1904-1968) = 25550 + 17 = 25567）
    long daysFrom1970 = static_cast<long>(realDaysFromEpoch) - 25567L;

    // Howard Hinnant 的 civil_from_days 算法
    // 输入：自 1970-01-01 起的天数
    // 输出：年/月/日
    long z = daysFrom1970 + 719468L;
    long era = (z >= 0 ? z : z - 146096L) / 146097L;
    unsigned doe = static_cast<unsigned>(z - era * 146097L);  // [0, 146096]
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;  // [0, 399]
    long y = static_cast<long>(yoe) + era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);  // [0, 365]
    unsigned mp = (5 * doy + 2) / 153;  // [0, 11]
    unsigned d = doy - (153 * mp + 2) / 5 + 1;  // [1, 31]
    unsigned m = mp < 10 ? mp + 3 : mp - 9;  // [1, 12]
    if (m <= 2) y += 1;  // 调整年

    dt.year = static_cast<int>(y);
    dt.month = static_cast<int>(m);
    dt.day = static_cast<int>(d);

    // 解析时间部分（小数部分 → 时/分/秒）
    // fraction ∈ [0, 1)，对应一天 86400 秒
    double totalSeconds = fraction * 86400.0;
    // 四舍五入到整数秒
    int totalSec = static_cast<int>(totalSeconds + 0.5);
    if (totalSec >= 86400) {
        // 进位到下一天
        totalSec -= 86400;
        // 注意：此处简化处理，不自动进位日期
    }
    dt.hour = totalSec / 3600;
    dt.minute = (totalSec % 3600) / 60;
    dt.second = totalSec % 60;

    return dt;
}

bool DateUtil::isInternalDateFormat(int formatIndex) {
    // Excel 内置日期/时间格式索引（ECMA-376 Part 1 §18.8.30 numFmt）
    // 14: m/d/yyyy
    // 15: d-mmm-yy
    // 16: d-mmm
    // 17: mmm-yy
    // 18: h:mm AM/PM
    // 19: h:mm:ss AM/PM
    // 20: h:mm
    // 21: h:mm:ss
    // 22: m/d/yyyy h:mm
    // 27: yyyy"年"m"月"  (中文/日文)
    // 28: m"月"d"日"      (中文/日文)
    // 29: m"月"d"日"      (中文)
    // 30: m-d-yy           (中文)
    // 31: yyyy"年"m"月"d"日" (中文)
    // 32: h"时"mm"分"      (中文)
    // 33: h"时"mm"分"ss"秒" (中文)
    // 34: 上午/下午 h"时"mm"分" (中文)
    // 35: 上午/下午 h"时"mm"分"ss"秒" (中文)
    // 36: yyyy"年"m"月"   (中文)
    // 45: mm:ss
    // 46: [h]:mm:ss
    // 47: mm:ss.0
    // 50: yyyy"年"m"月"   (中文)
    // 51: m"月"d"日"      (中文)
    // 52: yyyy"年"m"月    (日文)
    // 53: m"月"d"日"      (日文)
    // 54: m"月"d"日"      (韩文)
    // 55: yyyy"年"m"月"d"日" (韩文)
    // 56: yyyy"年"m"月    (韩文)
    // 57: m"月"d"日       (韩文)
    // 58: m"月"d"日       (中文)
    switch (formatIndex) {
        case 14: case 15: case 16: case 17: case 18: case 19:
        case 20: case 21: case 22:
        case 27: case 28: case 29: case 30: case 31: case 32:
        case 33: case 34: case 35: case 36:
        case 45: case 46: case 47:
        case 50: case 51: case 52: case 53: case 54: case 55:
        case 56: case 57: case 58:
            return true;
        default:
            return false;
    }
}

bool DateUtil::isADateFormat(int formatIndex, const std::string& formatStr) {
    // 1. 先检查内置格式索引
    if (formatIndex > 0 && isInternalDateFormat(formatIndex)) {
        return true;
    }

    // 2. 检查格式串
    if (formatStr.empty()) {
        return false;
    }

    // 清理格式串：移除引号内容、转义字符、颜色/条件段
    std::string cleaned = cleanFormatString(formatStr);
    if (cleaned.empty()) {
        return false;
    }

    // 3. 依次匹配 6 个日期/时间正则
    // 顺序很重要：先匹配复合模式（含年），再匹配简单模式
    // date_ptrn1（年-月-日）和 date_ptrn2（月-日-年/日-月-年）含年，最特定
    // date_ptrn3a/3b（月-日/日-月）次之
    // date_ptrn4（仅日）最宽泛，放最后
    // date_ptrn5（时间）独立
    if (std::regex_search(cleaned, date_ptrn1)) return true;
    if (std::regex_search(cleaned, date_ptrn2)) return true;
    if (std::regex_search(cleaned, date_ptrn5)) return true;
    if (std::regex_search(cleaned, date_ptrn3a)) return true;
    if (std::regex_search(cleaned, date_ptrn3b)) return true;
    if (std::regex_search(cleaned, date_ptrn4)) return true;

    return false;
}

// ---------------------------------------------------------------------------
// 私有静态方法
// ---------------------------------------------------------------------------

std::string DateUtil::cleanFormatString(const std::string& fmt) {
    // 清理规则：
    // 1. 移除 [颜色] 段：如 [Red]、[Green]
    // 2. 移除 [条件] 段：如 [>100]、[<=0]
    // 3. 移除引号内容：如 "年"、"文本"
    // 4. 移除转义字符：如 \-、\:
    // 5. 移除 AM/PM 标记
    // 6. 只保留日期/时间相关字符（y m d h s 及分隔符）

    std::string result;
    result.reserve(fmt.size());

    size_t i = 0;
    while (i < fmt.size()) {
        char c = fmt[i];

        // 跳过 [..] 段（颜色/条件/区域设置）
        if (c == '[') {
            // 查找匹配的 ]
            size_t end = fmt.find(']', i);
            if (end != std::string::npos) {
                // 但 [$-404] 等区域设置段需保留（不影响日期判断）
                // 简单处理：直接跳过所有 [..] 段
                i = end + 1;
                continue;
            }
        }

        // 跳过引号内容 "..."
        if (c == '"') {
            size_t end = fmt.find('"', i + 1);
            if (end != std::string::npos) {
                i = end + 1;
                continue;
            }
        }

        // 跳过转义字符 \x
        if (c == '\\') {
            i += 2;
            continue;
        }

        // 跳过 AM/PM 或 A/P 标记（不区分大小写）
        if (i + 4 <= fmt.size()) {
            if ((fmt[i] == 'A' || fmt[i] == 'a') &&
                (fmt[i + 1] == 'M' || fmt[i + 1] == 'm') &&
                fmt[i + 2] == '/') {
                // AM/PM
                if (i + 5 <= fmt.size() &&
                    (fmt[i + 3] == 'P' || fmt[i + 3] == 'p') &&
                    (fmt[i + 4] == 'M' || fmt[i + 4] == 'm')) {
                    i += 5;
                    continue;
                }
            }
            if ((fmt[i] == 'P' || fmt[i] == 'p') &&
                (fmt[i + 1] == 'M' || fmt[i + 1] == 'm') &&
                fmt[i + 2] == '/') {
                if (i + 5 <= fmt.size() &&
                    (fmt[i + 3] == 'A' || fmt[i + 3] == 'a') &&
                    (fmt[i + 4] == 'M' || fmt[i + 4] == 'm')) {
                    i += 5;
                    continue;
                }
            }
        }
        if (i + 2 <= fmt.size()) {
            if ((fmt[i] == 'A' || fmt[i] == 'a') &&
                (fmt[i + 1] == 'M' || fmt[i + 1] == 'm')) {
                i += 2;
                continue;
            }
            if ((fmt[i] == 'P' || fmt[i] == 'p') &&
                (fmt[i + 1] == 'M' || fmt[i + 1] == 'm')) {
                i += 2;
                continue;
            }
            if (fmt[i] == 'A' || fmt[i] == 'a') {
                if (i + 1 < fmt.size() && fmt[i + 1] == '/') {
                    if (i + 2 < fmt.size() && (fmt[i + 2] == 'P' || fmt[i + 2] == 'p')) {
                        i += 3;
                        continue;
                    }
                }
            }
        }

        // 保留日期/时间相关字符
        // y/Y = 年, m/M = 月或分, d/D = 日, h/H = 时, s/S = 秒
        // 分隔符 / - . : 空格 也保留（用于正则匹配的间距）
        if (c == 'y' || c == 'Y' || c == 'm' || c == 'M' ||
            c == 'd' || c == 'D' || c == 'h' || c == 'H' ||
            c == 's' || c == 'S') {
            result.push_back(c);
        } else if (c == '/' || c == '-' || c == '.' || c == ':' || c == ' ') {
            result.push_back(c);
        }
        // 其他字符（如 #, 0, ?, $, %）跳过 — 它们是数字/货币格式

        ++i;
    }

    return result;
}

}  // namespace numfmt
}  // namespace office
}  // namespace zq
