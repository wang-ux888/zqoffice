/**
 * 演示文稿工具栏 — 形状添加 + 格式设置 + 幻灯片操作
 */

import { SlideModel, Shape, ShapeFactory, pixelToEmu, PresetGeometry } from './slide-model';
import { SlideRenderEngine } from './slide-render-engine';
import { SlideEditEngine } from './slide-edit-engine';

export class SlideToolbar {
  private container: HTMLElement;
  private slide: SlideModel;
  private renderEngine: SlideRenderEngine;
  private editEngine: SlideEditEngine;

  constructor(
    container: HTMLElement,
    slide: SlideModel,
    renderEngine: SlideRenderEngine,
    editEngine: SlideEditEngine,
  ) {
    this.container = container;
    this.slide = slide;
    this.renderEngine = renderEngine;
    this.editEngine = editEngine;

    this.setupStyles();
    this.render();
  }

  private setupStyles(): void {
    if (document.getElementById('zq-slide-toolbar-styles')) return;
    const style = document.createElement('style');
    style.id = 'zq-slide-toolbar-styles';
    style.textContent = `
      .zq-slide-toolbar {
        display: flex;
        align-items: center;
        gap: 4px;
        padding: 4px 8px;
        background: #fafafa;
        border-bottom: 1px solid #e0e0e0;
        flex-wrap: wrap;
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
      }
      .zq-slide-toolbar-group {
        display: flex;
        align-items: center;
        gap: 2px;
        padding: 0 4px;
        border-right: 1px solid #e0e0e0;
      }
      .zq-slide-toolbar-group:last-child {
        border-right: none;
      }
      .zq-slide-btn {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        min-width: 28px;
        height: 28px;
        padding: 0 6px;
        border: 1px solid transparent;
        border-radius: 3px;
        background: transparent;
        cursor: pointer;
        font-size: 13px;
        color: #333;
        transition: all 0.15s;
      }
      .zq-slide-btn:hover {
        background: #e8e8e8;
        border-color: #d0d0d0;
      }
      .zq-slide-btn:active {
        background: #d0d0d0;
      }
      .zq-slide-btn.active {
        background: #4787f5;
        color: #fff;
        border-color: #4787f5;
      }
      .zq-slide-btn:disabled {
        opacity: 0.4;
        cursor: not-allowed;
      }
      .zq-slide-btn-icon {
        font-size: 14px;
      }
      .zq-slide-color-input {
        width: 28px;
        height: 28px;
        padding: 0;
        border: 1px solid #d0d0d0;
        border-radius: 3px;
        cursor: pointer;
        background: transparent;
      }
      .zq-slide-select {
        height: 28px;
        padding: 0 4px;
        border: 1px solid #d0d0d0;
        border-radius: 3px;
        font-size: 13px;
        background: #fff;
      }
      .zq-slide-separator {
        width: 1px;
        height: 20px;
        background: #d0d0d0;
        margin: 0 2px;
      }
    `;
    document.head.appendChild(style);
  }

  private render(): void {
    this.container.innerHTML = '';
    this.container.className = 'zq-slide-toolbar';

    // 撤销/重做
    const undoRedoGroup = this.createGroup();
    undoRedoGroup.appendChild(this.createButton('↶', '撤销', () => this.editEngine.undo()));
    undoRedoGroup.appendChild(this.createButton('↷', '重做', () => this.editEngine.redo()));
    this.container.appendChild(undoRedoGroup);

    // 添加形状
    const shapeGroup = this.createGroup();
    shapeGroup.appendChild(this.createButton('T', '文本框', () => this.addTextBox()));
    shapeGroup.appendChild(this.createButton('▭', '矩形', () => this.addShape('rect')));
    shapeGroup.appendChild(this.createButton('▢', '圆角矩形', () => this.addShape('roundRect')));
    shapeGroup.appendChild(this.createButton('○', '椭圆', () => this.addShape('ellipse')));
    shapeGroup.appendChild(this.createButton('△', '三角形', () => this.addShape('triangle')));
    shapeGroup.appendChild(this.createButton('◇', '菱形', () => this.addShape('diamond')));
    shapeGroup.appendChild(this.createButton('→', '右箭头', () => this.addShape('arrowRight')));
    shapeGroup.appendChild(this.createButton('←', '左箭头', () => this.addShape('arrowLeft')));
    shapeGroup.appendChild(this.createButton('↑', '上箭头', () => this.addShape('arrowUp')));
    shapeGroup.appendChild(this.createButton('↓', '下箭头', () => this.addShape('arrowDown')));
    shapeGroup.appendChild(this.createButton('★', '星形', () => this.addShape('star5')));
    shapeGroup.appendChild(this.createButton('—', '直线', () => this.addLine()));
    this.container.appendChild(shapeGroup);

    // 格式设置
    const formatGroup = this.createGroup();
    formatGroup.appendChild(this.createColorInput('填充色', '#4787F5', (color) => this.setFillColor(color)));
    formatGroup.appendChild(this.createColorInput('线条色', '#333333', (color) => this.setLineColor(color)));
    formatGroup.appendChild(this.createColorInput('文字色', '#333333', (color) => this.setFontColor(color)));
    this.container.appendChild(formatGroup);

    // 层级
    const orderGroup = this.createGroup();
    orderGroup.appendChild(this.createButton('⬆', '移到顶层', () => this.editEngine.bringToFront()));
    orderGroup.appendChild(this.createButton('⬇', '移到底层', () => this.editEngine.sendToBack()));
    this.container.appendChild(orderGroup);

    // 编辑操作
    const editGroup = this.createGroup();
    editGroup.appendChild(this.createButton('复制', '复制 (Ctrl+D)', () => this.editEngine.duplicateSelected()));
    editGroup.appendChild(this.createButton('删除', '删除 (Del)', () => this.editEngine.deleteSelected()));
    this.container.appendChild(editGroup);
  }

  private createGroup(): HTMLDivElement {
    const group = document.createElement('div');
    group.className = 'zq-slide-toolbar-group';
    return group;
  }

  private createButton(text: string, title: string, onClick: () => void): HTMLButtonElement {
    const btn = document.createElement('button');
    btn.className = 'zq-slide-btn';
    btn.textContent = text;
    btn.title = title;
    btn.addEventListener('click', onClick);
    return btn;
  }

  private createColorInput(title: string, defaultColor: string, onChange: (color: string) => void): HTMLInputElement {
    const input = document.createElement('input');
    input.type = 'color';
    input.className = 'zq-slide-color-input';
    input.title = title;
    input.value = `#${defaultColor}`;
    input.addEventListener('change', () => {
      const color = input.value.replace('#', '').toUpperCase();
      onChange(color);
    });
    return input;
  }

  // ============================================================
  // 形状添加
  // ============================================================

  /** 添加文本框到幻灯片中央 */
  private addTextBox(): void {
    const center = this.getSlideCenter();
    const width = pixelToEmu(200);
    const height = pixelToEmu(60);
    const shape = ShapeFactory.createTextBox(
      center.x - width / 2,
      center.y - height / 2,
      width, height,
      '双击编辑文本',
    );
    this.editEngine.addShape(shape);
  }

  /** 添加自选图形 */
  private addShape(prst: PresetGeometry): void {
    const center = this.getSlideCenter();
    const width = pixelToEmu(150);
    const height = pixelToEmu(100);
    const shape = ShapeFactory.createAutoShape(
      prst,
      center.x - width / 2,
      center.y - height / 2,
      width, height,
    );
    this.editEngine.addShape(shape);
  }

  /** 添加直线 */
  private addLine(): void {
    const center = this.getSlideCenter();
    const width = pixelToEmu(200);
    const shape = ShapeFactory.createLine(
      center.x - width / 2, center.y,
      width, 0,
    );
    this.editEngine.addShape(shape);
  }

  /** 获取幻灯片中心点 (EMU) */
  private getSlideCenter(): { x: number; y: number } {
    const size = this.renderEngine.getSlideSize();
    const width = size?.width ?? 9144000;
    const height = size?.height ?? 6858000;
    return { x: width / 2, y: height / 2 };
  }

  // ============================================================
  // 格式设置
  // ============================================================

  /** 设置选中形状的填充色 */
  private setFillColor(color: string): void {
    const sel = this.renderEngine.getSelection();
    if (!sel.shapeId) return;
    this.slide.setFill(sel.shapeId, { type: 'solid', color });
    this.renderEngine.refresh();
  }

  /** 设置选中形状的线条色 */
  private setLineColor(color: string): void {
    const sel = this.renderEngine.getSelection();
    if (!sel.shapeId) return;
    this.slide.setLine(sel.shapeId, { color, width: pixelToEmu(1) });
    this.renderEngine.refresh();
  }

  /** 设置选中形状的文字颜色 */
  private setFontColor(color: string): void {
    const sel = this.renderEngine.getSelection();
    if (!sel.shapeId) return;
    const shape = this.slide.getShape(sel.shapeId);
    if (!shape?.text) return;
    for (const para of shape.text.paragraphs) {
      for (const run of para.runs) {
        if (!run.font) run.font = {};
        run.font.color = color;
      }
    }
    this.renderEngine.refresh();
  }

  // ============================================================
  // 公开 API
  // ============================================================

  /** 更新幻灯片引用 */
  setSlide(slide: SlideModel): void {
    this.slide = slide;
  }

  /** 刷新按钮状态 */
  updateButtonStates(): void {
    // 可扩展: 根据选区更新按钮状态
  }
}
