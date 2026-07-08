// =============================================================================
// src/ooxml/zip_reader.cpp
//
// ZipReader 实现，基于 miniz 3.x
// =============================================================================
#include "zip_reader.h"

#include <miniz.h>
#include <cstring>
#include <algorithm>

namespace zq {
namespace office {
namespace ooxml {

// ---------------------------------------------------------------------------
// 构造 / 析构
// ---------------------------------------------------------------------------
ZipReader::ZipReader() = default;

ZipReader::~ZipReader() {
    close();
}

// ---------------------------------------------------------------------------
// 打开
// ---------------------------------------------------------------------------
bool ZipReader::open(const std::string& filePath) {
    close();

    auto* archive = new mz_zip_archive();
    mz_zip_zero_struct(archive);

    if (!mz_zip_reader_init_file(archive, filePath.c_str(), 0)) {
        error_ = "mz_zip_reader_init_file failed: ";
        error_ += mz_zip_get_error_string(mz_zip_get_last_error(archive));
        delete archive;
        return false;
    }

    archive_ = archive;
    if (!loadEntries_()) {
        close();
        return false;
    }
    return true;
}

bool ZipReader::openFromMemory(const std::vector<unsigned char>& data) {
    // 持有数据副本，避免调用方提前释放
    ownedData_ = data;
    return openFromMemory(ownedData_.data(), ownedData_.size());
}

bool ZipReader::openFromMemory(const unsigned char* data, std::size_t size) {
    close();

    if (!data || size == 0) {
        error_ = "openFromMemory: empty data";
        return false;
    }

    auto* archive = new mz_zip_archive();
    mz_zip_zero_struct(archive);

    // 注意：mz_zip_reader_init_mem 不复制数据，调用方需保证 data 有效
    if (!mz_zip_reader_init_mem(archive, data, size, 0)) {
        error_ = "mz_zip_reader_init_mem failed: ";
        error_ += mz_zip_get_error_string(mz_zip_get_last_error(archive));
        delete archive;
        return false;
    }

    archive_ = archive;
    externalData_ = data;
    externalSize_ = size;

    if (!loadEntries_()) {
        close();
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 关闭
// ---------------------------------------------------------------------------
void ZipReader::close() {
    if (archive_) {
        auto* archive = static_cast<mz_zip_archive*>(archive_);
        mz_zip_reader_end(archive);
        delete archive;
        archive_ = nullptr;
    }
    entries_.clear();
    ownedData_.clear();
    externalData_ = nullptr;
    externalSize_ = 0;
    error_.clear();
}

bool ZipReader::isOpen() const {
    return archive_ != nullptr;
}

// ---------------------------------------------------------------------------
// 条目索引加载
// ---------------------------------------------------------------------------
bool ZipReader::loadEntries_() {
    auto* archive = static_cast<mz_zip_archive*>(archive_);
    if (!archive) return false;

    std::size_t num = mz_zip_reader_get_num_files(archive);
    entries_.clear();
    entries_.reserve(num);

    for (std::size_t i = 0; i < num; ++i) {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(archive, static_cast<mz_uint>(i), &stat)) {
            error_ = "mz_zip_reader_file_stat failed at index ";
            error_ += std::to_string(i);
            return false;
        }

        ZipEntry e;
        e.name = stat.m_filename;
        e.uncompressedSize = stat.m_uncomp_size;
        e.compressedSize = stat.m_comp_size;
        e.crc = stat.m_crc32;
        // 目录条目：名称以 '/' 结尾，或外部属性标记为目录
        e.isDirectory = (!e.name.empty() && e.name.back() == '/')
                        || (stat.m_external_attr & 0x10) != 0;

        entries_.push_back(std::move(e));
    }
    return true;
}

// ---------------------------------------------------------------------------
// 条目访问
// ---------------------------------------------------------------------------
const std::vector<ZipEntry>& ZipReader::entries() const {
    return entries_;
}

std::size_t ZipReader::entryCount() const {
    return entries_.size();
}

const ZipEntry* ZipReader::entryAt(std::size_t index) const {
    if (index >= entries_.size()) return nullptr;
    return &entries_[index];
}

std::size_t ZipReader::findEntry_(const std::string& name) const {
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].name == name) {
            return i;
        }
    }
    return SIZE_MAX;
}

bool ZipReader::hasEntry(const std::string& name) const {
    return findEntry_(name) != SIZE_MAX;
}

// ---------------------------------------------------------------------------
// 读取条目内容
// ---------------------------------------------------------------------------
std::vector<unsigned char> ZipReader::readEntry(const std::string& name) const {
    std::size_t idx = findEntry_(name);
    if (idx == SIZE_MAX) {
        error_ = "entry not found: " + name;
        return {};
    }
    return readEntryAt(idx);
}

std::vector<unsigned char> ZipReader::readEntryAt(std::size_t index) const {
    if (!archive_) {
        error_ = "archive not open";
        return {};
    }
    if (index >= entries_.size()) {
        error_ = "index out of range";
        return {};
    }
    if (entries_[index].isDirectory) {
        error_ = "cannot read directory entry";
        return {};
    }

    auto* archive = static_cast<mz_zip_archive*>(archive_);
    std::size_t outSize = 0;
    void* buf = mz_zip_reader_extract_to_heap(archive,
                                               static_cast<mz_uint>(index),
                                               &outSize, 0);
    if (!buf) {
        error_ = "mz_zip_reader_extract_to_heap failed: ";
        error_ += mz_zip_get_error_string(mz_zip_get_last_error(archive));
        return {};
    }

    std::vector<unsigned char> result;
    result.resize(outSize);
    if (outSize > 0) {
        std::memcpy(result.data(), buf, outSize);
    }
    mz_free(buf);
    return result;
}

std::string ZipReader::readEntryText(const std::string& name) const {
    auto data = readEntry(name);
    if (data.empty() && !hasEntry(name)) {
        return {};
    }
    return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}

} // namespace ooxml
} // namespace office
} // namespace zq
