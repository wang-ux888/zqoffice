// =============================================================================
// src/drawing/escher_drawing_convert.h
//
// EscherDrawingConvert 类：Escher 二进制记录 → TTShape 转换器
//
//   入口：ConvertEscherShape / ConvertNew / ConvertBg / ConvertSlideBg
//   容器记录：ConvertSpContainerRecord / ConvertDgRecord /
//             ConvertSpgrContainerRecord / ConvertSpgrRecord / ConvertSpRecord
//   锚点：ConvertChildAnchorRecord / ConvertClientAnchorRecord /
//         ConvertClientDataRecord / ConvertAnchor
//   文本：ConvertClientTextbox / ConvertMsoAnchor / ConvertMsoWrap / ConvertTextFlow
//   OptRecord：ConvertAbstractOptRecord / DispatchProperty / DispatchRecord
//   形状构造：ConvertSpFreeForm / ConvertSpPictureFrame / ConvertSpPrst
//   Blip：ConvertBlipToClip / ConvertBlipToDisplayProp / GetPicPathFromList
//   变换：ConvertTransform（静态）
//
// UNIT_DIFFERENCE 常量：
//   EMU 与 doc coordinate system 之间的单位差值，用于旧版 Office（97-2003）
//   文档坐标使用 1/100 mm，EMU 是 1/360000 cm，二者比值 360。
// =============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "drawing/tt_shape.h"
#include "drawing/shape_pr.h"
#include "drawing/text_body.h"

namespace zq {
namespace office {
namespace drawing {

// ---------------------------------------------------------------------------
// 前向声明（接口在 D11/D12 实现，此处仅作指针引用）
// ---------------------------------------------------------------------------
class IOfficeArtClientTextbox;
class IOfficeArtClientAnchor;
class IOfficeArtClientData;
class IClient;
class DrawingContext;

} // namespace drawing

namespace escher {
class EscherRecord;
class EscherOptRecord;
class EscherTertiaryOptRecord;
class EscherChildAnchorRecord;
class EscherSpgrRecord;
class EscherSpRecord;
class EscherDgRecord;
class EscherContainerRecord;
class EscherProperty;
class EscherSimpleProperty;
} // namespace escher

namespace drawing {

// ---------------------------------------------------------------------------
// MSO 文本锚定方式（[MS-ODRAW] 2.1.2 MSOANCHOR）
// ---------------------------------------------------------------------------
enum class MsoAnchor : std::uint8_t {
    kTop = 0,           // 顶端对齐
    kMiddle = 1,        // 居中对齐
    kBottom = 2,        // 底端对齐
    kTopCentered = 3,   // 顶端居中
    kMiddleCentered = 4,// 中部居中
    kBottomCentered = 5,// 底端居中
};

// ---------------------------------------------------------------------------
// MSO 文本换行模式（[MS-ODRAW] 2.1.3 MSOWRAPMODE）
// ---------------------------------------------------------------------------
enum class MsoWrapMode : std::uint8_t {
    kSquare = 0,        // 方形换行
    kByPoints = 1,      // 按顶点换行
    kNone = 2,          // 不换行
    kTopBottom = 3,     // 上下环绕
};

// ---------------------------------------------------------------------------
// MSO 文本流向（[MS-ODRAW] 2.1.4 MSOTXFL）
// ---------------------------------------------------------------------------
enum class MsoTxfl : std::uint8_t {
    kHorz = 0,              // 水平
    kVertN = 1,             // 垂直（自上而下）
    kVertBrz = 2,           // 垂直巴西版
    kVert270 = 3,           // 垂直 270 度
    kArtVert = 4,           // 艺术垂直
};

// ---------------------------------------------------------------------------
// 可选的尺寸（用于 SpContainer 转换时的父尺寸）
// ---------------------------------------------------------------------------
struct OptionalDimension {
    bool valid = false;
    std::int32_t width = 0;
    std::int32_t height = 0;
};

/// Escher 二进制 → TTShape 转换器
///
///   - 入口方法接受 EscherRecord* 并返回 TTShape 列表
///   - 通过 IClient/DrawingContext 获取文档上下文（图片路径等）
///   - 通过 IOfficeArtClient* 接口回调客户端特定解析
class EscherDrawingConvert {
public:
    EscherDrawingConvert();
    ~EscherDrawingConvert();

    // -----------------------------------------------------------------------
    // 入口方法（public）
    // -----------------------------------------------------------------------

    /// 主入口：将 Escher 记录转为 TTShape 列表
    /// @param record  Escher 记录（通常为 DggContainer 或 SpgrContainer）
    /// @return 转换后的 TTShape 列表（调用方负责释放）
    std::vector<std::shared_ptr<TTShape>> convertEscherShape(
        const escher::EscherRecord* record);

    /// 转换新形状（单记录）
    std::unique_ptr<TTShape> convertNew(const escher::EscherRecord* record);

    /// 转换背景形状
    std::unique_ptr<TTShape> convertBg(const escher::EscherRecord* record,
                                         int width, int height);

    /// 转换 slide 背景
    std::unique_ptr<TTShape> convertSlideBg(
        const escher::EscherRecord* record,
        const escher::EscherRecord* pptRecord,
        int width, int height);

    /// 转 SpContainer
    std::unique_ptr<TTShape> convertSpContainerRecord(
        const escher::EscherContainerRecord* container,
        const OptionalDimension& parentSize,
        TTShape* parentShape,
        const escher::EscherSpgrRecord* spgrRecord);

    // -----------------------------------------------------------------------
    // 静态方法（public）
    // -----------------------------------------------------------------------

    /// 应用变换（x/y 缩放 + 旋转）到 ShapePr
    /// @param scaleX/scaleY 缩放因子
    /// @param rotateAngle 旋转角度（度）
    /// @param flipH/flipV 水平/垂直翻转
    /// @param shapePr 形状属性（输出）
    static void convertTransform(float scaleX, float scaleY,
                                   float rotateAngle,
                                   bool flipH, bool flipV,
                                   ShapePr* shapePr);

    /// MSO 锚定 → TextBody 垂直对齐
    static void convertMsoAnchor(MsoAnchor anchor, TextBody* textBody);

    /// MSO 换行 → TextBody 换行
    static void convertMsoWrap(MsoWrapMode wrap, TextBody* textBody);

    /// MSO 文本流向 → TextBody 文本流向
    static void convertTextFlow(MsoTxfl flow, TextBody* textBody);

    // -----------------------------------------------------------------------
    // 常量
    // -----------------------------------------------------------------------

    /// UNIT_DIFFERENCE: 旧版 Office（97-2003）二进制坐标与 EMU 之间的单位差
    ///   旧版 Office 内部坐标 = EMU / UNIT_DIFFERENCE
    ///   即旧坐标缩放因子 = 1/UNIT_DIFFERENCE
    ///   EMU 是 914400/英寸；旧 Office 内部是 12700/英寸
    ///   UNIT_DIFFERENCE = 914400 / 12700 = 72
    static constexpr std::int32_t UNIT_DIFFERENCE = 72;

    // -----------------------------------------------------------------------
    // 客户端注入（D11-D14 之后可用，此处保留接口）
    // -----------------------------------------------------------------------

    void setClient(IClient* client) { client_ = client; }
    void setDrawingContext(DrawingContext* ctx) { drawingCtx_ = ctx; }
    void setClientTextbox(IOfficeArtClientTextbox* tb) { clientTextbox_ = tb; }
    void setClientAnchor(IOfficeArtClientAnchor* an) { clientAnchor_ = an; }
    void setClientData(IOfficeArtClientData* cd) { clientData_ = cd; }

private:
    IClient* client_ = nullptr;
    DrawingContext* drawingCtx_ = nullptr;
    IOfficeArtClientTextbox* clientTextbox_ = nullptr;
    IOfficeArtClientAnchor* clientAnchor_ = nullptr;
    IOfficeArtClientData* clientData_ = nullptr;

    // 标志：是否为 PPT 上下文 / 是否为新解析模式
    bool isPpt_ = false;
    bool newMode_ = false;

    // -----------------------------------------------------------------------
    // 内部转换方法（private）
    // -----------------------------------------------------------------------

    /// 转 DgRecord
    int convertDgRecord(const escher::EscherDgRecord* dgRecord,
                         std::vector<std::shared_ptr<TTShape>>& outShapes);

    /// 转 SpgrContainerRecord
    int convertSpgrContainerRecord(
        const escher::EscherContainerRecord* container,
        std::vector<std::shared_ptr<TTShape>>& outShapes,
        TTShape* parentShape,
        const escher::EscherSpgrRecord* spgrRecord);

    /// 转 SpgrRecord（组合形状）
    int convertSpgrRecord(const escher::EscherSpgrRecord* spgrRecord,
                            std::vector<std::shared_ptr<TTShape>>& outShapes);

    /// 转 SpRecord（普通形状）
    int convertSpRecord(const escher::EscherSpRecord* spRecord,
                          std::vector<std::shared_ptr<TTShape>>& outShapes);

    /// 转 ChildAnchorRecord（子形状锚点）
    int convertChildAnchorRecord(
        const escher::EscherChildAnchorRecord* anchor,
        const escher::EscherSpgrRecord* spgrRecord,
        ShapePr* shapePr,
        TTShape* shape,
        float scale);

    /// 转 ClientAnchorRecord（客户端锚点）
    int convertClientAnchorRecord(
        const IOfficeArtClientAnchor* clientAnchor,
        ShapePr* shapePr,
        float scale);

    /// 转 ClientDataRecord（客户端数据）
    int convertClientDataRecord(
        const IOfficeArtClientData* clientData,
        std::vector<std::shared_ptr<TTShape>>& outShapes);

    /// 转 ClientTextbox（客户端文本框）
    std::unique_ptr<TextBody> convertClientTextbox(
        const IOfficeArtClientTextbox* clientTb,
        const escher::EscherOptRecord* optRecord,
        const IOfficeArtClientData* clientData);

    /// 转抽象 OptRecord
    int convertAbstractOptRecord(
        const escher::EscherOptRecord* optRecord,
        std::vector<std::shared_ptr<TTShape>>& outShapes);

    /// 转自由形状
    TTShape* convertSpFreeForm(
        const escher::EscherOptRecord* optRecord,
        const IOfficeArtClientTextbox* clientTb,
        const IOfficeArtClientData* clientData,
        std::unique_ptr<ShapePr> shapePr);

    /// 转图片框
    TTShape* convertSpPictureFrame(
        const escher::EscherOptRecord* optRecord,
        std::unique_ptr<ShapePr> shapePr);

    /// 转预设形状
    TTShape* convertSpPrst(
        const std::string& prstName,
        const escher::EscherOptRecord* optRecord,
        const IOfficeArtClientTextbox* clientTb,
        const IOfficeArtClientData* clientData,
        const escher::EscherTertiaryOptRecord* tertiaryOpt,
        std::unique_ptr<ShapePr> shapePr);

    /// Blip → 剪贴
    int convertBlipToClip(const escher::EscherOptRecord* optRecord,
                            TTShape* shape);

    /// Blip → 显示属性
    int convertBlipToDisplayProp(const escher::EscherProperty* property,
                                   TTShape* shape);

    /// 派发属性（访问者模式）
    int dispatchProperty(const escher::EscherProperty* property,
                          std::vector<std::shared_ptr<TTShape>>& outShapes);

    /// 派发记录（访问者模式）
    int dispatchRecord(const escher::EscherRecord* record,
                         std::vector<std::shared_ptr<TTShape>>& outShapes);

    /// 从图片列表获取路径
    std::string getPicPathFromList(int blipRef);

    /// 锚点转换辅助
    void convertAnchor(ShapePr* shapePr,
                         const OptionalDimension& parentSize,
                         const IOfficeArtClientAnchor* clientAnchor,
                         const escher::EscherChildAnchorRecord* childAnchor,
                         const escher::EscherSpgrRecord* spgrRecord,
                         TTShape* shape,
                         const escher::EscherSimpleProperty* simpleProp);
};

} // namespace drawing
} // namespace office
} // namespace zq
