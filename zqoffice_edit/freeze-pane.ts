/**
 * 冻结窗格模块 — 管理冻结行/列
 * 全部原创实现
 *
 * 职责:
 *   1. 冻结首行 / 首列 / 指定单元格
 *   2. 取消冻结
 *   3. 获取冻结信息
 *   4. 与 RenderEngine 集成 — 冻结区域单独渲染, 不随滚动移动
 *
 * 冻结语义:
 *   - freezeAt(row, col) 表示冻结 [0, row) 行 和 [0, col) 列
 *   - 即 frozenRowCount = row, frozenColumnCount = col
 *   - 例如 freezeAt(1, 0) = 冻结首行 (第0行)
 *   - 例如 freezeAt(0, 1) = 冻结首列 (第0列)
 */

import { SheetModel } from './data-model';
import { RenderEngine } from './render-engine';

/** 冻结信息 */
export interface FreezeInfo {
  /** 冻结的行数 (前 N 行被冻结) */
  frozenRowCount: number;
  /** 冻结的列数 (前 N 列被冻结) */
  frozenColumnCount: number;
  /** 冻结行总高度 (像素, 已跳过隐藏行) */
  freezeTopHeight: number;
  /** 冻结列总宽度 (像素, 已跳过隐藏列) */
  freezeLeftWidth: number;
  /** 是否有冻结 */
  hasFreeze: boolean;
}

export class FreezePane {
  private sheet: SheetModel;
  private renderEngine: RenderEngine;

  constructor(sheet: SheetModel, renderEngine: RenderEngine) {
    this.sheet = sheet;
    this.renderEngine = renderEngine;
  }

  // ============================================================
  // 冻结操作
  // ============================================================

  /**
   * 冻结到指定单元格位置
   * 冻结 [0, row) 行 和 [0, col) 列
   * @param row 行号 (0-based), 冻结前 row 行
   * @param col 列号 (0-based), 冻结前 col 列
   */
  freezeAt(row: number, col: number): void {
    const safeRow = Math.max(0, Math.min(row, this.sheet.rowCount));
    const safeCol = Math.max(0, Math.min(col, this.sheet.columnCount));
    this.sheet.setFreeze(safeRow, safeCol);
    this.refresh();
  }

  /**
   * 冻结首行 (第0行)
   * 等价于 freezeAt(1, 0)
   */
  freezeTopRow(): void {
    this.freezeAt(1, 0);
  }

  /**
   * 冻结首列 (第0列)
   * 等价于 freezeAt(0, 1)
   */
  freezeLeftColumn(): void {
    this.freezeAt(0, 1);
  }

  /**
   * 冻结首行和首列
   * 等价于 freezeAt(1, 1)
   */
  freezeTopRowAndLeftColumn(): void {
    this.freezeAt(1, 1);
  }

  /**
   * 取消所有冻结
   */
  unfreeze(): void {
    this.sheet.setFreeze(0, 0);
    this.refresh();
  }

  // ============================================================
  // 冻结信息查询
  // ============================================================

  /**
   * 获取冻结信息
   */
  getFreezeInfo(): FreezeInfo {
    const frozenRowCount = this.sheet.frozenRowCount;
    const frozenColumnCount = this.sheet.frozenColumnCount;
    const hasFreeze = frozenRowCount > 0 || frozenColumnCount > 0;

    // 计算冻结区域的实际像素尺寸 (跳过隐藏行列)
    const freezeTopHeight = this.calcFreezeTopHeight();
    const freezeLeftWidth = this.calcFreezeLeftWidth();

    return {
      frozenRowCount,
      frozenColumnCount,
      freezeTopHeight,
      freezeLeftWidth,
      hasFreeze,
    };
  }

  /**
   * 判断指定行是否在冻结区域内
   */
  isRowFrozen(row: number): boolean {
    return row >= 0 && row < this.sheet.frozenRowCount;
  }

  /**
   * 判断指定列是否在冻结区域内
   */
  isColumnFrozen(col: number): boolean {
    return col >= 0 && col < this.sheet.frozenColumnCount;
  }

  /**
   * 判断指定单元格是否在冻结角区域内 (同时满足行和列冻结)
   */
  isCellInFrozenCorner(row: number, col: number): boolean {
    return this.isRowFrozen(row) && this.isColumnFrozen(col);
  }

  // ============================================================
  // 内部辅助
  // ============================================================

  /** 计算冻结行的总像素高度 (跳过隐藏行) */
  private calcFreezeTopHeight(): number {
    const fr = this.sheet.frozenRowCount;
    if (fr <= 0) return 0;
    let height = 0;
    for (let r = 0; r < fr; r++) {
      if (!this.sheet.isRowHidden(r)) {
        height += this.sheet.getRowHeight(r);
      }
    }
    return height;
  }

  /** 计算冻结列的总像素宽度 (跳过隐藏列) */
  private calcFreezeLeftWidth(): number {
    const fc = this.sheet.frozenColumnCount;
    if (fc <= 0) return 0;
    let width = 0;
    for (let c = 0; c < fc; c++) {
      if (!this.sheet.isColumnHidden(c)) {
        width += this.sheet.getColumnWidth(c);
      }
    }
    return width;
  }

  /** 刷新渲染引擎 */
  private refresh(): void {
    this.renderEngine.refresh();
  }

  // ============================================================
  // 工作表切换
  // ============================================================

  /**
   * 更新工作表引用 (切换工作表时调用)
   */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
  }
}
