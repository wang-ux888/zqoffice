// =============================================================================
// zqoffice/index.d.ts
//
// ZQOffice TypeScript 类型声明
// =============================================================================

/** ExportResult JSON 结构 */
export interface ExportResult {
    /** 错误码：0=成功，2=InvalidArgument，3=FileNotFound，5=UnsupportedFormat，10=InternalError，11=NotImplemented，19=ConversionError */
    code: number;
    /** 错误描述（成功时为 "ok"） */
    message: string;
    /** 内层 JSON 字符串（结构因文档类型而异） */
    data: string;
    /** ZQOffice 版本号 */
    version: string;
}

/** PowerPointReader 不透明句柄（外部对象） */
export type PowerPointReader = unknown;

/** ZQOffice 引擎模块 */
export interface ZQOffice {
    // === 引擎生命周期 ===
    /** 初始化引擎 */
    zqofficeInitialize(workingDirectory: string | null, mode: number): void;
    /** 反初始化引擎 */
    zqofficeUninitialize(): void;
    /** 检测文件格式（返回 FileFormat 枚举值） */
    zqofficeDetectFileFormat(filePath: string): number;

    // === Excel：xlsx ↔ JSON ===
    /** xlsx 文件 → JSON */
    xlsxToZQSheet(
        filePath: string,
        dataDirectory: string | null,
        reserved: string | null,
        optionsJson: string
    ): string;
    /** JSON → xlsx 文件 */
    zqSheetToXlsx(
        clientVarsJson: string,
        outputPath: string,
        dataDirectory: string | null,
        reserved: string | null,
        optionsJson: string
    ): string;

    // === PowerPoint：pptx ↔ JSON ===
    /** pptx 文件 → JSON */
    pptxToZQSlide(
        filePath: string,
        dataDirectory: string | null,
        reserved: string | null,
        optionsJson: string
    ): string;
    /** JSON → pptx 文件 */
    zqSlideToPptx(
        clientVarsJson: string,
        outputPath: string,
        dataDirectory: string | null,
        reserved: string | null,
        optionsJson: string
    ): string;

    // === Word：docx ↔ JSON ===
    /** docx 文件 → JSON */
    docxToZQDoc(
        filePath: string,
        dataDirectory: string | null,
        reserved: string | null,
        optionsJson: string
    ): string;
    /** JSON → docx 文件 */
    zqDocToDocx(
        clientVarsJson: string,
        outputPath: string,
        dataDirectory: string | null,
        reserved: string | null,
        optionsJson: string
    ): string;

    // === 通用导入/导出 ===
    /** 通用导出：Office 文件 → JSON 文件 */
    zqofficeExport(
        sourcePath: string,
        outputPath: string,
        optionsJson: string
    ): string;
    /** 通用导入：JSON 字符串 → Office 文件 */
    zqofficeImport(
        clientVarsJson: string,
        outputPath: string,
        optionsJson: string
    ): string;
    /** 通用文件导出：Office 文件 → 指定格式 Office 文件 */
    zqExportFile(
        sourcePath: string,
        outputPath: string,
        formatName: string,
        optionsJson: string
    ): string;

    // === PowerPoint 流式读取器 ===
    /** 创建 PowerPoint 流式读取器（失败返回 null） */
    createPowerPointReader(filePath: string): PowerPointReader | null;
    /** 销毁 PowerPoint 流式读取器（也可由 GC 自动销毁） */
    destroyPowerPointReader(reader: PowerPointReader): void;

    // === 内存管理 ===
    /** 释放字符串缓冲（通常不需要手动调用，绑定层已自动管理） */
    freeStringBuffer(buffer: unknown): void;
}

declare const zqoffice: ZQOffice;
export default zqoffice;
