/**
 * 拖拽管理器 — 行/列拖拽移动 (数据操作 + DOM 交互)
 * 全部原创实现
 *
 * 职责:
 *   1. 行拖拽移动 dragRow / dragRows — 将一行/多行移动到新位置
 *   2. 列拖拽移动 dragColumn / dragColumns — 将一列/多列移动到新位置
 *   3. DOM 交互:
 *      - 行号/列号区域可拖拽 (mousedown 触发)
 *      - 拖拽时显示半透明预览 (跟随鼠标)
 *      - 拖拽时在目标位置显示插入指示线
 *      - 支持多选拖拽 (先选中多行/列再拖拽)
 *   4. 数据操作:
 *      - 移动行时, 单元格数据/行高/合并单元格/样式/隐藏状态一起移动
 *      - 移动列时同理
 *      - 公式引用自动调整 (所有被影响的公式单元格的引用行列号重映射)
 *
 * 语义约定:
 *   - toRow/toCol 表示 "插入到该索引之前" (insert before)
 *   - toRow = rowCount 表示插入到末尾
 *   - toRow 在 [fromStart, fromEnd+1] 范围内视为无效 (拖入自身), 不执行移动
 */

import { SheetModel, Cell, MergedCell } from './data-model';
import { RenderEngine } from './render-engine';

/** 拖拽状态 */
export interface DragState {
  /** 拖拽类型: 行或列 */
  type: 'row' | 'column';
  /** 源起始索引 (含) */
  fromStart: number;
  /** 源结束索引 (含) */
  fromEnd: number;
  /** 目标索引 (插入到此索引之前) */
  toIndex: number;
  /** 是否正在拖拽 (已越过拖拽阈值) */
  dragging: boolean;
}

/** 拖拽管理器回调 */
export interface DragManagerCallbacks {
  /** 内容变更 (拖拽完成后触发, 用于公式重算等) */
  onContentChange?: () => void;
  /** 拖拽开始 */
  onDragStart?: (state: DragState) => void;
  /** 拖拽结束 */
  onDragEnd?: (state: DragState) => void;
}

/** 拖拽阈值 (像素), 超过此距离才认定为拖拽而非点击 */
const DRAG_THRESHOLD = 5;

/** 拖拽预览的半透明背景色 */
const PREVIEW_COLOR = 'rgba(71, 135, 245, 0.2)';
const PREVIEW_BORDER = 'rgba(71, 135, 245, 0.6)';
const INDICATOR_COLOR = '#4787f5';

export class DragManager {
  private container: HTMLElement;
  private sheet: SheetModel;
  private renderEngine: RenderEngine;
  private callbacks: DragManagerCallbacks;

  /** 拖拽状态 */
  private dragState: DragState | null = null;
  /** 鼠标按下时的起始坐标 (用于计算拖拽距离) */
  private dragStartX: number = 0;
  private dragStartY: number = 0;

  /** 拖拽预览 DOM 元素 */
  private previewEl: HTMLDivElement | null = null;
  /** 插入指示线 DOM 元素 */
  private indicatorEl: HTMLDivElement | null = null;

  /** 已绑定的事件处理器引用 (用于解绑) */
  private boundMouseMove: (e: MouseEvent) => void;
  private boundMouseUp: (e: MouseEvent) => void;
  private boundMouseDown: (e: MouseEvent) => void;

  constructor(
    container: HTMLElement,
    sheet: SheetModel,
    renderEngine: RenderEngine,
    callbacks: DragManagerCallbacks = {}
  ) {
    this.container = container;
    this.sheet = sheet;
    this.renderEngine = renderEngine;
    this.callbacks = callbacks;

    // 预绑定事件处理器 (保持引用以便解绑)
    this.boundMouseDown = (e: MouseEvent) => this.handleMouseDown(e);
    this.boundMouseMove = (e: MouseEvent) => this.handleMouseMove(e);
    this.boundMouseUp = (e: MouseEvent) => this.handleMouseUp(e);

    this.setupStyles();
    this.setupDOM();
    // 使用捕获阶段监听 mousedown, 以便在 RenderEngine 修改选区之前记录当前选区
    this.container.addEventListener('mousedown', this.boundMouseDown, true);
  }

  /** 更新工作表引用 (切换工作表时调用) */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
  }

  // ============================================================
  // 样式与 DOM 初始化
  // ============================================================

  private setupStyles(): void {
    const styleId = 'zq-drag-manager-styles';
    if (document.getElementById(styleId)) return;
    const style = document.createElement('style');
    style.id = styleId;
    style.textContent = `
      .zq-drag-preview {
        position: absolute;
        background: ${PREVIEW_COLOR};
        border: 1px solid ${PREVIEW_BORDER};
        pointer-events: none;
        z-index: 50;
        display: none;
        box-sizing: border-box;
      }
      .zq-drag-indicator {
        position: absolute;
        background: ${INDICATOR_COLOR};
        pointer-events: none;
        z-index: 51;
        display: none;
        box-sizing: border-box;
      }
      .zq-drag-indicator.row {
        height: 2px;
        left: 40px;
        right: 0;
      }
      .zq-drag-indicator.column {
        width: 2px;
        top: 24px;
        bottom: 0;
      }
      .zq-drag-indicator::after {
        content: '';
        position: absolute;
        width: 8px;
        height: 8px;
        background: ${INDICATOR_COLOR};
        border-radius: 50%;
      }
      .zq-drag-indicator.row::after {
        left: 32px;
        top: -3px;
      }
      .zq-drag-indicator.column::after {
        top: 16px;
        left: -3px;
      }
    `;
    document.head.appendChild(style);
  }

  private setupDOM(): void {
    this.previewEl = document.createElement('div');
    this.previewEl.className = 'zq-drag-preview';
    this.indicatorEl = document.createElement('div');
    this.indicatorEl.className = 'zq-drag-indicator';
    this.container.appendChild(this.previewEl);
    this.container.appendChild(this.indicatorEl);
  }

  // ============================================================
  // 数据操作 — 行拖拽
  // ============================================================

  /**
   * 单行拖拽移动
   * @param fromRow 源行号 (0-based)
   * @param toRow 目标行号 (插入到此行之前, 0-based)
   */
  dragRow(fromRow: number, toRow: number): void {
    this.dragRows(fromRow, fromRow, toRow);
  }

  /**
   * 多行拖拽移动
   * @param fromStart 源起始行号 (0-based, 含)
   * @param fromEnd 源结束行号 (0-based, 含)
   * @param toRow 目标行号 (插入到此行之前, 0-based; = rowCount 表示末尾)
   */
  dragRows(fromStart: number, fromEnd: number, toRow: number): void {
    const count = fromEnd - fromStart + 1;

    // 校验: 目标在源范围内 (含末尾+1) → 无效或无操作
    if (toRow >= fromStart && toRow <= fromEnd + 1) return;
    // 边界校验
    if (toRow < 0 || toRow > this.sheet.rowCount) return;
    if (fromStart < 0 || fromEnd >= this.sheet.rowCount) return;

    // 1. 提取源数据
    const sourceCells = this.extractRowCells(fromStart, fromEnd);
    const sourceHeights = this.extractRowHeights(fromStart, fromEnd);
    const sourceMerges = this.extractRowMerges(fromStart, fromEnd);
    const sourceHidden = this.extractRowHidden(fromStart, fromEnd);

    // 2. 删除源行 (自动处理单元格移除、合并区域调整、行高/隐藏行平移)
    this.sheet.deleteRow(fromStart, count);

    // 3. 计算插入点 (删除后坐标)
    // 若 toRow > fromEnd (向下移), 删除后目标行上移 count
    // 若 toRow <= fromStart (向上移), 删除后目标行不变
    const insertAt = toRow > fromEnd ? toRow - count : toRow;

    // 4. 插入空行 (创建空间)
    this.sheet.insertRow(insertAt, count);

    // 5. 恢复源数据到新位置
    this.restoreRowData(insertAt, fromStart, sourceCells, sourceHeights, sourceMerges, sourceHidden);

    // 6. 调整所有公式中的行引用
    this.adjustFormulasForRowMove(fromStart, fromEnd, insertAt);

    // 7. 刷新渲染
    this.renderEngine.refresh();
    this.callbacks.onContentChange?.();
  }

  // ============================================================
  // 数据操作 — 列拖拽
  // ============================================================

  /**
   * 单列拖拽移动
   * @param fromCol 源列号 (0-based)
   * @param toCol 目标列号 (插入到此列之前, 0-based)
   */
  dragColumn(fromCol: number, toCol: number): void {
    this.dragColumns(fromCol, fromCol, toCol);
  }

  /**
   * 多列拖拽移动
   * @param fromStart 源起始列号 (0-based, 含)
   * @param fromEnd 源结束列号 (0-based, 含)
   * @param toCol 目标列号 (插入到此列之前, 0-based; = columnCount 表示末尾)
   */
  dragColumns(fromStart: number, fromEnd: number, toCol: number): void {
    const count = fromEnd - fromStart + 1;

    if (toCol >= fromStart && toCol <= fromEnd + 1) return;
    if (toCol < 0 || toCol > this.sheet.columnCount) return;
    if (fromStart < 0 || fromEnd >= this.sheet.columnCount) return;

    // 1. 提取源数据
    const sourceCells = this.extractColCells(fromStart, fromEnd);
    const sourceWidths = this.extractColWidths(fromStart, fromEnd);
    const sourceMerges = this.extractColMerges(fromStart, fromEnd);
    const sourceHidden = this.extractColHidden(fromStart, fromEnd);

    // 2. 删除源列
    this.sheet.deleteColumn(fromStart, count);

    // 3. 计算插入点
    const insertAt = toCol > fromEnd ? toCol - count : toCol;

    // 4. 插入空列
    this.sheet.insertColumn(insertAt, count);

    // 5. 恢复源数据
    this.restoreColData(insertAt, fromStart, sourceCells, sourceWidths, sourceMerges, sourceHidden);

    // 6. 调整所有公式中的列引用
    this.adjustFormulasForColMove(fromStart, fromEnd, insertAt);

    // 7. 刷新渲染
    this.renderEngine.refresh();
    this.callbacks.onContentChange?.();
  }

  // ============================================================
  // 数据提取/恢复 — 行
  // ============================================================

  /** 提取源行范围内的单元格 (深拷贝), 跳过部分重叠合并区域的从属单元格 */
  private extractRowCells(fromStart: number, fromEnd: number): Cell[] {
    const cells: Cell[] = [];
    for (let r = fromStart; r <= fromEnd; r++) {
      for (let c = 0; c < this.sheet.columnCount; c++) {
        const cell = this.sheet.getCell(r, c);
        if (!cell) continue;

        // 检查是否是合并区域主单元格, 若合并跨越源范围则跳过 (部分重叠不移动)
        const merge = this.sheet.getMerge(r, c);
        if (merge && merge.startRow === r && merge.startColumn === c) {
          if (merge.endRow > fromEnd) continue; // 合并区域部分在源范围外, 不移动
        }

        cells.push(this.cloneCell(cell));
      }
    }
    return cells;
  }

  /** 提取源行范围内的行高 */
  private extractRowHeights(fromStart: number, fromEnd: number): number[] {
    const heights: number[] = [];
    for (let r = fromStart; r <= fromEnd; r++) {
      heights.push(this.sheet.getRowHeight(r));
    }
    return heights;
  }

  /** 提取完全在源行范围内的合并区域 */
  private extractRowMerges(fromStart: number, fromEnd: number): MergedCell[] {
    const merges: MergedCell[] = [];
    for (const mc of this.sheet.getAllMerges()) {
      if (mc.startRow >= fromStart && mc.endRow <= fromEnd) {
        merges.push({ ...mc });
      }
    }
    return merges;
  }

  /** 提取源行范围内的隐藏状态 (存储相对偏移) */
  private extractRowHidden(fromStart: number, fromEnd: number): number[] {
    const hidden: number[] = [];
    for (let r = fromStart; r <= fromEnd; r++) {
      if (this.sheet.isRowHidden(r)) {
        hidden.push(r - fromStart);
      }
    }
    return hidden;
  }

  /** 恢复行数据到新位置 */
  private restoreRowData(
    insertAt: number,
    fromStart: number,
    cells: Cell[],
    heights: number[],
    merges: MergedCell[],
    hidden: number[]
  ): void {
    const offset = insertAt - fromStart;

    // 恢复合并区域 (先恢复合并, 确保主单元格存在)
    for (const mc of merges) {
      this.sheet.addMerge(
        mc.startRow + offset, mc.startColumn,
        mc.endRow + offset, mc.endColumn
      );
    }

    // 恢复单元格 (值 + 类型 + 公式 + 样式)
    for (const cell of cells) {
      const newRow = cell.row + offset;
      this.restoreCell(newRow, cell.column, cell);
    }

    // 恢复行高
    for (let i = 0; i < heights.length; i++) {
      this.sheet.setRowHeight(insertAt + i, heights[i]);
    }

    // 恢复隐藏状态
    for (const offsetVal of hidden) {
      this.sheet.hideRows(insertAt + offsetVal, 1);
    }
  }

  // ============================================================
  // 数据提取/恢复 — 列
  // ============================================================

  /** 提取源列范围内的单元格 (深拷贝) */
  private extractColCells(fromStart: number, fromEnd: number): Cell[] {
    const cells: Cell[] = [];
    for (let c = fromStart; c <= fromEnd; c++) {
      for (let r = 0; r < this.sheet.rowCount; r++) {
        const cell = this.sheet.getCell(r, c);
        if (!cell) continue;

        const merge = this.sheet.getMerge(r, c);
        if (merge && merge.startRow === r && merge.startColumn === c) {
          if (merge.endColumn > fromEnd) continue;
        }

        cells.push(this.cloneCell(cell));
      }
    }
    return cells;
  }

  /** 提取源列范围内的列宽 */
  private extractColWidths(fromStart: number, fromEnd: number): number[] {
    const widths: number[] = [];
    for (let c = fromStart; c <= fromEnd; c++) {
      widths.push(this.sheet.getColumnWidth(c));
    }
    return widths;
  }

  /** 提取完全在源列范围内的合并区域 */
  private extractColMerges(fromStart: number, fromEnd: number): MergedCell[] {
    const merges: MergedCell[] = [];
    for (const mc of this.sheet.getAllMerges()) {
      if (mc.startColumn >= fromStart && mc.endColumn <= fromEnd) {
        merges.push({ ...mc });
      }
    }
    return merges;
  }

  /** 提取源列范围内的隐藏状态 */
  private extractColHidden(fromStart: number, fromEnd: number): number[] {
    const hidden: number[] = [];
    for (let c = fromStart; c <= fromEnd; c++) {
      if (this.sheet.isColumnHidden(c)) {
        hidden.push(c - fromStart);
      }
    }
    return hidden;
  }

  /** 恢复列数据到新位置 */
  private restoreColData(
    insertAt: number,
    fromStart: number,
    cells: Cell[],
    widths: number[],
    merges: MergedCell[],
    hidden: number[]
  ): void {
    const offset = insertAt - fromStart;

    // 恢复合并区域
    for (const mc of merges) {
      this.sheet.addMerge(
        mc.startRow, mc.startColumn + offset,
        mc.endRow, mc.endColumn + offset
      );
    }

    // 恢复单元格
    for (const cell of cells) {
      const newCol = cell.column + offset;
      this.restoreCell(cell.row, newCol, cell);
    }

    // 恢复列宽
    for (let i = 0; i < widths.length; i++) {
      this.sheet.setColumnWidth(insertAt + i, widths[i]);
    }

    // 恢复隐藏状态
    for (const offsetVal of hidden) {
      this.sheet.hideColumns(insertAt + offsetVal, 1);
    }
  }

  // ============================================================
  // 单元格恢复辅助
  // ============================================================

  /** 恢复单个单元格的完整数据 (值/类型/公式/样式) */
  private restoreCell(row: number, col: number, source: Cell): void {
    if (source.formula) {
      this.sheet.setCellFormula(row, col, source.formula);
      const cell = this.sheet.getCell(row, col);
      if (cell) {
        cell.value = source.value;
        cell.type = source.type;
      }
    } else {
      this.sheet.setCellValue(row, col, source.value, source.type);
    }
    if (source.style) {
      this.sheet.setCellStyle(row, col, source.style);
    }
  }

  /** 深拷贝单元格 */
  private cloneCell(cell: Cell): Cell {
    return {
      row: cell.row,
      column: cell.column,
      type: cell.type,
      value: cell.value,
      formula: cell.formula,
      style: cell.style ? { ...cell.style } : undefined,
    };
  }

  // ============================================================
  // 公式引用调整
  // ============================================================

  /**
   * 调整所有公式中的行引用 (行移动后)
   * 将旧行号映射到新行号, 列号不变
   * @param fromStart 源起始行
   * @param fromEnd 源结束行
   * @param insertAt 移动后源数据的新起始行
   */
  private adjustFormulasForRowMove(
    fromStart: number, fromEnd: number, insertAt: number
  ): void {
    const count = fromEnd - fromStart + 1;

    /** 旧行号 → 新行号映射函数 */
    const remapRow = (oldRow: number): number => {
      // 源范围内的行 → 移动到新位置
      if (oldRow >= fromStart && oldRow <= fromEnd) {
        return insertAt + (oldRow - fromStart);
      }
      if (insertAt > fromEnd) {
        // 向下移动: [fromEnd+1, insertAt+count-1] 范围的行上移 count
        if (oldRow > fromEnd && oldRow < insertAt + count) {
          return oldRow - count;
        }
      } else {
        // 向上移动: [insertAt, fromStart-1] 范围的行下移 count
        if (oldRow >= insertAt && oldRow < fromStart) {
          return oldRow + count;
        }
      }
      return oldRow;
    };

    // 遍历所有单元格, 调整公式中的行引用
    const allCells = this.sheet.getAllCells();
    for (const cell of allCells) {
      if (cell.formula) {
        cell.formula = this.remapFormulaRows(cell.formula, remapRow);
      }
    }
  }

  /**
   * 调整所有公式中的列引用 (列移动后)
   * @param fromStart 源起始列
   * @param fromEnd 源结束列
   * @param insertAt 移动后源数据的新起始列
   */
  private adjustFormulasForColMove(
    fromStart: number, fromEnd: number, insertAt: number
  ): void {
    const count = fromEnd - fromStart + 1;

    /** 旧列号 → 新列号映射函数 */
    const remapCol = (oldCol: number): number => {
      if (oldCol >= fromStart && oldCol <= fromEnd) {
        return insertAt + (oldCol - fromStart);
      }
      if (insertAt > fromEnd) {
        if (oldCol > fromEnd && oldCol < insertAt + count) {
          return oldCol - count;
        }
      } else {
        if (oldCol >= insertAt && oldCol < fromStart) {
          return oldCol + count;
        }
      }
      return oldCol;
    };

    const allCells = this.sheet.getAllCells();
    for (const cell of allCells) {
      if (cell.formula) {
        cell.formula = this.remapFormulaCols(cell.formula, remapCol);
      }
    }
  }

  /**
   * 重映射公式中的行引用
   * 匹配格式: [列字母][行号] (如 A1, $B$2, C$3)
   * 行移动时列字母不变, 仅行号重映射
   */
  private remapFormulaRows(formula: string, remap: (row: number) => number): string {
    return formula.replace(/(\$?)([A-Z]+)(\$?)(\d+)/g, (match, colAbs, col, rowAbs, rowStr) => {
      const oldRow = parseInt(rowStr, 10) - 1; // 转为 0-based
      const newRow = remap(oldRow);
      return `${colAbs}${col}${rowAbs}${newRow + 1}`;
    });
  }

  /**
   * 重映射公式中的列引用
   * 列移动时行号不变, 仅列字母重映射
   */
  private remapFormulaCols(formula: string, remap: (col: number) => number): string {
    return formula.replace(/(\$?)([A-Z]+)(\$?)(\d+)/g, (match, colAbs, col, rowAbs, rowStr) => {
      const oldCol = SheetModel.letterToColumn(col);
      const newCol = remap(oldCol);
      return `${colAbs}${SheetModel.columnToLetter(newCol)}${rowAbs}${rowStr}`;
    });
  }

  // ============================================================
  // DOM 交互 — 拖拽事件处理
  // ============================================================

  /**
   * mousedown 处理 (捕获阶段, 在 RenderEngine 修改选区之前记录选区)
   */
  private handleMouseDown(e: MouseEvent): void {
    if (e.button !== 0) return; // 只处理左键
    const target = e.target as HTMLElement;
    if (!target || !target.classList.contains('zq-header-cell')) return;

    // 记录当前选区 (在 RenderEngine 改变它之前)
    const currentSel = this.renderEngine.getSelection();

    // 行头 (data-row 存在) → 行拖拽
    if (target.dataset.row !== undefined) {
      const row = parseInt(target.dataset.row, 10);
      let fromStart = row;
      let fromEnd = row;
      // 若当前选区是整行选择且包含该行, 则拖拽整个选区
      if (currentSel &&
          currentSel.startCol === 0 &&
          currentSel.endCol === this.sheet.columnCount - 1 &&
          row >= currentSel.startRow && row <= currentSel.endRow) {
        fromStart = currentSel.startRow;
        fromEnd = currentSel.endRow;
      }
      this.dragState = {
        type: 'row', fromStart, fromEnd, toIndex: -1, dragging: false,
      };
      this.dragStartX = e.clientX;
      this.dragStartY = e.clientY;
      this.attachDragListeners();
    }
    // 列头 (data-col 存在) → 列拖拽
    else if (target.dataset.col !== undefined) {
      const col = parseInt(target.dataset.col, 10);
      let fromStart = col;
      let fromEnd = col;
      if (currentSel &&
          currentSel.startRow === 0 &&
          currentSel.endRow === this.sheet.rowCount - 1 &&
          col >= currentSel.startCol && col <= currentSel.endCol) {
        fromStart = currentSel.startCol;
        fromEnd = currentSel.endCol;
      }
      this.dragState = {
        type: 'column', fromStart, fromEnd, toIndex: -1, dragging: false,
      };
      this.dragStartX = e.clientX;
      this.dragStartY = e.clientY;
      this.attachDragListeners();
    }
  }

  /** 绑定 document 级别的 mousemove/mouseup (拖拽期间) */
  private attachDragListeners(): void {
    document.addEventListener('mousemove', this.boundMouseMove);
    document.addEventListener('mouseup', this.boundMouseUp);
  }

  /** 解绑 document 级别的 mousemove/mouseup */
  private detachDragListeners(): void {
    document.removeEventListener('mousemove', this.boundMouseMove);
    document.removeEventListener('mouseup', this.boundMouseUp);
  }

  /**
   * mousemove 处理 (拖拽中)
   */
  private handleMouseMove(e: MouseEvent): void {
    if (!this.dragState) return;

    const dx = e.clientX - this.dragStartX;
    const dy = e.clientY - this.dragStartY;

    // 拖拽阈值检测
    if (!this.dragState.dragging) {
      if (this.dragState.type === 'row' && Math.abs(dy) < DRAG_THRESHOLD) return;
      if (this.dragState.type === 'column' && Math.abs(dx) < DRAG_THRESHOLD) return;
      this.dragState.dragging = true;
      this.callbacks.onDragStart?.(this.dragState);
    }

    // 更新预览和指示线
    if (this.dragState.type === 'row') {
      this.updateRowDragVisuals(e.clientY);
    } else {
      this.updateColumnDragVisuals(e.clientX);
    }
  }

  /**
   * mouseup 处理 (拖拽结束)
   */
  private handleMouseUp(_e: MouseEvent): void {
    if (!this.dragState) return;

    this.detachDragListeners();

    const wasDragging = this.dragState.dragging;
    const state = this.dragState;

    // 隐藏预览和指示线
    if (this.previewEl) this.previewEl.style.display = 'none';
    if (this.indicatorEl) this.indicatorEl.style.display = 'none';

    this.dragState = null;

    // 执行移动
    if (wasDragging && state.toIndex >= 0) {
      if (state.type === 'row') {
        this.dragRows(state.fromStart, state.fromEnd, state.toIndex);
      } else {
        this.dragColumns(state.fromStart, state.fromEnd, state.toIndex);
      }
      this.callbacks.onDragEnd?.(state);
    }
  }

  // ============================================================
  // DOM 交互 — 预览与指示线更新
  // ============================================================

  /**
   * 更新行拖拽的预览和指示线
   * @param mouseClientY 鼠标视口 Y 坐标
   */
  private updateRowDragVisuals(mouseClientY: number): void {
    if (!this.dragState || !this.previewEl || !this.indicatorEl) return;

    const containerRect = this.container.getBoundingClientRect();
    const canvasEl = this.getCanvasEl();
    if (!canvasEl) return;
    const canvasRect = canvasEl.getBoundingClientRect();

    // 鼠标在 canvas 内容中的 Y (含滚动偏移)
    const mouseYInContent = mouseClientY - canvasRect.top + canvasEl.scrollTop;

    // 计算目标行 (插入到此行之前)
    const targetRow = this.findRowFromY(mouseYInContent);

    // 源区域高度
    const dragHeight = this.calcRowsHeight(this.dragState.fromStart, this.dragState.fromEnd);

    // 预览 (半透明, 跟随鼠标)
    this.previewEl.style.display = 'block';
    this.previewEl.style.left = '40px';
    this.previewEl.style.right = '0';
    this.previewEl.style.height = `${dragHeight}px`;
    // 预览顶部 = 鼠标 Y (相对容器) - 源高度/2, 限制在 canvas 区域内
    let previewTop = mouseClientY - containerRect.top - dragHeight / 2;
    const minTop = canvasRect.top - containerRect.top;
    const maxTop = canvasRect.bottom - containerRect.top - dragHeight;
    previewTop = Math.max(minTop, Math.min(maxTop, previewTop));
    this.previewEl.style.top = `${previewTop}px`;

    // 校验目标是否有效 (不能拖入自身)
    if (targetRow >= this.dragState.fromStart && targetRow <= this.dragState.fromEnd + 1) {
      this.indicatorEl.style.display = 'none';
      this.dragState.toIndex = -1;
      return;
    }

    this.dragState.toIndex = targetRow;

    // 指示线 (水平线, 在目标行的上边界)
    this.indicatorEl.className = 'zq-drag-indicator row';
    this.indicatorEl.style.display = 'block';
    // 行上边界在 canvas 内容中的 Y, 转换为容器坐标
    const rowTopInContent = this.getRowOffset(targetRow);
    const indicatorTop = canvasRect.top - containerRect.top + rowTopInContent - canvasEl.scrollTop;
    this.indicatorEl.style.top = `${indicatorTop}px`;
  }

  /**
   * 更新列拖拽的预览和指示线
   * @param mouseClientX 鼠标视口 X 坐标
   */
  private updateColumnDragVisuals(mouseClientX: number): void {
    if (!this.dragState || !this.previewEl || !this.indicatorEl) return;

    const containerRect = this.container.getBoundingClientRect();
    const canvasEl = this.getCanvasEl();
    if (!canvasEl) return;
    const canvasRect = canvasEl.getBoundingClientRect();

    // 鼠标在 canvas 内容中的 X (含滚动偏移)
    const mouseXInContent = mouseClientX - canvasRect.left + canvasEl.scrollLeft;

    // 计算目标列 (插入到此列之前)
    const targetCol = this.findColFromX(mouseXInContent);

    // 源区域宽度
    const dragWidth = this.calcColsWidth(this.dragState.fromStart, this.dragState.fromEnd);

    // 预览
    this.previewEl.style.display = 'block';
    this.previewEl.style.top = '24px';
    this.previewEl.style.bottom = '0';
    this.previewEl.style.width = `${dragWidth}px`;
    let previewLeft = mouseClientX - containerRect.left - dragWidth / 2;
    const minLeft = canvasRect.left - containerRect.left;
    const maxLeft = canvasRect.right - containerRect.left - dragWidth;
    previewLeft = Math.max(minLeft, Math.min(maxLeft, previewLeft));
    this.previewEl.style.left = `${previewLeft}px`;

    // 校验目标有效性
    if (targetCol >= this.dragState.fromStart && targetCol <= this.dragState.fromEnd + 1) {
      this.indicatorEl.style.display = 'none';
      this.dragState.toIndex = -1;
      return;
    }

    this.dragState.toIndex = targetCol;

    // 指示线 (垂直线, 在目标列的左边界)
    this.indicatorEl.className = 'zq-drag-indicator column';
    this.indicatorEl.style.display = 'block';
    const colLeftInContent = this.getColOffset(targetCol);
    const indicatorLeft = canvasRect.left - containerRect.left + colLeftInContent - canvasEl.scrollLeft;
    this.indicatorEl.style.left = `${indicatorLeft}px`;
  }

  // ============================================================
  // 位置计算辅助
  // ============================================================

  /** 获取 canvas DOM 元素 */
  private getCanvasEl(): HTMLElement | null {
    return this.container.querySelector('.zq-sheet-canvas');
  }

  /**
   * 根据 Y 坐标 (canvas 内容坐标) 查找目标行索引
   * 返回应该插入到此行之前的位置 (0 ~ rowCount)
   * 以每行中点为分界: 鼠标在上半部分 → 插入到此行之前; 下半部分 → 插入到下一行之前
   */
  private findRowFromY(y: number): number {
    let acc = 0;
    for (let r = 0; r < this.sheet.rowCount; r++) {
      if (this.sheet.isRowHidden(r)) continue;
      const h = this.sheet.getRowHeight(r);
      if (y < acc + h / 2) return r;
      acc += h;
    }
    return this.sheet.rowCount; // 超过末尾 → 插入到最后
  }

  /**
   * 根据 X 坐标 (canvas 内容坐标) 查找目标列索引
   */
  private findColFromX(x: number): number {
    let acc = 0;
    for (let c = 0; c < this.sheet.columnCount; c++) {
      if (this.sheet.isColumnHidden(c)) continue;
      const w = this.sheet.getColumnWidth(c);
      if (x < acc + w / 2) return c;
      acc += w;
    }
    return this.sheet.columnCount;
  }

  /** 计算行在 canvas 内容中的 Y 偏移 (上边界) */
  private getRowOffset(row: number): number {
    let offset = 0;
    const limit = Math.min(row, this.sheet.rowCount);
    for (let r = 0; r < limit; r++) {
      if (!this.sheet.isRowHidden(r)) {
        offset += this.sheet.getRowHeight(r);
      }
    }
    return offset;
  }

  /** 计算列在 canvas 内容中的 X 偏移 (左边界) */
  private getColOffset(col: number): number {
    let offset = 0;
    const limit = Math.min(col, this.sheet.columnCount);
    for (let c = 0; c < limit; c++) {
      if (!this.sheet.isColumnHidden(c)) {
        offset += this.sheet.getColumnWidth(c);
      }
    }
    return offset;
  }

  /** 计算指定行范围的总高度 */
  private calcRowsHeight(fromStart: number, fromEnd: number): number {
    let total = 0;
    for (let r = fromStart; r <= fromEnd; r++) {
      if (!this.sheet.isRowHidden(r)) {
        total += this.sheet.getRowHeight(r);
      }
    }
    return Math.max(total, 1);
  }

  /** 计算指定列范围的总宽度 */
  private calcColsWidth(fromStart: number, fromEnd: number): number {
    let total = 0;
    for (let c = fromStart; c <= fromEnd; c++) {
      if (!this.sheet.isColumnHidden(c)) {
        total += this.sheet.getColumnWidth(c);
      }
    }
    return Math.max(total, 1);
  }

  // ============================================================
  // 公开方法
  // ============================================================

  /** 获取当前拖拽状态 (供外部查询, 如显示提示) */
  getDragState(): DragState | null {
    return this.dragState ? { ...this.dragState } : null;
  }

  /** 是否正在拖拽 */
  isDragging(): boolean {
    return this.dragState?.dragging ?? false;
  }

  /** 销毁 — 移除事件监听和 DOM 元素 */
  destroy(): void {
    this.container.removeEventListener('mousedown', this.boundMouseDown, true);
    this.detachDragListeners();
    if (this.previewEl) {
      this.previewEl.remove();
      this.previewEl = null;
    }
    if (this.indicatorEl) {
      this.indicatorEl.remove();
      this.indicatorEl = null;
    }
    this.dragState = null;
  }
}
