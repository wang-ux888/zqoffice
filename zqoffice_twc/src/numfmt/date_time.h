// =============================================================================
// src/numfmt/date_time.h
//
// zq::office::numfmt::DateTime 结构体
//   由 DateUtil::ConvertDateTime(double serial) 返回，表示 Excel 序列号对应的
//   年/月/日/时/分/秒。
//
// Excel 日期系统：1900 日期系统（默认）
//   - 序列号 1 = 1900-01-01
//   - 序列号 60 = 1900-02-29（不存在的日期，Excel 的 1900 闰年 bug）
//   - 序列号 61 = 1900-03-01
// =============================================================================
#pragma once

#include <cstdint>
#include <string>

namespace zq {
namespace office {
namespace numfmt {

/// Excel 序列号转换后的日期时间结构
struct DateTime {
    int year;     ///< 年 [1900, 9999]
    int month;    ///< 月 [1, 12]
    int day;      ///< 日 [1, 31]
    int hour;     ///< 时 [0, 23]
    int minute;   ///< 分 [0, 59]
    int second;   ///< 秒 [0, 59]

    /// 默认构造：1900-01-01 00:00:00
    DateTime()
        : year(1900), month(1), day(1), hour(0), minute(0), second(0) {}

    /// 全参数构造
    DateTime(int y, int mo, int d, int h, int mi, int s)
        : year(y), month(mo), day(d), hour(h), minute(mi), second(s) {}

    /// 相等比较
    bool operator==(const DateTime& other) const {
        return year == other.year && month == other.month && day == other.day &&
               hour == other.hour && minute == other.minute && second == other.second;
    }

    bool operator!=(const DateTime& other) const {
        return !(*this == other);
    }

    /// 转换为 ISO 8601 字符串（YYYY-MM-DD HH:MM:SS）
    std::string toString() const;
};

}  // namespace numfmt
}  // namespace office
}  // namespace zq
