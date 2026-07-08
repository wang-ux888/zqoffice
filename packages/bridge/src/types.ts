/**
 * ZQOffice 共享类型定义
 *
 * 与 zqoffice_edit 的数据模型完全对齐
 *   - data-model.ts  (Sheet/Excel)
 *   - slide-model.ts (Slide/PowerPoint)
 *   - doc-model.ts   (Doc/Word)
 */

// =================================================================
// 通用类型
// =================================================================

/** 文档核心属性 */
export interface CoreProperties {
  title?: string;
  creator?: string;
  created?: string;
  modified?: string;
}

// =================================================================
// Excel (Sheet) 类型 — 对齐 data-model.ts
// =================================================================

/** 单元格值类型 */
export type CellValueType = 'string' | 'number' | 'boolean' | 'error' | 'formula';

/** 边框样式 */
export interface BorderStyle {
  style: 'none' | 'thin' | 'medium' | 'thick' | 'dashed' | 'dotted' | 'double';
  color: string; // #RRGGBB
}

/** 单元格样式 */
export interface CellStyle {
  fontFamily?: string;
  fontSize?: number; // pt
  bold?: boolean;
  italic?: boolean;
  underline?: boolean;
  strikeThrough?: boolean;
  color?: string; // 文字颜色 #RRGGBB
  backgroundColor?: string; // 背景色 #RRGGBB
  horizontalAlignment?: 'left' | 'center' | 'right' | 'general';
  verticalAlignment?: 'top' | 'middle' | 'bottom';
  wrapText?: boolean;
  textRotation?: number; // 角度 0-180
  numberFormat?: string;
  borderTop?: BorderStyle;
  borderRight?: BorderStyle;
  borderBottom?: BorderStyle;
  borderLeft?: BorderStyle;
  borderDiagonalDown?: BorderStyle;
  borderDiagonalUp?: BorderStyle;
  indent?: number;
}

/** 单元格 */
export interface Cell {
  row: number; // 0-based
  column: number; // 0-based
  type: CellValueType;
  value: string | number | boolean;
  style?: CellStyle;
  formula?: string; // 公式文本 (如 "SUM(A1:A10)")
}

/** 合并区域 */
export interface MergedCell {
  startRow: number;
  startColumn: number;
  endRow: number;
  endColumn: number;
}

/** 列宽 */
export interface ColumnWidth {
  column: number;
  width: number; // 像素
}

/** 行高 */
export interface RowHeight {
  row: number;
  height: number; // 像素
}

/** 单元格批注 (支持回复线程) */
export interface CellComment {
  id: string;
  row: number; // 0-based
  col: number; // 0-based
  author: string;
  text: string;
  timestamp: number; // 毫秒
  replies: CellComment[];
  resolved?: boolean;
}

/** 分组大纲 */
export interface OutlineGroup {
  id: string;
  type: 'row' | 'column';
  start: number; // 0-based, 含
  end: number; // 0-based, 含
  collapsed: boolean;
  level: number; // 1 = 最外层
}

/** 工作表 */
export interface Sheet {
  sheetId: string;
  name: string;
  index: number;
  rowCount: number;
  columnCount: number;
  defaultRowHeight: number;
  defaultColumnWidth: number;
  cells: Cell[];
  mergedCells: MergedCell[];
  columnWidths: ColumnWidth[];
  rowHeights: RowHeight[];
  images: unknown[];
  charts: unknown[];
  frozenRowCount?: number; // 冻结行数
  frozenColumnCount?: number; // 冻结列数
  hiddenRows?: number[]; // 隐藏的行号列表 (0-based)
  hiddenColumns?: number[]; // 隐藏的列号列表 (0-based)
  comments?: CellComment[]; // 单元格批注列表
  outlines?: OutlineGroup[]; // 分组大纲列表
  hidden?: boolean;
  tabColor?: string;
}

/** 命名区域 */
export interface DefinedName {
  name: string;
  ref: string; // 如 "Sheet1!$A$1:$B$2"
}

/** ZQ Sheet 文档 (顶层) */
export interface ZQSheetDocument {
  sheets: Sheet[];
  definedNames?: DefinedName[];
  coreProperties?: CoreProperties;
}

// =================================================================
// PowerPoint (Slide) 类型 — 对齐 slide-model.ts
// =================================================================

/** 形状类型 */
export type ShapeType = 'text' | 'image' | 'chart' | 'group' | 'autoshape' | 'connector';

/** 几何位置 (EMU 单位) */
export interface Geometry {
  x: number;
  y: number;
  width: number;
  height: number;
}

/** 段落对齐方式 */
export type ParagraphAlignment = 'left' | 'center' | 'right' | 'justify';

/** 字体样式 */
export interface Font {
  family?: string;
  size?: number; // slide: 1/100 pt (OOXML sz); doc: pt
  bold?: boolean;
  italic?: boolean;
  underline?: boolean;
  strikeThrough?: boolean; // slide 用
  color?: string; // RRGGBB (无 # 前缀)
}

/** 文本运行 (Run) */
export interface TextRun {
  text: string;
  font?: Font;
}

/** 段落 */
export interface Paragraph {
  alignment?: ParagraphAlignment;
  runs: TextRun[];
  lineSpacing?: number; // pt (slide 用)
  spaceBefore?: number; // pt (slide 用)
  spaceAfter?: number; // pt (slide 用)
}

/** 文本内容 */
export interface TextContent {
  paragraphs: Paragraph[];
}

/** 填充 */
export interface Fill {
  type: 'solid' | 'none' | 'gradient';
  color?: string; // type=solid 时的颜色 RRGGBB
  gradient?: {
    angle: number;
    stops: Array<{ position: number; color: string }>;
  };
}

/** 线条/边框 */
export interface Line {
  noFill?: boolean;
  color?: string;
  width?: number; // EMU
}

/** 图片信息 */
export interface ImageInfo {
  path: string; // 如 "media/rId1"
  width: number; // EMU
  height: number; // EMU
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
  rotation?: number; // 1/60000 度 (slide)
  text?: TextContent; // type=text/autoshape 时存在
  fill?: Fill;
  line?: Line;
  image?: ImageInfo; // type=image 时存在
  prstGeom?: PresetGeometry; // type=autoshape 时的预设几何
  children?: Shape[]; // type=group 时的子形状
  zIndex?: number; // z-index 层叠顺序
  locked?: boolean; // 是否锁定
}

/** 幻灯片版式名 */
export type SlideLayout =
  | 'blank' | 'title' | 'titleContent' | 'sectionHeader'
  | 'twoContent' | 'comparison' | 'titleOnly' | 'content'
  | 'pictureWithCaption' | '';

/** 幻灯片过渡动画 */
export interface SlideTransition {
  type: 'none' | 'fade' | 'push' | 'wipe' | 'split' | 'zoom';
  duration: number; // 毫秒
  direction?: 'left' | 'right' | 'up' | 'down';
}

/** 幻灯片 */
export interface Slide {
  slideId: string;
  index: number;
  layout: SlideLayout;
  shapes: Shape[];
  notes?: string;
  backgroundColor?: string; // RRGGBB
  hidden?: boolean;
  transition?: SlideTransition;
}

/** 幻灯片尺寸 */
export interface SlideSize {
  width: number; // EMU
  height: number; // EMU
}

/** ZQ Slide 文档 (顶层) */
export interface ZQSlideDocument {
  slides: Slide[];
  slideSize: SlideSize;
  coreProperties?: CoreProperties;
}

// =================================================================
// Word (Doc) 类型 — 对齐 doc-model.ts
// =================================================================

/** Block 类型 */
export type BlockType = 'paragraph' | 'heading' | 'table' | 'image';

/** 段落 block */
export interface ParagraphBlock {
  style: string; // 段落样式名
  alignment: ParagraphAlignment;
  runs: TextRun[];
}

/** 标题 block */
export interface HeadingBlock {
  level: number; // 1-9
  text: string;
}

/** 表格单元格 */
export interface TableCell {
  row: number;
  column: number;
  text: string;
}

/** 表格 block */
export interface TableBlock {
  rows: number;
  columns: number;
  cells: TableCell[];
}

/** 图片 block */
export interface ImageBlock {
  path: string; // 路径或 data URL
  width: number; // EMU
  height: number; // EMU
}

/** 文档块 */
export interface Block {
  id: string;
  type: BlockType;
  paragraph?: ParagraphBlock;
  heading?: HeadingBlock;
  table?: TableBlock;
  image?: ImageBlock;
}

/** ZQ Doc 文档 (顶层) */
export interface ZQDocDocument {
  blocks: Block[];
  coreProperties?: CoreProperties;
}

// =================================================================
// 操作类型 (Agent 调用用)
// =================================================================

/** 编辑操作 */
export interface EditOperation {
  action: string;
  params: Record<string, any>;
}

/** 文件转换任务 */
export interface ConvertTask {
  input: string;
  output: string;
}

/** 转换结果 */
export interface ConvertResult {
  success: number;
  failed: number;
  errors: string[];
}
