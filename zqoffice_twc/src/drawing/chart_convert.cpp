// =============================================================================
// src/drawing/chart_convert.cpp
// =============================================================================
#include "chart_convert.h"

#include "ooxml/xml_reader.h"

namespace zq {
namespace office {
namespace drawing {

ChartConvert::ChartConvert() = default;
ChartConvert::~ChartConvert() = default;

void ChartConvert::clear() {
    top_ = left_ = width_ = height_ = 0;
    valLst_.clear();
    theme_.reset();
    series_.clear();
    categories_.clear();
    chartType_ = ChartType::kUnknown;
    error_.clear();
    success_ = false;
}

bool ChartConvert::convertFromOOXML(const std::string& chartXml) {
    success_ = false;
    error_.clear();

    if (chartXml.empty()) {
        error_ = "empty chart XML";
        return false;
    }

    ooxml::XmlReader reader;
    if (!reader.parse(chartXml)) {
        error_ = "XML parse error: " + reader.error();
        return false;
    }

    ooxml::XmlNode root = reader.root();
    if (!root.valid()) {
        error_ = "no root element";
        return false;
    }

    // 根元素应为 c:chartSpace
    std::string rootLocal = localName_(root.name());
    if (rootLocal != "chartSpace") {
        error_ = "root is not 'chartSpace': " + root.name();
        return false;
    }

    // 递归查找 chart/plotArea 下的图表类型元素和系列
    // XML 结构：chartSpace → chart → plotArea → barChart/lineChart/... → ser
    ooxml::XmlNode chartNode = root.firstChild("chart");
    if (!chartNode.valid()) {
        // 按名称查找失败，按 localName 遍历查找
        for (ooxml::XmlNode c = root.firstChild(); c.valid();
             c = c.nextSibling()) {
            if (localName_(c.name()) == "chart") {
                chartNode = c;
                break;
            }
        }
    }

    if (!chartNode.valid()) {
        error_ = "missing c:chart";
        return false;
    }

    // 查找 plotArea
    ooxml::XmlNode plotArea = chartNode.firstChild("plotArea");
    if (!plotArea.valid()) {
        for (ooxml::XmlNode c = chartNode.firstChild(); c.valid();
             c = c.nextSibling()) {
            if (localName_(c.name()) == "plotArea") {
                plotArea = c;
                break;
            }
        }
    }

    if (!plotArea.valid()) {
        error_ = "missing c:plotArea";
        return false;
    }

    // 遍历 plotArea 的子元素：barChart / lineChart / pieChart / ... / ser
    // 注意：ser 是图表类型的子元素，不是 plotArea 的直接子元素
    for (ooxml::XmlNode chartChild = plotArea.firstChild();
         chartChild.valid(); chartChild = chartChild.nextSibling()) {
        std::string cln = localName_(chartChild.name());

        // 图表类型识别
        if (cln == "barChart" || cln == "bar3DChart") {
            if (chartType_ == ChartType::kUnknown) {
                chartType_ = ChartType::kBar;
            }
        } else if (cln == "lineChart" || cln == "line3DChart") {
            if (chartType_ == ChartType::kUnknown) {
                chartType_ = ChartType::kLine;
            }
        } else if (cln == "pieChart" || cln == "pie3DChart") {
            if (chartType_ == ChartType::kUnknown) {
                chartType_ = ChartType::kPie;
            }
        } else if (cln == "areaChart" || cln == "area3DChart") {
            if (chartType_ == ChartType::kUnknown) {
                chartType_ = ChartType::kArea;
            }
        } else if (cln == "scatterChart") {
            if (chartType_ == ChartType::kUnknown) {
                chartType_ = ChartType::kScatter;
            }
        } else if (cln == "doughnutChart") {
            if (chartType_ == ChartType::kUnknown) {
                chartType_ = ChartType::kDoughnut;
            }
        } else if (cln == "radarChart") {
            if (chartType_ == ChartType::kUnknown) {
                chartType_ = ChartType::kRadar;
            }
        } else if (cln == "surfaceChart" || cln == "surface3DChart") {
            if (chartType_ == ChartType::kUnknown) {
                chartType_ = ChartType::kSurface;
            }
        } else if (cln == "bubbleChart") {
            if (chartType_ == ChartType::kUnknown) {
                chartType_ = ChartType::kBubble;
            }
        }

        // 在图表类型元素内部查找系列
        if (cln == "barChart" || cln == "bar3DChart" ||
            cln == "lineChart" || cln == "line3DChart" ||
            cln == "pieChart" || cln == "pie3DChart" ||
            cln == "areaChart" || cln == "area3DChart" ||
            cln == "scatterChart" || cln == "doughnutChart" ||
            cln == "radarChart" || cln == "surfaceChart" ||
            cln == "surface3DChart" || cln == "bubbleChart") {
            parseSeriesInChartElement_(chartChild);
        }
    }

    success_ = true;
    return true;
}

void ChartConvert::parseSeriesInChartElement_(const ooxml::XmlNode& chartElementNode) {
    if (!chartElementNode.valid()) return;

    // 遍历图表类型元素的子元素查找 c:ser
    for (ooxml::XmlNode child = chartElementNode.firstChild();
         child.valid(); child = child.nextSibling()) {
        std::string ln = localName_(child.name());
        if (ln != "ser") continue;

        ChartSeries series;
        series.valid = true;

        for (ooxml::XmlNode serChild = child.firstChild();
             serChild.valid(); serChild = serChild.nextSibling()) {
            std::string sln = localName_(serChild.name());

            if (sln == "tx") {
                // 系列标题：tx → strRef/strLit → strCache → pt → v
                for (ooxml::XmlNode txChild = serChild.firstChild();
                     txChild.valid(); txChild = txChild.nextSibling()) {
                    std::string txln = localName_(txChild.name());
                    if (txln == "strRef" || txln == "strLit") {
                        for (ooxml::XmlNode v = txChild.firstChild();
                             v.valid(); v = v.nextSibling()) {
                            if (localName_(v.name()) == "strCache") {
                                for (ooxml::XmlNode pt = v.firstChild();
                                     pt.valid(); pt = pt.nextSibling()) {
                                    if (localName_(pt.name()) == "pt") {
                                        for (ooxml::XmlNode pv = pt.firstChild();
                                             pv.valid(); pv = pv.nextSibling()) {
                                            if (localName_(pv.name()) == "v") {
                                                series.title = pv.text();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (sln == "val") {
                // 数值：val → numCache/numLit → pt → v
                for (ooxml::XmlNode valChild = serChild.firstChild();
                     valChild.valid(); valChild = valChild.nextSibling()) {
                    std::string vln = localName_(valChild.name());
                    if (vln == "numCache" || vln == "numLit") {
                        for (ooxml::XmlNode pt = valChild.firstChild();
                             pt.valid(); pt = pt.nextSibling()) {
                            if (localName_(pt.name()) == "pt") {
                                for (ooxml::XmlNode pv = pt.firstChild();
                                     pv.valid(); pv = pv.nextSibling()) {
                                    if (localName_(pv.name()) == "v") {
                                        try {
                                            series.values.push_back(
                                                std::stod(pv.text()));
                                        } catch (...) {}
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (sln == "cat") {
                // 类别：cat → strRef/strLit → strCache → pt → v
                for (ooxml::XmlNode catChild = serChild.firstChild();
                     catChild.valid(); catChild = catChild.nextSibling()) {
                    std::string cln = localName_(catChild.name());
                    if (cln == "strRef" || cln == "strLit") {
                        for (ooxml::XmlNode v = catChild.firstChild();
                             v.valid(); v = v.nextSibling()) {
                            if (localName_(v.name()) == "strCache") {
                                for (ooxml::XmlNode pt = v.firstChild();
                                     pt.valid(); pt = pt.nextSibling()) {
                                    if (localName_(pt.name()) == "pt") {
                                        for (ooxml::XmlNode pv = pt.firstChild();
                                             pv.valid(); pv = pv.nextSibling()) {
                                            if (localName_(pv.name()) == "v") {
                                                ChartCategory cat;
                                                cat.label = pv.text();
                                                cat.valid = true;
                                                categories_.push_back(cat);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        series_.push_back(series);
    }
}

std::string ChartConvert::localName_(const std::string& fullName) {
    auto pos = fullName.find(':');
    if (pos == std::string::npos) return fullName;
    return fullName.substr(pos + 1);
}

} // namespace drawing
} // namespace office
} // namespace zq
