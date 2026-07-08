// =============================================================================
// src/ooxml/zip_writer.cpp
//
// ZipWriter 实现，基于 miniz 3.x mz_zip_writer_* API
// =============================================================================
#include "zip_writer.h"

#include <miniz.h>
#include <cstring>

namespace zq {
namespace office {
namespace ooxml {

// ---------------------------------------------------------------------------
// 构造 / 析构
// ---------------------------------------------------------------------------
ZipWriter::ZipWriter() = default;

ZipWriter::~ZipWriter() {
    close();
}

// ---------------------------------------------------------------------------
// 打开
// ---------------------------------------------------------------------------
bool ZipWriter::open(const std::string& path) {
    close();

    if (path.empty()) {
        error_ = "open: empty path";
        return false;
    }

    auto* archive = new mz_zip_archive();
    mz_zip_zero_struct(archive);

    // mz_zip_writer_init_file 创建/覆盖目标文件并初始化写入器
    // 返回 MZ_FALSE 表示打开失败（路径不可写、磁盘满等）
    if (!mz_zip_writer_init_file(archive, path.c_str(), 0)) {
        error_ = "mz_zip_writer_init_file failed: ";
        error_ += mz_zip_get_error_string(mz_zip_get_last_error(archive));
        delete archive;
        return false;
    }

    archive_ = archive;
    opened_ = true;
    error_.clear();
    return true;
}

// ---------------------------------------------------------------------------
// 添加文件条目
// ---------------------------------------------------------------------------
bool ZipWriter::addFile(const std::string& name, const std::string& content) {
    return addFile(name, content.data(), content.size());
}

bool ZipWriter::addFile(const std::string& name, const void* data, std::size_t size) {
    if (!opened_) {
        error_ = "addFile: archive not opened";
        return false;
    }
    if (name.empty()) {
        error_ = "addFile: empty entry name";
        return false;
    }

    auto* archive = static_cast<mz_zip_archive*>(archive_);
    // MZ_DEFAULT_COMPRESSION 是无名枚举，需 static_cast 避免 C4245 符号不匹配警告
    if (!mz_zip_writer_add_mem(archive, name.c_str(), data, size,
                                static_cast<mz_uint>(MZ_DEFAULT_COMPRESSION))) {
        error_ = "mz_zip_writer_add_mem failed for '";
        error_ += name;
        error_ += "': ";
        error_ += mz_zip_get_error_string(mz_zip_get_last_error(archive));
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 关闭并 finalize
// ---------------------------------------------------------------------------
bool ZipWriter::close() {
    if (!opened_) {
        return true;  // 幂等：未打开时直接返回
    }

    auto* archive = static_cast<mz_zip_archive*>(archive_);

    // mz_zip_writer_finalize_archive 写出中央目录 + EOCD 并关闭归档
    // 调用后 archive 仍需通过 mz_zip_writer_end 释放（miniz 3.x 行为）
    bool ok = true;
    if (!mz_zip_writer_finalize_archive(archive)) {
        error_ = "mz_zip_writer_finalize_archive failed: ";
        error_ += mz_zip_get_error_string(mz_zip_get_last_error(archive));
        ok = false;
    }
    mz_zip_writer_end(archive);
    delete archive;

    archive_ = nullptr;
    opened_ = false;
    return ok;
}

} // namespace ooxml
} // namespace office
} // namespace zq
