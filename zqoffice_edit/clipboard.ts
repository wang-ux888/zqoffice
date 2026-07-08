/**
 * 剪贴板模块 — 系统剪贴板交互 + 内部富格式缓存
 * 全部原创实现
 *
 * 职责:
 *   1. 复制选区到系统剪贴板 (TSV + HTML + 自定义 JSON)
 *   2. 粘贴时解析多种格式 (TSV/HTML/纯文本)
 *   3. 内部缓存保留单元格样式 (跨应用复制时丢失样式是正常的)
 *   4. 剪切支持
 *   5. 选择性粘贴 (仅值/仅样式/仅公式)
 */

import { SheetModel, Cell, CellStyle } from './data-model';
import { RenderEngine, SelectionRange } from './render-engine';
import { FormulaEngine } from './formula-engine';
import { EditEngine } from './edit-engine';

/** 粘贴模式 */
export type PasteMode = 'all' | 'values' | 'styles' | 'formulas';

/** 剪贴板数据 */
export interface ClipboardData {
  /** TSV 文本 (用于系统剪贴板) */
  tsv: string;
  /** HTML 表格 (用于富文本剪贴板) */
  html: string;
  /** 内部格式 (保留完整样式和公式) */
  internal: {
    cells: ClipboardCell[][];
    sourceRange: SelectionRange;
    isCut: boolean;
  };
}

/** 剪贴板单元格 */
export interface ClipboardCell {
  value: string | number | boolean;
  type: string;
  formula?: string;
  style?: CellStyle;
}

export class ClipboardManager {
  private sheet: SheetModel;
  private renderEngine: RenderEngine;
  private formulaEngine: FormulaEngine;
  private editEngine: EditEngine;

  private internalClip: ClipboardData | null = null;

  constructor(
    sheet: SheetModel,
    renderEngine: RenderEngine,
    formulaEngine: FormulaEngine,
    editEngine: EditEngine
  ) {
    this.sheet = sheet;
    this.renderEngine = renderEngine;
    this.formulaEngine = formulaEngine;
    this.editEngine = editEngine;
  }

  // ============================================================
  // 复制
  // ============================================================

  /**
   * 复制当前选区
   */
  async copy(): Promise<void> {
    const sel = this.renderEngine.getSelection();
    if (!sel) return;

    const cells = this.readRange(sel);
    const tsv = this.cellsToTSV(cells);
    const html = this.cellsToHTML(cells, sel);

    this.internalClip = {
      tsv,
      html,
      internal: {
        cells,
        sourceRange: { ...sel },
        isCut: false,
      },
    };

    // 写入系统剪贴板
    await this.writeSystemClipboard(tsv, html);
  }

  /**
   * 剪切当前选区
   */
  async cut(): Promise<void> {
    await this.copy();
    if (this.internalClip) {
      this.internalClip.internal.isCut = true;
    }
  }

  // ============================================================
  // 粘贴
  // ============================================================

  /**
   * 粘贴到当前选区
   * @param mode 粘贴模式
   */
  async paste(mode: PasteMode = 'all'): Promise<void> {
    const sel = this.renderEngine.getSelection();
    if (!sel) return;

    // 优先使用内部剪贴板 (保留样式)
    if (this.internalClip) {
      this.pasteInternal(sel.startRow, sel.startCol, this.internalClip, mode);
      if (this.internalClip.internal.isCut) {
        // 剪切模式: 清空源
        this.clearRange(this.internalClip.internal.sourceRange);
        this.internalClip.internal.isCut = false;
      }
      return;
    }

    // 回退到系统剪贴板
    const text = await this.readSystemClipboard();
    if (text) {
      this.pasteTSV(sel.startRow, sel.startCol, text, mode);
    }
  }

  /**
   * 粘贴纯文本 (TSV)
   */
  pasteTSV(startRow: number, startCol: number, tsv: string, mode: PasteMode = 'all'): void {
    const lines = tsv.split('\n').filter(line => line !== '' || mode === 'all');
    for (let r = 0; r < lines.length; r++) {
      const cols = lines[r].split('\t');
      for (let c = 0; c < cols.length; c++) {
        const tr = startRow + r;
        const tc = startCol + c;
        if (tr >= this.sheet.rowCount || tc >= this.sheet.columnCount) continue;

        const val = cols[c];
        if (mode === 'values' || mode === 'all') {
          if (val.startsWith('=')) {
            this.sheet.setCellFormula(tr, tc, val.slice(1));
            const result = this.formulaEngine.calculate(val.slice(1));
            const cell = this.sheet.getCell(tr, tc);
            if (cell) cell.value = result as string | number | boolean;
          } else {
            const num = Number(val);
            if (val !== '' && !isNaN(num)) {
              this.sheet.setCellValue(tr, tc, num, 'number');
            } else {
              this.sheet.setCellValue(tr, tc, val, 'string');
            }
          }
        }
      }
    }
    this.editEngine.recalcAndRefresh();
    this.renderEngine.refresh();
  }

  /**
   * 粘贴内部格式 (保留样式)
   */
  private pasteInternal(startRow: number, startCol: number, clip: ClipboardData, mode: PasteMode): void {
    const src = clip.internal.cells;
    const srcRange = clip.internal.sourceRange;

    for (let r = 0; r < src.length; r++) {
      for (let c = 0; c < src[r].length; c++) {
        const tr = startRow + r;
        const tc = startCol + c;
        if (tr >= this.sheet.rowCount || tc >= this.sheet.columnCount) continue;

        const cell = src[r][c];
        if (!cell) continue;

        // 计算公式引用偏移
        const offsetRow = tr - srcRange.startRow;
        const offsetCol = tc - srcRange.startCol;

        if (mode === 'values' || mode === 'all') {
          if (cell.formula) {
            const adjusted = this.adjustFormulaRefs(cell.formula, offsetRow, offsetCol);
            this.sheet.setCellFormula(tr, tc, adjusted);
            const result = this.formulaEngine.calculate(adjusted);
            const updated = this.sheet.getCell(tr, tc);
            if (updated) updated.value = result as string | number | boolean;
          } else {
            this.sheet.setCellValue(tr, tc, cell.value, cell.type as Cell['type']);
          }
        }

        if ((mode === 'styles' || mode === 'all') && cell.style) {
          this.sheet.setCellStyle(tr, tc, cell.style);
        }

        if (mode === 'formulas' && cell.formula) {
          const adjusted = this.adjustFormulaRefs(cell.formula, offsetRow, offsetCol);
          this.sheet.setCellFormula(tr, tc, adjusted);
          const result = this.formulaEngine.calculate(adjusted);
          const updated = this.sheet.getCell(tr, tc);
          if (updated) updated.value = result as string | number | boolean;
        }
      }
    }

    this.renderEngine.refresh();
  }

  // ============================================================
  // 读取选区数据
  // ============================================================

  /**
   * 读取选区范围内的单元格数据
   */
  private readRange(range: SelectionRange): ClipboardCell[][] {
    const result: ClipboardCell[][] = [];
    for (let r = range.startRow; r <= range.endRow; r++) {
      const row: ClipboardCell[] = [];
      for (let c = range.startCol; c <= range.endCol; c++) {
        const cell = this.sheet.getCell(r, c);
        if (cell) {
          row.push({
            value: cell.value,
            type: cell.type,
            formula: cell.formula,
            style: cell.style,
          });
        } else {
          row.push({ value: '', type: 'string' });
        }
      }
      result.push(row);
    }
    return result;
  }

  /**
   * 单元格数据转 TSV
   */
  private cellsToTSV(cells: ClipboardCell[][]): string {
    return cells.map(row =>
      row.map(cell => {
        if (cell.formula) return `=${cell.formula}`;
        const v = String(cell.value);
        // 包含 tab/换行时用引号包裹
        if (v.includes('\t') || v.includes('\n') || v.includes('"')) {
          return `"${v.replace(/"/g, '""')}"`;
        }
        return v;
      }).join('\t')
    ).join('\n');
  }

  /**
   * 单元格数据转 HTML 表格
   */
  private cellsToHTML(cells: ClipboardCell[][], range: SelectionRange): string {
    const rows = cells.map(row => {
      const tds = row.map(cell => {
        const styleStr = this.styleToInlineCSS(cell.style);
        const val = cell.formula ? `=${cell.formula}` : this.escapeHTML(String(cell.value));
        return `<td style="${styleStr}">${val}</td>`;
      }).join('');
      return `<tr>${tds}</tr>`;
    }).join('');
    return `<table>${rows}</table>`;
  }

  // ============================================================
  // 系统剪贴板交互
  // ============================================================

  /**
   * 写入系统剪贴板 (TSV + HTML)
   */
  private async writeSystemClipboard(tsv: string, html: string): Promise<void> {
    if (!navigator.clipboard || !navigator.clipboard['write']) {
      // 回退: 只写文本
      try {
        await navigator.clipboard.writeText(tsv);
      } catch { /* 静默失败 */ }
      return;
    }

    try {
      const clipboardItem = new (window as any).ClipboardItem({
        'text/plain': new Blob([tsv], { type: 'text/plain' }),
        'text/html': new Blob([html], { type: 'text/html' }),
      });
      await navigator.clipboard['write']([clipboardItem]);
    } catch {
      // 回退: 只写文本
      try { await navigator.clipboard.writeText(tsv); } catch { /* 静默 */ }
    }
  }

  /**
   * 读取系统剪贴板文本
   */
  private async readSystemClipboard(): Promise<string | null> {
    if (!navigator.clipboard) return null;
    try {
      return await navigator.clipboard.readText();
    } catch {
      return null;
    }
  }

  // ============================================================
  // 辅助方法
  // ============================================================

  /** 清空范围内容 */
  private clearRange(range: SelectionRange): void {
    for (let r = range.startRow; r <= range.endRow; r++) {
      for (let c = range.startCol; c <= range.endCol; c++) {
        this.sheet.clearCellContent(r, c);
      }
    }
    this.renderEngine.refresh();
  }

  /**
   * 调整公式相对引用 (与 edit-engine 同逻辑, 独立实现避免循环依赖)
   */
  private adjustFormulaRefs(formula: string, offsetRow: number, offsetCol: number): string {
    return formula.replace(/(\$?)([A-Z]+)(\$?)(\d+)/g, (match, colAbs, col, rowAbs, row) => {
      const colIdx = SheetModel.letterToColumn(col);
      const rowIdx = parseInt(row) - 1;
      const newCol = colAbs ? col : SheetModel.columnToLetter(colIdx + offsetCol);
      const newRow = rowAbs ? row : String(rowIdx + 1 + offsetRow);
      return `${colAbs}${newCol}${rowAbs}${newRow}`;
    });
  }

  /** CellStyle 转内联 CSS */
  private styleToInlineCSS(style?: CellStyle): string {
    if (!style) return '';
    const parts: string[] = [];
    if (style.fontFamily) parts.push(`font-family: ${style.fontFamily}`);
    if (style.fontSize) parts.push(`font-size: ${style.fontSize}px`);
    if (style.bold) parts.push('font-weight: bold');
    if (style.italic) parts.push('font-style: italic');
    if (style.color) parts.push(`color: ${style.color}`);
    if (style.backgroundColor) parts.push(`background-color: ${style.backgroundColor}`);
    if (style.horizontalAlignment) parts.push(`text-align: ${style.horizontalAlignment}`);
    return parts.join('; ');
  }

  /** HTML 转义 */
  private escapeHTML(text: string): string {
    return text
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;');
  }

  /** 是否有内部剪贴板数据 */
  hasInternalData(): boolean {
    return this.internalClip !== null;
  }

  /** 更新工作表引用 (供 sheet-engine 切换工作表时调用) */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
  }

  /** 清空内部剪贴板 */
  clear(): void {
    this.internalClip = null;
  }
}
