/**
 * 演示文稿渲染引擎 — DOM 渲染 + 选区 + 缩略图
 *
 * 职责:
 *   1. 将幻灯片渲染为 DOM 元素 (编辑视图)
 *   2. 生成幻灯片缩略图
 *   3. 处理选区高亮和调整手柄
 *   4. 形状渲染 (文本框/自选图形/图片/连接线)
 */

import {
  SlideModel, Shape, Geometry, ShapeType, SlideSize,
  emuToPixel, pixelToEmu, szToPt, angleToDegree,
  PresetGeometry, Fill, Line, Paragraph, TextRun,
} from './slide-model';

/** 选区 */
export interface SlideSelection {
  shapeId: string | null;
}

/** 调整手柄类型 */
export type HandleType =
  | 'tl' | 'tr' | 'bl' | 'br'   // 四角
  | 't' | 'b' | 'l' | 'r'        // 四边
  | 'rot';                        // 旋转

/** 渲染引擎回调 */
export interface SlideRenderCallbacks {
  onShapeClick?: (shapeId: string, e: MouseEvent) => void;
  onShapeDoubleClick?: (shapeId: string, e: MouseEvent) => void;
  onBackgroundClick?: (e: MouseEvent) => void;
  onSelectionChange?: (selection: SlideSelection) => void;
  onContextMenu?: (shapeId: string | null, e: MouseEvent) => void;
}

/** 渲染缩放比例 */
export interface RenderScale {
  /** EMU → 显示像素的比例 */
  scale: number;
}

export class SlideRenderEngine {
  private container: HTMLElement;
  private slide: SlideModel;
  private slideSize: SlideSize;
  private callbacks: SlideRenderCallbacks;
  private selection: SlideSelection = { shapeId: null };

  // DOM 元素
  private slideEl!: HTMLDivElement;
  private shapeLayer!: HTMLDivElement;
  private selectionLayer!: HTMLDivElement;
  private shapeElements = new Map<string, HTMLElement>();

  // 渲染缩放
  private renderScale: number = 1;

  constructor(
    container: HTMLElement,
    slide: SlideModel,
    slideSize: SlideSize,
    callbacks: SlideRenderCallbacks = {},
  ) {
    this.container = container;
    this.slide = slide;
    this.slideSize = slideSize;
    this.callbacks = callbacks;

    this.setupStyles();
    this.createLayout();
    this.render();
  }

  // ============================================================
  // 样式和布局
  // ============================================================

  private setupStyles(): void {
    if (document.getElementById('zq-slide-render-styles')) return;
    const style = document.createElement('style');
    style.id = 'zq-slide-render-styles';
    style.textContent = `
      .zq-slide-stage {
        position: relative;
        background: #f0f0f0;
        overflow: auto;
        width: 100%;
        height: 100%;
        display: flex;
        align-items: center;
        justify-content: center;
        padding: 20px;
      }
      .zq-slide-canvas {
        position: relative;
        background: #fff;
        box-shadow: 0 2px 12px rgba(0,0,0,0.15);
        flex-shrink: 0;
      }
      .zq-slide-shape {
        position: absolute;
        cursor: move;
        user-select: none;
      }
      .zq-slide-shape.selected {
        outline: 2px solid #4787f5;
        outline-offset: 0;
      }
      .zq-slide-shape-locked {
        cursor: default;
      }
      .zq-slide-text {
        width: 100%;
        height: 100%;
        overflow: hidden;
        word-break: break-word;
        padding: 4px 8px;
        box-sizing: border-box;
      }
      .zq-slide-paragraph {
        margin: 0;
      }
      .zq-slide-selection-handle {
        position: absolute;
        width: 10px;
        height: 10px;
        background: #fff;
        border: 2px solid #4787f5;
        border-radius: 2px;
        z-index: 1000;
      }
      .zq-slide-selection-handle.tl { cursor: nwse-resize; }
      .zq-slide-selection-handle.tr { cursor: nesw-resize; }
      .zq-slide-selection-handle.bl { cursor: nesw-resize; }
      .zq-slide-selection-handle.br { cursor: nwse-resize; }
      .zq-slide-selection-handle.t { cursor: ns-resize; }
      .zq-slide-selection-handle.b { cursor: ns-resize; }
      .zq-slide-selection-handle.l { cursor: ew-resize; }
      .zq-slide-selection-handle.r { cursor: ew-resize; }
      .zq-slide-selection-handle.rot {
        cursor: crosshair;
        border-radius: 50%;
        background: #4787f5;
      }
      .zq-slide-thumbnail {
        position: relative;
        background: #fff;
        border: 2px solid transparent;
        cursor: pointer;
        box-shadow: 0 1px 4px rgba(0,0,0,0.1);
        flex-shrink: 0;
      }
      .zq-slide-thumbnail.active {
        border-color: #4787f5;
      }
      .zq-slide-thumbnail:hover {
        border-color: #a0c3f5;
      }
    `;
    document.head.appendChild(style);
  }

  private createLayout(): void {
    this.container.innerHTML = '';

    const stage = document.createElement('div');
    stage.className = 'zq-slide-stage';

    // 计算渲染缩放 (适配容器)
    this.renderScale = this.calculateScale();

    const canvasWidth = emuToPixel(this.slideSize.width) * this.renderScale;
    const canvasHeight = emuToPixel(this.slideSize.height) * this.renderScale;

    this.slideEl = document.createElement('div');
    this.slideEl.className = 'zq-slide-canvas';
    this.slideEl.style.width = `${canvasWidth}px`;
    this.slideEl.style.height = `${canvasHeight}px`;
    this.slideEl.style.position = 'relative';

    // 形状层
    this.shapeLayer = document.createElement('div');
    this.shapeLayer.style.position = 'absolute';
    this.shapeLayer.style.top = '0';
    this.shapeLayer.style.left = '0';
    this.shapeLayer.style.width = '100%';
    this.shapeLayer.style.height = '100%';

    // 选区层
    this.selectionLayer = document.createElement('div');
    this.selectionLayer.style.position = 'absolute';
    this.selectionLayer.style.top = '0';
    this.selectionLayer.style.left = '0';
    this.selectionLayer.style.width = '100%';
    this.selectionLayer.style.height = '100%';
    this.selectionLayer.style.pointerEvents = 'none';

    this.slideEl.appendChild(this.shapeLayer);
    this.slideEl.appendChild(this.selectionLayer);

    // 点击空白处取消选中
    this.slideEl.addEventListener('mousedown', (e) => {
      if (e.target === this.slideEl || e.target === this.shapeLayer) {
        this.setSelection(null);
        this.callbacks.onBackgroundClick?.(e);
      }
    });

    stage.appendChild(this.slideEl);
    this.container.appendChild(stage);
  }

  /** 计算渲染缩放比例 */
  private calculateScale(): number {
    const containerWidth = this.container.clientWidth - 40;  // padding
    const containerHeight = this.container.clientHeight - 40;
    const slideWidthPx = emuToPixel(this.slideSize.width);
    const slideHeightPx = emuToPixel(this.slideSize.height);
    const scaleX = containerWidth / slideWidthPx;
    const scaleY = containerHeight / slideHeightPx;
    return Math.min(scaleX, scaleY, 1);  // 最大 1:1
  }

  // ============================================================
  // 渲染
  // ============================================================

  /** 渲染所有形状 */
  render(): void {
    this.shapeLayer.innerHTML = '';
    this.shapeElements.clear();

    const shapes = this.slide.getShapes();
    for (const shape of shapes) {
      const el = this.renderShape(shape);
      this.shapeLayer.appendChild(el);
      this.shapeElements.set(shape.id, el);
    }

    this.renderSelection();
  }

  /** 渲染单个形状 */
  private renderShape(shape: Shape): HTMLElement {
    const el = document.createElement('div');
    el.className = 'zq-slide-shape';
    el.dataset.shapeId = shape.id;

    // 位置和尺寸
    const geo = shape.geometry;
    el.style.left = `${emuToPixel(geo.x) * this.renderScale}px`;
    el.style.top = `${emuToPixel(geo.y) * this.renderScale}px`;
    el.style.width = `${emuToPixel(geo.width) * this.renderScale}px`;
    el.style.height = `${emuToPixel(geo.height) * this.renderScale}px`;

    // 旋转
    if (shape.rotation && shape.rotation !== 0) {
      el.style.transform = `rotate(${angleToDegree(shape.rotation)}deg)`;
      el.style.transformOrigin = 'center center';
    }

    // 填充
    if (shape.fill) {
      this.applyFill(el, shape.fill);
    }

    // 线条
    if (shape.line) {
      this.applyLine(el, shape.line);
    }

    // 按类型渲染内容
    switch (shape.type) {
      case 'text':
        if (shape.text) {
          el.appendChild(this.renderText(shape.text));
        }
        break;
      case 'autoshape':
        if (shape.prstGeom) {
          this.applyPresetGeometry(el, shape.prstGeom);
        }
        if (shape.text) {
          el.appendChild(this.renderText(shape.text));
        }
        break;
      case 'image':
        if (shape.image) {
          this.applyImage(el, shape.image.path);
        }
        break;
      case 'connector':
        if (shape.prstGeom === 'line') {
          this.applyLineStyle(el);
        }
        break;
      case 'chart':
        el.style.background = '#f5f5f5';
        el.style.border = '1px solid #ddd';
        el.style.display = 'flex';
        el.style.alignItems = 'center';
        el.style.justifyContent = 'center';
        el.textContent = '图表';
        break;
    }

    // 选中状态
    if (this.selection.shapeId === shape.id) {
      el.classList.add('selected');
    }

    // 事件
    el.addEventListener('mousedown', (e) => {
      e.stopPropagation();
      this.setSelection(shape.id);
      this.callbacks.onShapeClick?.(shape.id, e);
    });
    el.addEventListener('dblclick', (e) => {
      e.stopPropagation();
      this.callbacks.onShapeDoubleClick?.(shape.id, e);
    });
    el.addEventListener('contextmenu', (e) => {
      e.preventDefault();
      e.stopPropagation();
      this.setSelection(shape.id);
      this.callbacks.onContextMenu?.(shape.id, e);
    });

    return el;
  }

  /** 渲染文本内容 */
  private renderText(text: { paragraphs: Paragraph[] }): HTMLElement {
    const container = document.createElement('div');
    container.className = 'zq-slide-text';

    for (const para of text.paragraphs) {
      const p = document.createElement('p');
      p.className = 'zq-slide-paragraph';
      p.style.margin = '0';
      p.style.textAlign = para.alignment ?? 'left';

      if (para.lineSpacing) {
        p.style.lineHeight = `${para.lineSpacing}`;
      }
      if (para.spaceBefore) {
        p.style.marginTop = `${para.spaceBefore}px`;
      }
      if (para.spaceAfter) {
        p.style.marginBottom = `${para.spaceAfter}px`;
      }

      for (const run of para.runs) {
        const span = document.createElement('span');
        span.textContent = run.text;
        if (run.font) {
          this.applyFont(span, run.font);
        }
        p.appendChild(span);
      }

      container.appendChild(p);
    }

    return container;
  }

  /** 应用字体样式 */
  private applyFont(el: HTMLElement, font: { bold?: boolean; italic?: boolean; underline?: boolean; strikeThrough?: boolean; size?: number; family?: string; color?: string }): void {
    if (font.bold) el.style.fontWeight = 'bold';
    if (font.italic) el.style.fontStyle = 'italic';
    if (font.underline) el.style.textDecoration = 'underline';
    if (font.strikeThrough) {
      el.style.textDecoration = el.style.textDecoration === 'underline' ? 'underline line-through' : 'line-through';
    }
    if (font.size) el.style.fontSize = `${szToPt(font.size) * this.renderScale}pt`;
    if (font.family) el.style.fontFamily = font.family;
    if (font.color) el.style.color = `#${font.color}`;
  }

  /** 应用填充 */
  private applyFill(el: HTMLElement, fill: Fill): void {
    if (fill.type === 'solid' && fill.color) {
      el.style.background = `#${fill.color}`;
    } else if (fill.type === 'gradient' && fill.gradient) {
      const stops = fill.gradient.stops.map(s => `#${s.color} ${s.position * 100}%`).join(', ');
      el.style.background = `linear-gradient(${fill.gradient.angle}deg, ${stops})`;
    } else {
      el.style.background = 'transparent';
    }
  }

  /** 应用线条 */
  private applyLine(el: HTMLElement, line: Line): void {
    if (line.noFill) {
      el.style.border = 'none';
    } else if (line.color) {
      const widthPx = line.width ? emuToPixel(line.width) : 1;
      el.style.border = `${widthPx * this.renderScale}px solid #${line.color}`;
    }
  }

  /** 应用预设几何形状 (使用 CSS clip-path 或 border-radius) */
  private applyPresetGeometry(el: HTMLElement, prst: PresetGeometry): void {
    switch (prst) {
      case 'rect':
        // 默认就是矩形
        break;
      case 'roundRect':
        el.style.borderRadius = '8px';
        break;
      case 'ellipse':
        el.style.borderRadius = '50%';
        break;
      case 'triangle':
        el.style.clipPath = 'polygon(50% 0%, 0% 100%, 100% 100%)';
        break;
      case 'diamond':
        el.style.clipPath = 'polygon(50% 0%, 100% 50%, 50% 100%, 0% 50%)';
        break;
      case 'parallelogram':
        el.style.clipPath = 'polygon(20% 0%, 100% 0%, 80% 100%, 0% 100%)';
        break;
      case 'trapezoid':
        el.style.clipPath = 'polygon(20% 0%, 80% 0%, 100% 100%, 0% 100%)';
        break;
      case 'pentagon':
        el.style.clipPath = 'polygon(50% 0%, 100% 38%, 82% 100%, 18% 100%, 0% 38%)';
        break;
      case 'hexagon':
        el.style.clipPath = 'polygon(25% 0%, 75% 0%, 100% 50%, 75% 100%, 25% 100%, 0% 50%)';
        break;
      case 'star5':
        el.style.clipPath = 'polygon(50% 0%, 61% 35%, 98% 35%, 68% 57%, 79% 91%, 50% 70%, 21% 91%, 32% 57%, 2% 35%, 39% 35%)';
        break;
      case 'arrowRight':
        el.style.clipPath = 'polygon(0% 20%, 60% 20%, 60% 0%, 100% 50%, 60% 100%, 60% 80%, 0% 80%)';
        break;
      case 'arrowLeft':
        el.style.clipPath = 'polygon(100% 20%, 40% 20%, 40% 0%, 0% 50%, 40% 100%, 40% 80%, 100% 80%)';
        break;
      case 'arrowUp':
        el.style.clipPath = 'polygon(20% 100%, 20% 40%, 0% 40%, 50% 0%, 100% 40%, 80% 40%, 80% 100%)';
        break;
      case 'arrowDown':
        el.style.clipPath = 'polygon(20% 0%, 20% 60%, 0% 60%, 50% 100%, 100% 60%, 80% 60%, 80% 0%)';
        break;
      default:
        // 其他形状保持矩形
        break;
    }
  }

  /** 应用图片 */
  private applyImage(el: HTMLElement, path: string): void {
    el.style.background = `url(${path}) center/cover no-repeat`;
    el.style.backgroundSize = '100% 100%';
  }

  /** 应用直线样式 */
  private applyLineStyle(el: HTMLElement): void {
    el.style.background = 'transparent';
    el.style.border = 'none';
    el.style.height = '0';
  }

  // ============================================================
  // 选区
  // ============================================================

  /** 设置选中形状 */
  setSelection(shapeId: string | null): void {
    this.selection.shapeId = shapeId;
    this.render();
    this.callbacks.onSelectionChange?.(this.selection);
  }

  /** 获取当前选区 */
  getSelection(): SlideSelection {
    return { ...this.selection };
  }

  /** 渲染选区手柄 */
  private renderSelection(): void {
    this.selectionLayer.innerHTML = '';
    if (!this.selection.shapeId) return;

    const el = this.shapeElements.get(this.selection.shapeId);
    if (!el) return;

    const rect = el.getBoundingClientRect();
    const layerRect = this.selectionLayer.getBoundingClientRect();

    // 创建 8 个调整手柄
    const handles: HandleType[] = ['tl', 'tr', 'bl', 'br', 't', 'b', 'l', 'r', 'rot'];
    for (const type of handles) {
      const handle = document.createElement('div');
      handle.className = `zq-slide-selection-handle ${type}`;
      handle.style.pointerEvents = 'auto';

      const x = rect.left - layerRect.left;
      const y = rect.top - layerRect.top;
      const w = rect.width;
      const h = rect.height;
      const hs = 5;  // 手柄半宽

      switch (type) {
        case 'tl': handle.style.left = `${x - hs}px`; handle.style.top = `${y - hs}px`; break;
        case 'tr': handle.style.left = `${x + w - hs}px`; handle.style.top = `${y - hs}px`; break;
        case 'bl': handle.style.left = `${x - hs}px`; handle.style.top = `${y + h - hs}px`; break;
        case 'br': handle.style.left = `${x + w - hs}px`; handle.style.top = `${y + h - hs}px`; break;
        case 't': handle.style.left = `${x + w / 2 - hs}px`; handle.style.top = `${y - hs}px`; break;
        case 'b': handle.style.left = `${x + w / 2 - hs}px`; handle.style.top = `${y + h - hs}px`; break;
        case 'l': handle.style.left = `${x - hs}px`; handle.style.top = `${y + h / 2 - hs}px`; break;
        case 'r': handle.style.left = `${x + w - hs}px`; handle.style.top = `${y + h / 2 - hs}px`; break;
        case 'rot': handle.style.left = `${x + w / 2 - hs}px`; handle.style.top = `${y - 25}px`; break;
      }

      this.selectionLayer.appendChild(handle);
    }
  }

  // ============================================================
  // 缩略图
  // ============================================================

  /** 生成缩略图 DOM */
  renderThumbnail(maxWidth: number = 160): HTMLElement {
    const thumb = document.createElement('div');
    thumb.className = 'zq-slide-thumbnail';

    const aspectRatio = this.slideSize.width / this.slideSize.height;
    const thumbWidth = maxWidth;
    const thumbHeight = Math.round(maxWidth / aspectRatio);

    thumb.style.width = `${thumbWidth}px`;
    thumb.style.height = `${thumbHeight}px`;
    thumb.style.position = 'relative';
    thumb.style.overflow = 'hidden';

    const scale = thumbWidth / emuToPixel(this.slideSize.width);

    const shapes = this.slide.getShapes();
    for (const shape of shapes) {
      const el = this.renderShapeForThumbnail(shape, scale);
      thumb.appendChild(el);
    }

    return thumb;
  }

  /** 缩略图中的形状渲染 (无交互) */
  private renderShapeForThumbnail(shape: Shape, scale: number): HTMLElement {
    const el = document.createElement('div');
    el.style.position = 'absolute';

    const geo = shape.geometry;
    el.style.left = `${emuToPixel(geo.x) * scale}px`;
    el.style.top = `${emuToPixel(geo.y) * scale}px`;
    el.style.width = `${emuToPixel(geo.width) * scale}px`;
    el.style.height = `${emuToPixel(geo.height) * scale}px`;

    if (shape.fill && shape.fill.type === 'solid' && shape.fill.color) {
      el.style.background = `#${shape.fill.color}`;
    }
    if (shape.line && !shape.line.noFill && shape.line.color) {
      const w = shape.line.width ? emuToPixel(shape.line.width) * scale : 1;
      el.style.border = `${w}px solid #${shape.line.color}`;
    }
    if (shape.type === 'autoshape' && shape.prstGeom) {
      this.applyPresetGeometry(el, shape.prstGeom);
    }
    if (shape.type === 'text' && shape.text) {
      // 缩略图中只显示纯文本
      const texts = shape.text.paragraphs
        .map(p => p.runs.map(r => r.text).join(''))
        .join(' ');
      el.textContent = texts;
      el.style.fontSize = `${10 * scale}px`;
      el.style.padding = '2px';
      el.style.overflow = 'hidden';
      el.style.whiteSpace = 'nowrap';
      el.style.textOverflow = 'ellipsis';
    }

    return el;
  }

  // ============================================================
  // 公开 API
  // ============================================================

  /** 更新幻灯片引用 */
  setSlide(slide: SlideModel): void {
    this.slide = slide;
    this.render();
  }

  /** 更新幻灯片尺寸 */
  setSlideSize(size: SlideSize): void {
    this.slideSize = size;
    this.createLayout();
    this.render();
  }

  /** 获取幻灯片尺寸 */
  getSlideSize(): SlideSize {
    return this.slideSize;
  }

  /** 刷新渲染 */
  refresh(): void {
    this.render();
  }

  /** 获取渲染缩放 */
  getRenderScale(): number {
    return this.renderScale;
  }

  /** EMU 转显示像素 */
  emuToDisplayPixel(emu: number): number {
    return emuToPixel(emu) * this.renderScale;
  }

  /** 显示像素转 EMU */
  displayPixelToEmu(px: number): number {
    return pixelToEmu(px / this.renderScale);
  }

  /** 销毁 */
  destroy(): void {
    this.container.innerHTML = '';
    this.shapeElements.clear();
  }
}
