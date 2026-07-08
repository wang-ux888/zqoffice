/**
 * Word 文档渲染引擎 — DOM 渲染
 *
 * 职责:
 *   1. 将 DocModel 渲染为 DOM (分页视图)
 *   2. 渲染各种 block (paragraph/heading/table/image)
 *   3. 选区管理 (块级和文本级)
 *   4. 光标定位
 */

import { DocModel, Block, Font, ParagraphAlignment, HEADING_SIZES } from './doc-model';

/** 渲染引擎回调 */
export interface DocRenderCallbacks {
  onBlockClick?: (blockId: string, e: MouseEvent) => void;
  onBlockDoubleClick?: (blockId: string, e: MouseEvent) => void;
  onBlockInput?: (blockId: string, text: string) => void;
  onSelectionChange?: (blockId: string | null) => void;
}

/** 渲染选项 */
export interface DocRenderOptions {
  pageWidth?: number;       // 页面宽度 px (默认 794 = A4 @ 96dpi)
  pageHeight?: number;      // 页面高度 px (默认 1123)
  pagePadding?: number;     // 页面边距 px (默认 96 = 1 inch)
  showRuler?: boolean;      // 显示标尺
  zoom?: number;            // 缩放 1.0 = 100%
}

export class DocRenderEngine {
  private container: HTMLElement;
  private docModel: DocModel;
  private callbacks: DocRenderCallbacks;
  private options: DocRenderOptions;

  // DOM 元素
  private pageEl!: HTMLDivElement;
  private contentEl!: HTMLDivElement;

  // 当前选中的块
  private currentBlockId: string | null = null;

  constructor(
    container: HTMLElement,
    docModel: DocModel,
    callbacks: DocRenderCallbacks = {},
    options: DocRenderOptions = {},
  ) {
    this.container = container;
    this.docModel = docModel;
    this.callbacks = callbacks;
    this.options = {
      pageWidth: 794,
      pageHeight: 1123,
      pagePadding: 96,
      showRuler: false,
      zoom: 1.0,
      ...options,
    };
    this.createLayout();
    this.render();
  }

  // ============================================================
  // 布局
  // ============================================================

  private createLayout(): void {
    this.container.innerHTML = '';
    this.container.style.overflow = 'auto';
    this.container.style.background = '#e0e0e4';
    this.container.style.padding = '24px 0';

    // 页面容器 (居中)
    const pageWrapper = document.createElement('div');
    pageWrapper.style.display = 'flex';
    pageWrapper.style.justifyContent = 'center';
    pageWrapper.style.minHeight = '100%';

    // 页面 (白色 A4)
    this.pageEl = document.createElement('div');
    this.pageEl.className = 'zq-doc-page';
    this.pageEl.style.width = `${this.options.pageWidth}px`;
    this.pageEl.style.minHeight = `${this.options.pageHeight}px`;
    this.pageEl.style.background = '#ffffff';
    this.pageEl.style.boxShadow = '0 2px 8px rgba(0,0,0,0.15)';
    this.pageEl.style.padding = `${this.options.pagePadding}px`;
    this.pageEl.style.boxSizing = 'border-box';
    this.pageEl.style.transform = `scale(${this.options.zoom})`;
    this.pageEl.style.transformOrigin = 'top center';

    // 内容区
    this.contentEl = document.createElement('div');
    this.contentEl.className = 'zq-doc-content';
    this.contentEl.style.outline = 'none';
    this.pageEl.appendChild(this.contentEl);
    pageWrapper.appendChild(this.pageEl);
    this.container.appendChild(pageWrapper);
  }

  // ============================================================
  // 渲染
  // ============================================================

  /** 全量渲染 */
  render(): void {
    this.contentEl.innerHTML = '';
    const blocks = this.docModel.getBlocks();
    for (const block of blocks) {
      const el = this.renderBlock(block);
      this.contentEl.appendChild(el);
    }
  }

  /** 渲染单个块 */
  private renderBlock(block: Block): HTMLElement {
    let el: HTMLElement;
    switch (block.type) {
      case 'paragraph':
        el = this.renderParagraph(block);
        break;
      case 'heading':
        el = this.renderHeading(block);
        break;
      case 'table':
        el = this.renderTable(block);
        break;
      case 'image':
        el = this.renderImage(block);
        break;
      default:
        el = this.renderParagraph(block);
    }
    el.dataset.blockId = block.id;
    el.classList.add('zq-doc-block');
    return el;
  }

  /** 渲染段落 */
  private renderParagraph(block: Block): HTMLElement {
    const p = document.createElement('div');
    p.contentEditable = 'true';
    p.style.minHeight = '1.5em';
    p.style.lineHeight = '1.6';
    p.style.margin = '0 0 8px 0';
    p.style.outline = 'none';
    p.style.wordBreak = 'break-word';

    const para = block.paragraph!;
    p.style.textAlign = para.alignment;

    if (para.runs.length === 0) {
      // 空段落显示一个 br 以维持高度
      p.innerHTML = '<br>';
    } else {
      for (const run of para.runs) {
        const span = this.createRunSpan(run.text, run.font);
        p.appendChild(span);
      }
    }

    this.attachBlockEvents(p, block.id);
    return p;
  }

  /** 渲染标题 */
  private renderHeading(block: Block): HTMLElement {
    const h = document.createElement('div');
    h.contentEditable = 'true';
    h.style.outline = 'none';
    h.style.margin = '16px 0 8px 0';
    h.style.fontWeight = 'bold';
    h.style.lineHeight = '1.3';
    h.style.color = '#1a1a1a';

    const heading = block.heading!;
    const fontSize = HEADING_SIZES[heading.level] ?? 16;
    h.style.fontSize = `${fontSize}px`;
    h.textContent = heading.text;

    this.attachBlockEvents(h, block.id);
    return h;
  }

  /** 渲染表格 */
  private renderTable(block: Block): HTMLElement {
    const wrapper = document.createElement('div');
    wrapper.style.margin = '8px 0';
    wrapper.style.overflowX = 'auto';

    const table = block.table!;
    const tbl = document.createElement('table');
    tbl.style.borderCollapse = 'collapse';
    tbl.style.width = '100%';
    tbl.style.fontSize = '11pt';

    for (let r = 0; r < table.rows; r++) {
      const tr = document.createElement('tr');
      for (let c = 0; c < table.columns; c++) {
        const td = document.createElement('td');
        td.style.border = '1px solid #d0d0d0';
        td.style.padding = '6px 8px';
        td.style.verticalAlign = 'top';
        td.style.minWidth = '60px';
        td.contentEditable = 'true';
        td.style.outline = 'none';

        const cell = table.cells.find(cell => cell.row === r && cell.column === c);
        td.textContent = cell?.text ?? '';

        // 单元格事件
        td.addEventListener('input', () => {
          if (cell) cell.text = td.textContent ?? '';
        });

        tr.appendChild(td);
      }
      tbl.appendChild(tr);
    }

    wrapper.appendChild(tbl);
    this.attachBlockEvents(wrapper, block.id);
    return wrapper;
  }

  /** 渲染图片 */
  private renderImage(block: Block): HTMLElement {
    const wrapper = document.createElement('div');
    wrapper.style.margin = '8px 0';
    wrapper.style.textAlign = 'center';

    const img = block.image!;
    const imgEl = document.createElement('img');
    imgEl.src = img.path;
    // EMU → px
    imgEl.style.width = `${Math.round(img.width / 9525)}px`;
    imgEl.style.height = `${Math.round(img.height / 9525)}px`;
    imgEl.style.maxWidth = '100%';
    imgEl.style.height = 'auto';
    imgEl.style.objectFit = 'contain';

    wrapper.appendChild(imgEl);
    this.attachBlockEvents(wrapper, block.id);
    return wrapper;
  }

  /** 创建 run 的 span 元素 */
  private createRunSpan(text: string, font?: Font): HTMLSpanElement {
    const span = document.createElement('span');
    span.textContent = text;
    if (font) this.applyFontStyle(span, font);
    return span;
  }

  /** 应用字体样式 */
  applyFontStyle(el: HTMLElement, font: Font): void {
    if (font.family) el.style.fontFamily = font.family;
    if (font.size) el.style.fontSize = `${font.size}pt`;
    if (font.bold) el.style.fontWeight = 'bold';
    if (font.italic) el.style.fontStyle = 'italic';
    if (font.underline) el.style.textDecoration = 'underline';
    if (font.color) el.style.color = `#${font.color}`;
  }

  /** 给元素附加事件 */
  private attachBlockEvents(el: HTMLElement, blockId: string): void {
    el.addEventListener('click', (e) => {
      this.setSelection(blockId);
      this.callbacks.onBlockClick?.(blockId, e);
    });
    el.addEventListener('dblclick', (e) => {
      this.callbacks.onBlockDoubleClick?.(blockId, e);
    });
    el.addEventListener('input', () => {
      const text = el.textContent ?? '';
      this.callbacks.onBlockInput?.(blockId, text);
    });
    el.addEventListener('focus', () => {
      this.setSelection(blockId);
    });
  }

  // ============================================================
  // 选区管理
  // ============================================================

  /** 设置当前选中块 */
  setSelection(blockId: string | null): void {
    if (this.currentBlockId === blockId) return;
    // 清除旧选区样式
    if (this.currentBlockId) {
      const oldEl = this.getBlockEl(this.currentBlockId);
      if (oldEl) oldEl.classList.remove('zq-doc-block-selected');
    }
    this.currentBlockId = blockId;
    // 设置新选区样式
    if (blockId) {
      const el = this.getBlockEl(blockId);
      if (el) el.classList.add('zq-doc-block-selected');
    }
    this.callbacks.onSelectionChange?.(blockId);
  }

  /** 获取当前选中块 ID */
  getSelection(): string | null {
    return this.currentBlockId;
  }

  /** 获取块的 DOM 元素 */
  getBlockEl(blockId: string): HTMLElement | null {
    return this.contentEl.querySelector(`[data-block-id="${blockId}"]`) as HTMLElement | null;
  }

  /** 聚焦到块 */
  focusBlock(blockId: string): void {
    const el = this.getBlockEl(blockId);
    if (el) {
      el.focus();
      // 将光标定位到末尾
      const selection = window.getSelection();
      const range = document.createRange();
      range.selectNodeContents(el);
      range.collapse(false);
      selection?.removeAllRanges();
      selection?.addRange(range);
    }
  }

  // ============================================================
  // 增量渲染
  // ============================================================

  /** 刷新单个块 */
  refreshBlock(blockId: string): void {
    const block = this.docModel.getBlock(blockId);
    if (!block) return;
    const oldEl = this.getBlockEl(blockId);
    if (!oldEl) return;
    const newEl = this.renderBlock(block);
    oldEl.replaceWith(newEl);
  }

  /** 在指定块后插入新块的 DOM */
  insertBlockAfter(afterId: string, block: Block): void {
    const afterEl = this.getBlockEl(afterId);
    const newEl = this.renderBlock(block);
    if (afterEl) {
      afterEl.after(newEl);
    } else {
      this.contentEl.appendChild(newEl);
    }
  }

  /** 删除块的 DOM */
  removeBlockDom(blockId: string): void {
    const el = this.getBlockEl(blockId);
    if (el) el.remove();
  }

  // ============================================================
  // 选项更新
  // ============================================================

  /** 设置缩放 */
  setZoom(zoom: number): void {
    this.options.zoom = zoom;
    this.pageEl.style.transform = `scale(${zoom})`;
  }

  /** 获取当前缩放 */
  getZoom(): number {
    return this.options.zoom ?? 1.0;
  }

  // ============================================================
  // 刷新
  // ============================================================

  /** 刷新整个文档 */
  refresh(): void {
    this.render();
  }

  /** 设置新模型 */
  setModel(docModel: DocModel): void {
    this.docModel = docModel;
    this.currentBlockId = null;
    this.render();
  }

  /** 销毁 */
  destroy(): void {
    this.container.innerHTML = '';
    this.currentBlockId = null;
  }

  /** 获取内容元素 (供 EditEngine 附加键盘事件) */
  getContentEl(): HTMLElement {
    return this.contentEl;
  }
}
