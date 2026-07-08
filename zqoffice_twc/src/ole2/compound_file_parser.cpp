// =============================================================================
// src/ole2/compound_file_parser.cpp
// =============================================================================
#include "compound_file_parser.h"

#include <fstream>
#include <iterator>

namespace zq {
namespace office {
namespace ole2 {

bool CompoundFileParser::Parse(const std::string& filePath) {
    parsed_ = false;
    error_.clear();

    std::ifstream f(filePath, std::ios::binary);
    if (!f.is_open()) {
        error_ = "cannot open file: " + filePath;
        return false;
    }
    std::vector<unsigned char> data(
        (std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>());
    f.close();

    if (data.empty()) {
        error_ = "file is empty";
        return false;
    }
    return ParseFromMemory(data);
}

bool CompoundFileParser::ParseFromMemory(const std::vector<unsigned char>& data) {
    parsed_ = false;
    error_.clear();

    if (!fs_.Load(data)) {
        error_ = fs_.error();
        return false;
    }
    parsed_ = true;
    return true;
}

Stream CompoundFileParser::GetStream(const std::string& name) {
    if (!parsed_) return Stream{};
    return fs_.OpenStream(name);
}

std::vector<std::string> CompoundFileParser::ListRootEntries() const {
    if (!parsed_) return {};
    return fs_.ListRootEntries();
}

} // namespace ole2
} // namespace office
} // namespace zq
