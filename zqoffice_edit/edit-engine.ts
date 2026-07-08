/**
 * 编辑引擎层 — 单元格编辑 + 选区操作 + 行列操作 + 键盘导航
 * 全部原创实现
 *
 * 职责:
 *   1. 单元格编辑 (双击/F2/直接输入激活编辑框, Enter/Tab/Escape 确认/取消)
 *   2. 键盘导航 (方向键/Tab/Enter 在单元格间移动)
 *   3. 选区操作 (Delete 清除/填充柄拖拽)
 *   4. 行列操作 (插入/删除/调整大小)
 *   5. 样式应用 (对选区批量设置样式)
 *   6. 公式重算 (编辑后触发 FormulaEngine 重算依赖链)
 *   7. 简单撤销/重做 (完整版在 undo-redo.ts, 这里只做基础栈)
 */

import { SheetModel, CellStyle, CellValueType } from './data-model';
import { RenderEngine, SelectionRange } from './render-engine';
import { StyleEngine } from './style-engine';
import { FormulaEngine } from './formula-engine';

/** 编辑操作类型 (用于撤销/重做) */
export type EditOperationType =
  | 'setCellValue'
  | 'setCellFormula'
  | 'clearContent'
  | 'setStyle'
  | 'insertRow'
  | 'insertColumn'
  | 'deleteRow'
  | 'deleteColumn'
  | 'resizeRow'
  | 'resizeColumn'
  | 'addMerge'
  | 'removeMerge'
  | 'fillRange';

/** 编辑操作记录 */
export interface EditOperation {
  type: EditOperationType;
  row?: number;
  col?: number;
  oldValue?: unknown;
  newValue?: unknown;
  // 批量操作
  cells?: Array<{ row: number; col: number; oldValue: unknown; newValue: unknown }>;
  // 行列操作
  startRow?: number;
  startCol?: number;
  count?: number;
  // 调整大小
  oldSize?: number;
  newSize?: number;
  // 合并
  merge?: { startRow: number; startCol: number; endRow: number; endCol: number };
}

/** 编辑引擎回调 */
export interface EditEngineCallbacks {
  onCellEditStart?: (row: number, col: number) => void;
  onCellEditEnd?: (row: number, col: number, value: string) => void;
  onSelectionChange?: (range: SelectionRange) => void;
  onContentChange?: () => void;
  onUndoRedo?: (canUndo: boolean, canRedo: boolean) => void;
}

/** 填充方向 */
export type FillDirection = 'up' | 'down' | 'left' | 'right';

export class EditEngine {
  private container: HTMLElement;
  private sheet: SheetModel;
  private renderEngine: RenderEngine;
  private styleEngine: StyleEngine;
  private formulaEngine: FormulaEngine;
  private callbacks: EditEngineCallbacks;

  // 编辑状态
  private editing: boolean = false;
  private editCell: { row: number; col: number } | null = null;
  private editorEl: HTMLInputElement | HTMLTextAreaElement | null = null;
  private originalValue: string = '';

  // 撤销/重做栈
  private undoStack: EditOperation[] = [];
  private redoStack: EditOperation[] = [];
  private readonly MAX_UNDO = 100;

  // 拖拽填充状态
  private fillDragging: boolean = false;

  // 行列调整大小状态
  private resizing: {
    type: 'row' | 'col';
    index: number;
    startPos: number;
    startSize: number;
  } | null = null;

  constructor(
    container: HTMLElement,
    sheet: SheetModel,
    renderEngine: RenderEngine,
    styleEngine: StyleEngine,
    formulaEngine: FormulaEngine,
    callbacks: EditEngineCallbacks = {}
  ) {
    this.container = container;
    this.sheet = sheet;
    this.renderEngine = renderEngine;
    this.styleEngine = styleEngine;
    this.formulaEngine = formulaEngine;
    this.callbacks = callbacks;

    this.setupEditorStyles();
    this.bindEvents();
  }

  // ============================================================
  // 样式
  // ============================================================

  private setupEditorStyles(): void {
    if (document.getElementById('zq-edit-styles')) return;
    const style = document.createElement('style');
    style.id = 'zq-edit-styles';
    style.textContent = `
      .zq-cell-editor {
        position: absolute;
        z-index: 100;
        box-sizing: border-box;
        border: 2px solid #4787f5;
        padding: 2px 4px;
        margin: 0;
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
        font-size: 13px;
        background: #fff;
        outline: none;
        overflow: hidden;
        white-space: nowrap;
        box-shadow: 0 2px 6px rgba(0,0,0,0.15);
      }
      .zq-cell-editortextarea {
        white-space: pre-wrap;
        word-break: break-all;
        resize: none;
      }
      .zq-fill-handle {
        position: absolute;
        width: 8px;
        height: 8px;
        background: #4787f5;
        border: 1px solid #fff;
        cursor: crosshair;
        z-index: 11;
      }
      .zq-resize-handle {
        position: absolute;
        background: transparent;
        z-index: 12;
      }
      .zq-resize-handle.row {
        cursor: ns-resize;
        height: 4px;
        width: 40px;
      }
      .zq-resize-handle.col {
        cursor: ew-resize;
        width: 4px;
        height: 24px;
      }
    `;
    document.head.appendChild(style);
  }

  // ============================================================
  // 事件绑定
  // ============================================================

  private bindEvents(): void {
    // 键盘事件 — 在 container 上监听
    this.container.addEventListener('keydown', (e) => this.handleKeyDown(e));

    // 双击进入编辑
    this.container.addEventListener('dblclick', (e) => {
      const target = e.target as HTMLElement;
      if (target.classList.contains('zq-cell')) {
        const row = parseInt(target.dataset.row || '0');
        const col = parseInt(target.dataset.col || '0');
        this.startEdit(row, col);
      }
    });

    // 点击编辑框外部确认编辑
    document.addEventListener('mousedown', (e) => {
      if (this.editing && this.editorEl && !this.editorEl.contains(e.target as Node)) {
        const target = e.target as HTMLElement;
        if (!target.classList.contains('zq-cell') && !target.classList.contains('zq-header-cell')) {
          this.commitEdit();
        }
      }
    });

    // 监听选区变化以更新填充柄
    // (通过 renderEngine 的回调间接获取)
  }

  // ============================================================
  // 单元格编辑
  // ============================================================

  /**
   * 开始编辑单元格
   */
  startEdit(row: number, col: number, initialValue?: string): void {
    // 如果正在编辑其他单元格, 先确认
    if (this.editing) {
      this.commitEdit();
    }

    this.editing = true;
    this.editCell = { row, col };

    // 获取当前值
    const cell = this.sheet.getCell(row, col);
    this.originalValue = initialValue ?? (cell ? String(cell.formula ? `=${cell.formula}` : cell.value) : '');

    // 创建编辑框
    const wrapText = this.sheet.getCellStyle(row, col)?.wrapText;
    if (wrapText) {
      this.editorEl = document.createElement('textarea');
      this.editorEl.className = 'zq-cell-editor zq-cell-editor-textarea';
    } else {
      this.editorEl = document.createElement('input');
      this.editorEl.type = 'text';
      this.editorEl.className = 'zq-cell-editor';
    }

    // 定位编辑框到单元格位置
    const cellEl = this.renderEngine.getRenderedCell(row, col);
    if (cellEl) {
      const rect = cellEl.getBoundingClientRect();
      const containerRect = this.container.getBoundingClientRect();
      this.editorEl.style.left = `${rect.left - containerRect.left}px`;
      this.editorEl.style.top = `${rect.top - containerRect.top}px`;
      this.editorEl.style.width = `${Math.max(rect.width, 100)}px`;
      this.editorEl.style.height = `${rect.height}px`;
    }

    // 应用单元格样式到编辑框
    const style = this.sheet.getCellStyle(row, col);
    this.styleEngine.applyStyle(this.editorEl, style);
    this.editorEl.style.border = '2px solid #4787f5';
    this.editorEl.style.background = '#fff';

    this.editorEl.value = this.originalValue;
    this.container.appendChild(this.editorEl);

    // 聚焦并选中文本
    setTimeout(() => {
      this.editorEl?.focus();
      this.editorEl?.select();
    }, 0);

    // 编辑框键盘事件
    this.editorEl.addEventListener('keydown', ((e: KeyboardEvent) => this.handleEditorKeyDown(e)) as EventListener);

    this.callbacks.onCellEditStart?.(row, col);
  }

  /**
   * 确认编辑 — 保存值并退出编辑模式
   */
  commitEdit(): void {
    if (!this.editing || !this.editCell || !this.editorEl) return;

    const { row, col } = this.editCell;
    const newValue = this.editorEl.value;
    this.removeEditor();

    // 值未变化时不记录
    if (newValue === this.originalValue) {
      this.editing = false;
      this.editCell = null;
      return;
    }

    // 记录旧值
    const oldCell = this.sheet.getCell(row, col);
    const oldValue = oldCell ? { value: oldCell.value, type: oldCell.type, formula: oldCell.formula } : null;

    // 判断是否是公式
    if (newValue.startsWith('=')) {
      const formula = newValue.slice(1);
      this.sheet.setCellFormula(row, col, formula);
      // 计算公式
      const result = this.formulaEngine.calculate(formula);
      this.sheet.setCellValue(row, col, this.valueToCellData(result), 'string');
      // 公式单元格的类型仍然是 formula
      const updated = this.sheet.getCell(row, col);
      if (updated) {
        updated.type = 'formula';
        updated.formula = formula;
        updated.value = this.valueToCellData(result);
      }
    } else {
      // 自动推断类型
      const typedValue = this.parseInputValue(newValue);
      this.sheet.setCellValue(row, col, typedValue.value, typedValue.type);
    }

    // 记录撤销
    this.pushUndo({
      type: 'setCellValue',
      row,
      col,
      oldValue,
      newValue: { value: newValue, formula: newValue.startsWith('=') ? newValue.slice(1) : undefined },
    });

    // 重算所有公式 (简单实现: 全量重算)
    this.recalcAndRefresh();

    this.editing = false;
    this.editCell = null;
    this.callbacks.onCellEditEnd?.(row, col, newValue);
    this.callbacks.onContentChange?.();
  }

  /**
   * 取消编辑
   */
  cancelEdit(): void {
    this.removeEditor();
    this.editing = false;
    this.editCell = null;
  }

  /** 移除编辑框 */
  private removeEditor(): void {
    if (this.editorEl) {
      this.editorEl.remove();
      this.editorEl = null;
    }
  }

  /** 编辑框内的键盘事件 */
  private handleEditorKeyDown(e: KeyboardEvent): void {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      this.commitEdit();
      // 移动到下一行
      if (this.editCell) {
        const { row, col } = this.editCell;
        this.moveCursor(row + 1, col);
      }
    } else if (e.key === 'Enter' && e.shiftKey) {
      e.preventDefault();
      this.commitEdit();
      if (this.editCell) {
        const { row, col } = this.editCell;
        this.moveCursor(row - 1, col);
      }
    } else if (e.key === 'Tab') {
      e.preventDefault();
      this.commitEdit();
      if (this.editCell) {
        const { row, col } = this.editCell;
        this.moveCursor(row, col + (e.shiftKey ? -1 : 1));
      }
    } else if (e.key === 'Escape') {
      e.preventDefault();
      this.cancelEdit();
    }
    // 其他按键继续在编辑框内处理
  }

  // ============================================================
  // 键盘导航 — 非编辑模式下
  // ============================================================

  /**
   * 主键盘事件处理
   */
  handleKeyDown(e: KeyboardEvent): void {
    // 如果正在编辑, 编辑框的事件优先
    if (this.editing) return;

    const sel = this.renderEngine.getSelection();
    if (!sel) return;

    // 方向键移动
    if (e.key === 'ArrowUp') {
      e.preventDefault();
      this.moveCursor(sel.startRow - 1, sel.startCol);
    } else if (e.key === 'ArrowDown') {
      e.preventDefault();
      this.moveCursor(sel.startRow + 1, sel.startCol);
    } else if (e.key === 'ArrowLeft') {
      e.preventDefault();
      this.moveCursor(sel.startRow, sel.startCol - 1);
    } else if (e.key === 'ArrowRight') {
      e.preventDefault();
      this.moveCursor(sel.startRow, sel.startCol + 1);
    } else if (e.key === 'Tab') {
      e.preventDefault();
      this.moveCursor(sel.startRow, sel.startCol + (e.shiftKey ? -1 : 1));
    } else if (e.key === 'Enter') {
      e.preventDefault();
      this.moveCursor(sel.startRow + (e.shiftKey ? -1 : 1), sel.startCol);
    }
    // F2 进入编辑
    else if (e.key === 'F2') {
      e.preventDefault();
      this.startEdit(sel.startRow, sel.startCol);
    }
    // Delete/Backspace 清除内容
    else if (e.key === 'Delete' || e.key === 'Backspace') {
      e.preventDefault();
      this.clearSelectionContent();
    }
    // Ctrl+Z 撤销
    else if ((e.ctrlKey || e.metaKey) && e.key === 'z' && !e.shiftKey) {
      e.preventDefault();
      this.undo();
    }
    // Ctrl+Y 或 Ctrl+Shift+Z 重做
    else if ((e.ctrlKey || e.metaKey) && (e.key === 'y' || (e.key === 'z' && e.shiftKey))) {
      e.preventDefault();
      this.redo();
    }
    // Ctrl+C 复制 (剪贴板实际操作需要 Clipboard API, 这里只做选区标记)
    else if ((e.ctrlKey || e.metaKey) && e.key === 'c') {
      e.preventDefault();
      this.copySelection();
    }
    // Ctrl+V 粘贴
    else if ((e.ctrlKey || e.metaKey) && e.key === 'v') {
      e.preventDefault();
      this.pasteToSelection();
    }
    // Ctrl+X 剪切
    else if ((e.ctrlKey || e.metaKey) && e.key === 'x') {
      e.preventDefault();
      this.cutSelection();
    }
    // 直接输入字符 — 进入编辑模式
    else if (e.key.length === 1 && !e.ctrlKey && !e.metaKey && !e.altKey) {
      e.preventDefault();
      this.startEdit(sel.startRow, sel.startCol, e.key);
    }
  }

  /** 移动光标到指定单元格 */
  private moveCursor(row: number, col: number): void {
    // 边界检查
    row = Math.max(0, Math.min(this.sheet.rowCount - 1, row));
    col = Math.max(0, Math.min(this.sheet.columnCount - 1, col));

    this.renderEngine.setSelection({ startRow: row, startCol: col, endRow: row, endCol: col });
    this.renderEngine.scrollToCell(row, col);
    this.callbacks.onSelectionChange?.({ startRow: row, startCol: col, endRow: row, endCol: col });
  }

  // ============================================================
  // 选区操作
  // ============================================================

  /**
   * 清除选区内容 (保留样式)
   */
  clearSelectionContent(): void {
    const sel = this.renderEngine.getSelection();
    if (!sel) return;

    const changes: EditOperation['cells'] = [];
    for (let r = sel.startRow; r <= sel.endRow; r++) {
      for (let c = sel.startCol; c <= sel.endCol; c++) {
        const cell = this.sheet.getCell(r, c);
        const oldValue = cell ? { value: cell.value, type: cell.type, formula: cell.formula } : null;
        this.sheet.clearCellContent(r, c);
        changes.push({ row: r, col: c, oldValue, newValue: null });
      }
    }

    this.pushUndo({ type: 'clearContent', cells: changes });
    this.recalcAndRefresh();
    this.callbacks.onContentChange?.();
  }

  /**
   * 复制选区到内部剪贴板
   * 注意: 真正的系统剪贴板需要 Clipboard API, 这里用内部缓存
   */
  private clipboard: { data: string[][]; isCut: boolean } | null = null;

  copySelection(): void {
    const sel = this.renderEngine.getSelection();
    if (!sel) return;

    const data: string[][] = [];
    for (let r = sel.startRow; r <= sel.endRow; r++) {
      const row: string[] = [];
      for (let c = sel.startCol; c <= sel.endCol; c++) {
        const cell = this.sheet.getCell(r, c);
        if (cell) {
          row.push(cell.formula ? `=${cell.formula}` : String(cell.value));
        } else {
          row.push('');
        }
      }
      data.push(row);
    }
    this.clipboard = { data, isCut: false };

    // 尝试写入系统剪贴板
    if (navigator.clipboard) {
      const text = data.map(row => row.join('\t')).join('\n');
      navigator.clipboard.writeText(text).catch(() => { /* 静默失败 */ });
    }
  }

  cutSelection(): void {
    this.copySelection();
    if (this.clipboard) {
      this.clipboard.isCut = true;
    }
  }

  pasteToSelection(): void {
    const sel = this.renderEngine.getSelection();
    if (!sel) return;

    // 优先使用系统剪贴板
    if (navigator.clipboard && navigator.clipboard.readText) {
      navigator.clipboard.readText().then((text) => {
        this.doPaste(sel.startRow, sel.startCol, text);
      }).catch(() => {
        // 回退到内部剪贴板
        if (this.clipboard) {
          const text = this.clipboard.data.map(row => row.join('\t')).join('\n');
          this.doPaste(sel.startRow, sel.startCol, text, this.clipboard.isCut);
        }
      });
    } else if (this.clipboard) {
      const text = this.clipboard.data.map(row => row.join('\t')).join('\n');
      this.doPaste(sel.startRow, sel.startCol, text, this.clipboard.isCut);
    }
  }

  /**
   * 执行粘贴
   */
  private doPaste(startRow: number, startCol: number, text: string, isCut: boolean = false): void {
    const lines = text.split('\n');
    const changes: EditOperation['cells'] = [];

    for (let r = 0; r < lines.length; r++) {
      const cols = lines[r].split('\t');
      for (let c = 0; c < cols.length; c++) {
        const targetRow = startRow + r;
        const targetCol = startCol + c;
        if (targetRow >= this.sheet.rowCount || targetCol >= this.sheet.columnCount) continue;

        const oldCell = this.sheet.getCell(targetRow, targetCol);
        const oldValue = oldCell ? { value: oldCell.value, type: oldCell.type, formula: oldCell.formula } : null;
        const pasteValue = cols[c];

        if (pasteValue.startsWith('=')) {
          const formula = pasteValue.slice(1);
          // 相对引用偏移
          const offsetRow = targetRow - (this.clipboard?.data[0]?.length ? startRow : startRow);
          const offsetCol = targetCol - startCol;
          const adjusted = this.adjustFormulaRefs(formula, offsetRow, offsetCol);
          this.sheet.setCellFormula(targetRow, targetCol, adjusted);
          const result = this.formulaEngine.calculate(adjusted);
          const updated = this.sheet.getCell(targetRow, targetCol);
          if (updated) {
            updated.value = this.valueToCellData(result);
          }
        } else {
          const typed = this.parseInputValue(pasteValue);
          this.sheet.setCellValue(targetRow, targetCol, typed.value, typed.type);
        }

        changes.push({ row: targetRow, col: targetCol, oldValue, newValue: pasteValue });
      }
    }

    // 如果是剪切, 清空源
    if (isCut && this.clipboard) {
      // (简化: 假设源选区就是当前选区, 实际应记录源)
      this.clipboard.isCut = false;
    }

    this.pushUndo({ type: 'setCellValue', cells: changes });
    this.recalcAndRefresh();
    this.callbacks.onContentChange?.();

    // 选中新粘贴的区域
    const endRow = startRow + lines.length - 1;
    const endCol = startCol + (lines[0]?.split('\t').length || 1) - 1;
    this.renderEngine.setSelection({ startRow, startCol, endRow, endCol });
  }

  /**
   * 填充 — 将选区内容沿方向填充
   */
  fillRange(direction: FillDirection, count: number): void {
    const sel = this.renderEngine.getSelection();
    if (!sel) return;

    const changes: EditOperation['cells'] = [];
    const srcCells: string[][] = [];

    // 读取源数据
    for (let r = sel.startRow; r <= sel.endRow; r++) {
      const row: string[] = [];
      for (let c = sel.startCol; c <= sel.endCol; c++) {
        const cell = this.sheet.getCell(r, c);
        row.push(cell ? (cell.formula ? `=${cell.formula}` : String(cell.value)) : '');
      }
      srcCells.push(row);
    }

    // 根据方向填充
    for (let i = 1; i <= count; i++) {
      for (let r = 0; r < srcCells.length; r++) {
        for (let c = 0; c < srcCells[r].length; c++) {
          let targetRow = sel.startRow + r;
          let targetCol = sel.startCol + c;
          if (direction === 'down') targetRow = sel.endRow + i;
          else if (direction === 'up') targetRow = sel.startRow - i;
          else if (direction === 'right') targetCol = sel.endCol + i;
          else if (direction === 'left') targetCol = sel.startCol - i;

          if (targetRow < 0 || targetRow >= this.sheet.rowCount) continue;
          if (targetCol < 0 || targetCol >= this.sheet.columnCount) continue;

          const oldCell = this.sheet.getCell(targetRow, targetCol);
          const oldValue = oldCell ? { value: oldCell.value, type: oldCell.type, formula: oldCell.formula } : null;
          const srcValue = srcCells[r][c];

          if (srcValue.startsWith('=')) {
            const formula = srcValue.slice(1);
            let offsetRow = 0, offsetCol = 0;
            if (direction === 'down') offsetRow = targetRow - sel.startRow;
            else if (direction === 'right') offsetCol = targetCol - sel.startCol;
            const adjusted = this.adjustFormulaRefs(formula, offsetRow, offsetCol);
            this.sheet.setCellFormula(targetRow, targetCol, adjusted);
            const result = this.formulaEngine.calculate(adjusted);
            const updated = this.sheet.getCell(targetRow, targetCol);
            if (updated) updated.value = this.valueToCellData(result);
          } else {
            const typed = this.parseInputValue(srcValue);
            this.sheet.setCellValue(targetRow, targetCol, typed.value, typed.type);
          }

          changes.push({ row: targetRow, col: targetCol, oldValue, newValue: srcValue });
        }
      }
    }

    this.pushUndo({ type: 'fillRange', cells: changes });
    this.recalcAndRefresh();
    this.callbacks.onContentChange?.();
  }

  // ============================================================
  // 行列操作
  // ============================================================

  /**
   * 插入行
   */
  insertRows(row: number, count: number = 1): void {
    this.sheet.insertRow(row, count);
    this.pushUndo({ type: 'insertRow', startRow: row, count });
    this.recalcAndRefresh();
    this.callbacks.onContentChange?.();
  }

  /** 插入列 */
  insertColumns(col: number, count: number = 1): void {
    this.sheet.insertColumn(col, count);
    this.pushUndo({ type: 'insertColumn', startCol: col, count });
    this.recalcAndRefresh();
    this.callbacks.onContentChange?.();
  }

  /** 删除行 */
  deleteRows(row: number, count: number = 1): void {
    // 记录被删除的数据用于撤销
    const deletedCells: EditOperation['cells'] = [];
    for (let r = row; r < row + count; r++) {
      for (let c = 0; c < this.sheet.columnCount; c++) {
        const cell = this.sheet.getCell(r, c);
        if (cell) {
          deletedCells.push({ row: r, col: c, oldValue: { value: cell.value, type: cell.type, formula: cell.formula }, newValue: null });
        }
      }
    }

    this.sheet.deleteRow(row, count);
    this.pushUndo({ type: 'deleteRow', startRow: row, count, cells: deletedCells });
    this.recalcAndRefresh();
    this.callbacks.onContentChange?.();
  }

  /** 删除列 */
  deleteColumns(col: number, count: number = 1): void {
    const deletedCells: EditOperation['cells'] = [];
    for (let c = col; c < col + count; c++) {
      for (let r = 0; r < this.sheet.rowCount; r++) {
        const cell = this.sheet.getCell(r, c);
        if (cell) {
          deletedCells.push({ row: r, col: c, oldValue: { value: cell.value, type: cell.type, formula: cell.formula }, newValue: null });
        }
      }
    }

    this.sheet.deleteColumn(col, count);
    this.pushUndo({ type: 'deleteColumn', startCol: col, count, cells: deletedCells });
    this.recalcAndRefresh();
    this.callbacks.onContentChange?.();
  }

  /** 设置行高 */
  setRowHeight(row: number, height: number): void {
    const oldHeight = this.sheet.getRowHeight(row);
    this.sheet.setRowHeight(row, height);
    this.pushUndo({ type: 'resizeRow', row, oldSize: oldHeight, newSize: height });
    this.renderEngine.refresh();
    this.callbacks.onContentChange?.();
  }

  /** 设置列宽 */
  setColumnWidth(col: number, width: number): void {
    const oldWidth = this.sheet.getColumnWidth(col);
    this.sheet.setColumnWidth(col, width);
    this.pushUndo({ type: 'resizeColumn', col, oldSize: oldWidth, newSize: width });
    this.renderEngine.refresh();
    this.callbacks.onContentChange?.();
  }

  // ============================================================
  // 样式应用
  // ============================================================

  /**
   * 对选区应用样式
   */
  applyStyleToSelection(style: Partial<CellStyle>): void {
    const sel = this.renderEngine.getSelection();
    if (!sel) return;

    const changes: EditOperation['cells'] = [];
    for (let r = sel.startRow; r <= sel.endRow; r++) {
      for (let c = sel.startCol; c <= sel.endCol; c++) {
        const oldStyle = this.sheet.getCellStyle(r, c);
        this.sheet.setCellStyle(r, c, style);
        changes.push({ row: r, col: c, oldValue: oldStyle, newValue: style });
      }
    }

    this.pushUndo({ type: 'setStyle', cells: changes });
    this.renderEngine.refresh();
    this.callbacks.onContentChange?.();
  }

  /**
   * 合并选区
   */
  mergeSelection(): void {
    const sel = this.renderEngine.getSelection();
    if (!sel) return;

    this.sheet.addMerge(sel.startRow, sel.startCol, sel.endRow, sel.endCol);
    this.pushUndo({ type: 'addMerge', merge: { ...sel } });
    this.renderEngine.refresh();
    this.callbacks.onContentChange?.();
  }

  /**
   * 取消合并
   */
  unmergeSelection(): void {
    const sel = this.renderEngine.getSelection();
    if (!sel) return;

    const merge = this.sheet.getMerge(sel.startRow, sel.startCol);
    if (merge) {
      this.sheet.removeMerge(sel.startRow, sel.startCol);
      this.pushUndo({ type: 'removeMerge', merge: { startRow: merge.startRow, startCol: merge.startColumn, endRow: merge.endRow, endCol: merge.endColumn } });
      this.renderEngine.refresh();
      this.callbacks.onContentChange?.();
    }
  }

  // ============================================================
  // 撤销/重做
  // ============================================================

  private pushUndo(op: EditOperation): void {
    this.undoStack.push(op);
    if (this.undoStack.length > this.MAX_UNDO) {
      this.undoStack.shift();
    }
    this.redoStack = [];
    this.notifyUndoRedoState();
  }

  canUndo(): boolean {
    return this.undoStack.length > 0;
  }

  canRedo(): boolean {
    return this.redoStack.length > 0;
  }

  /**
   * 撤销
   */
  undo(): void {
    const op = this.undoStack.pop();
    if (!op) return;
    this.redoStack.push(op);
    this.applyOperation(op, true);
    this.recalcAndRefresh();
    this.callbacks.onContentChange?.();
    this.notifyUndoRedoState();
  }

  /**
   * 重做
   */
  redo(): void {
    const op = this.redoStack.pop();
    if (!op) return;
    this.undoStack.push(op);
    this.applyOperation(op, false);
    this.recalcAndRefresh();
    this.callbacks.onContentChange?.();
    this.notifyUndoRedoState();
  }

  /** 通知撤销/重做状态 */
  private notifyUndoRedoState(): void {
    this.callbacks.onUndoRedo?.(this.canUndo(), this.canRedo());
  }

  /**
   * 应用操作 (撤销或重做)
   * @param op 操作记录
   * @param isUndo true=撤销, false=重做
   */
  private applyOperation(op: EditOperation, isUndo: boolean): void {
    switch (op.type) {
      case 'setCellValue':
      case 'clearContent':
      case 'setStyle':
      case 'fillRange':
        if (op.cells) {
          for (const change of op.cells) {
            const val = isUndo ? change.oldValue : change.newValue;
            if (val === null) {
              this.sheet.clearCellContent(change.row, change.col);
            } else if (op.type === 'setStyle') {
              if (isUndo) {
                this.sheet.setCellStyle(change.row, change.col, val as CellStyle);
              } else {
                this.sheet.setCellStyle(change.row, change.col, val as Partial<CellStyle>);
              }
            } else if (typeof val === 'object' && val !== null && 'formula' in val) {
              const v = val as { value: string | number | boolean; type: CellValueType; formula?: string };
              if (v.formula) {
                this.sheet.setCellFormula(change.row, change.col, v.formula);
                const updated = this.sheet.getCell(change.row, change.col);
                if (updated) updated.value = v.value;
              } else {
                this.sheet.setCellValue(change.row, change.col, v.value, v.type);
              }
            } else if (typeof val === 'string') {
              const typed = this.parseInputValue(val);
              this.sheet.setCellValue(change.row, change.col, typed.value, typed.type);
            }
          }
        } else if (op.row !== undefined && op.col !== undefined) {
          const val = isUndo ? op.oldValue : op.newValue;
          if (val === null) {
            this.sheet.clearCellContent(op.row, op.col);
          } else if (typeof val === 'object' && val !== null && 'formula' in val) {
            const v = val as { value: string | number | boolean; type: CellValueType; formula?: string };
            if (v.formula) {
              this.sheet.setCellFormula(op.row, op.col, v.formula);
              const updated = this.sheet.getCell(op.row, op.col);
              if (updated) updated.value = v.value;
            } else {
              this.sheet.setCellValue(op.row, op.col, v.value, v.type);
            }
          }
        }
        break;

      case 'insertRow':
        if (isUndo) {
          this.sheet.deleteRow(op.startRow!, op.count!);
        } else {
          this.sheet.insertRow(op.startRow!, op.count!);
        }
        break;

      case 'deleteRow':
        if (isUndo) {
          this.sheet.insertRow(op.startRow!, op.count!);
          // 恢复被删除的单元格
          if (op.cells) {
            for (const change of op.cells) {
              const val = change.oldValue as { value: string | number | boolean; type: CellValueType; formula?: string } | null;
              if (val) {
                if (val.formula) {
                  this.sheet.setCellFormula(change.row, change.col, val.formula);
                } else {
                  this.sheet.setCellValue(change.row, change.col, val.value, val.type);
                }
              }
            }
          }
        } else {
          this.sheet.deleteRow(op.startRow!, op.count!);
        }
        break;

      case 'insertColumn':
        if (isUndo) {
          this.sheet.deleteColumn(op.startCol!, op.count!);
        } else {
          this.sheet.insertColumn(op.startCol!, op.count!);
        }
        break;

      case 'deleteColumn':
        if (isUndo) {
          this.sheet.insertColumn(op.startCol!, op.count!);
          if (op.cells) {
            for (const change of op.cells) {
              const val = change.oldValue as { value: string | number | boolean; type: CellValueType; formula?: string } | null;
              if (val) {
                if (val.formula) {
                  this.sheet.setCellFormula(change.row, change.col, val.formula);
                } else {
                  this.sheet.setCellValue(change.row, change.col, val.value, val.type);
                }
              }
            }
          }
        } else {
          this.sheet.deleteColumn(op.startCol!, op.count!);
        }
        break;

      case 'resizeRow':
        if (op.row !== undefined) {
          this.sheet.setRowHeight(op.row, isUndo ? op.oldSize! : op.newSize!);
        }
        break;

      case 'resizeColumn':
        if (op.col !== undefined) {
          this.sheet.setColumnWidth(op.col, isUndo ? op.oldSize! : op.newSize!);
        }
        break;

      case 'addMerge':
        if (isUndo && op.merge) {
          this.sheet.removeMerge(op.merge.startRow, op.merge.startCol);
        } else if (!isUndo && op.merge) {
          this.sheet.addMerge(op.merge.startRow, op.merge.startCol, op.merge.endRow, op.merge.endCol);
        }
        break;

      case 'removeMerge':
        if (isUndo && op.merge) {
          this.sheet.addMerge(op.merge.startRow, op.merge.startCol, op.merge.endRow, op.merge.endCol);
        } else if (!isUndo && op.merge) {
          this.sheet.removeMerge(op.merge.startRow, op.merge.startCol);
        }
        break;
    }
    this.renderEngine.refresh();
  }

  /** 清空撤销/重做栈 */
  clearHistory(): void {
    this.undoStack = [];
    this.redoStack = [];
    this.notifyUndoRedoState();
  }

  // ============================================================
  // 辅助方法
  // ============================================================

  /**
   * 解析用户输入值, 推断类型
   */
  private parseInputValue(input: string): { value: string | number | boolean; type: CellValueType } {
    const trimmed = input.trim();

    // 空字符串
    if (trimmed === '') {
      return { value: '', type: 'string' };
    }

    // 布尔值
    if (trimmed.toLowerCase() === 'true') {
      return { value: true, type: 'boolean' };
    }
    if (trimmed.toLowerCase() === 'false') {
      return { value: false, type: 'boolean' };
    }

    // 数字
    const num = Number(trimmed);
    if (trimmed !== '' && !isNaN(num) && isFinite(num)) {
      return { value: num, type: 'number' };
    }

    // 错误值
    if (trimmed.startsWith('#') && trimmed.endsWith('!')) {
      return { value: trimmed, type: 'error' };
    }

    // 字符串
    return { value: trimmed, type: 'string' };
  }

  /**
   * 将公式计算结果转换为单元格可存储的值
   */
  private valueToCellData(value: number | string | boolean | null): string | number | boolean {
    if (value === null) return '';
    if (typeof value === 'number') return value;
    if (typeof value === 'boolean') return value;
    return String(value);
  }

  /**
   * 调整公式中的相对引用 (用于填充/复制)
   * 简化实现: 只处理 A1/B2 格式的相对引用, 不处理 $A$1 绝对引用
   * @param formula 公式文本
   * @param offsetRow 行偏移
   * @param offsetCol 列偏移
   */
  private adjustFormulaRefs(formula: string, offsetRow: number, offsetCol: number): string {
    // 匹配单元格引用: 字母+数字 (如 A1, AB12)
    // 不处理 $ 前缀的绝对引用
    return formula.replace(/(\$?)([A-Z]+)(\$?)(\d+)/g, (match, colAbs, col, rowAbs, row) => {
      const colIdx = SheetModel.letterToColumn(col);
      const rowIdx = parseInt(row) - 1;
      const newCol = colAbs ? col : SheetModel.columnToLetter(colIdx + offsetCol);
      const newRow = rowAbs ? row : String(rowIdx + 1 + offsetRow);
      return `${colAbs}${newCol}${rowAbs}${newRow}`;
    });
  }

  /** 更新工作表引用 (供 sheet-engine 切换工作表时调用) */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
  }

  /** 触发公式重算+渲染刷新 (供 clipboard 调用) */
  recalcAndRefresh(): void {
    this.formulaEngine.calculateAll();
    this.renderEngine.refresh();
  }

  /** 获取当前编辑状态 */
  isEditing(): boolean {
    return this.editing;
  }

  /** 获取正在编辑的单元格 */
  getEditingCell(): { row: number; col: number } | null {
    return this.editCell ? { ...this.editCell } : null;
  }

  /** 销毁 */
  destroy(): void {
    this.cancelEdit();
    this.undoStack = [];
    this.redoStack = [];
  }
}
