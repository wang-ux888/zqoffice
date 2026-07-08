// =============================================================================
// src/escher/escher_record.cpp
// =============================================================================
#include "escher_record.h"

namespace zq {
namespace office {
namespace escher {

const char* blipTypeName(std::uint16_t blipType) {
    switch (blipType) {
        case kBlipTypeError:   return "Error";
        case kBlipTypeUnknown: return "Unknown";
        case kBlipTypeWMF:     return "WMF";
        case kBlipTypeEMF:     return "EMF";
        case kBlipTypePICT:    return "PICT";
        case kBlipTypeJPEG:    return "JPEG";
        case kBlipTypePNG:     return "PNG";
        case kBlipTypeDIB:     return "DIB";
        case kBlipTypeTIFF:    return "TIFF";
        default:               return "Invalid";
    }
}

const char* recordTypeName(std::uint16_t recType) {
    switch (recType) {
        case kRecordTypeDggContainer:    return "DggContainer";
        case kRecordTypeBStoreContainer: return "BStoreContainer";
        case kRecordTypeDGContainer:     return "DGContainer";
        case kRecordTypeSpgrContainer:   return "SpgrContainer";
        case kRecordTypeSpContainer:     return "SpContainer";
        case kRecordTypeOPT:             return "OPT";
        case kRecordTypeTertiaryFOPT:    return "TertiaryFOPT";
        case kRecordTypeBSE:             return "BSE";
        case kRecordTypeChildAnchor:     return "ChildAnchor";
        case kRecordTypeClientAnchor:    return "ClientAnchor";
        case kRecordTypeClientData:      return "ClientData";
        case kRecordTypeColorMRU:        return "ColorMRU";
        case kRecordTypeCollection:      return "Collection";
        case kRecordTypeFRITContainer:   return "FRITContainer";
        case kRecordTypeSecondaryFOPT:   return "SecondaryFOPT";
        case kRecordTypeSplitMenuColor:  return "SplitMenuColor";
        default:
            if (recType >= kRecordTypeBlipFirst && recType <= kRecordTypeBlipLast)
                return "BLIP";
            return "Unknown";
    }
}

bool EscherRecord::Read(const unsigned char* buf, std::size_t size) {
    if (!buf) return false;
    if (!header_.Read(buf, size)) return false;
    // 数据指针指向头部之后
    data_ = buf + PptRecordHeader::kHeaderSize;
    return true;
}

} // namespace escher
} // namespace office
} // namespace zq
