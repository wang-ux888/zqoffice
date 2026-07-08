/**
 * Word 文档数据模型 — ZQ Doc JSON 操作
 *
 * ZQ Doc JSON 结构:
 *   {
 *     blocks: Block[],
 *     coreProperties: { title, creator, created, modified }
 *   }
 *
 * Block 类型:
 *   - paragraph: { style, alignment, runs: TextRun[] }
 *   - heading:   { level: 1-9, text }
 *   - table:     { rows, columns, cells: TableCell[] }
 *   - image:     { path, width, height }
 */

// ============================================================

/** 段落对齐方式 */
export type ParagraphAlignment = 'left' | 'center' | 'right' | 'justify';

/** Block 类型 */
export type BlockType = 'paragraph' | 'heading' | 'table' | 'image';

/** 字体属性 */
export interface Font {
  family?: string;       // 字体族名
  size?: number;         // 字号 pt
  bold?: boolean;
  italic?: boolean;
  underline?: boolean;
  color?: string;        // RRGGBB 六位十六进制 (无 # 前缀)
}

/** 文本 run */
export interface TextRun {
  text: string;
  font?: Font;
}

/** 段落 block */
export interface ParagraphBlock {
  style: string;                       // 段落样式名
  alignment: ParagraphAlignment;
  runs: TextRun[];
}

/** 标题 block */
export interface HeadingBlock {
  level: number;        // 1-9
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
  path: string;         // 路径或 data URL
  width: number;        // EMU
  height: number;       // EMU
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

/** 核心属性 */
export interface CoreProperties {
  title?: string;
  creator?: string;
  created?: string;
  modified?: string;
}

/** ZQ Doc 顶层结构 */
export interface ZQDocDocument {
  blocks: Block[];
  coreProperties?: CoreProperties;
}

// ============================================================
// 单位转换
// ============================================================

const EMU_PER_PX = 9525;       // 1 px = 9525 EMU (96 DPI)
const EMU_PER_INCH = 914400;
const EMU_PER_CM = 360000;

export function pixelToEmu(px: number): number { return Math.round(px * EMU_PER_PX); }
export function emuToPixel(emu: number): number { return Math.round(emu / EMU_PER_PX); }
export function inchToEmu(inch: number): number { return Math.round(inch * EMU_PER_INCH); }
export function emuToInch(emu: number): number { return emu / EMU_PER_INCH; }
export function cmToEmu(cm: number): number { return Math.round(cm * EMU_PER_CM); }
export function emuToCm(emu: number): number { return emu / EMU_PER_CM; }

// ============================================================
// 文档模型
// ============================================================

/** 块 ID 生成器 */
let blockIdCounter = 0;
function generateBlockId(): string {
  return `block_${Date.now().toString(36)}_${blockIdCounter++}`;
}

/** 默认字体 */
export const DEFAULT_FONT: Font = {
  family: 'Calibri, -apple-system, "Microsoft YaHei", "Segoe UI", sans-serif',
  size: 11,
  bold: false,
  italic: false,
  underline: false,
  color: '333333',
};

/** 标题字号映射 */
export const HEADING_SIZES: Record<number, number> = {
  1: 24, 2: 22, 3: 18, 4: 16, 5: 14, 6: 13, 7: 12, 8: 11, 9: 11,
};

/** 文档模型类 */
export class DocModel {
  private blocks: Block[] = [];
  private coreProperties: CoreProperties;

  constructor(data?: ZQDocDocument) {
    this.coreProperties = data?.coreProperties ?? {
      title: '',
      creator: '',
      created: new Date().toISOString(),
      modified: new Date().toISOString(),
    };

    if (data?.blocks) {
      // 深拷贝
      this.blocks = data.blocks.map(b => this.cloneBlock(b));
    }

    // 确保至少有一个段落
    if (this.blocks.length === 0) {
      this.blocks.push(this.createParagraphBlock());
    }
  }

  // ============================================================
  // Block CRUD
  // ============================================================

  /** 获取所有块 */
  getBlocks(): Block[] {
    return [...this.blocks];
  }

  /** 获取块 */
  getBlock(id: string): Block | null {
    return this.blocks.find(b => b.id === id) ?? null;
  }

  /** 获取块索引 */
  getBlockIndex(id: string): number {
    return this.blocks.findIndex(b => b.id === id);
  }

  /** 添加块 */
  addBlock(block: Block, index?: number): void {
    if (index === undefined || index < 0 || index > this.blocks.length) {
      this.blocks.push(block);
    } else {
      this.blocks.splice(index, 0, block);
    }
  }

  /** 在指定块后插入新块 */
  insertBlockAfter(afterId: string, block: Block): void {
    const idx = this.getBlockIndex(afterId);
    if (idx === -1) {
      this.blocks.push(block);
    } else {
      this.blocks.splice(idx + 1, 0, block);
    }
  }

  /** 更新块 */
  updateBlock(id: string, updates: Partial<Block>): void {
    const idx = this.getBlockIndex(id);
    if (idx === -1) return;
    this.blocks[idx] = { ...this.blocks[idx], ...updates };
  }

  /** 删除块 */
  deleteBlock(id: string): void {
    const idx = this.getBlockIndex(id);
    if (idx === -1) return;
    this.blocks.splice(idx, 1);
    // 确保至少有一个段落
    if (this.blocks.length === 0) {
      this.blocks.push(this.createParagraphBlock());
    }
  }

  /** 移动块 */
  moveBlock(id: string, direction: 'up' | 'down'): void {
    const idx = this.getBlockIndex(id);
    if (idx === -1) return;
    if (direction === 'up' && idx > 0) {
      [this.blocks[idx - 1], this.blocks[idx]] = [this.blocks[idx], this.blocks[idx - 1]];
    } else if (direction === 'down' && idx < this.blocks.length - 1) {
      [this.blocks[idx + 1], this.blocks[idx]] = [this.blocks[idx], this.blocks[idx + 1]];
    }
  }

  // ============================================================
  // 创建 Block 的工厂方法
  // ============================================================

  /** 创建段落块 */
  createParagraphBlock(text: string = '', alignment: ParagraphAlignment = 'left'): Block {
    return {
      id: generateBlockId(),
      type: 'paragraph',
      paragraph: {
        style: 'Normal',
        alignment,
        runs: text ? [{ text, font: { ...DEFAULT_FONT } }] : [],
      },
    };
  }

  /** 创建标题块 */
  createHeadingBlock(level: number, text: string = ''): Block {
    return {
      id: generateBlockId(),
      type: 'heading',
      heading: { level, text },
    };
  }

  /** 创建表格块 */
  createTableBlock(rows: number, columns: number): Block {
    const cells: TableCell[] = [];
    for (let r = 0; r < rows; r++) {
      for (let c = 0; c < columns; c++) {
        cells.push({ row: r, column: c, text: '' });
      }
    }
    return {
      id: generateBlockId(),
      type: 'table',
      table: { rows, columns, cells },
    };
  }

  /** 创建图片块 */
  createImageBlock(path: string, width: number, height: number): Block {
    return {
      id: generateBlockId(),
      type: 'image',
      image: { path, width, height },
    };
  }

  // ============================================================
  // 段落操作
  // ============================================================

  /** 获取段落文本 */
  getParagraphText(id: string): string {
    const block = this.getBlock(id);
    if (!block?.paragraph) return '';
    return block.paragraph.runs.map(r => r.text).join('');
  }

  /** 设置段落文本 (作为单一 run) */
  setParagraphText(id: string, text: string, font?: Font): void {
    const block = this.getBlock(id);
    if (!block?.paragraph) return;
    block.paragraph.runs = text ? [{ text, font: font ?? { ...DEFAULT_FONT } }] : [];
  }

  /** 设置段落对齐 */
  setParagraphAlignment(id: string, alignment: ParagraphAlignment): void {
    const block = this.getBlock(id);
    if (!block?.paragraph) return;
    block.paragraph.alignment = alignment;
  }

  /** 设置标题文本 */
  setHeadingText(id: string, text: string): void {
    const block = this.getBlock(id);
    if (!block?.heading) return;
    block.heading.text = text;
  }

  /** 修改标题级别 */
  setHeadingLevel(id: string, level: number): void {
    const block = this.getBlock(id);
    if (!block?.heading) return;
    block.heading.level = Math.max(1, Math.min(9, level));
  }

  // ============================================================
  // 表格操作
  // ============================================================

  /** 获取表格单元格 */
  getTableCell(blockId: string, row: number, col: number): TableCell | null {
    const block = this.getBlock(blockId);
    if (!block?.table) return null;
    return block.table.cells.find(c => c.row === row && c.column === col) ?? null;
  }

  /** 设置表格单元格文本 */
  setTableCellText(blockId: string, row: number, col: number, text: string): void {
    const block = this.getBlock(blockId);
    if (!block?.table) return;
    const cell = block.table.cells.find(c => c.row === row && c.column === col);
    if (cell) cell.text = text;
  }

  /** 添加表格行 */
  addTableRow(blockId: string, atIndex?: number): void {
    const block = this.getBlock(blockId);
    if (!block?.table) return;
    const newRow = atIndex ?? block.table.rows;
    // 现有行索引 >= newRow 的下移
    for (const cell of block.table.cells) {
      if (cell.row >= newRow) cell.row++;
    }
    // 插入新行
    for (let c = 0; c < block.table.columns; c++) {
      block.table.cells.push({ row: newRow, column: c, text: '' });
    }
    block.table.rows++;
  }

  /** 添加表格列 */
  addTableColumn(blockId: string, atIndex?: number): void {
    const block = this.getBlock(blockId);
    if (!block?.table) return;
    const newCol = atIndex ?? block.table.columns;
    for (const cell of block.table.cells) {
      if (cell.column >= newCol) cell.column++;
    }
    for (let r = 0; r < block.table.rows; r++) {
      block.table.cells.push({ row: r, column: newCol, text: '' });
    }
    block.table.columns++;
  }

  /** 删除表格行 */
  deleteTableRow(blockId: string, row: number): void {
    const block = this.getBlock(blockId);
    if (!block?.table) return;
    if (block.table.rows <= 1) return;
    block.table.cells = block.table.cells.filter(c => c.row !== row);
    // 重新编号
    for (const cell of block.table.cells) {
      if (cell.row > row) cell.row--;
    }
    block.table.rows--;
  }

  /** 删除表格列 */
  deleteTableColumn(blockId: string, col: number): void {
    const block = this.getBlock(blockId);
    if (!block?.table) return;
    if (block.table.columns <= 1) return;
    block.table.cells = block.table.cells.filter(c => c.column !== col);
    for (const cell of block.table.cells) {
      if (cell.column > col) cell.column--;
    }
    block.table.columns--;
  }

  // ============================================================
  // 图片操作
  // ============================================================

  /** 设置图片尺寸 */
  setImageSize(blockId: string, width: number, height: number): void {
    const block = this.getBlock(blockId);
    if (!block?.image) return;
    block.image.width = width;
    block.image.height = height;
  }

  /** 设置图片路径 */
  setImagePath(blockId: string, path: string): void {
    const block = this.getBlock(blockId);
    if (!block?.image) return;
    block.image.path = path;
  }

  // ============================================================
  // 核心属性
  // ============================================================

  getCoreProperties(): CoreProperties { return { ...this.coreProperties }; }
  setCoreProperties(props: Partial<CoreProperties>): void {
    this.coreProperties = { ...this.coreProperties, ...props };
  }

  // ============================================================
  // 序列化
  // ============================================================

  /** 转 JSON 对象 */
  toJSON(): ZQDocDocument {
    return {
      blocks: this.blocks.map(b => this.cloneBlock(b)),
      coreProperties: { ...this.coreProperties },
    };
  }

  /** 转字符串 */
  toJSONString(): string {
    return JSON.stringify(this.toJSON(), null, 2);
  }

  /** 深拷贝 Block */
  private cloneBlock(b: Block): Block {
    const clone: Block = { id: b.id, type: b.type };
    if (b.paragraph) {
      clone.paragraph = {
        style: b.paragraph.style,
        alignment: b.paragraph.alignment,
        runs: b.paragraph.runs.map(r => ({
          text: r.text,
          font: r.font ? { ...r.font } : undefined,
        })),
      };
    }
    if (b.heading) clone.heading = { ...b.heading };
    if (b.table) {
      clone.table = {
        rows: b.table.rows,
        columns: b.table.columns,
        cells: b.table.cells.map(c => ({ ...c })),
      };
    }
    if (b.image) clone.image = { ...b.image };
    return clone;
  }
}
