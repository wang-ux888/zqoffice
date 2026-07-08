/**
 * 边框管理器 — 选区边框设置/清除/查询 + 工具栏预览渲染
 * 全部原创实现
 *
 * 职责:
 *   1. 设置选区边框 setBorder(range, borderType, style?)
 *      支持 all/outer/inner/top/bottom/left/right/none/diagonalDown/diagonalUp
 *   2. 清除选区边框 clearBorder(range)
 *   3. 获取单元格边框 getCellBorder(row, col)
 *   4. 绘制边框预览 drawBorderPreview(...) — 用于工具栏边框选择器
 *
 * 边框应用逻辑:
 *   - 'all'           选区内每个单元格的四边都设置
 *   - 'outer'         只设置选区外边框 (顶部行上边框/底部行下边框/左侧列左边框/右侧列右边框)
 *   - 'inner'         只设置选区内边框 (相邻单元格之间的边框)
 *   - 'top'           只设置选区顶部外边框
 *   - 'bottom'        只设置选区底部外边框
 *   - 'left'          只设置选区左侧外边框
 *   - 'right'         只设置选区右侧外边框
 *   - 'none'          清除选区内所有边框 (含对角线)
 *   - 'diagonalDown'  设置每个单元格的左上→右下对角线
 *   - 'diagonalUp'    设置每个单元格的左下→右上对角线
 */

import { SheetModel, CellStyle, BorderStyle } from './data-model';

/** 边框类型 */
export type BorderType =
  | 'all'
  | 'outer'
  | 'top'
  | 'bottom'
  | 'left'
  | 'right'
  | 'inner'
  | 'none'
  | 'diagonalDown'
  | 'diagonalUp';

/** 边框样式选项 (用户可指定的线条样式) */
export interface BorderStyleOption {
  /** 颜色 #RRGGBB */
  color: string;
  /** 线型 */
  style: 'thin' | 'medium' | 'thick' | 'dashed' | 'dotted' | 'double';
}

/** 选区范围 (与 render-engine 的 SelectionRange 对齐) */
export interface BorderRange {
  startRow: number;
  startCol: number;
  endRow: number;
  endCol: number;
}

/** 单元格完整边框信息 (六个方向) */
export interface CellBorderInfo {
  top?: BorderStyle;
  bottom?: BorderStyle;
  left?: BorderStyle;
  right?: BorderStyle;
  diagonalDown?: BorderStyle;
  diagonalUp?: BorderStyle;
}

export class BorderManager {
  private sheet: SheetModel;

  constructor(sheet: SheetModel) {
    this.sheet = sheet;
  }

  /** 更新工作表引用 (切换工作表时调用) */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
  }

  // ============================================================
  // 设置选区边框
  // ============================================================

  /**
   * 设置选区边框
   * @param range 选区范围
   * @param borderType 边框类型
   * @param style 边框样式 (borderType 为 'none' 时忽略)
   */
  setBorder(range: BorderRange, borderType: BorderType, style?: BorderStyleOption): void {
    // 规范化选区 (确保 start <= end)
    const sr = Math.min(range.startRow, range.endRow);
    const er = Math.max(range.startRow, range.endRow);
    const sc = Math.min(range.startCol, range.endCol);
    const ec = Math.max(range.startCol, range.endCol);

    // 'none' — 清除所有边框
    if (borderType === 'none') {
      this.clearBorder(range);
      return;
    }

    // 构造 BorderStyle 对象
    const borderStyle: BorderStyle = style
      ? { style: style.style, color: style.color }
      : { style: 'thin', color: '#000000' };

    switch (borderType) {
      case 'all':
        this.applyAll(sr, sc, er, ec, borderStyle);
        break;
      case 'outer':
        this.applyOuter(sr, sc, er, ec, borderStyle);
        break;
      case 'inner':
        this.applyInner(sr, sc, er, ec, borderStyle);
        break;
      case 'top':
        this.applyTop(sr, sc, ec, borderStyle);
        break;
      case 'bottom':
        this.applyBottom(er, sc, ec, borderStyle);
        break;
      case 'left':
        this.applyLeft(sr, er, sc, borderStyle);
        break;
      case 'right':
        this.applyRight(sr, er, ec, borderStyle);
        break;
      case 'diagonalDown':
        this.applyDiagonal(sr, sc, er, ec, 'down', borderStyle);
        break;
      case 'diagonalUp':
        this.applyDiagonal(sr, sc, er, ec, 'up', borderStyle);
        break;
    }
  }

  // ============================================================
  // 边框应用 — 各类型的具体实现
  // ============================================================

  /**
   * 'all' — 选区内每个单元格的四边都设置
   * 内部边框线会被相邻两个单元格同时持有 (数据冗余但渲染一致)
   */
  private applyAll(sr: number, sc: number, er: number, ec: number, border: BorderStyle): void {
    for (let r = sr; r <= er; r++) {
      for (let c = sc; c <= ec; c++) {
        const patch: Partial<CellStyle> = {
          borderTop: border,
          borderRight: border,
          borderBottom: border,
          borderLeft: border,
        };
        this.sheet.setCellStyle(r, c, patch);
      }
    }
  }

  /**
   * 'outer' — 只设置选区外边框
   * 顶部行的上边框、底部行的下边框、左侧列的左边框、右侧列的右边框
   */
  private applyOuter(sr: number, sc: number, er: number, ec: number, border: BorderStyle): void {
    // 顶部边框: 顶行每个单元格的 borderTop
    for (let c = sc; c <= ec; c++) {
      this.sheet.setCellStyle(sr, c, { borderTop: border });
    }
    // 底部边框: 底行每个单元格的 borderBottom
    for (let c = sc; c <= ec; c++) {
      this.sheet.setCellStyle(er, c, { borderBottom: border });
    }
    // 左侧边框: 左列每个单元格的 borderLeft
    for (let r = sr; r <= er; r++) {
      this.sheet.setCellStyle(r, sc, { borderLeft: border });
    }
    // 右侧边框: 右列每个单元格的 borderRight
    for (let r = sr; r <= er; r++) {
      this.sheet.setCellStyle(r, ec, { borderRight: border });
    }
  }

  /**
   * 'inner' — 只设置选区内边框 (相邻单元格之间的边框)
   * 水平内线: 行 [sr, er-1] 每个单元格的 borderBottom
   * 垂直内线: 列 [sc, ec-1] 每个单元格的 borderRight
   * 单行/单列选区无内边框
   */
  private applyInner(sr: number, sc: number, er: number, ec: number, border: BorderStyle): void {
    // 水平内线 (行间)
    if (er > sr) {
      for (let r = sr; r < er; r++) {
        for (let c = sc; c <= ec; c++) {
          this.sheet.setCellStyle(r, c, { borderBottom: border });
        }
      }
    }
    // 垂直内线 (列间)
    if (ec > sc) {
      for (let c = sc; c < ec; c++) {
        for (let r = sr; r <= er; r++) {
          this.sheet.setCellStyle(r, c, { borderRight: border });
        }
      }
    }
  }

  /** 'top' — 只设置选区顶部外边框 */
  private applyTop(sr: number, sc: number, ec: number, border: BorderStyle): void {
    for (let c = sc; c <= ec; c++) {
      this.sheet.setCellStyle(sr, c, { borderTop: border });
    }
  }

  /** 'bottom' — 只设置选区底部外边框 */
  private applyBottom(er: number, sc: number, ec: number, border: BorderStyle): void {
    for (let c = sc; c <= ec; c++) {
      this.sheet.setCellStyle(er, c, { borderBottom: border });
    }
  }

  /** 'left' — 只设置选区左侧外边框 */
  private applyLeft(sr: number, er: number, sc: number, border: BorderStyle): void {
    for (let r = sr; r <= er; r++) {
      this.sheet.setCellStyle(r, sc, { borderLeft: border });
    }
  }

  /** 'right' — 只设置选区右侧外边框 */
  private applyRight(sr: number, er: number, ec: number, border: BorderStyle): void {
    for (let r = sr; r <= er; r++) {
      this.sheet.setCellStyle(r, ec, { borderRight: border });
    }
  }

  /**
   * 'diagonalDown' / 'diagonalUp' — 设置对角线
   * 选区内每个单元格都设置对应方向的对角线
   */
  private applyDiagonal(
    sr: number, sc: number, er: number, ec: number,
    direction: 'down' | 'up',
    border: BorderStyle
  ): void {
    const patch: Partial<CellStyle> =
      direction === 'down' ? { borderDiagonalDown: border } : { borderDiagonalUp: border };
    for (let r = sr; r <= er; r++) {
      for (let c = sc; c <= ec; c++) {
        this.sheet.setCellStyle(r, c, patch);
      }
    }
  }

  // ============================================================
  // 清除选区边框
  // ============================================================

  /**
   * 清除选区内所有边框 (含对角线)
   * @param range 选区范围
   */
  clearBorder(range: BorderRange): void {
    const sr = Math.min(range.startRow, range.endRow);
    const er = Math.max(range.startRow, range.endRow);
    const sc = Math.min(range.startCol, range.endCol);
    const ec = Math.max(range.startCol, range.endCol);

    // 清除 patch — 将所有边框字段置为 undefined
    const clearPatch: Partial<CellStyle> = {
      borderTop: undefined,
      borderRight: undefined,
      borderBottom: undefined,
      borderLeft: undefined,
      borderDiagonalDown: undefined,
      borderDiagonalUp: undefined,
    };

    for (let r = sr; r <= er; r++) {
      for (let c = sc; c <= ec; c++) {
        this.sheet.setCellStyle(r, c, clearPatch);
      }
    }
  }

  // ============================================================
  // 获取单元格边框
  // ============================================================

  /**
   * 获取单元格的完整边框信息 (六个方向)
   * @param row 行号 (0-based)
   * @param col 列号 (0-based)
   */
  getCellBorder(row: number, col: number): CellBorderInfo {
    const style = this.sheet.getCellStyle(row, col);
    return {
      top: style.borderTop,
      bottom: style.borderBottom,
      left: style.borderLeft,
      right: style.borderRight,
      diagonalDown: style.borderDiagonalDown,
      diagonalUp: style.borderDiagonalUp,
    };
  }

  /**
   * 判断单元格是否有任意边框
   */
  hasAnyBorder(row: number, col: number): boolean {
    const b = this.getCellBorder(row, col);
    return !!(b.top || b.bottom || b.left || b.right || b.diagonalDown || b.diagonalUp);
  }

  /**
   * 获取选区内所有单元格的边框信息
   * 用于撤销/重做或批量检查
   */
  getRangeBorders(range: BorderRange): Map<string, CellBorderInfo> {
    const sr = Math.min(range.startRow, range.endRow);
    const er = Math.max(range.startRow, range.endRow);
    const sc = Math.min(range.startCol, range.endCol);
    const ec = Math.max(range.startCol, range.endCol);

    const result = new Map<string, CellBorderInfo>();
    for (let r = sr; r <= er; r++) {
      for (let c = sc; c <= ec; c++) {
        result.set(`${r}:${c}`, this.getCellBorder(r, c));
      }
    }
    return result;
  }

  // ============================================================
  // 工具栏边框选择器预览渲染
  // ============================================================

  /**
   * 在指定 canvas 上绘制边框预览 (用于工具栏边框选择器)
   * @param canvas 目标 canvas 元素
   * @param borderType 要预览的边框类型
   * @param style 边框样式 (可选, 默认细黑线)
   */
  drawBorderPreview(
    canvas: HTMLCanvasElement,
    borderType: BorderType,
    style?: BorderStyleOption
  ): void {
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const w = canvas.width;
    const h = canvas.height;
    ctx.clearRect(0, 0, w, h);

    // 预览以 2x2 网格展示 (模拟选区), 居中绘制
    const gridLeft = w * 0.2;
    const gridTop = h * 0.2;
    const gridW = w * 0.6;
    const gridH = h * 0.6;
    const cellW = gridW / 2;
    const cellH = gridH / 2;

    // 线宽与颜色
    const lineW =
      style?.style === 'thick' ? 3 :
      style?.style === 'medium' ? 2 : 1;
    const color = style?.color ?? '#333333';

    // 先绘制浅灰色网格底线 (区分无边框区域)
    ctx.strokeStyle = '#e0e0e0';
    ctx.lineWidth = 1;
    for (let i = 0; i <= 2; i++) {
      // 垂直线
      ctx.beginPath();
      ctx.moveTo(gridLeft + i * cellW, gridTop);
      ctx.lineTo(gridLeft + i * cellW, gridTop + gridH);
      ctx.stroke();
      // 水平线
      ctx.beginPath();
      ctx.moveTo(gridLeft, gridTop + i * cellH);
      ctx.lineTo(gridLeft + gridW, gridTop + i * cellH);
      ctx.stroke();
    }

    // 设置边框样式
    const setLineDash = (borderStyle: string | undefined): void => {
      if (borderStyle === 'dashed') ctx.setLineDash([4, 2]);
      else if (borderStyle === 'dotted') ctx.setLineDash([2, 2]);
      else ctx.setLineDash([]);
    };
    setLineDash(style?.style);
    ctx.strokeStyle = color;
    ctx.lineWidth = lineW;

    // 根据边框类型绘制
    const drawLine = (x1: number, y1: number, x2: number, y2: number) => {
      ctx.beginPath();
      ctx.moveTo(x1, y1);
      ctx.lineTo(x2, y2);
      ctx.stroke();
    };

    // 选区外边框坐标
    const outerLeft = gridLeft;
    const outerRight = gridLeft + gridW;
    const outerTop = gridTop;
    const outerBottom = gridTop + gridH;
    // 选区内线坐标
    const midX = gridLeft + cellW;
    const midY = gridTop + cellH;

    switch (borderType) {
      case 'all':
        // 2x2 网格所有线 (外 + 内)
        drawLine(outerLeft, outerTop, outerRight, outerTop);     // 上
        drawLine(outerLeft, outerBottom, outerRight, outerBottom); // 下
        drawLine(outerLeft, outerTop, outerLeft, outerBottom);   // 左
        drawLine(outerRight, outerTop, outerRight, outerBottom); // 右
        drawLine(midX, outerTop, midX, outerBottom);             // 竖内线
        drawLine(outerLeft, midY, outerRight, midY);             // 横内线
        break;
      case 'outer':
        drawLine(outerLeft, outerTop, outerRight, outerTop);
        drawLine(outerLeft, outerBottom, outerRight, outerBottom);
        drawLine(outerLeft, outerTop, outerLeft, outerBottom);
        drawLine(outerRight, outerTop, outerRight, outerBottom);
        break;
      case 'inner':
        drawLine(midX, outerTop, midX, outerBottom);
        drawLine(outerLeft, midY, outerRight, midY);
        break;
      case 'top':
        drawLine(outerLeft, outerTop, outerRight, outerTop);
        break;
      case 'bottom':
        drawLine(outerLeft, outerBottom, outerRight, outerBottom);
        break;
      case 'left':
        drawLine(outerLeft, outerTop, outerLeft, outerBottom);
        break;
      case 'right':
        drawLine(outerRight, outerTop, outerRight, outerBottom);
        break;
      case 'none':
        // 不绘制任何边框 (只保留浅灰网格)
        break;
      case 'diagonalDown':
        // 左上 → 右下, 两个单元格各画一条
        drawLine(outerLeft, outerTop, midX, midY);
        drawLine(midX, midY, outerRight, outerBottom);
        break;
      case 'diagonalUp':
        // 左下 → 右上, 两个单元格各画一条
        drawLine(outerLeft, outerBottom, midX, midY);
        drawLine(midX, midY, outerRight, outerTop);
        break;
    }

    ctx.setLineDash([]);
  }

  /**
   * 创建边框预览的 data URL (用于 CSS background-image / img src)
   * 适用于工具栏下拉菜单中每项的图标
   */
  createBorderPreviewDataURL(borderType: BorderType, style?: BorderStyleOption, size = 32): string {
    const canvas = document.createElement('canvas');
    canvas.width = size;
    canvas.height = size;
    this.drawBorderPreview(canvas, borderType, style);
    return canvas.toDataURL();
  }
}
