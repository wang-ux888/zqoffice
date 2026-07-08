/**
 * 表格编辑器主引擎 — 协调所有模块
 * 全部原创实现
 *
 * 职责:
 *   1. 创建和初始化所有子模块 (data/render/style/formula/edit/toolbar/clipboard/undo-redo)
 *   2. 事件路由: RenderEngine 事件 → EditEngine → Toolbar 状态更新
 *   3. 文档加载/保存 (对接 IPC)
 *   4. 工作表切换
 *   5. 公式重算触发
 *   6. 窗口级快捷键 (Ctrl+S 保存等)
 */

import { DocumentModel, SheetModel, ZQSheetDocument } from './data-model';
import { RenderEngine, SelectionRange, RenderEngineCallbacks } from './render-engine';
import { StyleEngine } from './style-engine';
import { FormulaEngine } from './formula-engine';
import { EditEngine, EditEngineCallbacks } from './edit-engine';
import { ClipboardManager } from './clipboard';
import { UndoRedoManager } from './undo-redo';
import { Toolbar } from './toolbar';
import { FreezePane } from './freeze-pane';
import { HideManager } from './hide-manager';
import { FilterSortEngine } from './filter-sort';
import { FindReplaceEngine } from './find-replace';
import { ConditionalFormatEngine } from './conditional-format';
import { DataValidationManager } from './data-validation';
import { CellCommentManager } from './cell-comment';
import { GroupOutlineManager } from './group-outline';
import { ImageManager } from './image-manager';
import { ChartEngine } from './chart-engine';
import { BorderManager } from './border-manager';
import { DragManager } from './drag-manager';
import { PivotTableEngine } from './pivot-table';
import { PrintManager } from './print-manager';

/** 主引擎配置 */
export interface SheetEngineConfig {
  /** 容器元素 */
  container: HTMLElement;
  /** 初始文档 JSON (可选) */
  initialData?: ZQSheetDocument;
  /** 文件路径 (打开已有文件时) */
  filePath?: string;
  /** 文件名 */
  fileName?: string;
  /** 是否只读 */
  readOnly?: boolean;
  /** 保存回调 (由 IPC 注入) */
  onSave?: (json: string, filePath?: string) => Promise<void>;
  /** 内容变化回调 */
  onContentChange?: () => void;
  /** 标题更新回调 */
  onTitleChange?: (title: string) => void;
}

export class SheetEngine {
  private config: SheetEngineConfig;
  private container: HTMLElement;

  // 子模块 — 核心层
  private docModel: DocumentModel;
  private sheet: SheetModel;
  private styleEngine: StyleEngine;
  private formulaEngine: FormulaEngine;
  private renderEngine: RenderEngine;
  private editEngine: EditEngine;
  private clipboard: ClipboardManager;
  private undoRedo: UndoRedoManager;
  private toolbar: Toolbar;

  // 子模块 — 功能层
  private freezePane!: FreezePane;
  private hideManager!: HideManager;
  private filterSort!: FilterSortEngine;
  private findReplace!: FindReplaceEngine;
  private conditionalFormat!: ConditionalFormatEngine;
  private dataValidation!: DataValidationManager;
  private commentManager!: CellCommentManager;
  private outlineManager!: GroupOutlineManager;
  private imageManager!: ImageManager;
  private chartEngine!: ChartEngine;
  private borderManager!: BorderManager;
  private dragManager!: DragManager;
  private pivotEngine!: PivotTableEngine;
  private printManager!: PrintManager;

  // 状态
  private isDirty: boolean = false;
  private currentSheetIndex: number = 0;

  // DOM 元素
  private toolbarContainer!: HTMLDivElement;
  private gridContainer!: HTMLDivElement;
  private sheetTabContainer!: HTMLDivElement;
  private statusBar!: HTMLDivElement;
  private formulaBar!: HTMLInputElement;

  // 模块引用 (用于解决循环依赖)
  private editEngineRef: EditEngine | null = null;

  constructor(config: SheetEngineConfig) {
    this.config = config;
    this.container = config.container;

    // 1. 创建数据模型
    this.docModel = new DocumentModel(config.initialData);
    this.sheet = this.docModel.getActiveSheet();

    // 2. 创建布局 DOM
    const layout = this.createLayout();
    this.container.appendChild(layout);

    // 3. 创建样式引擎
    this.styleEngine = new StyleEngine();

    // 4. 创建公式引擎
    this.formulaEngine = new FormulaEngine(this.sheet);

    // 5. 创建渲染引擎 (带回调, 回调引用 editEngineRef 解决循环依赖)
    const renderCallbacks: RenderEngineCallbacks = {
      onCellClick: (row, col, e) => {
        this.updateFormulaBar(row, col);
      },
      onCellDoubleClick: (row, col, e) => {
        this.editEngineRef?.startEdit(row, col);
      },
      onSelectionChange: (range) => {
        this.updateFormulaBar(range.startRow, range.startCol);
        this.toolbar.updateButtonStates();
        this.updateStatusBar(range);
      },
      onContextMenu: (row, col, e) => {
        this.showContextMenu(row, col, e);
      },
      onScroll: (viewport) => {
        // 可扩展: 滚动时更新状态
      },
    };

    this.renderEngine = new RenderEngine(
      this.gridContainer,
      this.sheet,
      this.styleEngine,
      renderCallbacks
    );

    // 6. 创建编辑引擎
    const editCallbacks: EditEngineCallbacks = {
      onCellEditStart: (row, col) => {
        this.formulaBar.value = this.getCellValueString(row, col);
      },
      onCellEditEnd: (row, col, value) => {
        this.markDirty();
        this.updateFormulaBar(row, col);
      },
      onSelectionChange: (range) => {
        this.updateFormulaBar(range.startRow, range.startCol);
        this.updateStatusBar(range);
      },
      onContentChange: () => {
        this.markDirty();
      },
      onUndoRedo: (canUndo, canRedo) => {
        this.toolbar.updateButtonStates();
      },
    };

    this.editEngine = new EditEngine(
      this.gridContainer,
      this.sheet,
      this.renderEngine,
      this.styleEngine,
      this.formulaEngine,
      editCallbacks
    );
    this.editEngineRef = this.editEngine;

    // 7. 创建剪贴板管理器
    this.clipboard = new ClipboardManager(
      this.sheet,
      this.renderEngine,
      this.formulaEngine,
      this.editEngine
    );

    // 8. 创建撤销/重做管理器 (增强版, 可选使用)
    this.undoRedo = new UndoRedoManager(100);

    // 9. 创建工具栏
    this.toolbar = new Toolbar(
      this.toolbarContainer,
      this.sheet,
      this.renderEngine,
      this.editEngine,
      this.clipboard,
      this.undoRedo
    );

    // 10. 创建功能层模块
    this.freezePane = new FreezePane(this.sheet, this.renderEngine);
    this.hideManager = new HideManager(this.sheet, this.renderEngine);
    this.filterSort = new FilterSortEngine(this.sheet);
    this.findReplace = new FindReplaceEngine(this.container, this.sheet, this.renderEngine, this.editEngine, {
      onContentChange: () => this.markDirty(),
    });
    this.findReplace.bindGlobalShortcuts();
    this.conditionalFormat = new ConditionalFormatEngine(this.sheet);
    // FormulaEvaluator 签名为 (formula, row, col, sheet) => boolean
    // formulaEngine.calculate 返回 FormulaValue，需包装为 boolean
    this.conditionalFormat.setFormulaEvaluator((formula: string) => {
      const result = this.formulaEngine.calculate(formula);
      return result === true || (typeof result === 'number' && result !== 0);
    });
    this.dataValidation = new DataValidationManager(this.container, this.sheet, this.renderEngine);
    this.commentManager = new CellCommentManager(this.container, this.sheet, this.renderEngine, {
      onContentChange: () => this.markDirty(),
    });
    this.outlineManager = new GroupOutlineManager(this.container, this.sheet, this.renderEngine);
    this.imageManager = new ImageManager(this.gridContainer, this.sheet);
    this.chartEngine = new ChartEngine(this.gridContainer, this.sheet);
    this.borderManager = new BorderManager(this.sheet);
    this.dragManager = new DragManager(this.container, this.sheet, this.renderEngine, {
      onContentChange: () => this.markDirty(),
    });
    this.pivotEngine = new PivotTableEngine(this.sheet);
    this.printManager = new PrintManager(this.sheet);

    // 11. 绑定全局事件
    this.bindGlobalEvents();

    // 12. 渲染工作表标签
    this.renderSheetTabs();

    // 13. 初始计算
    this.formulaEngine.calculateAll();
    this.renderEngine.refresh();

    // 14. 设置只读模式
    if (config.readOnly) {
      this.setReadOnly(true);
    }

    // 15. 更新标题
    if (config.fileName) {
      this.config.onTitleChange?.(config.fileName);
    }
  }

  // ============================================================
  // 布局创建
  // ============================================================

  /**
   * 创建编辑器布局
   * 结构:
   *   ┌─────────────────────────────┐
   *   │        工具栏                │
   *   ├─────────────────────────────┤
   *   │  公式栏  |  fx  |  =value   │
   *   ├─────────────────────────────┤
   *   │                             │
   *   │        表格区域              │
   *   │                             │
   *   ├─────────────────────────────┤
   *   │  Sheet1 | Sheet2 | +         │
   *   ├─────────────────────────────┤
   *   │  状态栏: 就绪 | 平均: 5 | ...│
   *   └─────────────────────────────┘
   */
  private createLayout(): HTMLElement {
    this.setupLayoutStyles();

    const wrapper = document.createElement('div');
    wrapper.className = 'zq-sheet-wrapper';

    // 工具栏
    this.toolbarContainer = document.createElement('div');
    this.toolbarContainer.className = 'zq-sheet-toolbar-container';
    wrapper.appendChild(this.toolbarContainer);

    // 公式栏
    const formulaBarWrap = document.createElement('div');
    formulaBarWrap.className = 'zq-sheet-formula-bar';
    const fxLabel = document.createElement('span');
    fxLabel.className = 'zq-sheet-fx';
    fxLabel.textContent = 'fx';
    const cellRef = document.createElement('span');
    cellRef.className = 'zq-sheet-cell-ref';
    cellRef.textContent = 'A1';
    this.formulaBar = document.createElement('input');
    this.formulaBar.type = 'text';
    this.formulaBar.className = 'zq-sheet-formula-input';
    this.formulaBar.placeholder = '输入公式或值';
    this.formulaBar.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') {
        e.preventDefault();
        const sel = this.renderEngine.getSelection();
        if (sel) {
          this.editEngine.startEdit(sel.startRow, sel.startCol, this.formulaBar.value);
          this.editEngine.commitEdit();
        }
      }
    });
    formulaBarWrap.appendChild(fxLabel);
    formulaBarWrap.appendChild(cellRef);
    formulaBarWrap.appendChild(this.formulaBar);
    wrapper.appendChild(formulaBarWrap);

    // 表格区域
    this.gridContainer = document.createElement('div');
    this.gridContainer.className = 'zq-sheet-grid-container';
    this.gridContainer.tabIndex = 0;  // 可聚焦以接收键盘事件
    wrapper.appendChild(this.gridContainer);

    // 工作表标签
    this.sheetTabContainer = document.createElement('div');
    this.sheetTabContainer.className = 'zq-sheet-tabs';
    wrapper.appendChild(this.sheetTabContainer);

    // 状态栏
    this.statusBar = document.createElement('div');
    this.statusBar.className = 'zq-sheet-status-bar';
    this.statusBar.textContent = '就绪';
    wrapper.appendChild(this.statusBar);

    return wrapper;
  }

  private setupLayoutStyles(): void {
    if (document.getElementById('zq-sheet-layout-styles')) return;
    const style = document.createElement('style');
    style.id = 'zq-sheet-layout-styles';
    style.textContent = `
      .zq-sheet-wrapper {
        display: flex;
        flex-direction: column;
        width: 100%;
        height: 100%;
        background: #fff;
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
      }
      .zq-sheet-toolbar-container {
        flex-shrink: 0;
      }
      .zq-sheet-formula-bar {
        display: flex;
        align-items: center;
        gap: 4px;
        padding: 4px 8px;
        border-bottom: 1px solid #e0e0e0;
        background: #fafafa;
        flex-shrink: 0;
      }
      .zq-sheet-fx {
        font-style: italic;
        font-weight: bold;
        color: #4787f5;
        padding: 0 4px;
      }
      .zq-sheet-cell-ref {
        min-width: 60px;
        padding: 2px 8px;
        border: 1px solid #d0d0d0;
        border-radius: 3px;
        background: #fff;
        font-size: 12px;
        color: #333;
      }
      .zq-sheet-formula-input {
        flex: 1;
        height: 26px;
        border: 1px solid #d0d0d0;
        border-radius: 3px;
        padding: 0 8px;
        font-size: 13px;
        outline: none;
      }
      .zq-sheet-formula-input:focus {
        border-color: #4787f5;
      }
      .zq-sheet-grid-container {
        flex: 1;
        position: relative;
        overflow: hidden;
        outline: none;
      }
      .zq-sheet-tabs {
        display: flex;
        align-items: center;
        gap: 2px;
        padding: 4px 8px;
        border-top: 1px solid #e0e0e0;
        background: #fafafa;
        flex-shrink: 0;
        overflow-x: auto;
      }
      .zq-sheet-tab {
        display: inline-flex;
        align-items: center;
        padding: 4px 12px;
        border: 1px solid #d0d0d0;
        border-radius: 3px;
        background: #fff;
        cursor: pointer;
        font-size: 12px;
        color: #333;
        white-space: nowrap;
      }
      .zq-sheet-tab.active {
        background: #4787f5;
        color: #fff;
        border-color: #4787f5;
      }
      .zq-sheet-tab:hover:not(.active) {
        background: #e8e8e8;
      }
      .zq-sheet-tab-add {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        width: 24px;
        height: 24px;
        border: 1px solid #d0d0d0;
        border-radius: 3px;
        background: #fff;
        cursor: pointer;
        font-size: 16px;
        color: #666;
      }
      .zq-sheet-tab-add:hover {
        background: #e8e8e8;
      }
      .zq-sheet-status-bar {
        padding: 2px 12px;
        border-top: 1px solid #e0e0e0;
        background: #fafafa;
        font-size: 12px;
        color: #666;
        flex-shrink: 0;
        display: flex;
        justify-content: space-between;
      }
    `;
    document.head.appendChild(style);
  }

  // ============================================================
  // 全局事件
  // ============================================================

  private bindGlobalEvents(): void {
    // 点击表格区域时聚焦
    this.gridContainer.addEventListener('mousedown', () => {
      this.gridContainer.focus();
    });

    // Ctrl+S 保存
    document.addEventListener('keydown', (e) => {
      if ((e.ctrlKey || e.metaKey) && e.key === 's') {
        e.preventDefault();
        this.save();
      }
      // Ctrl+B 加粗
      if ((e.ctrlKey || e.metaKey) && e.key === 'b') {
        e.preventDefault();
        const sel = this.renderEngine.getSelection();
        if (sel) {
          const style = this.sheet.getCellStyle(sel.startRow, sel.startCol);
          this.editEngine.applyStyleToSelection({ bold: !style.bold });
        }
      }
      // Ctrl+I 斜体
      if ((e.ctrlKey || e.metaKey) && e.key === 'i') {
        e.preventDefault();
        const sel = this.renderEngine.getSelection();
        if (sel) {
          const style = this.sheet.getCellStyle(sel.startRow, sel.startCol);
          this.editEngine.applyStyleToSelection({ italic: !style.italic });
        }
      }
      // Ctrl+U 下划线
      if ((e.ctrlKey || e.metaKey) && e.key === 'u') {
        e.preventDefault();
        const sel = this.renderEngine.getSelection();
        if (sel) {
          const style = this.sheet.getCellStyle(sel.startRow, sel.startCol);
          this.editEngine.applyStyleToSelection({ underline: !style.underline });
        }
      }
    });
  }

  // ============================================================
  // 公式栏
  // ============================================================

  /** 更新公式栏 */
  private updateFormulaBar(row: number, col: number): void {
    const cellRef = this.formulaBar.parentElement?.querySelector('.zq-sheet-cell-ref');
    if (cellRef) {
      cellRef.textContent = SheetModel.cellAddress(row, col);
    }
    this.formulaBar.value = this.getCellValueString(row, col);
  }

  /** 获取单元格值字符串 */
  private getCellValueString(row: number, col: number): string {
    const cell = this.sheet.getCell(row, col);
    if (!cell) return '';
    if (cell.formula) return `=${cell.formula}`;
    return String(cell.value);
  }

  // ============================================================
  // 状态栏
  // ============================================================

  /** 更新状态栏 */
  private updateStatusBar(range: SelectionRange): void {
    let sum = 0;
    let count = 0;
    let hasNumbers = false;

    for (let r = range.startRow; r <= range.endRow; r++) {
      for (let c = range.startCol; c <= range.endCol; c++) {
        const cell = this.sheet.getCell(r, c);
        if (cell && typeof cell.value === 'number') {
          sum += cell.value;
          count++;
          hasNumbers = true;
        }
      }
    }

    const parts: string[] = [];
    const cellCount = (range.endRow - range.startRow + 1) * (range.endCol - range.startCol + 1);
    parts.push(`单元格: ${cellCount}`);

    if (hasNumbers) {
      parts.push(`求和: ${Math.round(sum * 100) / 100}`);
      parts.push(`平均: ${Math.round((sum / count) * 100) / 100}`);
      parts.push(`计数: ${count}`);
    }

    this.statusBar.textContent = parts.join('  |  ');
  }

  // ============================================================
  // 工作表标签
  // ============================================================

  /** 渲染工作表标签 */
  private renderSheetTabs(): void {
    this.sheetTabContainer.innerHTML = '';

    const sheets = this.docModel.getAllSheets();
    for (let i = 0; i < sheets.length; i++) {
      const tab = document.createElement('div');
      tab.className = 'zq-sheet-tab' + (i === this.currentSheetIndex ? ' active' : '');
      tab.textContent = sheets[i].name;
      tab.addEventListener('click', () => this.switchSheet(i));
      this.sheetTabContainer.appendChild(tab);
    }

    // 添加新工作表按钮
    const addBtn = document.createElement('div');
    addBtn.className = 'zq-sheet-tab-add';
    addBtn.textContent = '+';
    addBtn.title = '新建工作表';
    addBtn.addEventListener('click', () => this.addSheet());
    this.sheetTabContainer.appendChild(addBtn);
  }

  /** 切换工作表 */
  switchSheet(index: number): void {
    const sheet = this.docModel.getSheet(index);
    if (!sheet) return;

    this.currentSheetIndex = index;
    this.sheet = sheet;
    this.docModel.setActiveSheet(index);

    // 更新所有模块的工作表引用
    this.renderEngine.setSheet(sheet);
    this.formulaEngine.setSheet(sheet);
    this.editEngine.setSheet(sheet);
    this.clipboard.setSheet(sheet);
    this.toolbar.setSheet(sheet);
    this.freezePane.setSheet(sheet);
    this.hideManager.setSheet(sheet);
    this.filterSort.setSheet(sheet);
    this.findReplace.setSheet(sheet);
    this.conditionalFormat.setSheet(sheet);
    this.dataValidation.setSheet(sheet);
    this.commentManager.setSheet(sheet);
    this.outlineManager.setSheet(sheet);
    this.imageManager.setSheet(sheet);
    this.chartEngine.setSheet(sheet);
    this.borderManager = new BorderManager(sheet);
    this.dragManager.setSheet(sheet);
    // PivotTableEngine 和 PrintManager 没有 setSheet 方法, 重新创建实例
    this.pivotEngine = new PivotTableEngine(sheet);
    this.printManager = new PrintManager(sheet);

    this.renderSheetTabs();
    this.renderEngine.refresh();
  }

  /** 添加工作表 */
  addSheet(): void {
    const newSheet = new SheetModel({
      name: `Sheet${this.docModel.getSheetCount() + 1}`,
      index: this.docModel.getSheetCount(),
    });
    this.docModel.addSheet(newSheet);
    this.renderSheetTabs();
    this.switchSheet(this.docModel.getSheetCount() - 1);
    this.markDirty();
  }

  // ============================================================
  // 右键菜单
  // ============================================================

  /** 显示右键菜单 */
  private showContextMenu(row: number, col: number, e: MouseEvent): void {
    // 移除已有菜单
    const existing = document.querySelector('.zq-context-menu');
    if (existing) existing.remove();

    const menu = document.createElement('div');
    menu.className = 'zq-context-menu';
    menu.style.position = 'fixed';
    menu.style.left = `${e.clientX}px`;
    menu.style.top = `${e.clientY}px`;
    menu.style.background = '#fff';
    menu.style.border = '1px solid #d0d0d0';
    menu.style.borderRadius = '4px';
    menu.style.boxShadow = '0 4px 12px rgba(0,0,0,0.15)';
    menu.style.padding = '4px 0';
    menu.style.zIndex = '1000';
    menu.style.minWidth = '160px';
    menu.style.fontSize = '13px';

    const items = [
      { label: '剪切', action: () => this.clipboard.cut(), shortcut: 'Ctrl+X' },
      { label: '复制', action: () => this.clipboard.copy(), shortcut: 'Ctrl+C' },
      { label: '粘贴', action: () => this.clipboard.paste('all'), shortcut: 'Ctrl+V' },
      { divider: true },
      { label: '插入行', action: () => this.editEngine.insertRows(row, 1) },
      { label: '插入列', action: () => this.editEngine.insertColumns(col, 1) },
      { label: '删除行', action: () => this.editEngine.deleteRows(row, 1) },
      { label: '删除列', action: () => this.editEngine.deleteColumns(col, 1) },
      { divider: true },
      { label: '清除内容', action: () => this.editEngine.clearSelectionContent(), shortcut: 'Delete' },
      { label: '设置单元格格式', action: () => { /* TODO: 打开格式对话框 */ } },
    ];

    for (const item of items) {
      if (item.divider) {
        const div = document.createElement('div');
        div.style.height = '1px';
        div.style.background = '#e0e0e0';
        div.style.margin = '4px 0';
        menu.appendChild(div);
        continue;
      }
      const el = document.createElement('div');
      el.style.padding = '6px 16px';
      el.style.cursor = 'pointer';
      el.style.display = 'flex';
      el.style.justifyContent = 'space-between';
      el.style.gap = '24px';
      el.innerHTML = `<span>${item.label}</span>${item.shortcut ? `<span style="color:#999;font-size:11px">${item.shortcut}</span>` : ''}`;
      el.addEventListener('mouseenter', () => { el.style.background = '#e8e8e8'; });
      el.addEventListener('mouseleave', () => { el.style.background = 'transparent'; });
      el.addEventListener('click', () => {
        item.action?.();
        menu.remove();
      });
      menu.appendChild(el);
    }

    document.body.appendChild(menu);

    // 点击外部关闭
    const closeHandler = (ev: MouseEvent) => {
      if (!menu.contains(ev.target as Node)) {
        menu.remove();
        document.removeEventListener('mousedown', closeHandler);
      }
    };
    setTimeout(() => document.addEventListener('mousedown', closeHandler), 0);
  }

  // ============================================================
  // 文档加载/保存
  // ============================================================

  /**
   * 加载文档
   * @param json ZQ Sheet JSON
   */
  loadDocument(json: ZQSheetDocument): void {
    this.docModel.loadFromJSON(json);
    this.currentSheetIndex = 0;
    this.sheet = this.docModel.getActiveSheet();
    this.renderEngine.setSheet(this.sheet);
    this.formulaEngine.setSheet(this.sheet);
    this.editEngine.setSheet(this.sheet);
    this.clipboard.setSheet(this.sheet);
    this.toolbar.setSheet(this.sheet);
    this.freezePane.setSheet(this.sheet);
    this.hideManager.setSheet(this.sheet);
    this.filterSort.setSheet(this.sheet);
    this.findReplace.setSheet(this.sheet);
    this.conditionalFormat.setSheet(this.sheet);
    this.dataValidation.setSheet(this.sheet);
    this.commentManager.setSheet(this.sheet);
    this.outlineManager.setSheet(this.sheet);
    this.imageManager.setSheet(this.sheet);
    this.chartEngine.setSheet(this.sheet);
    this.borderManager = new BorderManager(this.sheet);
    this.dragManager.setSheet(this.sheet);
    // PivotTableEngine 和 PrintManager 没有 setSheet 方法, 重新创建实例
    this.pivotEngine = new PivotTableEngine(this.sheet);
    this.printManager = new PrintManager(this.sheet);
    this.formulaEngine.calculateAll();
    this.renderEngine.refresh();
    this.renderSheetTabs();
    this.editEngine.clearHistory();
    this.isDirty = false;
  }

  /**
   * 获取文档 JSON
   */
  getDocumentJSON(): ZQSheetDocument {
    return this.docModel.toJSON();
  }

  /**
   * 获取文档 JSON 字符串
   */
  getDocumentJSONString(): string {
    return this.docModel.toJSONString();
  }

  /**
   * 保存文档
   * 调用 IPC 回调将 JSON 传递给主进程
   */
  async save(): Promise<void> {
    if (!this.config.onSave) {
      console.warn('SheetEngine: 未配置 onSave 回调');
      return;
    }

    const json = this.getDocumentJSONString();
    try {
      await this.config.onSave(json, this.config.filePath);
      this.isDirty = false;
      this.updateTitle();
    } catch (err) {
      console.error('SheetEngine: 保存失败', err);
      alert('保存失败: ' + (err as Error).message);
    }
  }

  /** 标记内容已修改 */
  private markDirty(): void {
    this.isDirty = true;
    this.updateTitle();
    this.config.onContentChange?.();
  }

  /** 更新标题 */
  private updateTitle(): void {
    const title = (this.config.fileName || '未命名') + (this.isDirty ? ' *' : '');
    this.config.onTitleChange?.(title);
  }

  /** 是否已修改 */
  isModified(): boolean {
    return this.isDirty;
  }

  // ============================================================
  // 其他
  // ============================================================

  /** 设置只读模式 */
  setReadOnly(readOnly: boolean): void {
    this.gridContainer.style.pointerEvents = readOnly ? 'none' : 'auto';
    this.toolbarContainer.style.pointerEvents = readOnly ? 'none' : 'auto';
    this.toolbarContainer.style.opacity = readOnly ? '0.6' : '1';
  }

  /** 获取当前选区 */
  getSelection(): SelectionRange | null {
    return this.renderEngine.getSelection();
  }

  /** 销毁 */
  destroy(): void {
    this.editEngine.destroy();
    this.renderEngine.destroy();
    this.toolbar.destroy();
    this.findReplace.destroy();
    this.dataValidation.destroy();
    this.commentManager.destroy();
    this.imageManager.destroy();
    this.chartEngine.destroy();
    this.dragManager.destroy();
    this.undoRedo.clear();
    this.container.innerHTML = '';
  }

  // ============================================================
  // 模块访问器 — 供外部调用各功能层 API
  // ============================================================

  get freeze() { return this.freezePane; }
  get hide() { return this.hideManager; }
  get filterSortEngine() { return this.filterSort; }
  get find() { return this.findReplace; }
  get condFormat() { return this.conditionalFormat; }
  get validation() { return this.dataValidation; }
  get comments() { return this.commentManager; }
  get outline() { return this.outlineManager; }
  get images() { return this.imageManager; }
  get charts() { return this.chartEngine; }
  get border() { return this.borderManager; }
  get drag() { return this.dragManager; }
  get pivot() { return this.pivotEngine; }
  get print() { return this.printManager; }
}
