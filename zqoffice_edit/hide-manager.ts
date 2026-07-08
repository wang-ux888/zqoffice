/**
 * 行列隐藏管理模块 — 管理行/列的隐藏与显示
 * 全部原创实现
 *
 * 职责:
 *   1. 隐藏/显示行 (hideRows/showRows)
 *   2. 隐藏/显示列 (hideColumns/showColumns)
 *   3. 判断行/列是否隐藏 (isRowHidden/isColumnHidden)
 *   4. 获取所有隐藏行/列列表 (getHiddenRows/getHiddenColumns)
 *   5. 与 RenderEngine 集成 — 隐藏的行列不渲染 (高度/宽度计为 0)
 *
 * 隐藏语义:
 *   - hideRows(startRow, count) 隐藏 [startRow, startRow + count) 范围内的行
 *   - 隐藏行的高度在渲染时计为 0, 不显示在视图中
 *   - 隐藏状态持久化到 Sheet.hiddenRows / Sheet.hiddenColumns
 */

import { SheetModel } from './data-model';
import { RenderEngine } from './render-engine';

/** 隐藏范围信息 */
export interface HiddenRange {
  /** 起始行/列号 (0-based, 含) */
  start: number;
  /** 结束行/列号 (0-based, 含) */
  end: number;
  /** 数量 */
  count: number;
}

export class HideManager {
  private sheet: SheetModel;
  private renderEngine: RenderEngine;

  constructor(sheet: SheetModel, renderEngine: RenderEngine) {
    this.sheet = sheet;
    this.renderEngine = renderEngine;
  }

  // ============================================================
  // 行隐藏
  // ============================================================

  /**
   * 隐藏指定行范围
   * @param startRow 起始行号 (0-based, 含)
   * @param count 隐藏行数 (默认 1)
   */
  hideRows(startRow: number, count: number = 1): void {
    if (count <= 0) return;
    this.sheet.hideRows(startRow, count);
    this.refresh();
  }

  /**
   * 显示指定行范围
   * @param startRow 起始行号 (0-based, 含)
   * @param count 显示行数 (默认 1)
   */
  showRows(startRow: number, count: number = 1): void {
    if (count <= 0) return;
    this.sheet.showRows(startRow, count);
    this.refresh();
  }

  /**
   * 判断指定行是否隐藏
   */
  isRowHidden(row: number): boolean {
    return this.sheet.isRowHidden(row);
  }

  /**
   * 获取所有隐藏行号列表 (升序)
   */
  getHiddenRows(): number[] {
    return this.sheet.getHiddenRows();
  }

  /**
   * 获取隐藏行的连续范围列表
   * 将离散的行号合并为连续区间, 例如 [2,3,4,7,8] => [{start:2,end:4,count:3},{start:7,end:8,count:2}]
   */
  getHiddenRowRanges(): HiddenRange[] {
    return this.toRanges(this.sheet.getHiddenRows());
  }

  // ============================================================
  // 列隐藏
  // ============================================================

  /**
   * 隐藏指定列范围
   * @param startCol 起始列号 (0-based, 含)
   * @param count 隐藏列数 (默认 1)
   */
  hideColumns(startCol: number, count: number = 1): void {
    if (count <= 0) return;
    this.sheet.hideColumns(startCol, count);
    this.refresh();
  }

  /**
   * 显示指定列范围
   * @param startCol 起始列号 (0-based, 含)
   * @param count 显示列数 (默认 1)
   */
  showColumns(startCol: number, count: number = 1): void {
    if (count <= 0) return;
    this.sheet.showColumns(startCol, count);
    this.refresh();
  }

  /**
   * 判断指定列是否隐藏
   */
  isColumnHidden(col: number): boolean {
    return this.sheet.isColumnHidden(col);
  }

  /**
   * 获取所有隐藏列号列表 (升序)
   */
  getHiddenColumns(): number[] {
    return this.sheet.getHiddenColumns();
  }

  /**
   * 获取隐藏列的连续范围列表
   */
  getHiddenColumnRanges(): HiddenRange[] {
    return this.toRanges(this.sheet.getHiddenColumns());
  }

  // ============================================================
  // 批量操作
  // ============================================================

  /**
   * 显示所有隐藏行
   */
  showAllRows(): void {
    this.sheet.clearHiddenRows();
    this.refresh();
  }

  /**
   * 显示所有隐藏列
   */
  showAllColumns(): void {
    this.sheet.clearHiddenColumns();
    this.refresh();
  }

  /**
   * 显示所有隐藏的行和列
   */
  showAll(): void {
    this.sheet.clearHiddenRows();
    this.sheet.clearHiddenColumns();
    this.refresh();
  }

  // ============================================================
  // 查询辅助
  // ============================================================

  /**
   * 获取隐藏行数量
   */
  getHiddenRowCount(): number {
    return this.sheet.getHiddenRows().length;
  }

  /**
   * 获取隐藏列数量
   */
  getHiddenColumnCount(): number {
    return this.sheet.getHiddenColumns().length;
  }

  /**
   * 判断是否存在隐藏行
   */
  hasHiddenRows(): boolean {
    return this.sheet.getHiddenRows().length > 0;
  }

  /**
   * 判断是否存在隐藏列
   */
  hasHiddenColumns(): boolean {
    return this.sheet.getHiddenColumns().length > 0;
  }

  // ============================================================
  // 内部辅助
  // ============================================================

  /**
   * 将离散的行/列号列表合并为连续区间
   * 例如 [2,3,4,7,8] => [{start:2,end:4,count:3},{start:7,end:8,count:2}]
   */
  private toRanges(indices: number[]): HiddenRange[] {
    if (indices.length === 0) return [];
    const ranges: HiddenRange[] = [];
    let start = indices[0];
    let prev = indices[0];

    for (let i = 1; i < indices.length; i++) {
      const cur = indices[i];
      if (cur === prev + 1) {
        // 连续, 继续累积
        prev = cur;
      } else {
        // 断开, 收尾当前区间
        ranges.push({ start, end: prev, count: prev - start + 1 });
        start = cur;
        prev = cur;
      }
    }
    // 收尾最后一个区间
    ranges.push({ start, end: prev, count: prev - start + 1 });

    return ranges;
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
