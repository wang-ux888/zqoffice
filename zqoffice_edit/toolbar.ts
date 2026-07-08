/**
 * 工具栏模块 — UI 按钮组 (字体/字号/加粗/对齐/边框/合并/插入行列等)
 * 全部原创实现
 *
 * 职责:
 *   1. 创建 DOM 工具栏
 *   2. 按钮事件 → 调用 EditEngine 方法
 *   3. 选区变化时同步按钮状态 (加粗/斜体等高亮)
 *   4. 中文标签
 */

import { SheetModel, CellStyle } from './data-model';
import { RenderEngine, SelectionRange } from './render-engine';
import { EditEngine } from './edit-engine';
import { ClipboardManager } from './clipboard';
import { UndoRedoManager } from './undo-redo';

/** 工具栏按钮定义 */
interface ToolbarButton {
  id: string;
  label: string;
  title: string;
  icon?: string;
  action: () => void;
  getState?: () => boolean;  // 用于高亮状态
}

/** 字体列表 */
const FONT_FAMILIES = [
  { name: '微软雅黑', value: '"Microsoft YaHei", sans-serif' },
  { name: '宋体', value: 'SimSun, serif' },
  { name: '黑体', value: 'SimHei, sans-serif' },
  { name: '楷体', value: 'KaiTi, serif' },
  { name: 'Arial', value: 'Arial, sans-serif' },
  { name: 'Times New Roman', value: '"Times New Roman", serif' },
  { name: 'Calibri', value: 'Calibri, sans-serif' },
];

/** 字号列表 */
const FONT_SIZES = [10, 11, 12, 13, 14, 16, 18, 20, 22, 24, 28, 32, 36, 48];

/** 数字格式列表 */
const NUMBER_FORMATS = [
  { name: '常规', value: 'General' },
  { name: '数字', value: '0.00' },
  { name: '整数', value: '0' },
  { name: '千分位', value: '#,##0.00' },
  { name: '百分比', value: '0.00%' },
  { name: '日期', value: 'yyyy-mm-dd' },
  { name: '时间', value: 'yyyy-mm-dd hh:mm:ss' },
  { name: '货币', value: '¥#,##0.00' },
];

export class Toolbar {
  private container: HTMLElement;
  private sheet: SheetModel;
  private renderEngine: RenderEngine;
  private editEngine: EditEngine;
  private clipboard: ClipboardManager;
  private undoRedo: UndoRedoManager;

  private toolbarEl: HTMLDivElement;
  private buttons = new Map<string, HTMLElement>();

  constructor(
    container: HTMLElement,
    sheet: SheetModel,
    renderEngine: RenderEngine,
    editEngine: EditEngine,
    clipboard: ClipboardManager,
    undoRedo: UndoRedoManager
  ) {
    this.container = container;
    this.sheet = sheet;
    this.renderEngine = renderEngine;
    this.editEngine = editEngine;
    this.clipboard = clipboard;
    this.undoRedo = undoRedo;

    this.toolbarEl = document.createElement('div');
    this.toolbarEl.className = 'zq-toolbar';
    this.container.appendChild(this.toolbarEl);

    this.setupStyles();
    this.render();
    this.bindEvents();
  }

  // ============================================================
  // 样式
  // ============================================================

  private setupStyles(): void {
    if (document.getElementById('zq-toolbar-styles')) return;
    const style = document.createElement('style');
    style.id = 'zq-toolbar-styles';
    style.textContent = `
      .zq-toolbar {
        display: flex;
        align-items: center;
        gap: 2px;
        padding: 4px 8px;
        background: #fafafa;
        border-bottom: 1px solid #e0e0e0;
        flex-wrap: wrap;
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
        font-size: 13px;
      }
      .zq-toolbar-group {
        display: flex;
        align-items: center;
        gap: 2px;
        padding: 0 4px;
        border-right: 1px solid #e0e0e0;
      }
      .zq-toolbar-group:last-child {
        border-right: none;
      }
      .zq-toolbar-btn {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        min-width: 28px;
        height: 28px;
        padding: 0 6px;
        border: 1px solid transparent;
        border-radius: 4px;
        background: transparent;
        cursor: pointer;
        color: #333;
        font-size: 13px;
        user-select: none;
        transition: background 0.1s, border-color 0.1s;
      }
      .zq-toolbar-btn:hover {
        background: #e8e8e8;
        border-color: #d0d0d0;
      }
      .zq-toolbar-btn.active {
        background: #e3edf7;
        border-color: #4787f5;
        color: #4787f5;
      }
      .zq-toolbar-btn:disabled {
        opacity: 0.4;
        cursor: not-allowed;
      }
      .zq-toolbar-select {
        height: 28px;
        border: 1px solid #d0d0d0;
        border-radius: 4px;
        background: #fff;
        font-size: 13px;
        padding: 0 4px;
        cursor: pointer;
      }
      .zq-toolbar-select:hover {
        border-color: #4787f5;
      }
      .zq-toolbar-divider {
        width: 1px;
        height: 20px;
        background: #d0d0d0;
        margin: 0 2px;
      }
    `;
    document.head.appendChild(style);
  }

  // ============================================================
  // 渲染
  // ============================================================

  private render(): void {
    this.toolbarEl.innerHTML = '';

    // 撤销/重做
    this.renderGroup([
      { id: 'undo', label: '↶', title: '撤销 (Ctrl+Z)', action: () => this.editEngine.undo() },
      { id: 'redo', label: '↷', title: '重做 (Ctrl+Y)', action: () => this.editEngine.redo() },
    ]);

    this.renderDivider();

    // 复制/粘贴
    this.renderGroup([
      { id: 'copy', label: '复制', title: '复制 (Ctrl+C)', action: () => this.clipboard.copy() },
      { id: 'paste', label: '粘贴', title: '粘贴 (Ctrl+V)', action: () => this.clipboard.paste('all') },
      { id: 'cut', label: '剪切', title: '剪切 (Ctrl+X)', action: () => this.clipboard.cut() },
    ]);

    this.renderDivider();

    // 字体
    this.renderFontFamily();
    this.renderFontSize();

    this.renderDivider();

    // 加粗/斜体/下划线
    this.renderGroup([
      {
        id: 'bold', label: '<b>B</b>', title: '加粗 (Ctrl+B)',
        action: () => this.toggleStyle({ bold: true }),
        getState: () => this.isStyleActive('bold'),
      },
      {
        id: 'italic', label: '<i>I</i>', title: '斜体 (Ctrl+I)',
        action: () => this.toggleStyle({ italic: true }),
        getState: () => this.isStyleActive('italic'),
      },
      {
        id: 'underline', label: '<u>U</u>', title: '下划线 (Ctrl+U)',
        action: () => this.toggleStyle({ underline: true }),
        getState: () => this.isStyleActive('underline'),
      },
      {
        id: 'strikeThrough', label: '<s>S</s>', title: '删除线',
        action: () => this.toggleStyle({ strikeThrough: true }),
        getState: () => this.isStyleActive('strikeThrough'),
      },
    ]);

    this.renderDivider();

    // 对齐
    this.renderGroup([
      {
        id: 'alignLeft', label: '左', title: '左对齐',
        action: () => this.editEngine.applyStyleToSelection({ horizontalAlignment: 'left' }),
        getState: () => this.isStyleActive('horizontalAlignment', 'left'),
      },
      {
        id: 'alignCenter', label: '中', title: '居中对齐',
        action: () => this.editEngine.applyStyleToSelection({ horizontalAlignment: 'center' }),
        getState: () => this.isStyleActive('horizontalAlignment', 'center'),
      },
      {
        id: 'alignRight', label: '右', title: '右对齐',
        action: () => this.editEngine.applyStyleToSelection({ horizontalAlignment: 'right' }),
        getState: () => this.isStyleActive('horizontalAlignment', 'right'),
      },
    ]);

    this.renderDivider();

    // 颜色
    this.renderColorButton('color', '文字颜色', 'A', '#333333');
    this.renderColorButton('backgroundColor', '背景颜色', '🎨', '#ffff00');

    this.renderDivider();

    // 数字格式
    this.renderNumberFormat();

    this.renderDivider();

    // 合并
    this.renderGroup([
      { id: 'merge', label: '合并', title: '合并单元格', action: () => this.editEngine.mergeSelection() },
      { id: 'unmerge', label: '取消合并', title: '取消合并', action: () => this.editEngine.unmergeSelection() },
    ]);

    this.renderDivider();

    // 行列操作
    this.renderGroup([
      { id: 'insertRow', label: '↑插行', title: '在上方插入行', action: () => this.insertRowAbove() },
      { id: 'insertCol', label: '←插列', title: '在左侧插入列', action: () => this.insertColLeft() },
      { id: 'deleteRow', label: '删行', title: '删除行', action: () => this.deleteRows() },
      { id: 'deleteCol', label: '删列', title: '删除列', action: () => this.deleteCols() },
    ]);
  }

  /** 渲染按钮组 */
  private renderGroup(buttons: ToolbarButton[]): void {
    const group = document.createElement('div');
    group.className = 'zq-toolbar-group';
    for (const btn of buttons) {
      const el = document.createElement('button');
      el.className = 'zq-toolbar-btn';
      el.title = btn.title;
      el.innerHTML = btn.label;
      el.addEventListener('click', (e) => {
        e.preventDefault();
        btn.action();
        this.updateButtonStates();
      });
      group.appendChild(el);
      this.buttons.set(btn.id, el);
    }
    this.toolbarEl.appendChild(group);
  }

  /** 渲染分隔线 */
  private renderDivider(): void {
    const div = document.createElement('div');
    div.className = 'zq-toolbar-divider';
    this.toolbarEl.appendChild(div);
  }

  /** 渲染字体选择 */
  private renderFontFamily(): void {
    const select = document.createElement('select');
    select.className = 'zq-toolbar-select';
    select.title = '字体';
    for (const font of FONT_FAMILIES) {
      const opt = document.createElement('option');
      opt.value = font.value;
      opt.textContent = font.name;
      select.appendChild(opt);
    }
    select.addEventListener('change', () => {
      this.editEngine.applyStyleToSelection({ fontFamily: select.value });
    });
    this.toolbarEl.appendChild(select);
  }

  /** 渲染字号选择 */
  private renderFontSize(): void {
    const select = document.createElement('select');
    select.className = 'zq-toolbar-select';
    select.style.width = '60px';
    select.title = '字号';
    for (const size of FONT_SIZES) {
      const opt = document.createElement('option');
      opt.value = String(size);
      opt.textContent = String(size);
      if (size === 13) opt.selected = true;
      select.appendChild(opt);
    }
    select.addEventListener('change', () => {
      this.editEngine.applyStyleToSelection({ fontSize: Number(select.value) });
    });
    this.toolbarEl.appendChild(select);
  }

  /** 渲染颜色按钮 */
  private renderColorButton(styleKey: keyof CellStyle, title: string, label: string, defaultColor: string): void {
    const wrapper = document.createElement('div');
    wrapper.style.display = 'inline-flex';
    wrapper.style.alignItems = 'center';

    const btn = document.createElement('button');
    btn.className = 'zq-toolbar-btn';
    btn.title = title;
    btn.innerHTML = label;
    btn.style.position = 'relative';

    const colorInput = document.createElement('input');
    colorInput.type = 'color';
    colorInput.value = defaultColor;
    colorInput.style.position = 'absolute';
    colorInput.style.opacity = '0';
    colorInput.style.width = '100%';
    colorInput.style.height = '100%';
    colorInput.style.cursor = 'pointer';

    colorInput.addEventListener('change', () => {
      const style: Partial<CellStyle> = {};
      (style as any)[styleKey] = colorInput.value;
      this.editEngine.applyStyleToSelection(style);
    });

    btn.appendChild(colorInput);
    wrapper.appendChild(btn);
    this.toolbarEl.appendChild(wrapper);
  }

  /** 渲染数字格式 */
  private renderNumberFormat(): void {
    const select = document.createElement('select');
    select.className = 'zq-toolbar-select';
    select.title = '数字格式';
    for (const fmt of NUMBER_FORMATS) {
      const opt = document.createElement('option');
      opt.value = fmt.value;
      opt.textContent = fmt.name;
      select.appendChild(opt);
    }
    select.addEventListener('change', () => {
      this.editEngine.applyStyleToSelection({ numberFormat: select.value });
    });
    this.toolbarEl.appendChild(select);
  }

  // ============================================================
  // 事件
  // ============================================================

  private bindEvents(): void {
    // 监听选区变化更新按钮状态
    // (通过 onSelectionChange 回调, 由 sheet-engine 转发)
  }

  /**
   * 更新按钮状态 (选区变化时调用)
   */
  updateButtonStates(): void {
    const sel = this.renderEngine.getSelection();
    if (!sel) return;

    // 撤销/重做按钮
    const undoBtn = this.buttons.get('undo') as HTMLButtonElement | undefined;
    const redoBtn = this.buttons.get('redo') as HTMLButtonElement | undefined;
    if (undoBtn) {
      undoBtn.disabled = !this.editEngine.canUndo();
    }
    if (redoBtn) {
      redoBtn.disabled = !this.editEngine.canRedo();
    }

    // 加粗/斜体等
    for (const [id, el] of this.buttons) {
      if (id === 'bold' || id === 'italic' || id === 'underline' || id === 'strikeThrough') {
        const key = id as keyof CellStyle;
        el.classList.toggle('active', this.isStyleActive(key));
      }
      if (id === 'alignLeft' || id === 'alignCenter' || id === 'alignRight') {
        const value = id === 'alignLeft' ? 'left' : id === 'alignCenter' ? 'center' : 'right';
        el.classList.toggle('active', this.isStyleActive('horizontalAlignment', value));
      }
    }
  }

  // ============================================================
  // 样式辅助
  // ============================================================

  /**
   * 切换布尔样式 (加粗/斜体等)
   */
  private toggleStyle(style: Partial<CellStyle>): void {
    const sel = this.renderEngine.getSelection();
    if (!sel) return;

    const key = Object.keys(style)[0] as keyof CellStyle;
    const current = this.sheet.getCellStyle(sel.startRow, sel.startCol);
    const currentValue = current[key] as boolean | undefined;
    const newValue = !currentValue;

    this.editEngine.applyStyleToSelection({ [key]: newValue } as Partial<CellStyle>);
  }

  /**
   * 判断样式是否激活
   */
  private isStyleActive(key: keyof CellStyle, value?: string): boolean {
    const sel = this.renderEngine.getSelection();
    if (!sel) return false;
    const style = this.sheet.getCellStyle(sel.startRow, sel.startCol);
    if (value) {
      return style[key] === value;
    }
    return Boolean(style[key]);
  }

  // ============================================================
  // 行列操作
  // ============================================================

  private insertRowAbove(): void {
    const sel = this.renderEngine.getSelection();
    if (!sel) return;
    this.editEngine.insertRows(sel.startRow, 1);
  }

  private insertColLeft(): void {
    const sel = this.renderEngine.getSelection();
    if (!sel) return;
    this.editEngine.insertColumns(sel.startCol, 1);
  }

  private deleteRows(): void {
    const sel = this.renderEngine.getSelection();
    if (!sel) return;
    const count = sel.endRow - sel.startRow + 1;
    this.editEngine.deleteRows(sel.startRow, count);
  }

  private deleteCols(): void {
    const sel = this.renderEngine.getSelection();
    if (!sel) return;
    const count = sel.endCol - sel.startCol + 1;
    this.editEngine.deleteColumns(sel.startCol, count);
  }

  /** 更新工作表引用 (供 sheet-engine 切换工作表时调用) */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
  }

  /** 销毁 */
  destroy(): void {
    this.toolbarEl.remove();
  }
}
