/**
 * 演示文稿数据模型 — ZQ Slide JSON 操作(CRUD)
 *
 * 单位说明:
 *   - geometry/位置/尺寸: EMU (English Metric Unit, 1 inch = 914400 EMU)
 *   - font.size: 1/100 pt (OOXML sz 约定, 如 3200 = 32pt)
 *   - rotation: 1/60000 度 (OOXML 约定)
 *   - color: RRGGBB 字符串 (如 "FF0000")
 */

// ============================================================

/** 形状类型枚举 */
export type ShapeType = 'text' | 'image' | 'chart' | 'group' | 'autoshape' | 'connector';

/** 几何位置 (EMU 单位) */
export interface Geometry {
  x: number;
  y: number;
  width: number;
  height: number;
}

/** 字体样式 */
export interface SlideFont {
  bold?: boolean;
  italic?: boolean;
  underline?: boolean;
  strikeThrough?: boolean;
  size?: number;       // 1/100 pt (OOXML sz, 如 3200 = 32pt)
  family?: string;     // 字体族 (如 "Calibri")
  color?: string;      // RRGGBB (如 "FF0000")
}

/** 文本运行 (Run) */
export interface TextRun {
  text: string;
  font?: SlideFont;
}

/** 段落对齐方式 */
export type ParagraphAlignment = 'left' | 'center' | 'right' | 'justify';

/** 段落 */
export interface Paragraph {
  alignment?: ParagraphAlignment;
  runs: TextRun[];
  /** 行间距 (pt) */
  lineSpacing?: number;
  /** 段前间距 (pt) */
  spaceBefore?: number;
  /** 段后间距 (pt) */
  spaceAfter?: number;
}

/** 文本内容 */
export interface TextContent {
  paragraphs: Paragraph[];
}

/** 填充类型 */
export interface Fill {
  type: 'solid' | 'none' | 'gradient';
  color?: string;        // type=solid 时的颜色 RRGGBB
  gradient?: {
    angle: number;
    stops: Array<{ position: number; color: string }>;
  };
}

/** 线条/边框 */
export interface Line {
  noFill?: boolean;
  color?: string;
  width?: number;        // EMU
}

/** 图片信息 */
export interface ImageInfo {
  path: string;          // 如 "media/rId1"
  width: number;         // EMU
  height: number;        // EMU
}

/** 预设几何形状名 (autoshape) */
export type PresetGeometry =
  | 'rect' | 'roundRect' | 'ellipse' | 'triangle'
  | 'diamond' | 'parallelogram' | 'trapezoid'
  | 'pentagon' | 'hexagon' | 'octagon' | 'star5'
  | 'arrowRight' | 'arrowLeft' | 'arrowUp' | 'arrowDown'
  | 'line' | 'bracePair' | 'callout1' | 'cloud';

/** 形状 */
export interface Shape {
  id: string;
  name: string;
  type: ShapeType;
  geometry: Geometry;
  rotation?: number;           // 1/60000 度
  text?: TextContent;          // type=text/autoshape 时存在
  fill?: Fill;
  line?: Line;
  image?: ImageInfo;           // type=image 时存在
  prstGeom?: PresetGeometry;   // type=autoshape 时的预设几何
  /** 子形状 (type=group 时存在) */
  children?: Shape[];
  /** z-index 层叠顺序 */
  zIndex?: number;
  /** 是否锁定 */
  locked?: boolean;
}

/** 幻灯片版式名 */
export type SlideLayout =
  | 'blank' | 'title' | 'titleContent' | 'sectionHeader'
  | 'twoContent' | 'comparison' | 'titleOnly' | 'content'
  | 'pictureWithCaption' | '';

/** 幻灯片 */
export interface Slide {
  slideId: string;
  index: number;
  layout: SlideLayout;
  shapes: Shape[];
  notes?: string;
  /** 背景色 RRGGBB */
  backgroundColor?: string;
  /** 是否隐藏 */
  hidden?: boolean;
  /** 过渡动画 */
  transition?: SlideTransition;
}

/** 幻灯片过渡动画 */
export interface SlideTransition {
  type: 'none' | 'fade' | 'push' | 'wipe' | 'split' | 'zoom';
  duration: number;       // 毫秒
  direction?: 'left' | 'right' | 'up' | 'down';
}

/** 幻灯片尺寸 */
export interface SlideSize {
  width: number;          // EMU
  height: number;         // EMU
}

/** 文档核心属性 */
export interface SlideCoreProperties {
  title?: string;
  creator?: string;
  created?: string;
  modified?: string;
}

/** ZQ Slide 文档 (顶层) */
export interface ZQSlideDocument {
  slides: Slide[];
  slideSize: SlideSize;
  coreProperties?: SlideCoreProperties;
}

// ============================================================
// 常量
// ============================================================

/** EMU 转像素 (96 DPI) */
export const EMU_PER_PIXEL = 9525;  // 914400 / 96

/** 像素转 EMU */
export function pixelToEmu(px: number): number {
  return Math.round(px * EMU_PER_PIXEL);
}

/** EMU 转像素 */
export function emuToPixel(emu: number): number {
  return Math.round(emu / EMU_PER_PIXEL);
}

/** EMU 转英寸 */
export function emuToInch(emu: number): number {
  return emu / 914400;
}

/** 英寸转 EMU */
export function inchToEmu(inch: number): number {
  return Math.round(inch * 914400);
}

/** 1/100 pt 转 pt */
export function szToPt(sz: number): number {
  return sz / 100;
}

/** pt 转 1/100 pt */
export function ptToSz(pt: number): number {
  return Math.round(pt * 100);
}

/** 1/60000 度 转 度 */
export function angleToDegree(angle: number): number {
  return angle / 60000;
}

/** 度 转 1/60000 度 */
export function degreeToAngle(degree: number): number {
  return Math.round(degree * 60000);
}

/** 默认幻灯片尺寸 (16:9 宽屏) */
export const DEFAULT_SLIDE_SIZE: SlideSize = {
  width: 12192000,  // 13.333 inch
  height: 6858000,  // 7.5 inch
};

/** 默认幻灯片尺寸 (4:3) */
export const SLIDE_SIZE_4_3: SlideSize = {
  width: 9144000,
  height: 6858000,
};

/** 默认幻灯片尺寸 (16:9 宽屏) */
export const SLIDE_SIZE_16_9: SlideSize = {
  width: 12192000,  // 13.333 inch
  height: 6858000,  // 7.5 inch
};

// ============================================================
// 幻灯片模型
// ============================================================

/** 幻灯片模型 — 单张幻灯片的数据操作 */
export class SlideModel {
  slideId: string;
  index: number;
  layout: SlideLayout;
  private shapes: Shape[] = [];
  notes: string;
  backgroundColor: string | null;
  hidden: boolean;
  transition: SlideTransition | null;

  constructor(data?: Partial<Slide>) {
    this.slideId = data?.slideId ?? `slide_${Date.now()}`;
    this.index = data?.index ?? 0;
    this.layout = data?.layout ?? 'blank';
    this.notes = data?.notes ?? '';
    this.backgroundColor = data?.backgroundColor ?? null;
    this.hidden = data?.hidden ?? false;
    this.transition = data?.transition ?? null;

    if (data?.shapes) {
      this.shapes = data.shapes.map(s => ({ ...s }));
    }
  }

  // ---------- 形状 CRUD ----------

  /** 获取所有形状 */
  getShapes(): Shape[] {
    return [...this.shapes];
  }

  /** 按 ID 获取形状 */
  getShape(id: string): Shape | null {
    return this.shapes.find(s => s.id === id) ?? null;
  }

  /** 添加形状 */
  addShape(shape: Shape): void {
    if (!shape.zIndex) {
      shape.zIndex = this.shapes.length;
    }
    this.shapes.push(shape);
  }

  /** 插入形状到指定位置 */
  insertShape(index: number, shape: Shape): void {
    this.shapes.splice(index, 0, shape);
  }

  /** 移除形状 */
  removeShape(id: string): boolean {
    const idx = this.shapes.findIndex(s => s.id === id);
    if (idx === -1) return false;
    this.shapes.splice(idx, 1);
    return true;
  }

  /** 更新形状 */
  updateShape(id: string, updates: Partial<Shape>): boolean {
    const shape = this.getShape(id);
    if (!shape) return false;
    Object.assign(shape, updates);
    return true;
  }

  /** 移动形状到顶层 */
  bringToFront(id: string): void {
    const idx = this.shapes.findIndex(s => s.id === id);
    if (idx === -1) return;
    const [shape] = this.shapes.splice(idx, 1);
    this.shapes.push(shape);
    this.reindexZOrder();
  }

  /** 移动形状到底层 */
  sendToBack(id: string): void {
    const idx = this.shapes.findIndex(s => s.id === id);
    if (idx === -1) return;
    const [shape] = this.shapes.splice(idx, 1);
    this.shapes.unshift(shape);
    this.reindexZOrder();
  }

  /** 重新计算 z-index */
  private reindexZOrder(): void {
    this.shapes.forEach((s, i) => { s.zIndex = i; });
  }

  /** 获取形状数量 */
  getShapeCount(): number {
    return this.shapes.length;
  }

  // ---------- 文本操作 ----------

  /** 设置形状文本 */
  setShapeText(id: string, text: string): boolean {
    const shape = this.getShape(id);
    if (!shape) return false;
    if (!shape.text) {
      shape.text = { paragraphs: [] };
    }
    // 简单设置: 清空并添加一个段落
    shape.text.paragraphs = [{ runs: [{ text }] }];
    return true;
  }

  /** 添加段落 */
  addParagraph(id: string, paragraph: Paragraph): boolean {
    const shape = this.getShape(id);
    if (!shape || !shape.text) return false;
    shape.text.paragraphs.push(paragraph);
    return true;
  }

  /** 更新段落 */
  updateParagraph(id: string, index: number, updates: Partial<Paragraph>): boolean {
    const shape = this.getShape(id);
    if (!shape || !shape.text || !shape.text.paragraphs[index]) return false;
    Object.assign(shape.text.paragraphs[index], updates);
    return true;
  }

  /** 移除段落 */
  removeParagraph(id: string, index: number): boolean {
    const shape = this.getShape(id);
    if (!shape || !shape.text || !shape.text.paragraphs[index]) return false;
    shape.text.paragraphs.splice(index, 1);
    return true;
  }

  // ---------- 几何操作 ----------

  /** 移动形状 */
  moveShape(id: string, dx: number, dy: number): boolean {
    const shape = this.getShape(id);
    if (!shape) return false;
    shape.geometry.x += dx;
    shape.geometry.y += dy;
    return true;
  }

  /** 调整形状大小 */
  resizeShape(id: string, width: number, height: number): boolean {
    const shape = this.getShape(id);
    if (!shape) return false;
    shape.geometry.width = Math.max(1, width);
    shape.geometry.height = Math.max(1, height);
    return true;
  }

  /** 设置形状位置 */
  setShapePosition(id: string, x: number, y: number): boolean {
    const shape = this.getShape(id);
    if (!shape) return false;
    shape.geometry.x = x;
    shape.geometry.y = y;
    return true;
  }

  /** 旋转形状 */
  setShapeRotation(id: string, rotation: number): boolean {
    const shape = this.getShape(id);
    if (!shape) return false;
    shape.rotation = rotation;
    return true;
  }

  // ---------- 样式操作 ----------

  /** 设置填充 */
  setFill(id: string, fill: Fill): boolean {
    const shape = this.getShape(id);
    if (!shape) return false;
    shape.fill = fill;
    return true;
  }

  /** 设置线条 */
  setLine(id: string, line: Line): boolean {
    const shape = this.getShape(id);
    if (!shape) return false;
    shape.line = line;
    return true;
  }

  // ---------- 备注 ----------

  /** 设置备注 */
  setNotes(notes: string): void {
    this.notes = notes;
  }

  // ---------- 序列化 ----------

  /** 序列化为 JSON */
  toJSON(): Slide {
    return {
      slideId: this.slideId,
      index: this.index,
      layout: this.layout,
      shapes: this.shapes.map(s => this.cloneShape(s)),
      notes: this.notes,
      backgroundColor: this.backgroundColor ?? undefined,
      hidden: this.hidden,
      transition: this.transition ?? undefined,
    };
  }

  /** 深拷贝形状 */
  private cloneShape(shape: Shape): Shape {
    return JSON.parse(JSON.stringify(shape));
  }

  /** 序列化为 JSON 字符串 */
  toJSONString(): string {
    return JSON.stringify(this.toJSON());
  }
}

// ============================================================
// 演示文稿文档模型
// ============================================================

/** 演示文稿文档模型 — 管理多张幻灯片 */
export class SlideDocumentModel {
  private slides: SlideModel[] = [];
  private slideSize: SlideSize;
  private coreProperties: SlideCoreProperties;
  private activeIndex: number = 0;

  constructor(data?: ZQSlideDocument) {
    this.slideSize = data?.slideSize ?? { ...DEFAULT_SLIDE_SIZE };
    this.coreProperties = data?.coreProperties ?? {};

    if (data?.slides) {
      this.slides = data.slides.map(s => new SlideModel(s));
    }
    if (this.slides.length === 0) {
      // 默认创建一张空白幻灯片
      this.addSlide();
    }
  }

  // ---------- 幻灯片管理 ----------

  /** 获取所有幻灯片 */
  getAllSlides(): SlideModel[] {
    return [...this.slides];
  }

  /** 获取当前活动幻灯片 */
  getActiveSlide(): SlideModel {
    return this.slides[this.activeIndex];
  }

  /** 获取活动索引 */
  getActiveIndex(): number {
    return this.activeIndex;
  }

  /** 设置活动幻灯片 */
  setActiveIndex(index: number): void {
    if (index >= 0 && index < this.slides.length) {
      this.activeIndex = index;
    }
  }

  /** 获取幻灯片数量 */
  getSlideCount(): number {
    return this.slides.length;
  }

  /** 按索引获取幻灯片 */
  getSlide(index: number): SlideModel | null {
    return this.slides[index] ?? null;
  }

  /** 按 ID 获取幻灯片 */
  getSlideById(id: string): SlideModel | null {
    return this.slides.find(s => s.slideId === id) ?? null;
  }

  /** 添加幻灯片 */
  addSlide(slide?: SlideModel, insertIndex?: number): SlideModel {
    const newSlide = slide ?? new SlideModel({
      slideId: `slide_${Date.now()}_${this.slides.length}`,
      index: this.slides.length,
      layout: 'blank',
    });
    if (insertIndex !== undefined && insertIndex >= 0 && insertIndex <= this.slides.length) {
      this.slides.splice(insertIndex, 0, newSlide);
      this.reindexSlides();
      this.activeIndex = insertIndex;
    } else {
      this.slides.push(newSlide);
      this.activeIndex = this.slides.length - 1;
    }
    return newSlide;
  }

  /** 复制幻灯片 */
  duplicateSlide(index: number): SlideModel | null {
    const original = this.slides[index];
    if (!original) return null;
    const cloned = new SlideModel(original.toJSON());
    cloned.slideId = `slide_${Date.now()}_${this.slides.length}`;
    this.slides.splice(index + 1, 0, cloned);
    this.reindexSlides();
    this.activeIndex = index + 1;
    return cloned;
  }

  /** 删除幻灯片 */
  removeSlide(index: number): boolean {
    if (this.slides.length <= 1) return false; // 至少保留一张
    if (index < 0 || index >= this.slides.length) return false;
    this.slides.splice(index, 1);
    this.reindexSlides();
    if (this.activeIndex >= this.slides.length) {
      this.activeIndex = this.slides.length - 1;
    }
    return true;
  }

  /** 移动幻灯片 */
  moveSlide(fromIndex: number, toIndex: number): boolean {
    if (fromIndex < 0 || fromIndex >= this.slides.length) return false;
    if (toIndex < 0 || toIndex >= this.slides.length) return false;
    const [slide] = this.slides.splice(fromIndex, 1);
    this.slides.splice(toIndex, 0, slide);
    this.reindexSlides();
    this.activeIndex = toIndex;
    return true;
  }

  /** 重新索引幻灯片 */
  private reindexSlides(): void {
    this.slides.forEach((s, i) => { s.index = i; });
  }

  // ---------- 尺寸 ----------

  /** 获取幻灯片尺寸 */
  getSlideSize(): SlideSize {
    return { ...this.slideSize };
  }

  /** 设置幻灯片尺寸 */
  setSlideSize(size: SlideSize): void {
    this.slideSize = { ...size };
  }

  // ---------- 核心属性 ----------

  /** 获取核心属性 */
  getCoreProperties(): SlideCoreProperties {
    return { ...this.coreProperties };
  }

  /** 设置核心属性 */
  setCoreProperties(props: Partial<SlideCoreProperties>): void {
    Object.assign(this.coreProperties, props);
  }

  // ---------- 序列化 ----------

  /** 序列化为 JSON */
  toJSON(): ZQSlideDocument {
    return {
      slides: this.slides.map(s => s.toJSON()),
      slideSize: { ...this.slideSize },
      coreProperties: { ...this.coreProperties },
    };
  }

  /** 序列化为 JSON 字符串 */
  toJSONString(): string {
    return JSON.stringify(this.toJSON());
  }
}

// ============================================================
// 形状工厂 — 创建常用形状
// ============================================================

export class ShapeFactory {
  private static idCounter = 0;

  /** 生成唯一 ID */
  private static nextId(): string {
    return `shape_${Date.now()}_${++this.idCounter}`;
  }

  /** 创建文本框 */
  static createTextBox(x: number, y: number, width: number, height: number, text: string = ''): Shape {
    return {
      id: this.nextId(),
      name: '文本框',
      type: 'text',
      geometry: { x, y, width, height },
      rotation: 0,
      text: {
        paragraphs: [{
          alignment: 'left',
          runs: [{ text, font: { family: 'Microsoft YaHei', size: 1800, color: '333333' } }],
        }],
      },
      fill: { type: 'none' },
      line: { noFill: true },
      zIndex: 0,
    };
  }

  /** 创建标题占位符 */
  static createTitle(x: number, y: number, width: number, height: number, text: string = '标题'): Shape {
    return {
      id: this.nextId(),
      name: '标题',
      type: 'text',
      geometry: { x, y, width, height },
      rotation: 0,
      text: {
        paragraphs: [{
          alignment: 'center',
          runs: [{ text, font: { family: 'Microsoft YaHei', size: 4400, bold: true, color: '333333' } }],
        }],
      },
      fill: { type: 'none' },
      line: { noFill: true },
      zIndex: 0,
    };
  }

  /** 创建自选图形 */
  static createAutoShape(
    prstGeom: PresetGeometry,
    x: number, y: number, width: number, height: number,
  ): Shape {
    return {
      id: this.nextId(),
      name: prstGeom,
      type: 'autoshape',
      geometry: { x, y, width, height },
      rotation: 0,
      prstGeom,
      fill: { type: 'solid', color: '4787F5' },
      line: { color: '2E5C99', width: 9525 },
      zIndex: 0,
    };
  }

  /** 创建矩形 */
  static createRectangle(x: number, y: number, width: number, height: number): Shape {
    return this.createAutoShape('rect', x, y, width, height);
  }

  /** 创建圆角矩形 */
  static createRoundRect(x: number, y: number, width: number, height: number): Shape {
    return this.createAutoShape('roundRect', x, y, width, height);
  }

  /** 创建椭圆 */
  static createEllipse(x: number, y: number, width: number, height: number): Shape {
    return this.createAutoShape('ellipse', x, y, width, height);
  }

  /** 创建三角形 */
  static createTriangle(x: number, y: number, width: number, height: number): Shape {
    return this.createAutoShape('triangle', x, y, width, height);
  }

  /** 创建箭头 */
  static createArrow(direction: 'right' | 'left' | 'up' | 'down', x: number, y: number, width: number, height: number): Shape {
    const prstMap: Record<'right' | 'left' | 'up' | 'down', PresetGeometry> = {
      right: 'arrowRight',
      left: 'arrowLeft',
      up: 'arrowUp',
      down: 'arrowDown',
    };
    return this.createAutoShape(prstMap[direction], x, y, width, height);
  }

  /** 创建直线 */
  static createLine(x: number, y: number, width: number, height: number): Shape {
    return {
      id: this.nextId(),
      name: '直线',
      type: 'connector',
      geometry: { x, y, width, height },
      rotation: 0,
      prstGeom: 'line',
      fill: { type: 'none' },
      line: { color: '333333', width: 9525 },
      zIndex: 0,
    };
  }

  /** 创建图片占位符 */
  static createImage(x: number, y: number, width: number, height: number, path: string): Shape {
    return {
      id: this.nextId(),
      name: '图片',
      type: 'image',
      geometry: { x, y, width, height },
      rotation: 0,
      image: { path, width, height },
      fill: { type: 'none' },
      line: { noFill: true },
      zIndex: 0,
    };
  }
}
