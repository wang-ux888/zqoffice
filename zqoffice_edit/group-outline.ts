/**
 * 分组大纲模块 — 行/列分组与折叠展开
 * 全部原创实现
 *
 * 职责:
 *   1. 行分组 groupRows(startRow, endRow) — 将连续行分组
 *   2. 列分组 groupColumns(startCol, endCol)
 *   3. 取消分组 ungroupRows / ungroupColumns
 *   4. 折叠/展开 toggleGroup(groupId)
 *   5. 折叠全部/展开全部 collapseAll / expandAll
 *   6. 判断行列是否在分组内 isInGroup
 *   7. DOM 渲染: 在行号/列号旁显示 +/- 按钮和分组层级线
 *
 * 分组语义:
 *   - 行分组 [startRow, endRow] 折叠后隐藏该范围内的所有行
 *   - 列分组 [startCol, endCol] 折叠后隐藏该范围内的所有列
 *   - 摘要位置: 行分组摘要在 endRow+1 (下方), 列分组摘要在 endCol+1 (右侧)
 *   - +/- 按钮位于摘要位置
 *   - 嵌套层级 level: 被其他分组包含时 +1, 最外层为 1
 */

import { SheetModel, OutlineGroup } from './data-model';
import { RenderEngine } from './render-engine';

/** 分组大纲管理器回调 */
export interface GroupOutlineCallbacks {
  /** 内容变化时调用 (默认刷新渲染, 若设置则由调用方负责) */
  onContentChange?: () => void;
}

export class GroupOutlineManager {
  private container: HTMLElement;
  private sheet: SheetModel;
  private renderEngine: RenderEngine;
  private callbacks: GroupOutlineCallbacks;

  /** DOM 元素 */
  private rowOutlineLayerEl: HTMLDivElement;   // 行分组覆盖层 (行头区域)
  private colOutlineLayerEl: HTMLDivElement;   // 列分组覆盖层 (列头区域)
  /** 全局按钮区 (折叠全部/展开全部) */
  private summaryRowBtnEl: HTMLDivElement;      // 行大纲汇总按钮 (左下角)
  private summaryColBtnEl: HTMLDivElement;      // 列大纲汇总按钮 (右上角)

  /** ID 自增计数器 */
  private idCounter: number = 0;
  /** 滚动重绘节流句柄 */
  private scrollRafId: number | null = null;
  /** 已绑定的事件监听器引用 */
  private boundScrollHandler: (() => void) | null = null;

  /** 行头宽度 (与 RenderEngine 一致) */
  private readonly ROW_HEADER_WIDTH = 40;
  /** 列头高度 (与 RenderEngine 一致) */
  private readonly COL_HEADER_HEIGHT = 24;
  /** 每级层级线间距 (像素) */
  private readonly LEVEL_SPACING = 6;
  /** 层级线起始偏移 */
  private readonly LEVEL_OFFSET = 4;

  constructor(
    container: HTMLElement,
    sheet: SheetModel,
    renderEngine: RenderEngine,
    callbacks: GroupOutlineCallbacks = {}
  ) {
    this.container = container;
    this.sheet = sheet;
    this.renderEngine = renderEngine;
    this.callbacks = callbacks;

    // 行分组覆盖层 (覆盖在行头区域上)
    this.rowOutlineLayerEl = document.createElement('div');
    this.rowOutlineLayerEl.className = 'zq-outline-row-layer';
    this.container.appendChild(this.rowOutlineLayerEl);

    // 列分组覆盖层 (覆盖在列头区域上)
    this.colOutlineLayerEl = document.createElement('div');
    this.colOutlineLayerEl.className = 'zq-outline-col-layer';
    this.container.appendChild(this.colOutlineLayerEl);

    // 行大纲汇总按钮 (左下角, "1" "2" "3" 数字按钮 + 折叠/展开全部)
    this.summaryRowBtnEl = document.createElement('div');
    this.summaryRowBtnEl.className = 'zq-outline-summary-row';
    this.container.appendChild(this.summaryRowBtnEl);

    // 列大纲汇总按钮 (右上角)
    this.summaryColBtnEl = document.createElement('div');
    this.summaryColBtnEl.className = 'zq-outline-summary-col';
    this.container.appendChild(this.summaryColBtnEl);

    this.setupStyles();
    this.bindDomEvents();
    this.renderOutlines();
  }

  // ============================================================
  // 样式注入
  // ============================================================

  private setupStyles(): void {
    if (document.getElementById('zq-outline-styles')) return;
    const style = document.createElement('style');
    style.id = 'zq-outline-styles';
    style.textContent = `
      .zq-outline-row-layer {
        position: absolute;
        top: 24px;
        left: 0;
        width: 40px;
        bottom: 0;
        overflow: hidden;
        pointer-events: none;
        z-index: 7;
      }
      .zq-outline-col-layer {
        position: absolute;
        top: 0;
        left: 40px;
        right: 0;
        height: 24px;
        overflow: hidden;
        pointer-events: none;
        z-index: 7;
      }
      .zq-outline-line-row {
        position: absolute;
        width: 2px;
        background: #4787f5;
        opacity: 0.5;
        border-radius: 1px;
      }
      .zq-outline-line-col {
        position: absolute;
        height: 2px;
        background: #4787f5;
        opacity: 0.5;
        border-radius: 1px;
      }
      .zq-outline-btn {
        position: absolute;
        width: 14px;
        height: 14px;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 11px;
        font-weight: bold;
        color: #666;
        background: #fff;
        border: 1px solid #c0c0c0;
        border-radius: 2px;
        cursor: pointer;
        pointer-events: auto;
        user-select: none;
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
        line-height: 1;
      }
      .zq-outline-btn:hover {
        background: #e8f0fe;
        border-color: #4787f5;
        color: #4787f5;
      }
      .zq-outline-summary-row {
        position: absolute;
        left: 0;
        bottom: 0;
        width: 40px;
        height: 24px;
        display: none;
        align-items: center;
        justify-content: center;
        gap: 2px;
        background: #f5f5f5;
        border-right: 1px solid #d0d0d0;
        border-top: 1px solid #d0d0d0;
        z-index: 8;
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
      }
      .zq-outline-summary-col {
        position: absolute;
        top: 0;
        right: 0;
        width: 60px;
        height: 24px;
        display: none;
        align-items: center;
        justify-content: center;
        gap: 2px;
        background: #f5f5f5;
        border-left: 1px solid #d0d0d0;
        border-bottom: 1px solid #d0d0d0;
        z-index: 8;
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
      }
      .zq-outline-summary-btn {
        font-size: 11px;
        font-weight: bold;
        color: #666;
        cursor: pointer;
        padding: 1px 4px;
        border-radius: 2px;
        background: #fff;
        border: 1px solid #c0c0c0;
        user-select: none;
      }
      .zq-outline-summary-btn:hover {
        background: #e8f0fe;
        border-color: #4787f5;
        color: #4787f5;
      }
    `;
    document.head.appendChild(style);
  }

  // ============================================================
  // DOM 事件绑定
  // ============================================================

  private bindDomEvents(): void {
    const canvas = this.container.querySelector('.zq-sheet-canvas') as HTMLElement | null;
    if (canvas) {
      this.boundScrollHandler = (): void => {
        if (this.scrollRafId !== null) cancelAnimationFrame(this.scrollRafId);
        this.scrollRafId = requestAnimationFrame(() => {
          this.renderOutlines();
        });
      };
      canvas.addEventListener('scroll', this.boundScrollHandler);
    }
  }

  // ============================================================
  // 分组操作
  // ============================================================

  /**
   * 行分组 — 将连续行 [startRow, endRow] 分组
   * @param startRow 起始行 (0-based, 含)
   * @param endRow 结束行 (0-based, 含)
   * @returns 新建的分组对象, 若范围无效或与已有分组重叠返回 null
   */
  groupRows(startRow: number, endRow: number): OutlineGroup | null {
    const sr = Math.min(startRow, endRow);
    const er = Math.max(startRow, endRow);
    if (sr < 0 || er >= this.sheet.rowCount) return null;
    if (sr >= er) return null; // 至少 2 行才能分组

    // 检查与已有行分组是否重叠 (不允许部分重叠, 只允许完全包含或完全独立)
    const outlines = this.sheet.getOutlines();
    for (const o of outlines) {
      if (o.type !== 'row') continue;
      if (this.rangesOverlap(sr, er, o.start, o.end)) {
        // 只允许完全包含 (嵌套) 或完全不重叠
        if (!(sr <= o.start && er >= o.end) && !(sr >= o.start && er <= o.end)) {
          return null;
        }
      }
    }

    const level = this.calculateLevel('row', sr, er, outlines);
    const group: OutlineGroup = {
      id: this.generateId(),
      type: 'row',
      start: sr,
      end: er,
      collapsed: false,
      level,
    };
    outlines.push(group);
    this.sheet.setOutlines(outlines);

    this.afterChange();
    return group;
  }

  /**
   * 列分组 — 将连续列 [startCol, endCol] 分组
   * @param startCol 起始列 (0-based, 含)
   * @param endCol 结束列 (0-based, 含)
   * @returns 新建的分组对象, 若范围无效或与已有分组重叠返回 null
   */
  groupColumns(startCol: number, endCol: number): OutlineGroup | null {
    const sc = Math.min(startCol, endCol);
    const ec = Math.max(startCol, endCol);
    if (sc < 0 || ec >= this.sheet.columnCount) return null;
    if (sc >= ec) return null;

    const outlines = this.sheet.getOutlines();
    for (const o of outlines) {
      if (o.type !== 'column') continue;
      if (this.rangesOverlap(sc, ec, o.start, o.end)) {
        if (!(sc <= o.start && ec >= o.end) && !(sc >= o.start && ec <= o.end)) {
          return null;
        }
      }
    }

    const level = this.calculateLevel('column', sc, ec, outlines);
    const group: OutlineGroup = {
      id: this.generateId(),
      type: 'column',
      start: sc,
      end: ec,
      collapsed: false,
      level,
    };
    outlines.push(group);
    this.sheet.setOutlines(outlines);

    this.afterChange();
    return group;
  }

  /**
   * 取消行分组 — 移除与 [startRow, endRow] 范围匹配的行分组
   * @param startRow 起始行 (0-based)
   * @param endRow 结束行 (0-based)
   * @returns 是否移除了至少一个分组
   */
  ungroupRows(startRow: number, endRow: number): boolean {
    const outlines = this.sheet.getOutlines();
    const before = outlines.length;
    const filtered = outlines.filter(o => {
      if (o.type !== 'row') return true;
      // 完全匹配或被包含在指定范围内时移除
      if (o.start >= startRow && o.end <= endRow) {
        // 如果分组是折叠的, 先展开
        if (o.collapsed) {
          this.sheet.showRows(o.start, o.end - o.start + 1);
        }
        return false;
      }
      return true;
    });
    if (filtered.length === before) return false;
    this.sheet.setOutlines(filtered);
    this.afterChange();
    return true;
  }

  /**
   * 取消列分组 — 移除与 [startCol, endCol] 范围匹配的列分组
   * @param startCol 起始列 (0-based)
   * @param endCol 结束列 (0-based)
   * @returns 是否移除了至少一个分组
   */
  ungroupColumns(startCol: number, endCol: number): boolean {
    const outlines = this.sheet.getOutlines();
    const before = outlines.length;
    const filtered = outlines.filter(o => {
      if (o.type !== 'column') return true;
      if (o.start >= startCol && o.end <= endCol) {
        if (o.collapsed) {
          this.sheet.showColumns(o.start, o.end - o.start + 1);
        }
        return false;
      }
      return true;
    });
    if (filtered.length === before) return false;
    this.sheet.setOutlines(filtered);
    this.afterChange();
    return true;
  }

  /**
   * 折叠/展开指定分组
   * @param groupId 分组 ID
   * @returns 是否操作成功
   */
  toggleGroup(groupId: string): boolean {
    const outlines = this.sheet.getOutlines();
    const group = outlines.find(o => o.id === groupId);
    if (!group) return false;

    if (group.collapsed) {
      this.expandGroup(group, outlines);
    } else {
      this.collapseGroup(group, outlines);
    }

    this.sheet.setOutlines(outlines);
    this.afterChange();
    return true;
  }

  /**
   * 折叠所有分组
   * @param type 分组类型 (可选, 不传则折叠行和列)
   */
  collapseAll(type?: 'row' | 'column'): void {
    const outlines = this.sheet.getOutlines();
    let changed = false;
    for (const o of outlines) {
      if (type && o.type !== type) continue;
      if (!o.collapsed) {
        this.collapseGroup(o, outlines);
        changed = true;
      }
    }
    if (changed) {
      this.sheet.setOutlines(outlines);
      this.afterChange();
    }
  }

  /**
   * 展开所有分组
   * @param type 分组类型 (可选, 不传则展开行和列)
   */
  expandAll(type?: 'row' | 'column'): void {
    const outlines = this.sheet.getOutlines();
    let changed = false;
    for (const o of outlines) {
      if (type && o.type !== type) continue;
      if (o.collapsed) {
        this.expandGroup(o, outlines);
        changed = true;
      }
    }
    if (changed) {
      this.sheet.setOutlines(outlines);
      this.afterChange();
    }
  }

  // ============================================================
  // 查询
  // ============================================================

  /**
   * 判断指定行/列是否在某个分组内
   * @param type 分组类型
   * @param index 行号或列号 (0-based)
   * @returns 包含该索引的最内层分组, 若无返回 null
   */
  isInGroup(type: 'row' | 'column', index: number): OutlineGroup | null {
    const outlines = this.sheet.getOutlines();
    let result: OutlineGroup | null = null;
    let maxLevel = 0;
    for (const o of outlines) {
      if (o.type !== type) continue;
      if (index >= o.start && index <= o.end) {
        if (o.level > maxLevel) {
          maxLevel = o.level;
          result = o;
        }
      }
    }
    return result;
  }

  /**
   * 判断指定行是否在某个折叠的分组内 (即该行应被隐藏)
   * @param row 行号 (0-based)
   */
  isRowHiddenByOutline(row: number): boolean {
    const outlines = this.sheet.getOutlines();
    for (const o of outlines) {
      if (o.type !== 'row') continue;
      if (o.collapsed && row >= o.start && row <= o.end) {
        return true;
      }
    }
    return false;
  }

  /**
   * 判断指定列是否在某个折叠的分组内 (即该列应被隐藏)
   * @param col 列号 (0-based)
   */
  isColumnHiddenByOutline(col: number): boolean {
    const outlines = this.sheet.getOutlines();
    for (const o of outlines) {
      if (o.type !== 'column') continue;
      if (o.collapsed && col >= o.start && col <= o.end) {
        return true;
      }
    }
    return false;
  }

  /**
   * 按 ID 获取分组
   */
  getGroup(groupId: string): OutlineGroup | null {
    const outlines = this.sheet.getOutlines();
    return outlines.find(o => o.id === groupId) ?? null;
  }

  /**
   * 获取所有分组
   */
  getAllOutlines(): OutlineGroup[] {
    return this.sheet.getOutlines();
  }

  /**
   * 获取指定类型的所有分组
   */
  getOutlinesByType(type: 'row' | 'column'): OutlineGroup[] {
    return this.sheet.getOutlines().filter(o => o.type === type);
  }

  /**
   * 获取最大层级
   */
  getMaxLevel(type?: 'row' | 'column'): number {
    const outlines = this.sheet.getOutlines();
    let max = 0;
    for (const o of outlines) {
      if (type && o.type !== type) continue;
      if (o.level > max) max = o.level;
    }
    return max;
  }

  /**
   * 是否存在分组
   */
  hasOutlines(): boolean {
    return this.sheet.getOutlines().length > 0;
  }

  // ============================================================
  // DOM 渲染
  // ============================================================

  /**
   * 渲染分组大纲 — 在行头/列头区域显示 +/- 按钮和层级线
   */
  renderOutlines(): void {
    this.rowOutlineLayerEl.innerHTML = '';
    this.colOutlineLayerEl.innerHTML = '';

    const outlines = this.sheet.getOutlines();
    if (outlines.length === 0) {
      this.summaryRowBtnEl.style.display = 'none';
      this.summaryColBtnEl.style.display = 'none';
      return;
    }

    const canvas = this.container.querySelector('.zq-sheet-canvas') as HTMLElement | null;
    if (!canvas) return;
    const scrollTop = canvas.scrollTop;
    const scrollLeft = canvas.scrollLeft;

    const rowOffsets = this.computeRowOffsets();
    const colOffsets = this.computeColOffsets();

    const rowGroups = outlines.filter(o => o.type === 'row');
    const colGroups = outlines.filter(o => o.type === 'column');

    // 渲染行分组
    for (const g of rowGroups) {
      this.renderRowGroup(g, rowOffsets, scrollTop);
    }

    // 渲染列分组
    for (const g of colGroups) {
      this.renderColGroup(g, colOffsets, scrollLeft);
    }

    // 渲染汇总按钮
    this.renderSummaryButtons(rowGroups.length > 0, colGroups.length > 0);
  }

  /**
   * 渲染单个行分组 — 层级线 + +/- 按钮
   */
  private renderRowGroup(
    group: OutlineGroup,
    rowOffsets: number[],
    scrollTop: number
  ): void {
    const startY = rowOffsets[group.start] - scrollTop;
    const endY = rowOffsets[group.end + 1] - scrollTop;
    const summaryRow = group.end + 1; // 摘要行 = endRow + 1
    const summaryY = summaryRow < this.sheet.rowCount
      ? rowOffsets[summaryRow] - scrollTop
      : endY;

    const x = this.LEVEL_OFFSET + (group.level - 1) * this.LEVEL_SPACING;

    // 层级线 (从 startRow 到 endRow)
    const line = document.createElement('div');
    line.className = 'zq-outline-line-row';
    line.style.left = `${x}px`;
    line.style.top = `${Math.max(0, startY)}px`;
    line.style.height = `${Math.max(0, endY - startY)}px`;
    this.rowOutlineLayerEl.appendChild(line);

    // +/- 按钮 (在摘要行位置)
    const btn = document.createElement('div');
    btn.className = 'zq-outline-btn';
    btn.textContent = group.collapsed ? '+' : '−';
    btn.style.left = `${x - 6}px`;
    btn.style.top = `${summaryY + 2}px`;
    btn.title = `${group.start + 1}-${group.end + 1} 行`;
    btn.addEventListener('click', (e) => {
      e.stopPropagation();
      this.toggleGroup(group.id);
    });
    this.rowOutlineLayerEl.appendChild(btn);
  }

  /**
   * 渲染单个列分组 — 层级线 + +/- 按钮
   */
  private renderColGroup(
    group: OutlineGroup,
    colOffsets: number[],
    scrollLeft: number
  ): void {
    const startX = colOffsets[group.start] - scrollLeft;
    const endX = colOffsets[group.end + 1] - scrollLeft;
    const summaryCol = group.end + 1;
    const summaryX = summaryCol < this.sheet.columnCount
      ? colOffsets[summaryCol] - scrollLeft
      : endX;

    const y = this.LEVEL_OFFSET + (group.level - 1) * this.LEVEL_SPACING;

    // 层级线 (从 startCol 到 endCol)
    const line = document.createElement('div');
    line.className = 'zq-outline-line-col';
    line.style.top = `${y}px`;
    line.style.left = `${Math.max(0, startX)}px`;
    line.style.width = `${Math.max(0, endX - startX)}px`;
    this.colOutlineLayerEl.appendChild(line);

    // +/- 按钮 (在摘要列位置)
    const btn = document.createElement('div');
    btn.className = 'zq-outline-btn';
    btn.textContent = group.collapsed ? '+' : '−';
    btn.style.top = `${y - 6}px`;
    btn.style.left = `${summaryX + 2}px`;
    btn.title = `${SheetModel.columnToLetter(group.start)}-${SheetModel.columnToLetter(group.end)} 列`;
    btn.addEventListener('click', (e) => {
      e.stopPropagation();
      this.toggleGroup(group.id);
    });
    this.colOutlineLayerEl.appendChild(btn);
  }

  /**
   * 渲染汇总按钮 (折叠全部/展开全部)
   */
  private renderSummaryButtons(hasRowGroups: boolean, hasColGroups: boolean): void {
    // 行汇总按钮 (左下角)
    if (hasRowGroups) {
      this.summaryRowBtnEl.style.display = 'flex';
      this.summaryRowBtnEl.innerHTML = '';
      const maxLevel = this.getMaxLevel('row');
      for (let lv = 1; lv <= maxLevel; lv++) {
        const btn = document.createElement('span');
        btn.className = 'zq-outline-summary-btn';
        btn.textContent = String(lv);
        btn.title = `展开到第 ${lv} 级`;
        btn.addEventListener('click', (e) => {
          e.stopPropagation();
          this.expandToLevel('row', lv);
        });
        this.summaryRowBtnEl.appendChild(btn);
      }
    } else {
      this.summaryRowBtnEl.style.display = 'none';
    }

    // 列汇总按钮 (右上角)
    if (hasColGroups) {
      this.summaryColBtnEl.style.display = 'flex';
      this.summaryColBtnEl.innerHTML = '';
      const maxLevel = this.getMaxLevel('column');
      for (let lv = 1; lv <= maxLevel; lv++) {
        const btn = document.createElement('span');
        btn.className = 'zq-outline-summary-btn';
        btn.textContent = String(lv);
        btn.title = `展开到第 ${lv} 级`;
        btn.addEventListener('click', (e) => {
          e.stopPropagation();
          this.expandToLevel('column', lv);
        });
        this.summaryColBtnEl.appendChild(btn);
      }
    } else {
      this.summaryColBtnEl.style.display = 'none';
    }
  }

  // ============================================================
  // 内部辅助 — 折叠/展开逻辑
  // ============================================================

  /**
   * 折叠分组 — 隐藏范围内的行/列
   */
  private collapseGroup(group: OutlineGroup, outlines: OutlineGroup[]): void {
    group.collapsed = true;
    if (group.type === 'row') {
      this.sheet.hideRows(group.start, group.end - group.start + 1);
    } else {
      this.sheet.hideColumns(group.start, group.end - group.start + 1);
    }
    // 不需要递归折叠子分组, 因为隐藏范围已经覆盖了子分组的行/列
  }

  /**
   * 展开分组 — 显示范围内的行/列, 然后重新折叠内部已折叠的子分组
   */
  private expandGroup(group: OutlineGroup, outlines: OutlineGroup[]): void {
    group.collapsed = false;
    if (group.type === 'row') {
      this.sheet.showRows(group.start, group.end - group.start + 1);
    } else {
      this.sheet.showColumns(group.start, group.end - group.start + 1);
    }
    // 重新折叠内部已折叠的子分组
    for (const o of outlines) {
      if (o.type !== group.type) continue;
      if (o.id === group.id) continue;
      // 子分组: 完全包含在当前分组内
      if (o.start >= group.start && o.end <= group.end && o.collapsed) {
        if (o.type === 'row') {
          this.sheet.hideRows(o.start, o.end - o.start + 1);
        } else {
          this.sheet.hideColumns(o.start, o.end - o.start + 1);
        }
      }
    }
  }

  /**
   * 展开到指定层级 (折叠所有 level > targetLevel 的分组)
   */
  private expandToLevel(type: 'row' | 'column', targetLevel: number): void {
    const outlines = this.sheet.getOutlines();
    // 先展开所有该类型的分组
    for (const o of outlines) {
      if (o.type !== type) continue;
      if (o.collapsed) {
        this.expandGroup(o, outlines);
      }
    }
    // 然后折叠所有 level >= targetLevel + 1 的分组 (从外到内)
    // 先按 level 降序排列, 从高到低折叠
    const toCollapse = outlines
      .filter(o => o.type === type && o.level > targetLevel)
      .sort((a, b) => a.level - b.level); // 先折叠低层级 (更内层)? 不, 先折叠外层
    // 实际上应该先折叠外层 (level 较小的), 再折叠内层
    // 但 level > targetLevel 的都是内层, 顺序不影响因为它们互不重叠或嵌套
    for (const o of toCollapse) {
      if (!o.collapsed) {
        this.collapseGroup(o, outlines);
      }
    }
    this.sheet.setOutlines(outlines);
    this.afterChange();
  }

  // ============================================================
  // 内部辅助 — 范围计算
  // ============================================================

  /**
   * 判断两个范围是否重叠 (不含端点相邻)
   */
  private rangesOverlap(s1: number, e1: number, s2: number, e2: number): boolean {
    return !(e1 < s2 || e2 < s1);
  }

  /**
   * 计算新分组的嵌套层级
   * level = 1 + (完全包含此范围的已有分组的最大 level)
   */
  private calculateLevel(
    type: 'row' | 'column',
    start: number,
    end: number,
    outlines: OutlineGroup[]
  ): number {
    let maxContainerLevel = 0;
    for (const o of outlines) {
      if (o.type !== type) continue;
      // 完全包含 [start, end]
      if (o.start <= start && o.end >= end) {
        if (o.level > maxContainerLevel) {
          maxContainerLevel = o.level;
        }
      }
    }
    return maxContainerLevel + 1;
  }

  /** 生成唯一 ID */
  private generateId(): string {
    this.idCounter++;
    return `ol_${Date.now()}_${this.idCounter}`;
  }

  /**
   * 计算行偏移量数组 (与 RenderEngine 一致)
   */
  private computeRowOffsets(): number[] {
    const offsets = new Array(this.sheet.rowCount + 1);
    offsets[0] = 0;
    for (let i = 0; i < this.sheet.rowCount; i++) {
      const h = this.sheet.isRowHidden(i) ? 0 : this.sheet.getRowHeight(i);
      offsets[i + 1] = offsets[i] + h;
    }
    return offsets;
  }

  /**
   * 计算列偏移量数组 (与 RenderEngine 一致)
   */
  private computeColOffsets(): number[] {
    const offsets = new Array(this.sheet.columnCount + 1);
    offsets[0] = 0;
    for (let i = 0; i < this.sheet.columnCount; i++) {
      const w = this.sheet.isColumnHidden(i) ? 0 : this.sheet.getColumnWidth(i);
      offsets[i + 1] = offsets[i] + w;
    }
    return offsets;
  }

  /** 数据变更后的统一处理 */
  private afterChange(): void {
    if (this.callbacks.onContentChange) {
      this.callbacks.onContentChange();
    } else {
      this.renderEngine.refresh();
    }
    this.renderOutlines();
  }

  // ============================================================
  // 公开方法
  // ============================================================

  /** 刷新大纲渲染 (外部渲染变化后调用) */
  refresh(): void {
    this.renderOutlines();
  }

  /** 更新工作表引用 (切换工作表时调用) */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
    this.renderOutlines();
  }

  /** 销毁 — 移除 DOM 和事件监听 */
  destroy(): void {
    const canvas = this.container.querySelector('.zq-sheet-canvas') as HTMLElement | null;
    if (canvas && this.boundScrollHandler) {
      canvas.removeEventListener('scroll', this.boundScrollHandler);
    }
    if (this.scrollRafId !== null) {
      cancelAnimationFrame(this.scrollRafId);
    }
    this.rowOutlineLayerEl.remove();
    this.colOutlineLayerEl.remove();
    this.summaryRowBtnEl.remove();
    this.summaryColBtnEl.remove();
  }
}
