/**
 * Word 文档工具栏 — 按钮、下拉菜单、状态同步
 */

import { DocModel, ParagraphAlignment, DEFAULT_FONT } from './doc-model';
import { DocRenderEngine } from './doc-render-engine';
import { DocEditEngine } from './doc-edit-engine';

/** 工具栏回调 */
export interface DocToolbarCallbacks {
  onContentChange?: () => void;
}

export class DocToolbar {
  private container: HTMLElement;
  private docModel: DocModel;
  private renderEngine: DocRenderEngine;
  private editEngine: DocEditEngine;
  private callbacks: DocToolbarCallbacks;

  // 工具栏元素
  private toolbarEl!: HTMLDivElement;

  constructor(
    container: HTMLElement,
    docModel: DocModel,
    renderEngine: DocRenderEngine,
    editEngine: DocEditEngine,
    callbacks: DocToolbarCallbacks = {},
  ) {
    this.container = container;
    this.docModel = docModel;
    this.renderEngine = renderEngine;
    this.editEngine = editEngine;
    this.callbacks = callbacks;
    this.create();
  }

  private create(): void {
    this.toolbarEl = document.createElement('div');
    this.toolbarEl.className = 'zq-doc-toolbar';
    this.toolbarEl.style.cssText = `
      display: flex;
      flex-wrap: wrap;
      gap: 4px;
      padding: 6px 12px;
      background: #fafafa;
      border-bottom: 1px solid #e0e0e0;
      align-items: center;
      font-size: 13px;
    `;

    // 撤销重做
    this.addButton('↶', '撤销', () => this.editEngine.undo());
    this.addButton('↷', '重做', () => this.editEngine.redo());
    this.addSeparator();

    // 标题级别
    this.addHeadingSelect();
    this.addSeparator();

    // 字体族
    this.addFontFamilySelect();
    // 字号
    this.addFontSizeSelect();
    this.addSeparator();

    // 加粗/斜体/下划线
    this.addButton('<b>B</b>', '加粗 (Ctrl+B)', () => this.execToggle('bold'));
    this.addButton('<i>I</i>', '斜体 (Ctrl+I)', () => this.execToggle('italic'));
    this.addButton('<u>U</u>', '下划线 (Ctrl+U)', () => this.execToggle('underline'));
    this.addSeparator();

    // 文字颜色
    this.addColorPicker('A', '文字颜色', (color) => this.execSetFont({ color }));
    this.addSeparator();

    // 对齐
    this.addButton('≡', '左对齐', () => this.execSetAlignment('left'));
    this.addButton('☰', '居中', () => this.execSetAlignment('center'));
    this.addButton('≣', '右对齐', () => this.execSetAlignment('right'));
    this.addButton('☰☰', '两端对齐', () => this.execSetAlignment('justify'));
    this.addSeparator();

    // 插入
    this.addButton('¶', '插入段落', () => this.execInsertParagraph());
    this.addButton('tbl', '插入表格', () => this.execInsertTable());
    this.addButton('img', '插入图片', () => this.execInsertImage());
    this.addSeparator();

    // 编辑操作
    this.addButton('⎘', '复制块 (Ctrl+D)', () => this.editEngine.duplicateBlock());
    this.addButton('↑', '上移', () => this.execMoveUp());
    this.addButton('↓', '下移', () => this.execMoveDown());
    this.addButton('🗑', '删除块', () => this.execDeleteBlock());

    this.container.appendChild(this.toolbarEl);
  }

  // ============================================================
  // UI 工具方法
  // ============================================================

  private addButton(label: string, title: string, onClick: () => void): HTMLButtonElement {
    const btn = document.createElement('button');
    btn.innerHTML = label;
    btn.title = title;
    btn.style.cssText = `
      padding: 4px 8px;
      border: 1px solid transparent;
      background: transparent;
      cursor: pointer;
      border-radius: 3px;
      min-width: 26px;
      min-height: 26px;
      font-size: 13px;
      color: #333;
      display: inline-flex;
      align-items: center;
      justify-content: center;
    `;
    btn.addEventListener('mouseenter', () => {
      btn.style.background = '#e8e8e8';
      btn.style.border = '1px solid #d0d0d0';
    });
    btn.addEventListener('mouseleave', () => {
      btn.style.background = 'transparent';
      btn.style.border = '1px solid transparent';
    });
    btn.addEventListener('click', onClick);
    this.toolbarEl.appendChild(btn);
    return btn;
  }

  private addSeparator(): void {
    const sep = document.createElement('div');
    sep.style.cssText = `
      width: 1px;
      height: 18px;
      background: #d0d0d0;
      margin: 0 4px;
    `;
    this.toolbarEl.appendChild(sep);
  }

  private addHeadingSelect(): void {
    const select = document.createElement('select');
    select.title = '段落样式';
    select.style.cssText = `
      padding: 3px 6px;
      border: 1px solid #d0d0d0;
      border-radius: 3px;
      background: white;
      font-size: 13px;
      min-width: 90px;
    `;
    const options = [
      { value: 'p', label: '正文' },
      { value: 'h1', label: '标题 1' },
      { value: 'h2', label: '标题 2' },
      { value: 'h3', label: '标题 3' },
      { value: 'h4', label: '标题 4' },
      { value: 'h5', label: '标题 5' },
      { value: 'h6', label: '标题 6' },
    ];
    for (const opt of options) {
      const o = document.createElement('option');
      o.value = opt.value;
      o.textContent = opt.label;
      select.appendChild(o);
    }
    select.addEventListener('change', () => {
      const blockId = this.editEngine.getSelectedBlockId();
      if (!blockId) return;
      if (select.value === 'p') {
        this.editEngine.convertToParagraph(blockId);
      } else {
        const level = parseInt(select.value.slice(1));
        this.editEngine.convertToHeading(blockId, level);
      }
    });
    this.toolbarEl.appendChild(select);
  }

  private addFontFamilySelect(): void {
    const select = document.createElement('select');
    select.title = '字体';
    select.style.cssText = `
      padding: 3px 6px;
      border: 1px solid #d0d0d0;
      border-radius: 3px;
      background: white;
      font-size: 13px;
      min-width: 110px;
    `;
    const fonts = [
      { value: 'Calibri', label: 'Calibri' },
      { value: 'Arial', label: 'Arial' },
      { value: 'Times New Roman', label: 'Times New Roman' },
      { value: 'Microsoft YaHei', label: '微软雅黑' },
      { value: 'SimSun', label: '宋体' },
      { value: 'Courier New', label: 'Courier New' },
    ];
    for (const f of fonts) {
      const o = document.createElement('option');
      o.value = f.value;
      o.textContent = f.label;
      select.appendChild(o);
    }
    select.value = 'Calibri';
    select.addEventListener('change', () => {
      this.execSetFont({ family: select.value });
    });
    this.toolbarEl.appendChild(select);
  }

  private addFontSizeSelect(): void {
    const select = document.createElement('select');
    select.title = '字号';
    select.style.cssText = `
      padding: 3px 6px;
      border: 1px solid #d0d0d0;
      border-radius: 3px;
      background: white;
      font-size: 13px;
      min-width: 60px;
    `;
    for (const s of [8, 9, 10, 11, 12, 14, 16, 18, 20, 24, 28, 32, 48]) {
      const o = document.createElement('option');
      o.value = String(s);
      o.textContent = String(s);
      select.appendChild(o);
    }
    select.value = '11';
    select.addEventListener('change', () => {
      this.execSetFont({ size: parseInt(select.value) });
    });
    this.toolbarEl.appendChild(select);
  }

  private addColorPicker(label: string, title: string, onPick: (color: string) => void): void {
    const wrapper = document.createElement('div');
    wrapper.style.cssText = `position: relative; display: inline-block;`;
    const btn = document.createElement('button');
    btn.innerHTML = label;
    btn.title = title;
    btn.style.cssText = `
      padding: 4px 8px;
      border: 1px solid transparent;
      background: transparent;
      cursor: pointer;
      border-radius: 3px;
      min-width: 26px;
      min-height: 26px;
      font-size: 13px;
    `;
    btn.addEventListener('mouseenter', () => {
      btn.style.background = '#e8e8e8';
      btn.style.border = '1px solid #d0d0d0';
    });
    btn.addEventListener('mouseleave', () => {
      btn.style.background = 'transparent';
      btn.style.border = '1px solid transparent';
    });
    const input = document.createElement('input');
    input.type = 'color';
    input.style.cssText = `
      position: absolute;
      top: 0; left: 0;
      width: 100%; height: 100%;
      opacity: 0;
      cursor: pointer;
    `;
    input.value = '#333333';
    input.addEventListener('change', () => {
      // #RRGGBB → RRGGBB
      const color = input.value.slice(1);
      onPick(color);
    });
    btn.appendChild(input);
    wrapper.appendChild(btn);
    this.toolbarEl.appendChild(wrapper);
  }

  // ============================================================
  // 命令执行
  // ============================================================

  private execToggle(attr: 'bold' | 'italic' | 'underline'): void {
    const blockId = this.editEngine.getSelectedBlockId();
    if (!blockId) return;
    if (attr === 'bold') this.editEngine.toggleBold(blockId);
    if (attr === 'italic') this.editEngine.toggleItalic(blockId);
    if (attr === 'underline') this.editEngine.toggleUnderline(blockId);
  }

  private execSetFont(font: Partial<typeof DEFAULT_FONT>): void {
    const blockId = this.editEngine.getSelectedBlockId();
    if (!blockId) return;
    this.editEngine.setFont(blockId, font);
  }

  private execSetAlignment(alignment: ParagraphAlignment): void {
    const blockId = this.editEngine.getSelectedBlockId();
    if (!blockId) return;
    this.editEngine.setAlignment(blockId, alignment);
  }

  private execInsertParagraph(): void {
    const blockId = this.editEngine.getSelectedBlockId();
    const blocks = this.docModel.getBlocks();
    const afterId = blockId ?? blocks[blocks.length - 1].id;
    const newId = this.editEngine.insertParagraphAfter(afterId, '');
    this.renderEngine.focusBlock(newId);
  }

  private execInsertTable(): void {
    const blockId = this.editEngine.getSelectedBlockId();
    const blocks = this.docModel.getBlocks();
    const afterId = blockId ?? blocks[blocks.length - 1].id;
    this.editEngine.insertTableAfter(afterId, 3, 3);
  }

  private execInsertImage(): void {
    // 弹出文件选择
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = 'image/*';
    input.addEventListener('change', async () => {
      const file = input.files?.[0];
      if (!file) return;
      // 转 data URL
      const reader = new FileReader();
      reader.onload = () => {
        const dataUrl = reader.result as string;
        const blockId = this.editEngine.getSelectedBlockId();
        const blocks = this.docModel.getBlocks();
        const afterId = blockId ?? blocks[blocks.length - 1].id;
        // 默认尺寸 400x300 px → EMU
        this.editEngine.insertImageAfter(afterId, dataUrl, 400 * 9525, 300 * 9525);
      };
      reader.readAsDataURL(file);
    });
    input.click();
  }

  private execMoveUp(): void {
    const blockId = this.editEngine.getSelectedBlockId();
    if (blockId) this.editEngine.moveBlockUp(blockId);
  }

  private execMoveDown(): void {
    const blockId = this.editEngine.getSelectedBlockId();
    if (blockId) this.editEngine.moveBlockDown(blockId);
  }

  private execDeleteBlock(): void {
    const blockId = this.editEngine.getSelectedBlockId();
    if (blockId) this.editEngine.deleteBlock(blockId);
  }

  // ============================================================
  // 工具方法
  // ============================================================

  /** 获取工具栏元素 */
  getElement(): HTMLDivElement {
    return this.toolbarEl;
  }

  /** 设置新模型 (用于加载新文档) */
  setModel(docModel: DocModel): void {
    this.docModel = docModel;
  }

  /** 销毁 */
  destroy(): void {
    this.container.innerHTML = '';
  }
}
