// =============================================================================
// src/drawing/escher_drawing_convert.cpp
//
// EscherDrawingConvert 实现
// =============================================================================
#include "drawing/escher_drawing_convert.h"

#include "escher/escher_record.h"
#include "drawing/enums/text_enums.h"

namespace zq {
namespace office {
namespace drawing {

// ===========================================================================
// 构造 / 析构
// ===========================================================================

EscherDrawingConvert::EscherDrawingConvert() = default;
EscherDrawingConvert::~EscherDrawingConvert() = default;

// ===========================================================================
// 入口方法
// ===========================================================================

std::vector<std::shared_ptr<TTShape>> EscherDrawingConvert::convertEscherShape(
    const escher::EscherRecord* record) {
    std::vector<std::shared_ptr<TTShape>> shapes;
    if (!record) return shapes;

    // 派发到 dispatchRecord 处理
    dispatchRecord(record, shapes);
    return shapes;
}

std::unique_ptr<TTShape> EscherDrawingConvert::convertNew(
    const escher::EscherRecord* record) {
    if (!record) return nullptr;

    auto shape = std::make_unique<TTShape>();
    // 简化实现：仅根据记录类型创建基础形状
    // 完整实现需要解析子记录（OPT/Anchor/Textbox）
    shape->setShapeType(PrstShapeType::kRect);
    return shape;
}

std::unique_ptr<TTShape> EscherDrawingConvert::convertBg(
    const escher::EscherRecord* record, int width, int height) {
    if (!record) return nullptr;

    auto shape = std::make_unique<TTShape>();
    shape->setShapeType(PrstShapeType::kRect);
    shape->SetX(0);
    shape->SetY(0);
    shape->SetWidth(width);
    shape->SetHeight(height);
    return shape;
}

std::unique_ptr<TTShape> EscherDrawingConvert::convertSlideBg(
    const escher::EscherRecord* record,
    const escher::EscherRecord* pptRecord,
    int width, int height) {
    // Slide 背景先复用 convertBg，待 D11-D14 接口实现后扩展
    return convertBg(record, width, height);
}

std::unique_ptr<TTShape> EscherDrawingConvert::convertSpContainerRecord(
    const escher::EscherContainerRecord* container,
    const OptionalDimension& parentSize,
    TTShape* parentShape,
    const escher::EscherSpgrRecord* spgrRecord) {
    if (!container) return nullptr;

    auto shape = std::make_unique<TTShape>();
    shape->setShapeType(PrstShapeType::kRect);

    // 若有父尺寸，应用缩放
    if (parentSize.valid) {
        // 完整实现需要从 EscherChildAnchorRecord/ClientAnchorRecord 读取锚点
        // 这里仅设置默认值
        shape->SetWidth(parentSize.width);
        shape->SetHeight(parentSize.height);
    }

    return shape;
}

// ===========================================================================
// 静态方法
// ===========================================================================

void EscherDrawingConvert::convertTransform(
    float scaleX, float scaleY, float rotateAngle,
    bool flipH, bool flipV, ShapePr* shapePr) {
    if (!shapePr) return;

    // 应用缩放：将 ShapePr 中的 W/H 按缩放因子调整
    if (scaleX != 0.0f && scaleX != 1.0f) {
        std::int64_t w = shapePr->W();
        shapePr->SetW(static_cast<std::int64_t>(w * scaleX));
    }
    if (scaleY != 0.0f && scaleY != 1.0f) {
        std::int64_t h = shapePr->H();
        shapePr->SetH(static_cast<std::int64_t>(h * scaleY));
    }

    // 应用旋转
    if (rotateAngle != 0.0f) {
        shapePr->setRotation(rotateAngle);
    }

    // 应用翻转（总是设置，确保幂等性）
    shapePr->setFlipH(flipH);
    shapePr->setFlipV(flipV);
}

void EscherDrawingConvert::convertMsoAnchor(MsoAnchor anchor,
                                              TextBody* textBody) {
    if (!textBody) return;

    // MSOANCHOR → TextAnchoringType
    switch (anchor) {
        case MsoAnchor::kTop:
            textBody->setVerticalAlign(TextAnchoringType::kTop);
            break;
        case MsoAnchor::kMiddle:
            textBody->setVerticalAlign(TextAnchoringType::kCenter);
            break;
        case MsoAnchor::kBottom:
            textBody->setVerticalAlign(TextAnchoringType::kBottom);
            break;
        case MsoAnchor::kTopCentered:
            // 顶端居中：锚定顶部但水平居中（与 OOXML anchor=t 一致）
            textBody->setVerticalAlign(TextAnchoringType::kTop);
            break;
        case MsoAnchor::kMiddleCentered:
            textBody->setVerticalAlign(TextAnchoringType::kCenter);
            break;
        case MsoAnchor::kBottomCentered:
            textBody->setVerticalAlign(TextAnchoringType::kBottom);
            break;
    }
}

void EscherDrawingConvert::convertMsoWrap(MsoWrapMode wrap,
                                            TextBody* textBody) {
    if (!textBody) return;

    // MSOWRAPMODE → TextWrapType
    // OOXML wrap 取值：square / none / topAndBottom
    switch (wrap) {
        case MsoWrapMode::kSquare:
            textBody->setWrapText(TextWrapType::kSquare);
            break;
        case MsoWrapMode::kByPoints:
            // 按顶点换行 - 等同 square
            textBody->setWrapText(TextWrapType::kSquare);
            break;
        case MsoWrapMode::kNone:
            textBody->setWrapText(TextWrapType::kNone);
            break;
        case MsoWrapMode::kTopBottom:
            // 上下环绕 - OOXML 没有直接对应，使用 square 近似
            textBody->setWrapText(TextWrapType::kSquare);
            break;
    }
}

void EscherDrawingConvert::convertTextFlow(MsoTxfl flow,
                                              TextBody* textBody) {
    if (!textBody) return;

    // MSOTXFL → TextVerticalType
    // OOXML vert 取值：horz / vert / vert270 / wordArtVert
    switch (flow) {
        case MsoTxfl::kHorz:
            textBody->setTextFlow(TextVerticalType::kHorizontal);
            break;
        case MsoTxfl::kVertN:
            textBody->setTextFlow(TextVerticalType::kVertical);
            break;
        case MsoTxfl::kVertBrz:
            // 巴西版垂直 - 与 vert 等同
            textBody->setTextFlow(TextVerticalType::kVertical);
            break;
        case MsoTxfl::kVert270:
            textBody->setTextFlow(TextVerticalType::kVertical270);
            break;
        case MsoTxfl::kArtVert:
            // 艺术垂直 - OOXML wordArtVert
            textBody->setTextFlow(TextVerticalType::kWordArtVertical);
            break;
    }
}

// ===========================================================================
// 内部方法
// ===========================================================================

int EscherDrawingConvert::convertDgRecord(
    const escher::EscherDgRecord* dgRecord,
    std::vector<std::shared_ptr<TTShape>>& outShapes) {
    if (!dgRecord) return 0;
    // 简化实现：DgRecord 主要是容器，子记录由 dispatchRecord 处理
    return 0;
}

int EscherDrawingConvert::convertSpgrContainerRecord(
    const escher::EscherContainerRecord* container,
    std::vector<std::shared_ptr<TTShape>>& outShapes,
    TTShape* parentShape,
    const escher::EscherSpgrRecord* spgrRecord) {
    if (!container) return 0;
    // 简化实现：组合形状容器，子记录由 dispatchRecord 处理
    return 0;
}

int EscherDrawingConvert::convertSpgrRecord(
    const escher::EscherSpgrRecord* spgrRecord,
    std::vector<std::shared_ptr<TTShape>>& outShapes) {
    if (!spgrRecord) return 0;
    // 组合形状记录，创建一个组合 TTShape
    auto shape = std::make_shared<TTShape>();
    shape->setShapeType(PrstShapeType::kRect);
    outShapes.push_back(shape);
    return 1;
}

int EscherDrawingConvert::convertSpRecord(
    const escher::EscherSpRecord* spRecord,
    std::vector<std::shared_ptr<TTShape>>& outShapes) {
    if (!spRecord) return 0;
    // 普通形状记录，创建一个 TTShape
    auto shape = std::make_shared<TTShape>();
    shape->setShapeType(PrstShapeType::kRect);
    outShapes.push_back(shape);
    return 1;
}

int EscherDrawingConvert::convertChildAnchorRecord(
    const escher::EscherChildAnchorRecord* anchor,
    const escher::EscherSpgrRecord* spgrRecord,
    ShapePr* shapePr,
    TTShape* shape,
    float scale) {
    if (!anchor || !shapePr) return 0;
    // 简化实现：实际需要从记录数据中读取 4 个 int32 (left/top/right/bottom)
    // 并按 scale 缩放后设置到 shapePr
    return 0;
}

int EscherDrawingConvert::convertClientAnchorRecord(
    const IOfficeArtClientAnchor* clientAnchor,
    ShapePr* shapePr,
    float scale) {
    if (!clientAnchor || !shapePr) return 0;
    // 委托给 IOfficeArtClientAnchor 实现（D12 提供）
    return 0;
}

int EscherDrawingConvert::convertClientDataRecord(
    const IOfficeArtClientData* clientData,
    std::vector<std::shared_ptr<TTShape>>& outShapes) {
    if (!clientData) return 0;
    // 委托给 IOfficeArtClientData 实现（后续阶段提供）
    return 0;
}

std::unique_ptr<TextBody> EscherDrawingConvert::convertClientTextbox(
    const IOfficeArtClientTextbox* clientTb,
    const escher::EscherOptRecord* optRecord,
    const IOfficeArtClientData* clientData) {
    if (!clientTb) return nullptr;
    // 委托给 IOfficeArtClientTextbox::ConvertToTextBody（D11/D13 提供）
    return nullptr;
}

int EscherDrawingConvert::convertAbstractOptRecord(
    const escher::EscherOptRecord* optRecord,
    std::vector<std::shared_ptr<TTShape>>& outShapes) {
    if (!optRecord) return 0;
    // 解析 OptRecord 中的属性表，派发到 dispatchProperty
    return 0;
}

TTShape* EscherDrawingConvert::convertSpFreeForm(
    const escher::EscherOptRecord* optRecord,
    const IOfficeArtClientTextbox* clientTb,
    const IOfficeArtClientData* clientData,
    std::unique_ptr<ShapePr> shapePr) {
    // 自由形状（Custom Geometry）
    // 完整实现需要读取 pVerticies 等属性
    if (!shapePr) return nullptr;

    auto shape = new TTShape();
    shape->setShapeType(PrstShapeType::kUnknown);  // 自由形状无预设
    return shape;
}

TTShape* EscherDrawingConvert::convertSpPictureFrame(
    const escher::EscherOptRecord* optRecord,
    std::unique_ptr<ShapePr> shapePr) {
    // 图片框
    if (!shapePr) return nullptr;

    auto shape = new TTShape();
    shape->setShapeType(PrstShapeType::kRect);
    return shape;
}

TTShape* EscherDrawingConvert::convertSpPrst(
    const std::string& prstName,
    const escher::EscherOptRecord* optRecord,
    const IOfficeArtClientTextbox* clientTb,
    const IOfficeArtClientData* clientData,
    const escher::EscherTertiaryOptRecord* tertiaryOpt,
    std::unique_ptr<ShapePr> shapePr) {
    // 预设形状
    if (!shapePr) return nullptr;

    auto shape = new TTShape();
    // 通过 DrawingUtils 反查 MSO 形状码 → PrstShapeType
    // 完整实现需要从 optRecord 读取 shapeType 属性
    shape->setShapeType(PrstShapeType::kRect);  // 默认矩形
    return shape;
}

int EscherDrawingConvert::convertBlipToClip(
    const escher::EscherOptRecord* optRecord, TTShape* shape) {
    if (!optRecord || !shape) return 0;
    // Blip → 剪贴图片引用
    // 完整实现需要读取 pib 属性，调用 GetPicPathFromList
    return 0;
}

int EscherDrawingConvert::convertBlipToDisplayProp(
    const escher::EscherProperty* property, TTShape* shape) {
    if (!property || !shape) return 0;
    // Blip → 显示属性
    return 0;
}

int EscherDrawingConvert::dispatchProperty(
    const escher::EscherProperty* property,
    std::vector<std::shared_ptr<TTShape>>& outShapes) {
    if (!property) return 0;
    // 访问者模式：根据属性类型派发到对应的处理方法
    return 0;
}

int EscherDrawingConvert::dispatchRecord(
    const escher::EscherRecord* record,
    std::vector<std::shared_ptr<TTShape>>& outShapes) {
    if (!record) return 0;

    // 根据记录类型派发
    std::uint16_t recType = record->type();

    switch (recType) {
        case 0xF000:  // DggContainer - 顶层容器，递归处理子记录
        case 0xF002:  // DGContainer
        case 0xF003:  // SpgrContainer
        case 0xF004:  // SpContainer
            // 容器记录：遍历子记录
            // 简化实现：仅记录存在，完整实现需要枚举子记录
            break;

        case 0xF005:  // OPT
            // 形状属性表
            break;

        case 0xF006:  // TertiaryFOPT
            break;

        case 0xF008:  // ChildAnchor
            break;

        case 0xF009:  // ClientAnchor
            break;

        case 0xF00A:  // ClientData
            break;

        default:
            // 其他记录类型：忽略
            break;
    }

    return 0;
}

std::string EscherDrawingConvert::getPicPathFromList(int blipRef) {
    // 从 DrawingContext 的图片列表中获取路径
    // 完整实现需要 DrawingContext 配合
    (void)blipRef;  // 未使用
    return "";
}

void EscherDrawingConvert::convertAnchor(
    ShapePr* shapePr,
    const OptionalDimension& parentSize,
    const IOfficeArtClientAnchor* clientAnchor,
    const escher::EscherChildAnchorRecord* childAnchor,
    const escher::EscherSpgrRecord* spgrRecord,
    TTShape* shape,
    const escher::EscherSimpleProperty* simpleProp) {
    if (!shapePr) return;

    // 锚点转换逻辑：
    // 1. 若有 ChildAnchor，从其中读取 left/top/right/bottom
    // 2. 若有 ClientAnchor，调用 IOfficeArtClientAnchor::fillFields
    // 3. 应用 UNIT_DIFFERENCE 缩放（如需要）
    // 完整实现需要 IOfficeArtClientAnchor 接口（D12）

    (void)parentSize;
    (void)clientAnchor;
    (void)childAnchor;
    (void)spgrRecord;
    (void)shape;
    (void)simpleProp;
}

} // namespace drawing
} // namespace office
} // namespace zq
