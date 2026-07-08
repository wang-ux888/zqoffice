// =============================================================================
// src/ooxml/pml/pml_writer.cpp
//
// PmlWriter 实现：生成 PresentationML XML 部件
// =============================================================================
#include "pml_writer.h"

#include <cstdio>
#include <sstream>
#include <string>

namespace zq {
namespace office {
namespace ooxml {
namespace pml {

// ===========================================================================
// 工具函数
// ===========================================================================

std::string PmlWriter::escapeXml(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:   out.push_back(c); break;
        }
    }
    return out;
}

const char* PmlWriter::alignmentAttr(WriteAlignment a) {
    switch (a) {
        case WriteAlignment::Left:    return "l";
        case WriteAlignment::Center:  return "ctr";
        case WriteAlignment::Right:   return "r";
        case WriteAlignment::Justify: return "just";
    }
    return "l";
}

const char* PmlWriter::shapeTypeName(WriteShapeType t) {
    switch (t) {
        case WriteShapeType::Text:       return "text";
        case WriteShapeType::Image:      return "image";
        case WriteShapeType::Chart:      return "chart";
        case WriteShapeType::Group:      return "group";
        case WriteShapeType::AutoShape:  return "autoshape";
    }
    return "text";
}

// ===========================================================================
// XML 部件生成
// ===========================================================================

// [Content_Types].xml
std::string PmlWriter::writeContentTypes(std::size_t slideCount, bool hasCoreProps) {
    std::string xml;
    xml.reserve(2048);

    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">";

    // 默认扩展名
    xml += "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>";
    xml += "<Default Extension=\"xml\" ContentType=\"application/xml\"/>";

    // Presentation 主部件
    xml += "<Override PartName=\"/ppt/presentation.xml\" "
           "ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml\"/>";

    // 各 slide
    for (std::size_t i = 0; i < slideCount; ++i) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "/ppt/slides/slide%zu.xml", i + 1);
        xml += "<Override PartName=\"";
        xml += buf;
        xml += "\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>";
    }

    // slideMaster + slideLayout + theme
    xml += "<Override PartName=\"/ppt/slideMasters/slideMaster1.xml\" "
           "ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml\"/>";
    xml += "<Override PartName=\"/ppt/slideLayouts/slideLayout1.xml\" "
           "ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml\"/>";
    xml += "<Override PartName=\"/ppt/theme/theme1.xml\" "
           "ContentType=\"application/vnd.openxmlformats-officedocument.theme+xml\"/>";

    // core.xml（可选）
    if (hasCoreProps) {
        xml += "<Override PartName=\"/docProps/core.xml\" "
               "ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>";
    }

    xml += "</Types>";
    return xml;
}

// _rels/.rels
std::string PmlWriter::writeRootRels() {
    std::string xml;
    xml.reserve(512);
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">";
    xml += "<Relationship Id=\"rId1\" "
           "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" "
           "Target=\"ppt/presentation.xml\"/>";
    xml += "<Relationship Id=\"rId2\" "
           "Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" "
           "Target=\"docProps/core.xml\"/>";
    xml += "</Relationships>";
    return xml;
}

// ppt/presentation.xml
std::string PmlWriter::writePresentation(std::size_t slideCount, const WriteSlideSize& slideSize) {
    std::string xml;
    xml.reserve(1024);
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<p:presentation xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
           "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
           "xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\" "
           "saveSubsetFonts=\"1\">";

    // sldMasterIdLst
    xml += "<p:sldMasterIdLst>";
    xml += "<p:sldMasterId id=\"2147483648\" r:id=\"rId1\"/>";
    xml += "</p:sldMasterIdLst>";

    // sldIdLst
    xml += "<p:sldIdLst>";
    for (std::size_t i = 0; i < slideCount; ++i) {
        char buf[64];
        // rId 从 2 开始（rId1 已用于 slideMaster）
        std::snprintf(buf, sizeof(buf),
                      "<p:sldId id=\"%zu\" r:id=\"rId%zu\"/>",
                      i + 256, i + 2);
        xml += buf;
    }
    xml += "</p:sldIdLst>";

    // sldSz
    char szBuf[128];
    std::snprintf(szBuf, sizeof(szBuf),
                  "<p:sldSz cx=\"%lld\" cy=\"%lld\" type=\"screen4x3\"/>",
                  static_cast<long long>(slideSize.width),
                  static_cast<long long>(slideSize.height));
    xml += szBuf;

    // notesSz（备注页尺寸，固定 7.5x5 inch）
    xml += "<p:notesSz cx=\"6858000\" cy=\"9144000\"/>";

    xml += "</p:presentation>";
    return xml;
}

// ppt/_rels/presentation.xml.rels
std::string PmlWriter::writePresentationRels(std::size_t slideCount) {
    std::string xml;
    xml.reserve(512);
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">";

    // rId1 → slideMaster1.xml
    xml += "<Relationship Id=\"rId1\" "
           "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster\" "
           "Target=\"slideMasters/slideMaster1.xml\"/>";

    // rId2..rIdN → slides/slideN.xml
    for (std::size_t i = 0; i < slideCount; ++i) {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "<Relationship Id=\"rId%zu\" "
                      "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\" "
                      "Target=\"slides/slide%zu.xml\"/>",
                      i + 2, i + 1);
        xml += buf;
    }

    xml += "</Relationships>";
    return xml;
}

// ppt/slides/slideN.xml
std::string PmlWriter::writeSlide(const WriteSlide& slide) {
    std::string xml;
    xml.reserve(2048);
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<p:sld xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
           "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
           "xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">";

    // cSld
    xml += "<p:cSld>";

    // spTree
    xml += "<p:spTree>";
    xml += "<p:nvGrpSpPr>";
    xml += "<p:cNvPr id=\"1\" name=\"\"/>";
    xml += "<p:cNvGrpSpPr/>";
    xml += "<p:nvPr/>";
    xml += "</p:nvGrpSpPr>";
    xml += "<p:grpSpPr>";
    xml += "<a:xfrm>";
    xml += "<a:off x=\"0\" y=\"0\"/>";
    xml += "<a:ext cx=\"0\" cy=\"0\"/>";
    xml += "<a:chOff x=\"0\" y=\"0\"/>";
    xml += "<a:chExt cx=\"0\" cy=\"0\"/>";
    xml += "</a:xfrm>";
    xml += "</p:grpSpPr>";

    // 遍历 shapes
    for (std::size_t i = 0; i < slide.shapes.size(); ++i) {
        const auto& shape = slide.shapes[i];
        std::size_t shapeId = i + 2;  // ID 从 2 开始（1 已用于 spTree 根）

        switch (shape.type) {
            case WriteShapeType::Text:
            case WriteShapeType::AutoShape: {
                xml += "<p:sp>";
                // nvSpPr
                xml += "<p:nvSpPr>";
                char idBuf[64];
                std::snprintf(idBuf, sizeof(idBuf),
                              "<p:cNvPr id=\"%zu\" name=\"%s\"/>",
                              shapeId,
                              escapeXml(shape.name.empty() ? "TextBox" : shape.name).c_str());
                xml += idBuf;
                xml += "<p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr>";
                xml += "<p:nvPr/>";
                xml += "</p:nvSpPr>";

                // spPr
                xml += "<p:spPr>";
                char xfrmBuf[256];
                std::snprintf(xfrmBuf, sizeof(xfrmBuf),
                              "<a:xfrm rot=\"%d\">"
                              "<a:off x=\"%lld\" y=\"%lld\"/>"
                              "<a:ext cx=\"%lld\" cy=\"%lld\"/>"
                              "</a:xfrm>",
                              shape.rotation,
                              static_cast<long long>(shape.geometry.x),
                              static_cast<long long>(shape.geometry.y),
                              static_cast<long long>(shape.geometry.width),
                              static_cast<long long>(shape.geometry.height));
                xml += xfrmBuf;
                // prstGeom（rect 为默认）
                xml += "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>";

                // 填充
                if (!shape.fillColor.empty()) {
                    xml += "<a:solidFill>";
                    xml += "<a:srgbClr val=\"";
                    xml += shape.fillColor;
                    xml += "\"/>";
                    xml += "</a:solidFill>";
                } else {
                    xml += "<a:noFill/>";
                }
                xml += "</p:spPr>";

                // txBody
                xml += "<p:txBody>";
                xml += "<a:bodyPr wrap=\"square\" rtlCol=\"0\">";
                xml += "<a:spAutoFit/>";
                xml += "</a:bodyPr>";
                xml += "<a:lstStyle/>";

                // 段落
                if (shape.paragraphs.empty()) {
                    xml += "<a:p/>";
                } else {
                    for (const auto& para : shape.paragraphs) {
                        xml += "<a:p>";
                        char pPrBuf[64];
                        std::snprintf(pPrBuf, sizeof(pPrBuf),
                                      "<a:pPr algn=\"%s\"/>",
                                      alignmentAttr(para.alignment));
                        xml += pPrBuf;

                        if (para.runs.empty()) {
                            // 空段落也需要一个 endParaRPr
                            xml += "<a:endParaRPr lang=\"zh-CN\"/>";
                        } else {
                            for (const auto& run : para.runs) {
                                xml += "<a:r>";
                                // rPr
                                xml += "<a:rPr lang=\"zh-CN\"";
                                if (run.font.bold) xml += " b=\"1\"";
                                if (run.font.italic) xml += " i=\"1\"";
                                if (run.font.underline) xml += " u=\"sng\"";
                                if (run.font.size > 0) {
                                    char szBuf[32];
                                    std::snprintf(szBuf, sizeof(szBuf),
                                                  " sz=\"%d\"", run.font.size);
                                    xml += szBuf;
                                }
                                if (!run.font.color.empty()) {
                                    xml += ">";
                                    xml += "<a:solidFill>";
                                    xml += "<a:srgbClr val=\"";
                                    xml += run.font.color;
                                    xml += "\"/>";
                                    xml += "</a:solidFill>";
                                    // typeface 在 latin 元素
                                    if (!run.font.family.empty()) {
                                        xml += "<a:latin typeface=\"";
                                        xml += escapeXml(run.font.family);
                                        xml += "\"/>";
                                    }
                                    xml += "</a:rPr>";
                                } else {
                                    if (!run.font.family.empty()) {
                                        xml += ">";
                                        xml += "<a:latin typeface=\"";
                                        xml += escapeXml(run.font.family);
                                        xml += "\"/>";
                                        xml += "</a:rPr>";
                                    } else {
                                        xml += "/>";
                                    }
                                }
                                // t
                                xml += "<a:t>";
                                xml += escapeXml(run.text);
                                xml += "</a:t>";
                                xml += "</a:r>";
                            }
                        }
                        xml += "</a:p>";
                    }
                }
                xml += "</p:txBody>";
                xml += "</p:sp>";
                break;
            }

            case WriteShapeType::Image: {
                xml += "<p:pic>";
                // nvPicPr
                xml += "<p:nvPicPr>";
                char idBuf[64];
                std::snprintf(idBuf, sizeof(idBuf),
                              "<p:cNvPr id=\"%zu\" name=\"%s\"/>",
                              shapeId,
                              escapeXml(shape.name.empty() ? "Picture" : shape.name).c_str());
                xml += idBuf;
                xml += "<p:cNvPicPr><a:picLocks noChangeAspect=\"1\"/></p:cNvPicPr>";
                xml += "<p:nvPr/>";
                xml += "</p:nvPicPr>";

                // blipFill
                xml += "<p:blipFill>";
                xml += "<a:blip r:embed=\"rId1\"/>";
                xml += "<a:stretch><a:fillRect/></a:stretch>";
                xml += "</p:blipFill>";

                // spPr
                xml += "<p:spPr>";
                char xfrmBuf[256];
                std::snprintf(xfrmBuf, sizeof(xfrmBuf),
                              "<a:xfrm rot=\"%d\">"
                              "<a:off x=\"%lld\" y=\"%lld\"/>"
                              "<a:ext cx=\"%lld\" cy=\"%lld\"/>"
                              "</a:xfrm>",
                              shape.rotation,
                              static_cast<long long>(shape.geometry.x),
                              static_cast<long long>(shape.geometry.y),
                              static_cast<long long>(shape.geometry.width),
                              static_cast<long long>(shape.geometry.height));
                xml += xfrmBuf;
                xml += "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>";
                xml += "</p:spPr>";

                xml += "</p:pic>";
                break;
            }

            case WriteShapeType::Chart:
            case WriteShapeType::Group:
                // Phase H-Part3 暂不写入 chart/group（占位）
                break;
        }
    }

    xml += "</p:spTree>";
    xml += "</p:cSld>";

    // clrMapOvr
    xml += "<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>";

    xml += "</p:sld>";
    return xml;
}

// ppt/slideLayouts/slideLayout1.xml（最小化空白版式）
std::string PmlWriter::writeSlideLayout() {
    std::string xml;
    xml.reserve(1024);
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<p:sldLayout xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
           "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
           "xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\" "
           "type=\"blank\" preserve=\"1\">";

    xml += "<p:cSld name=\"Blank\">";
    xml += "<p:spTree>";
    xml += "<p:nvGrpSpPr>";
    xml += "<p:cNvPr id=\"1\" name=\"\"/>";
    xml += "<p:cNvGrpSpPr/>";
    xml += "<p:nvPr/>";
    xml += "</p:nvGrpSpPr>";
    xml += "<p:grpSpPr>";
    xml += "<a:xfrm>";
    xml += "<a:off x=\"0\" y=\"0\"/>";
    xml += "<a:ext cx=\"0\" cy=\"0\"/>";
    xml += "<a:chOff x=\"0\" y=\"0\"/>";
    xml += "<a:chExt cx=\"0\" cy=\"0\"/>";
    xml += "</a:xfrm>";
    xml += "</p:grpSpPr>";
    xml += "</p:spTree>";
    xml += "</p:cSld>";

    xml += "<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>";
    xml += "</p:sldLayout>";
    return xml;
}

// ppt/slideMasters/slideMaster1.xml（最小化母版）
std::string PmlWriter::writeSlideMaster() {
    std::string xml;
    xml.reserve(2048);
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<p:sldMaster xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
           "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
           "xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">";

    xml += "<p:cSld>";

    // bg（背景）
    xml += "<p:bg>";
    xml += "<p:bgRef idx=\"1001\">";
    xml += "<a:schemeClr val=\"bg1\"/>";
    xml += "</p:bgRef>";
    xml += "</p:bg>";

    xml += "<p:spTree>";
    xml += "<p:nvGrpSpPr>";
    xml += "<p:cNvPr id=\"1\" name=\"\"/>";
    xml += "<p:cNvGrpSpPr/>";
    xml += "<p:nvPr/>";
    xml += "</p:nvGrpSpPr>";
    xml += "<p:grpSpPr>";
    xml += "<a:xfrm>";
    xml += "<a:off x=\"0\" y=\"0\"/>";
    xml += "<a:ext cx=\"0\" cy=\"0\"/>";
    xml += "<a:chOff x=\"0\" y=\"0\"/>";
    xml += "<a:chExt cx=\"0\" cy=\"0\"/>";
    xml += "</a:xfrm>";
    xml += "</p:grpSpPr>";
    xml += "</p:spTree>";
    xml += "</p:cSld>";

    // clrMap
    xml += "<p:clrMap bg1=\"lt1\" tx1=\"dk1\" bg2=\"lt2\" tx2=\"dk2\" "
           "accent1=\"accent1\" accent2=\"accent2\" accent3=\"accent3\" accent4=\"accent4\" "
           "accent5=\"accent5\" accent6=\"accent6\" hlink=\"hlink\" folHlink=\"folHlink\"/>";

    // sldLayoutIdLst
    xml += "<p:sldLayoutIdLst>";
    xml += "<p:sldLayoutId id=\"2147483649\" r:id=\"rId1\"/>";
    xml += "</p:sldLayoutIdLst>";

    // txStyles
    xml += "<p:txStyles>";
    xml += "<p:titleStyle>";
    xml += "<a:lvl1pPr algn=\"ctr\">";
    xml += "<a:defRPr sz=\"4400\" b=\"1\">";
    xml += "<a:latin typeface=\"Calibri\"/>";
    xml += "</a:defRPr>";
    xml += "</a:lvl1pPr>";
    xml += "</p:titleStyle>";
    xml += "<p:bodyStyle>";
    xml += "<a:lvl1pPr>";
    xml += "<a:defRPr sz=\"2400\">";
    xml += "<a:latin typeface=\"Calibri\"/>";
    xml += "</a:defRPr>";
    xml += "</a:lvl1pPr>";
    xml += "</p:bodyStyle>";
    xml += "<p:otherStyle/>";
    xml += "</p:txStyles>";

    xml += "</p:sldMaster>";
    return xml;
}

// ppt/_rels/slideMasters/slideMaster1.xml.rels
std::string PmlWriter::writeSlideMasterRels() {
    std::string xml;
    xml.reserve(256);
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">";
    xml += "<Relationship Id=\"rId1\" "
           "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" "
           "Target=\"../slideLayouts/slideLayout1.xml\"/>";
    xml += "<Relationship Id=\"rId2\" "
           "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme\" "
           "Target=\"../theme/theme1.xml\"/>";
    xml += "</Relationships>";
    return xml;
}

// ppt/theme/theme1.xml（最小化主题）
std::string PmlWriter::writeTheme() {
    std::string xml;
    xml.reserve(4096);
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<a:theme xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" name=\"Office Theme\">";

    // themeElements
    xml += "<a:themeElements>";
    xml += "<a:clrScheme name=\"Office\">";
    xml += "<a:dk1><a:sysClr val=\"windowText\" lastClr=\"000000\"/></a:dk1>";
    xml += "<a:lt1><a:sysClr val=\"window\" lastClr=\"FFFFFF\"/></a:lt1>";
    xml += "<a:dk2><a:srgbClr val=\"44546A\"/></a:dk2>";
    xml += "<a:lt2><a:srgbClr val=\"E7E6E6\"/></a:lt2>";
    xml += "<a:accent1><a:srgbClr val=\"4472C4\"/></a:accent1>";
    xml += "<a:accent2><a:srgbClr val=\"ED7D31\"/></a:accent2>";
    xml += "<a:accent3><a:srgbClr val=\"A5A5A5\"/></a:accent3>";
    xml += "<a:accent4><a:srgbClr val=\"FFC000\"/></a:accent4>";
    xml += "<a:accent5><a:srgbClr val=\"5B9BD5\"/></a:accent5>";
    xml += "<a:accent6><a:srgbClr val=\"70AD47\"/></a:accent6>";
    xml += "<a:hlink><a:srgbClr val=\"0563C1\"/></a:hlink>";
    xml += "<a:folHlink><a:srgbClr val=\"954F72\"/></a:folHlink>";
    xml += "</a:clrScheme>";

    xml += "<a:fontScheme name=\"Office\">";
    xml += "<a:majorFont>";
    xml += "<a:latin typeface=\"Calibri Light\"/>";
    xml += "<a:ea typeface=\"\"/>";
    xml += "<a:cs typeface=\"\"/>";
    xml += "</a:majorFont>";
    xml += "<a:minorFont>";
    xml += "<a:latin typeface=\"Calibri\"/>";
    xml += "<a:ea typeface=\"\"/>";
    xml += "<a:cs typeface=\"\"/>";
    xml += "</a:minorFont>";
    xml += "</a:fontScheme>";

    xml += "<a:fmtScheme name=\"Office\">";
    xml += "<a:fillStyleLst>";
    xml += "<a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill>";
    xml += "<a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill>";
    xml += "<a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill>";
    xml += "</a:fillStyleLst>";
    xml += "<a:lnStyleLst>";
    xml += "<a:ln w=\"9525\" cap=\"flat\" cmpd=\"sng\" algn=\"ctr\">";
    xml += "<a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill>";
    xml += "<a:prstDash val=\"solid\"/>";
    xml += "</a:ln>";
    xml += "<a:ln w=\"9525\" cap=\"flat\" cmpd=\"sng\" algn=\"ctr\">";
    xml += "<a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill>";
    xml += "<a:prstDash val=\"solid\"/>";
    xml += "</a:ln>";
    xml += "<a:ln w=\"9525\" cap=\"flat\" cmpd=\"sng\" algn=\"ctr\">";
    xml += "<a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill>";
    xml += "<a:prstDash val=\"solid\"/>";
    xml += "</a:ln>";
    xml += "</a:lnStyleLst>";
    xml += "<a:effectStyleLst>";
    xml += "<a:effectStyle><a:effectLst/></a:effectStyle>";
    xml += "<a:effectStyle><a:effectLst/></a:effectStyle>";
    xml += "<a:effectStyle><a:effectLst/></a:effectStyle>";
    xml += "</a:effectStyleLst>";
    xml += "<a:bgFillStyleLst>";
    xml += "<a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill>";
    xml += "<a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill>";
    xml += "<a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill>";
    xml += "</a:bgFillStyleLst>";
    xml += "</a:fmtScheme>";
    xml += "</a:themeElements>";

    xml += "<a:objectDefaults/>";
    xml += "<a:extraClrSchemeLst/>";
    xml += "</a:theme>";
    return xml;
}

// docProps/core.xml
std::string PmlWriter::writeCoreProperties(const WriteCoreProperties& props) {
    std::string xml;
    xml.reserve(512);
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<cp:coreProperties "
           "xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
           "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
           "xmlns:dcterms=\"http://purl.org/dc/terms/\" "
           "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">";

    if (!props.title.empty()) {
        xml += "<dc:title>";
        xml += escapeXml(props.title);
        xml += "</dc:title>";
    }
    if (!props.creator.empty()) {
        xml += "<dc:creator>";
        xml += escapeXml(props.creator);
        xml += "</dc:creator>";
    }
    if (!props.created.empty()) {
        xml += "<dcterms:created xsi:type=\"dcterms:W3CDTF\">";
        xml += escapeXml(props.created);
        xml += "</dcterms:created>";
    }
    if (!props.modified.empty()) {
        xml += "<dcterms:modified xsi:type=\"dcterms:W3CDTF\">";
        xml += escapeXml(props.modified);
        xml += "</dcterms:modified>";
    }

    xml += "</cp:coreProperties>";
    return xml;
}

} // namespace pml
} // namespace ooxml
} // namespace office
} // namespace zq
