/**
 * 渲染引擎层 — 虚拟滚动 + DOM 渲染
 * 全部原创实现, 使用 DOM div grid 渲染 (非 canvas)
 */

import { SheetModel, CellStyle, Cell, MergedCell } from './data-model';
import { StyleEngine } from './style-engine';

/** 选区范围 */
export interface SelectionRange {
  startRow: number;
  startCol: number;
  endRow: number;
  endCol: number;
}

/** 可见区域 */
export interface Viewport {
  startRow: number;
  endRow: number;
  startCol: number;
  endCol: number;
}

/** 渲染引擎事件回调 */
export interface RenderEngineCallbacks {
  onCellClick?: (row: number, col: number, event: MouseEvent) => void;
  onCellDoubleClick?: (row: number, col: number, event: MouseEvent) => void;
  onSelectionChange?: (range: SelectionRange) => void;
  onContextMenu?: (row: number, col: number, event: MouseEvent) => void;
  onScroll?: (viewport: Viewport) => void;
}

export class RenderEngine {
  private container: HTMLElement;
  private sheet: SheetModel;
  private styleEngine: StyleEngine;
  private callbacks: RenderEngineCallbacks;

  // DOM 元素
  private gridEl: HTMLDivElement;
  private headerRowEl: HTMLDivElement;    // 列头 (A, B, C...)
  private headerColEl: HTMLDivElement;    // 行头 (1, 2, 3...)
  private cornerEl: HTMLDivElement;       // 左上角
  private selectionEl: HTMLDivElement;    // 选区高亮
  private canvasEl: HTMLDivElement;       // 单元格区域
  // 冻结区域独立渲染层 (不随 canvasEl 滚动)
  private frozenCornerLayerEl: HTMLDivElement; // 冻结角 (前 frozenRowCount 行 × 前 frozenColumnCount 列)
  private frozenTopLayerEl: HTMLDivElement;    // 冻结行 (前 frozenRowCount 行, 非冻结列)
  private frozenLeftLayerEl: HTMLDivElement;   // 冻结列 (前 frozenColumnCount 列, 非冻结行)

  // 虚拟滚动状态
  private scrollTop: number = 0;
  private scrollLeft: number = 0;
  private viewportHeight: number = 600;
  private viewportWidth: number = 800;

  // 缓存的行高/列宽累加
  private rowOffsets: number[] = [];   // rowOffsets[i] = 前 i 行的累计高度
  private colOffsets: number[] = [];   // colOffsets[i] = 前 i 列的累计宽度
  private totalHeight: number = 0;
  private totalWidth: number = 0;
  // 冻结区域尺寸 (跳过隐藏行列后的实际像素尺寸)
  private freezeTopHeight: number = 0;  // 冻结行总高度
  private freezeLeftWidth: number = 0;  // 冻结列总宽度

  // 当前选区
  private selection: SelectionRange | null = null;

  // 已渲染的单元格 DOM 映射, key = "row:col"
  private renderedCells = new Map<string, HTMLDivElement>();

  // 渲染缓冲行数/列数 (超出可见区域额外渲染的行/列数)
  private readonly BUFFER_ROWS = 5;
  private readonly BUFFER_COLS = 3;

  constructor(
    container: HTMLElement,
    sheet: SheetModel,
    styleEngine: StyleEngine,
    callbacks: RenderEngineCallbacks = {}
  ) {
    this.container = container;
    this.sheet = sheet;
    this.styleEngine = styleEngine;
    this.callbacks = callbacks;

    // 创建 DOM 结构
    this.gridEl = document.createElement('div');
    this.gridEl.className = 'zq-sheet-grid';
    this.cornerEl = document.createElement('div');
    this.cornerEl.className = 'zq-sheet-corner';
    this.headerRowEl = document.createElement('div');
    this.headerRowEl.className = 'zq-sheet-header-row';
    this.headerColEl = document.createElement('div');
    this.headerColEl.className = 'zq-sheet-header-col';
    this.selectionEl = document.createElement('div');
    this.selectionEl.className = 'zq-sheet-selection';
    this.canvasEl = document.createElement('div');
    this.canvasEl.className = 'zq-sheet-canvas';
    // 冻结层
    this.frozenCornerLayerEl = document.createElement('div');
    this.frozenCornerLayerEl.className = 'zq-sheet-frozen-corner';
    this.frozenTopLayerEl = document.createElement('div');
    this.frozenTopLayerEl.className = 'zq-sheet-frozen-top';
    this.frozenLeftLayerEl = document.createElement('div');
    this.frozenLeftLayerEl.className = 'zq-sheet-frozen-left';

    // 组装
    this.gridEl.appendChild(this.cornerEl);
    this.gridEl.appendChild(this.headerRowEl);
    this.gridEl.appendChild(this.headerColEl);
    this.gridEl.appendChild(this.canvasEl);
    this.canvasEl.appendChild(this.selectionEl);
    this.gridEl.appendChild(this.frozenLeftLayerEl);
    this.gridEl.appendChild(this.frozenTopLayerEl);
    this.gridEl.appendChild(this.frozenCornerLayerEl);
    this.container.appendChild(this.gridEl);

    this.setupStyles();
    this.setupEvents();
    this.recalculateOffsets();
    this.render();
  }

  // ============================================================
  // 样式设置
  // ============================================================

  private setupStyles(): void {
    if (document.getElementById('zq-sheet-styles')) return;
    const style = document.createElement('style');
    style.id = 'zq-sheet-styles';
    style.textContent = `
      .zq-sheet-grid {
        position: relative;
        width: 100%;
        height: 100%;
        overflow: hidden;
        background: #fff;
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
        font-size: 13px;
        user-select: none;
      }
      .zq-sheet-corner {
        position: absolute;
        top: 0; left: 0;
        width: 40px; height: 24px;
        background: #f5f5f5;
        border-right: 1px solid #d0d0d0;
        border-bottom: 1px solid #d0d0d0;
        z-index: 3;
      }
      .zq-sheet-header-row {
        position: absolute;
        top: 0; left: 40px;
        height: 24px;
        background: #f5f5f5;
        border-bottom: 1px solid #d0d0d0;
        z-index: 2;
        overflow: hidden;
      }
      .zq-sheet-header-col {
        position: absolute;
        top: 24px; left: 0;
        width: 40px;
        background: #f5f5f5;
        border-right: 1px solid #d0d0d0;
        z-index: 2;
        overflow: hidden;
      }
      .zq-sheet-canvas {
        position: absolute;
        top: 24px; left: 40px;
        right: 0; bottom: 0;
        overflow: auto;
        z-index: 1;
      }
      .zq-sheet-selection {
        position: absolute;
        border: 2px solid #4787f5;
        background: rgba(71, 135, 245, 0.08);
        pointer-events: none;
        z-index: 10;
        display: none;
      }
      .zq-cell {
        position: absolute;
        box-sizing: border-box;
        border-right: 1px solid #e0e0e0;
        border-bottom: 1px solid #e0e0e0;
        padding: 2px 4px;
        overflow: hidden;
        white-space: nowrap;
        text-overflow: ellipsis;
        cursor: cell;
      }
      .zq-header-cell {
        position: absolute;
        box-sizing: border-box;
        display: flex;
        align-items: center;
        justify-content: center;
        background: #f5f5f5;
        border-right: 1px solid #d0d0d0;
        border-bottom: 1px solid #d0d0d0;
        color: #666;
        font-size: 12px;
        cursor: pointer;
      }
      .zq-header-cell.selected {
        background: #e3edf7;
        color: #4787f5;
      }
      /* 冻结区域层 — 不随主 canvas 滚动 */
      .zq-sheet-frozen-corner,
      .zq-sheet-frozen-top,
      .zq-sheet-frozen-left {
        position: absolute;
        overflow: hidden;
        pointer-events: auto;
        background: #fff;
        display: none; /* 默认隐藏, 有冻结时显示 */
      }
      .zq-sheet-frozen-corner {
        top: 24px;
        left: 40px;
        z-index: 6;
        border-right: 1px solid #d0d0d0;
        border-bottom: 1px solid #d0d0d0;
      }
      .zq-sheet-frozen-top {
        top: 24px;
        z-index: 5;
        border-bottom: 1px solid #d0d0d0;
      }
      .zq-sheet-frozen-left {
        left: 40px;
        z-index: 5;
        border-right: 1px solid #d0d0d0;
      }
    `;
    document.head.appendChild(style);
  }

  // ============================================================
  // 事件处理
  // ============================================================

  private setupEvents(): void {
    // 滚动事件 — 虚拟滚动核心
    this.canvasEl.addEventListener('scroll', () => {
      this.scrollTop = this.canvasEl.scrollTop;
      this.scrollLeft = this.canvasEl.scrollLeft;
      this.render();
    });

    // 点击事件
    this.canvasEl.addEventListener('mousedown', (e) => {
      const target = e.target as HTMLElement;
      if (target.classList.contains('zq-cell')) {
        const row = parseInt(target.dataset.row || '0');
        const col = parseInt(target.dataset.col || '0');

        if (e.shiftKey && this.selection) {
          // 扩展选区
          this.selection.endRow = row;
          this.selection.endCol = col;
        } else {
          this.selection = { startRow: row, startCol: col, endRow: row, endCol: col };
        }
        this.normalizeSelection();
        this.updateSelectionUI();
        this.callbacks.onSelectionChange?.(this.selection!);
        this.callbacks.onCellClick?.(row, col, e);
      }
    });

    // 双击事件 — 进入编辑模式
    this.canvasEl.addEventListener('dblclick', (e) => {
      const target = e.target as HTMLElement;
      if (target.classList.contains('zq-cell')) {
        const row = parseInt(target.dataset.row || '0');
        const col = parseInt(target.dataset.col || '0');
        this.callbacks.onCellDoubleClick?.(row, col, e);
      }
    });

    // 右键菜单
    this.canvasEl.addEventListener('contextmenu', (e) => {
      e.preventDefault();
      const target = e.target as HTMLElement;
      if (target.classList.contains('zq-cell')) {
        const row = parseInt(target.dataset.row || '0');
        const col = parseInt(target.dataset.col || '0');
        this.callbacks.onContextMenu?.(row, col, e);
      }
    });

    // 拖拽选区
    let isDragging = false;
    this.canvasEl.addEventListener('mousedown', (e) => {
      if (e.button === 0) isDragging = true;
    });
    document.addEventListener('mousemove', (e) => {
      if (!isDragging || !this.selection) return;
      const rect = this.canvasEl.getBoundingClientRect();
      const x = e.clientX - rect.left + this.scrollLeft;
      const y = e.clientY - rect.top + this.scrollTop;
      const col = this.findColByX(x);
      const row = this.findRowByY(y);
      if (row >= 0 && col >= 0) {
        this.selection.endRow = row;
        this.selection.endCol = col;
        this.normalizeSelection();
        this.updateSelectionUI();
        this.callbacks.onSelectionChange?.(this.selection);
      }
    });
    document.addEventListener('mouseup', () => {
      isDragging = false;
    });

    // 列头点击 — 选中整列
    this.headerRowEl.addEventListener('mousedown', (e) => {
      const target = e.target as HTMLElement;
      if (target.classList.contains('zq-header-cell')) {
        const col = parseInt(target.dataset.col || '0');
        this.selection = { startRow: 0, startCol: col, endRow: this.sheet.rowCount - 1, endCol: col };
        this.updateSelectionUI();
        this.callbacks.onSelectionChange?.(this.selection);
      }
    });

    // 行头点击 — 选中整行
    this.headerColEl.addEventListener('mousedown', (e) => {
      const target = e.target as HTMLElement;
      if (target.classList.contains('zq-header-cell')) {
        const row = parseInt(target.dataset.row || '0');
        this.selection = { startRow: row, startCol: 0, endRow: row, endCol: this.sheet.columnCount - 1 };
        this.updateSelectionUI();
        this.callbacks.onSelectionChange?.(this.selection);
      }
    });
  }

  // ============================================================
  // 偏移量计算 — 预计算行高/列宽的累加值
  // ============================================================

  recalculateOffsets(): void {
    const rowCount = this.sheet.rowCount;
    const colCount = this.sheet.columnCount;

    this.rowOffsets = new Array(rowCount + 1);
    this.colOffsets = new Array(colCount + 1);

    // 行偏移: 隐藏行高度计为 0
    this.rowOffsets[0] = 0;
    for (let i = 0; i < rowCount; i++) {
      const h = this.sheet.isRowHidden(i) ? 0 : this.sheet.getRowHeight(i);
      this.rowOffsets[i + 1] = this.rowOffsets[i] + h;
    }
    this.totalHeight = this.rowOffsets[rowCount];

    // 列偏移: 隐藏列宽度计为 0
    this.colOffsets[0] = 0;
    for (let i = 0; i < colCount; i++) {
      const w = this.sheet.isColumnHidden(i) ? 0 : this.sheet.getColumnWidth(i);
      this.colOffsets[i + 1] = this.colOffsets[i] + w;
    }
    this.totalWidth = this.colOffsets[colCount];

    // 计算冻结区域尺寸 (前 frozenRowCount 行 / 前 frozenColumnCount 列的累计尺寸)
    const fr = Math.min(this.sheet.frozenRowCount, rowCount);
    const fc = Math.min(this.sheet.frozenColumnCount, colCount);
    this.freezeTopHeight = fr > 0 ? this.rowOffsets[fr] : 0;
    this.freezeLeftWidth = fc > 0 ? this.colOffsets[fc] : 0;
  }

  // ============================================================
  // 二分查找 — 根据 scrollTop/scrollLeft 找到可见区域
  // ============================================================

  private findRowByY(y: number): number {
    let lo = 0, hi = this.sheet.rowCount - 1;
    while (lo < hi) {
      const mid = (lo + hi + 1) >> 1;
      if (this.rowOffsets[mid] <= y) lo = mid;
      else hi = mid - 1;
    }
    return lo;
  }

  private findColByX(x: number): number {
    let lo = 0, hi = this.sheet.columnCount - 1;
    while (lo < hi) {
      const mid = (lo + hi + 1) >> 1;
      if (this.colOffsets[mid] <= x) lo = mid;
      else hi = mid - 1;
    }
    return lo;
  }

  private getViewport(): Viewport {
    this.viewportHeight = this.canvasEl.clientHeight;
    this.viewportWidth = this.canvasEl.clientWidth;

    const startRow = this.findRowByY(this.scrollTop);
    const startCol = this.findColByX(this.scrollLeft);

    let endRow = this.findRowByY(this.scrollTop + this.viewportHeight);
    endRow = Math.min(endRow + this.BUFFER_ROWS, this.sheet.rowCount - 1);

    let endCol = this.findColByX(this.scrollLeft + this.viewportWidth);
    endCol = Math.min(endCol + this.BUFFER_COLS, this.sheet.columnCount - 1);

    return { startRow, endRow, startCol, endCol };
  }

  // ============================================================
  // 渲染主循环
  // ============================================================

  render(): void {
    const vp = this.getViewport();

    // 设置 canvas 滚动区域大小
    const spacer = this.canvasEl.querySelector('.zq-sheet-spacer') as HTMLDivElement;
    if (spacer) {
      spacer.style.width = `${this.totalWidth}px`;
      spacer.style.height = `${this.totalHeight}px`;
    } else {
      const s = document.createElement('div');
      s.className = 'zq-sheet-spacer';
      s.style.width = `${this.totalWidth}px`;
      s.style.height = `${this.totalHeight}px`;
      s.style.position = 'absolute';
      s.style.top = '0';
      s.style.left = '0';
      this.canvasEl.appendChild(s);
    }

    // 渲染列头
    this.renderColumnHeaders(vp.startCol, vp.endCol);

    // 渲染行头
    this.renderRowHeaders(vp.startRow, vp.endRow);

    // 渲染单元格 (普通区域, 跳过冻结/隐藏行列)
    this.renderCells(vp);

    // 渲染冻结区域 (独立层, 不随滚动移动)
    this.renderFrozenLayers(vp);

    // 更新选区
    this.updateSelectionUI();

    // 通知回调
    this.callbacks.onScroll?.(vp);
  }

  /** 渲染列头 (A, B, C...) */
  private renderColumnHeaders(startCol: number, endCol: number): void {
    this.headerRowEl.innerHTML = '';
    this.headerRowEl.style.width = `${this.totalWidth}px`;

    const fc = this.sheet.frozenColumnCount;
    // 渲染冻结列的列头 (始终显示, 通过 left+scrollLeft 反向抵消 headerRowEl 的滚动)
    for (let c = 0; c < fc; c++) {
      if (this.sheet.isColumnHidden(c)) continue;
      this.renderColumnHeaderCell(c, this.colOffsets[c] + this.scrollLeft);
    }
    // 渲染可见列的列头 (跳过冻结列和隐藏列)
    const realStart = Math.max(startCol, fc);
    for (let c = realStart; c <= endCol; c++) {
      if (this.sheet.isColumnHidden(c)) continue;
      this.renderColumnHeaderCell(c, this.colOffsets[c]);
    }

    // 同步滚动位置
    this.headerRowEl.scrollLeft = this.scrollLeft;
  }

  /** 渲染单个列头单元格 */
  private renderColumnHeaderCell(col: number, left: number): void {
    const el = document.createElement('div');
    el.className = 'zq-header-cell';
    el.dataset.col = String(col);
    el.textContent = SheetModel.columnToLetter(col);
    el.style.left = `${left}px`;
    el.style.width = `${this.sheet.getColumnWidth(col)}px`;
    el.style.height = '24px';
    el.style.position = 'absolute';

    // 高亮选中的列
    if (this.selection && col >= this.selection.startCol && col <= this.selection.endCol) {
      el.classList.add('selected');
    }

    this.headerRowEl.appendChild(el);
  }

  /** 渲染行头 (1, 2, 3...) */
  private renderRowHeaders(startRow: number, endRow: number): void {
    this.headerColEl.innerHTML = '';
    this.headerColEl.style.height = `${this.totalHeight}px`;

    const fr = this.sheet.frozenRowCount;
    // 渲染冻结行的行头 (始终显示, 通过 top+scrollTop 反向抵消 headerColEl 的滚动)
    for (let r = 0; r < fr; r++) {
      if (this.sheet.isRowHidden(r)) continue;
      this.renderRowHeaderCell(r, this.rowOffsets[r] + this.scrollTop);
    }
    // 渲染可见行的行头 (跳过冻结行和隐藏行)
    const realStart = Math.max(startRow, fr);
    for (let r = realStart; r <= endRow; r++) {
      if (this.sheet.isRowHidden(r)) continue;
      this.renderRowHeaderCell(r, this.rowOffsets[r]);
    }

    // 同步滚动位置
    this.headerColEl.scrollTop = this.scrollTop;
  }

  /** 渲染单个行头单元格 */
  private renderRowHeaderCell(row: number, top: number): void {
    const el = document.createElement('div');
    el.className = 'zq-header-cell';
    el.dataset.row = String(row);
    el.textContent = String(row + 1);
    el.style.top = `${top}px`;
    el.style.width = '40px';
    el.style.height = `${this.sheet.getRowHeight(row)}px`;
    el.style.position = 'absolute';

    // 高亮选中的行
    if (this.selection && row >= this.selection.startRow && row <= this.selection.endRow) {
      el.classList.add('selected');
    }

    this.headerColEl.appendChild(el);
  }

  /** 渲染单元格 (普通区域, 跳过冻结/隐藏行列) */
  private renderCells(vp: Viewport): void {
    // 清除旧的单元格
    const oldCells = this.canvasEl.querySelectorAll('.zq-cell');
    oldCells.forEach(el => el.remove());
    this.renderedCells.clear();

    const fr = this.sheet.frozenRowCount;
    const fc = this.sheet.frozenColumnCount;

    // 渲染可见区域的单元格 (跳过冻结行列和隐藏行列)
    for (let r = vp.startRow; r <= vp.endRow; r++) {
      if (this.sheet.isRowHidden(r)) continue;
      if (r < fr) continue; // 冻结行由冻结层渲染
      for (let c = vp.startCol; c <= vp.endCol; c++) {
        if (this.sheet.isColumnHidden(c)) continue;
        if (c < fc) continue; // 冻结列由冻结层渲染
        this.renderCell(r, c);
      }
    }

    // 渲染合并区域的单元格 (可能超出可见区域, 但仅在普通区域)
    const merges = this.sheet.getAllMerges();
    for (const mc of merges) {
      // 合并区域完全在冻结区域内 (由冻结层处理)
      if (mc.endRow < fr && mc.endColumn < fc) continue;
      // 合并区域主单元格在冻结区域内 (由冻结层处理)
      if (mc.startRow < fr && mc.startColumn < fc) continue;
      // 跳过完全在可见区域内的 (已经渲染)
      if (mc.startRow >= vp.startRow && mc.startRow <= vp.endRow &&
          mc.startColumn >= vp.startCol && mc.startColumn <= vp.endCol) {
        continue;
      }
      // 渲染与可见区域重叠的合并区域
      if (mc.endRow >= vp.startRow && mc.startRow <= vp.endRow &&
          mc.endColumn >= vp.startCol && mc.startColumn <= vp.endCol) {
        this.renderMergedCell(mc);
      }
    }
  }

  /** 渲染单个单元格 */
  private renderCell(row: number, col: number): void {
    // 检查是否是合并区域的从属单元格 (不渲染)
    const merge = this.sheet.getMerge(row, col);
    if (merge && (merge.startRow !== row || merge.startColumn !== col)) {
      return; // 从属单元格不渲染
    }

    const cell = this.sheet.getCell(row, col);
    const el = document.createElement('div');
    el.className = 'zq-cell';
    el.dataset.row = String(row);
    el.dataset.col = String(col);

    // 定位
    el.style.left = `${this.colOffsets[col]}px`;
    el.style.top = `${this.rowOffsets[row]}px`;
    el.style.width = `${this.sheet.getColumnWidth(col)}px`;
    el.style.height = `${this.sheet.getRowHeight(row)}px`;

    // 如果是合并区域的主单元格, 设置宽高跨越整个区域
    if (merge) {
      const mergeWidth = this.colOffsets[merge.endColumn + 1] - this.colOffsets[merge.startColumn];
      const mergeHeight = this.rowOffsets[merge.endRow + 1] - this.rowOffsets[merge.startRow];
      el.style.width = `${mergeWidth}px`;
      el.style.height = `${mergeHeight}px`;
      el.style.zIndex = '5';
    }

    // 应用样式
    const style = this.sheet.getCellStyle(row, col);
    this.styleEngine.applyStyle(el, style);

    // 设置内容
    if (cell) {
      let displayValue: string;
      if (cell.formula) {
        // 公式单元格显示计算结果 (由 EditEngine 设置 value)
        displayValue = this.formatCellValue(cell.value, style);
      } else {
        displayValue = this.formatCellValue(cell.value, style);
      }
      el.textContent = displayValue;
    }

    this.canvasEl.appendChild(el);
    this.renderedCells.set(`${row}:${col}`, el);
  }

  /** 渲染合并区域的主单元格 */
  private renderMergedCell(mc: MergedCell): void {
    this.renderCell(mc.startRow, mc.startColumn);
  }

  // ============================================================
  // 冻结区域渲染 — 独立层, 不随 canvasEl 滚动
  // ============================================================

  /** 渲染冻结区域 (冻结角 / 冻结行 / 冻结列) */
  private renderFrozenLayers(vp: Viewport): void {
    const fr = this.sheet.frozenRowCount;
    const fc = this.sheet.frozenColumnCount;

    // 无冻结时隐藏所有冻结层
    if (fr <= 0 && fc <= 0) {
      this.frozenCornerLayerEl.style.display = 'none';
      this.frozenTopLayerEl.style.display = 'none';
      this.frozenLeftLayerEl.style.display = 'none';
      return;
    }

    // 冻结角层 (前 fr 行 × 前 fc 列)
    if (fr > 0 && fc > 0) {
      this.frozenCornerLayerEl.style.display = 'block';
      this.frozenCornerLayerEl.style.width = `${this.freezeLeftWidth}px`;
      this.frozenCornerLayerEl.style.height = `${this.freezeTopHeight}px`;
      this.renderFrozenRegion(this.frozenCornerLayerEl, 0, fr - 1, 0, fc - 1, 0, 0);
    } else {
      this.frozenCornerLayerEl.style.display = 'none';
    }

    // 冻结行层 (前 fr 行 × 可见非冻结列)
    if (fr > 0) {
      this.frozenTopLayerEl.style.display = 'block';
      this.frozenTopLayerEl.style.left = `${40 + this.freezeLeftWidth}px`;
      this.frozenTopLayerEl.style.height = `${this.freezeTopHeight}px`;
      this.frozenTopLayerEl.style.width = `${this.viewportWidth - this.freezeLeftWidth}px`;
      // 可见列范围 (排除冻结列), 单元格 left 需要减去 scrollLeft 和 freezeLeftWidth
      const startCol = Math.max(vp.startCol, fc);
      const endCol = vp.endCol;
      this.renderFrozenRegion(this.frozenTopLayerEl, 0, fr - 1, startCol, endCol,
        -this.scrollLeft - this.freezeLeftWidth, 0);
    } else {
      this.frozenTopLayerEl.style.display = 'none';
    }

    // 冻结列层 (可见非冻结行 × 前 fc 列)
    if (fc > 0) {
      this.frozenLeftLayerEl.style.display = 'block';
      this.frozenLeftLayerEl.style.top = `${24 + this.freezeTopHeight}px`;
      this.frozenLeftLayerEl.style.width = `${this.freezeLeftWidth}px`;
      this.frozenLeftLayerEl.style.height = `${this.viewportHeight - this.freezeTopHeight}px`;
      // 可见行范围 (排除冻结行), 单元格 top 需要减去 scrollTop 和 freezeTopHeight
      const startRow = Math.max(vp.startRow, fr);
      const endRow = vp.endRow;
      this.renderFrozenRegion(this.frozenLeftLayerEl, startRow, endRow, 0, fc - 1,
        0, -this.scrollTop - this.freezeTopHeight);
    } else {
      this.frozenLeftLayerEl.style.display = 'none';
    }
  }

  /**
   * 渲染冻结区域内的单元格到指定层
   * @param layer 目标 DOM 层
   * @param startRow 渲染起始行 (含)
   * @param endRow 渲染结束行 (含)
   * @param startCol 渲染起始列 (含)
   * @param endCol 渲染结束列 (含)
   * @param offsetX 单元格 left 的额外偏移 (用于抵消滚动)
   * @param offsetY 单元格 top 的额外偏移 (用于抵消滚动)
   */
  private renderFrozenRegion(
    layer: HTMLDivElement,
    startRow: number, endRow: number,
    startCol: number, endCol: number,
    offsetX: number, offsetY: number
  ): void {
    layer.innerHTML = '';

    for (let r = startRow; r <= endRow; r++) {
      if (this.sheet.isRowHidden(r)) continue;
      for (let c = startCol; c <= endCol; c++) {
        if (this.sheet.isColumnHidden(c)) continue;

        // 检查是否是合并区域的从属单元格 (不渲染)
        const merge = this.sheet.getMerge(r, c);
        if (merge && (merge.startRow !== r || merge.startColumn !== c)) {
          continue;
        }

        const cell = this.sheet.getCell(r, c);
        const el = document.createElement('div');
        el.className = 'zq-cell';
        el.dataset.row = String(r);
        el.dataset.col = String(c);

        // 定位 (相对于层左上角, 加上偏移)
        el.style.left = `${this.colOffsets[c] + offsetX}px`;
        el.style.top = `${this.rowOffsets[r] + offsetY}px`;
        el.style.width = `${this.sheet.getColumnWidth(c)}px`;
        el.style.height = `${this.sheet.getRowHeight(r)}px`;

        // 合并区域主单元格: 设置跨越宽高
        if (merge) {
          const mergeWidth = this.colOffsets[merge.endColumn + 1] - this.colOffsets[merge.startColumn];
          const mergeHeight = this.rowOffsets[merge.endRow + 1] - this.rowOffsets[merge.startRow];
          el.style.width = `${mergeWidth}px`;
          el.style.height = `${mergeHeight}px`;
          el.style.zIndex = '5';
        }

        // 应用样式
        const style = this.sheet.getCellStyle(r, c);
        this.styleEngine.applyStyle(el, style);

        // 设置内容
        if (cell) {
          el.textContent = this.formatCellValue(cell.value, style);
        }

        layer.appendChild(el);
        // 冻结层的单元格也加入 renderedCells 映射, 供 edit-engine 定位
        this.renderedCells.set(`${r}:${c}`, el);
      }
    }
  }

  /** 格式化单元格显示值 */
  private formatCellValue(value: string | number | boolean, style: CellStyle): string {
    if (value === null || value === undefined || value === '') return '';
    if (typeof value === 'boolean') return value ? 'TRUE' : 'FALSE';
    if (typeof value === 'number') {
      const fmt = style?.numberFormat || 'General';
      return this.styleEngine.formatNumber(value, fmt);
    }
    return String(value);
  }

  // ============================================================
  // 选区管理
  // ============================================================

  /** 设置选区 */
  setSelection(range: SelectionRange): void {
    this.selection = { ...range };
    this.normalizeSelection();
    this.updateSelectionUI();
    this.callbacks.onSelectionChange?.(this.selection);
  }

  /** 获取选区 */
  getSelection(): SelectionRange | null {
    return this.selection ? { ...this.selection } : null;
  }

  /** 规范化选区 (确保 start <= end) */
  private normalizeSelection(): void {
    if (!this.selection) return;
    const sr = Math.min(this.selection.startRow, this.selection.endRow);
    const er = Math.max(this.selection.startRow, this.selection.endRow);
    const sc = Math.min(this.selection.startCol, this.selection.endCol);
    const ec = Math.max(this.selection.startCol, this.selection.endCol);
    this.selection.startRow = sr;
    this.selection.endRow = er;
    this.selection.startCol = sc;
    this.selection.endCol = ec;
  }

  /** 更新选区 UI */
  private updateSelectionUI(): void {
    if (!this.selection) {
      this.selectionEl.style.display = 'none';
      return;
    }

    const x = this.colOffsets[this.selection.startCol];
    const y = this.rowOffsets[this.selection.startRow];
    const w = this.colOffsets[this.selection.endCol + 1] - x;
    const h = this.rowOffsets[this.selection.endRow + 1] - y;

    this.selectionEl.style.display = 'block';
    this.selectionEl.style.left = `${x}px`;
    this.selectionEl.style.top = `${y}px`;
    this.selectionEl.style.width = `${w}px`;
    this.selectionEl.style.height = `${h}px`;
  }

  // ============================================================
  // 公开方法
  // ============================================================

  /** 更新数据模型后刷新渲染 */
  refresh(): void {
    this.recalculateOffsets();
    this.render();
  }

  /** 刷新单个单元格 */
  refreshCell(row: number, col: number): void {
    const key = `${row}:${col}`;
    const el = this.renderedCells.get(key);
    if (el) {
      const cell = this.sheet.getCell(row, col);
      const style = this.sheet.getCellStyle(row, col);
      this.styleEngine.applyStyle(el, style);
      if (cell) {
        el.textContent = this.formatCellValue(cell.value, style);
      } else {
        el.textContent = '';
      }
    } else {
      // 单元格不在渲染范围内, 重新渲染
      this.render();
    }
  }

  /** 滚动到指定单元格 */
  scrollToCell(row: number, col: number): void {
    const y = this.rowOffsets[row];
    const x = this.colOffsets[col];
    const cellH = this.sheet.getRowHeight(row);
    const cellW = this.sheet.getColumnWidth(col);

    if (y < this.scrollTop) {
      this.canvasEl.scrollTop = y;
    } else if (y + cellH > this.scrollTop + this.viewportHeight) {
      this.canvasEl.scrollTop = y + cellH - this.viewportHeight;
    }

    if (x < this.scrollLeft) {
      this.canvasEl.scrollLeft = x;
    } else if (x + cellW > this.scrollLeft + this.viewportWidth) {
      this.canvasEl.scrollLeft = x + cellW - this.viewportWidth;
    }

    this.scrollTop = this.canvasEl.scrollTop;
    this.scrollLeft = this.canvasEl.scrollLeft;
    this.render();
  }

  /** 更新工作表 */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
    this.selection = null;
    this.recalculateOffsets();
    this.render();
  }

  /** 获取已渲染的单元格 DOM 元素 (供 edit-engine 定位编辑框) */
  getRenderedCell(row: number, col: number): HTMLDivElement | null {
    return this.renderedCells.get(`${row}:${col}`) ?? null;
  }

  /** 销毁 */
  destroy(): void {
    this.gridEl.remove();
  }
}
