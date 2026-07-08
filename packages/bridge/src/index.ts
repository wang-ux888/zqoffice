/**
 * @zqoffice/bridge — 统一桥接层
 *
 * 连接 twc（C++ 转换引擎）和 edit（TypeScript 编辑引擎），
 * 提供面向用户的统一 API。
 *
 * @example
 * ```typescript
 * import { ZQOffice } from '@zqoffice/bridge';
 *
 * const zq = new ZQOffice({ workers: 4 });
 *
 * // 读取 → 编辑 → 保存
 * await zq.editXlsx('input.xlsx', (data) => {
 *   data.sheets[0].cells.push({ row: 0, column: 0, type: 'string', value: 'Hello' });
 *   return data;
 * }, 'output.xlsx');
 *
 * // 批量转换
 * await zq.batchConvert([
 *   { input: 'a.xlsx', output: 'a.json' },
 *   { input: 'b.xlsx', output: 'b.json' },
 * ], 'xlsx');
 *
 * await zq.destroy();
 * ```
 */

import { OfficeWorkerPool, WorkerPoolOptions } from './worker-pool';
import { Converter } from './converter';
import type {
  ZQSheetDocument,
  ZQSlideDocument,
  ZQDocDocument,
  EditOperation,
  ConvertTask,
  ConvertResult,
} from './types';

export { Converter } from './converter';
export { OfficeWorkerPool } from './worker-pool';
export * from './types';

export interface ZQOfficeOptions extends WorkerPoolOptions {
  /** 是否自动初始化引擎（默认 true） */
  autoInit?: boolean;
}

export class ZQOffice {
  private pool: OfficeWorkerPool;
  private converter: Converter;

  constructor(options: ZQOfficeOptions = {}) {
    this.pool = new OfficeWorkerPool(options);
    this.converter = new Converter(this.pool);
  }

  // =================================================================
  // 转换 API（通过 Worker Pool 并发执行）
  // =================================================================

  /** xlsx 文件 → JSON 对象 */
  async xlsxToJson(filePath: string, options?: Record<string, any>): Promise<ZQSheetDocument> {
    return this.converter.xlsxToJson(filePath, options);
  }

  /** JSON 对象 → xlsx 文件 */
  async jsonToXlsx(data: ZQSheetDocument, outputPath: string, options?: Record<string, any>): Promise<void> {
    return this.converter.jsonToXlsx(data, outputPath, options);
  }

  /** pptx 文件 → JSON 对象 */
  async pptxToJson(filePath: string, options?: Record<string, any>): Promise<ZQSlideDocument> {
    return this.converter.pptxToJson(filePath, options);
  }

  /** JSON 对象 → pptx 文件 */
  async jsonToPptx(data: ZQSlideDocument, outputPath: string, options?: Record<string, any>): Promise<void> {
    return this.converter.jsonToPptx(data, outputPath, options);
  }

  /** docx 文件 → JSON 对象 */
  async docxToJson(filePath: string, options?: Record<string, any>): Promise<ZQDocDocument> {
    return this.converter.docxToJson(filePath, options);
  }

  /** JSON 对象 → docx 文件 */
  async jsonToDocx(data: ZQDocDocument, outputPath: string, options?: Record<string, any>): Promise<void> {
    return this.converter.jsonToDocx(data, outputPath, options);
  }

  /** 检测文件格式 */
  async detectFormat(filePath: string): Promise<number> {
    return this.converter.detectFormat(filePath);
  }

  /** 通用导出：Office → JSON 文件 */
  async exportToJson(sourcePath: string, outputPath: string, options?: Record<string, any>): Promise<string> {
    return this.converter.exportToJson(sourcePath, outputPath, options);
  }

  /** 通用导入：JSON → Office 文件 */
  async importFromJson(jsonStr: string, outputPath: string, options?: Record<string, any>): Promise<string> {
    return this.converter.importFromJson(jsonStr, outputPath, options);
  }

  /** 格式转换：Office → Office */
  async convertFile(sourcePath: string, outputPath: string, format: string, options?: Record<string, any>): Promise<string> {
    return this.converter.convertFile(sourcePath, outputPath, format, options);
  }

  // =================================================================
  // 一体化管线（读取 → 编辑函数 → 保存）
  // =================================================================

  /**
   * 读取 xlsx → 执行编辑回调 → 保存
   *
   * @example
   * await zq.editXlsx('input.xlsx', (data) => {
   *   data.sheets[0].cells.push({ row: 0, column: 0, type: 'string', value: 'Hello' });
   *   return data;
   * }, 'output.xlsx');
   */
  async editXlsx(
    inputPath: string,
    modifier: (data: ZQSheetDocument) => ZQSheetDocument,
    outputPath: string,
    options?: Record<string, any>,
  ): Promise<void> {
    const data = await this.xlsxToJson(inputPath, options);
    const modified = modifier(data);
    await this.jsonToXlsx(modified, outputPath, options);
  }

  /**
   * 读取 pptx → 执行编辑回调 → 保存
   */
  async editPptx(
    inputPath: string,
    modifier: (data: ZQSlideDocument) => ZQSlideDocument,
    outputPath: string,
    options?: Record<string, any>,
  ): Promise<void> {
    const data = await this.pptxToJson(inputPath, options);
    const modified = modifier(data);
    await this.jsonToPptx(modified, outputPath, options);
  }

  /**
   * 读取 docx → 执行编辑回调 → 保存
   */
  async editDocx(
    inputPath: string,
    modifier: (data: ZQDocDocument) => ZQDocDocument,
    outputPath: string,
    options?: Record<string, any>,
  ): Promise<void> {
    const data = await this.docxToJson(inputPath, options);
    const modified = modifier(data);
    await this.jsonToDocx(modified, outputPath, options);
  }

  // =================================================================
  // 操作序列执行（Agent 友好）
  // =================================================================

  /**
   * 执行操作序列（Agent 通过 JSON 描述操作）
   *
   * @example
   * await zq.applyOperations('input.xlsx', 'output.xlsx', [
   *   { action: 'setCell', params: { sheet: 0, row: 0, col: 0, value: 'Hello' } },
   *   { action: 'insertRow', params: { sheet: 0, index: 5, count: 3 } },
   *   { action: 'setFormula', params: { sheet: 0, row: 0, col: 1, formula: '=A1*2' } },
   * ]);
   */
  async applyOperations(
    inputPath: string,
    outputPath: string,
    operations: EditOperation[],
  ): Promise<void> {
    await this.editXlsx(inputPath, (data) => {
      for (const op of operations) {
        this.applyOperation(data, op);
      }
      return data;
    }, outputPath);
  }

  private applyOperation(data: ZQSheetDocument, op: EditOperation): void {
    // addSheet 和 setCoreProperties 不需要现有 sheet
    if (op.action === 'addSheet') {
      const { name } = op.params;
      data.sheets.push({
        sheetId: `sheet_${Date.now()}`,
        name: name ?? `Sheet${data.sheets.length + 1}`,
        index: data.sheets.length,
        rowCount: 100,
        columnCount: 26,
        defaultRowHeight: 24,
        defaultColumnWidth: 88,
        cells: [],
        mergedCells: [],
        columnWidths: [],
        rowHeights: [],
        images: [],
        charts: [],
        frozenRowCount: 0,
        frozenColumnCount: 0,
      });
      return;
    }

    if (op.action === 'setCoreProperties') {
      data.coreProperties = { ...data.coreProperties, ...op.params };
      return;
    }

    const sheetIndex = op.params.sheet ?? 0;
    const sheet = data.sheets[sheetIndex];
    if (!sheet) throw new Error(`Sheet index ${sheetIndex} not found`);

    switch (op.action) {
      case 'setCell': {
        const { row, col, value, type } = op.params;
        const existing = sheet.cells.findIndex((c) => c.row === row && c.column === col);
        const cell = { row, column: col, type: type ?? 'string', value };
        if (existing !== -1) {
          sheet.cells[existing] = cell;
        } else {
          sheet.cells.push(cell);
        }
        break;
      }
      case 'setFormula': {
        const { row, col, formula } = op.params;
        const existing = sheet.cells.findIndex((c) => c.row === row && c.column === col);
        const cell = { row, column: col, type: 'formula' as const, value: formula, formula };
        if (existing !== -1) {
          sheet.cells[existing] = cell;
        } else {
          sheet.cells.push(cell);
        }
        break;
      }
      case 'insertRow': {
        const { index, count = 1 } = op.params;
        // 移动单元格
        for (const cell of sheet.cells) {
          if (cell.row >= index) cell.row += count;
        }
        // 同步合并单元格
        for (const mc of sheet.mergedCells) {
          if (mc.startRow >= index) mc.startRow += count;
          if (mc.endRow >= index) mc.endRow += count;
        }
        // 同步行高
        for (const rh of sheet.rowHeights) {
          if (rh.row >= index) rh.row += count;
        }
        // 同步冻结行数
        if (sheet.frozenRowCount !== undefined && sheet.frozenRowCount >= index) {
          sheet.frozenRowCount += count;
        }
        sheet.rowCount += count;
        break;
      }
      case 'insertColumn': {
        const { index, count = 1 } = op.params;
        // 移动单元格
        for (const cell of sheet.cells) {
          if (cell.column >= index) cell.column += count;
        }
        // 同步合并单元格
        for (const mc of sheet.mergedCells) {
          if (mc.startColumn >= index) mc.startColumn += count;
          if (mc.endColumn >= index) mc.endColumn += count;
        }
        // 同步列宽
        for (const cw of sheet.columnWidths) {
          if (cw.column >= index) cw.column += count;
        }
        // 同步冻结列数
        if (sheet.frozenColumnCount !== undefined && sheet.frozenColumnCount >= index) {
          sheet.frozenColumnCount += count;
        }
        sheet.columnCount += count;
        break;
      }
      case 'deleteRow': {
        const { index, count = 1 } = op.params;
        // 移除范围内的单元格, 后续行号上移
        sheet.cells = sheet.cells.filter((c) => c.row < index || c.row >= index + count);
        for (const cell of sheet.cells) {
          if (cell.row >= index + count) cell.row -= count;
        }
        // 同步合并单元格: 移除完全被覆盖的, 调整其余
        sheet.mergedCells = sheet.mergedCells
          .filter((mc) => !(mc.startRow >= index && mc.endRow < index + count))
          .map((mc) => {
            let sr = mc.startRow;
            let er = mc.endRow;
            if (sr >= index + count) sr -= count;
            else if (sr >= index) sr = index;
            if (er >= index + count) er -= count;
            else if (er >= index) er = index - 1;
            return { ...mc, startRow: sr, endRow: er };
          });
        // 同步行高: 移除范围内的, 后续行号上移
        sheet.rowHeights = sheet.rowHeights
          .filter((rh) => rh.row < index || rh.row >= index + count)
          .map((rh) => ({ ...rh, row: rh.row >= index + count ? rh.row - count : rh.row }));
        // 同步冻结行数
        if (sheet.frozenRowCount !== undefined) {
          if (sheet.frozenRowCount >= index + count) {
            sheet.frozenRowCount = Math.max(0, sheet.frozenRowCount - count);
          } else if (sheet.frozenRowCount > index) {
            sheet.frozenRowCount = index;
          }
        }
        sheet.rowCount = Math.max(0, sheet.rowCount - count);
        break;
      }
      case 'deleteColumn': {
        const { index, count = 1 } = op.params;
        // 移除范围内的单元格, 后续列号左移
        sheet.cells = sheet.cells.filter((c) => c.column < index || c.column >= index + count);
        for (const cell of sheet.cells) {
          if (cell.column >= index + count) cell.column -= count;
        }
        // 同步合并单元格: 移除完全被覆盖的, 调整其余
        sheet.mergedCells = sheet.mergedCells
          .filter((mc) => !(mc.startColumn >= index && mc.endColumn < index + count))
          .map((mc) => {
            let sc = mc.startColumn;
            let ec = mc.endColumn;
            if (sc >= index + count) sc -= count;
            else if (sc >= index) sc = index;
            if (ec >= index + count) ec -= count;
            else if (ec >= index) ec = index - 1;
            return { ...mc, startColumn: sc, endColumn: ec };
          });
        // 同步列宽: 移除范围内的, 后续列号左移
        sheet.columnWidths = sheet.columnWidths
          .filter((cw) => cw.column < index || cw.column >= index + count)
          .map((cw) => ({ ...cw, column: cw.column >= index + count ? cw.column - count : cw.column }));
        // 同步冻结列数
        if (sheet.frozenColumnCount !== undefined) {
          if (sheet.frozenColumnCount >= index + count) {
            sheet.frozenColumnCount = Math.max(0, sheet.frozenColumnCount - count);
          } else if (sheet.frozenColumnCount > index) {
            sheet.frozenColumnCount = index;
          }
        }
        sheet.columnCount = Math.max(0, sheet.columnCount - count);
        break;
      }
      case 'setCellStyle': {
        const { row, col, style } = op.params;
        const existing = sheet.cells.findIndex((c) => c.row === row && c.column === col);
        if (existing !== -1) {
          sheet.cells[existing].style = { ...(sheet.cells[existing].style ?? {}), ...style };
        } else {
          // 单元格不存在时创建 (与 setCell 行为一致)
          sheet.cells.push({ row, column: col, type: 'string', value: '', style: { ...style } });
        }
        break;
      }
      case 'renameSheet': {
        const { name } = op.params;
        sheet.name = name;
        break;
      }
      default:
        throw new Error(`Unknown operation: ${op.action}`);
    }
  }

  // =================================================================
  // 批量操作
  // =================================================================

  /** 批量转换（自动并发） */
  async batchConvert(tasks: ConvertTask[], format: 'xlsx' | 'pptx' | 'docx'): Promise<ConvertResult> {
    return this.converter.batchConvert(tasks, format);
  }

  // =================================================================
  // 生命周期
  // =================================================================

  /** Worker Pool 状态 */
  get status() {
    return {
      workers: this.pool.size,
      pending: this.pool.pending,
    };
  }

  /** 销毁所有 Worker */
  async destroy() {
    await this.pool.destroy();
  }
}
