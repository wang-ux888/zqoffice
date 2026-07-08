/**
 * 数据模型层 — ZQ Sheet JSON 操作(CRUD)
 */

// ============================================================

/** 单元格值类型 */
export type CellValueType = 'string' | 'number' | 'boolean' | 'error' | 'formula';

/** 单元格样式 */
export interface CellStyle {
  fontFamily?: string;
  fontSize?: number;          // pt
  bold?: boolean;
  italic?: boolean;
  underline?: boolean;
  strikeThrough?: boolean;
  color?: string;             // 文字颜色 #RRGGBB
  backgroundColor?: string;   // 背景色 #RRGGBB
  horizontalAlignment?: 'left' | 'center' | 'right' | 'general';
  verticalAlignment?: 'top' | 'middle' | 'bottom';
  wrapText?: boolean;
  textRotation?: number;      // 角度 0-180
  numberFormat?: string;      // General/0.00/#,##0/0%/yyyy-mm-dd 等
  borderTop?: BorderStyle;
  borderRight?: BorderStyle;
  borderBottom?: BorderStyle;
  borderLeft?: BorderStyle;
  borderDiagonalDown?: BorderStyle;  // 对角线 (左上 -> 右下)
  borderDiagonalUp?: BorderStyle;    // 对角线 (左下 -> 右上)
  indent?: number;
}

/** 边框样式 */
export interface BorderStyle {
  style: 'none' | 'thin' | 'medium' | 'thick' | 'dashed' | 'dotted' | 'double';
  color: string;  // #RRGGBB
}

/** 单元格 */
export interface Cell {
  row: number;          // 0-based
  column: number;       // 0-based
  type: CellValueType;
  value: string | number | boolean;
  style?: CellStyle;
  formula?: string;     // 公式文本 (如 "SUM(A1:A10)")
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
  width: number;  // 像素
}

/** 行高 */
export interface RowHeight {
  row: number;
  height: number;  // 像素
}

/** 单元格批注 (支持回复线程) */
export interface CellComment {
  /** 唯一 ID */
  id: string;
  /** 行号 (0-based) */
  row: number;
  /** 列号 (0-based) */
  col: number;
  /** 作者 */
  author: string;
  /** 批注正文 */
  text: string;
  /** 创建时间戳 (毫秒) */
  timestamp: number;
  /** 回复列表 (嵌套) */
  replies: CellComment[];
  /** 是否已解决 */
  resolved?: boolean;
}

/** 分组大纲 */
export interface OutlineGroup {
  /** 唯一 ID */
  id: string;
  /** 分组类型: 行分组或列分组 */
  type: 'row' | 'column';
  /** 起始索引 (0-based, 含) */
  start: number;
  /** 结束索引 (0-based, 含) */
  end: number;
  /** 是否已折叠 */
  collapsed: boolean;
  /** 嵌套层级 (1 = 最外层) */
  level: number;
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
  frozenRowCount?: number;    // 冻结行数
  frozenColumnCount?: number; // 冻结列数
  hiddenRows?: number[];      // 隐藏的行号列表 (0-based)
  hiddenColumns?: number[];   // 隐藏的列号列表 (0-based)
  comments?: CellComment[];   // 单元格批注列表
  outlines?: OutlineGroup[];  // 分组大纲列表
  hidden?: boolean;
  tabColor?: string;
}

/** 命名区域 */
export interface DefinedName {
  name: string;
  ref: string;  // 如 "Sheet1!$A$1:$B$2"
}

/** 文档核心属性 */
export interface CoreProperties {
  title?: string;
  creator?: string;
  created?: string;
  modified?: string;
}

/** ZQ Sheet 文档 (顶层) */
export interface ZQSheetDocument {
  sheets: Sheet[];
  definedNames?: DefinedName[];
  coreProperties?: CoreProperties;
}

// ============================================================
// 内部数据结构 — 使用 Map 加速查找
// ============================================================

/** 单元格键: `${row}:${column}` */
function cellKey(row: number, col: number): string {
  return `${row}:${col}`;
}

/** 工作表内部表示 */
export class SheetModel {
  sheetId: string;
  name: string;
  index: number;
  rowCount: number;
  columnCount: number;
  defaultRowHeight: number;
  defaultColumnWidth: number;
  frozenRowCount: number;
  frozenColumnCount: number;
  hidden: boolean;
  tabColor: string | null;

  // 用 Map 存储单元格, key = "row:col"
  private cells = new Map<string, Cell>();
  private mergedCells: MergedCell[] = [];
  private columnWidths = new Map<number, number>();
  private rowHeights = new Map<number, number>();
  // 隐藏行/列集合 (使用 Set 加速查找)
  private hiddenRows = new Set<number>();
  private hiddenColumns = new Set<number>();
  // 批注列表
  private comments: CellComment[] = [];
  // 分组大纲列表
  private outlines: OutlineGroup[] = [];

  // 合并区域索引, key = "row:col" -> 主单元格的 "row:col"
  private mergeIndex = new Map<string, string>();

  constructor(data?: Partial<Sheet>) {
    this.sheetId = data?.sheetId ?? `sheet_${Date.now()}`;
    this.name = data?.name ?? 'Sheet1';
    this.index = data?.index ?? 0;
    this.rowCount = data?.rowCount ?? 100;
    this.columnCount = data?.columnCount ?? 26;
    this.defaultRowHeight = data?.defaultRowHeight ?? 24;
    this.defaultColumnWidth = data?.defaultColumnWidth ?? 88;
    this.frozenRowCount = data?.frozenRowCount ?? 0;
    this.frozenColumnCount = data?.frozenColumnCount ?? 0;
    this.hidden = data?.hidden ?? false;
    this.tabColor = data?.tabColor ?? null;

    // 从 cells 数组加载到 Map
    if (data?.cells) {
      for (const cell of data.cells) {
        this.cells.set(cellKey(cell.row, cell.column), { ...cell });
      }
    }

    // 加载合并区域
    if (data?.mergedCells) {
      for (const mc of data.mergedCells) {
        this.addMerge(mc.startRow, mc.startColumn, mc.endRow, mc.endColumn);
      }
    }

    // 加载列宽
    if (data?.columnWidths) {
      for (const cw of data.columnWidths) {
        this.columnWidths.set(cw.column, cw.width);
      }
    }

    // 加载行高
    if (data?.rowHeights) {
      for (const rh of data.rowHeights) {
        this.rowHeights.set(rh.row, rh.height);
      }
    }

    // 加载隐藏行/列
    if (data?.hiddenRows) {
      for (const r of data.hiddenRows) {
        this.hiddenRows.add(r);
      }
    }
    if (data?.hiddenColumns) {
      for (const c of data.hiddenColumns) {
        this.hiddenColumns.add(c);
      }
    }

    // 加载批注
    if (data?.comments) {
      this.comments = data.comments.map(c => this.cloneComment(c));
    }

    // 加载分组大纲
    if (data?.outlines) {
      this.outlines = data.outlines.map(o => ({ ...o }));
    }
  }

  // ============================================================
  // 单元格 CRUD
  // ============================================================

  /** 获取单元格 */
  getCell(row: number, col: number): Cell | null {
    return this.cells.get(cellKey(row, col)) ?? null;
  }

  /** 获取单元格值 (如果是合并区域, 返回主单元格的值) */
  getCellValue(row: number, col: number): string | number | boolean | null {
    const masterKey = this.mergeIndex.get(cellKey(row, col));
    const key = masterKey ?? cellKey(row, col);
    const cell = this.cells.get(key);
    return cell ? cell.value : null;
  }

  /** 设置单元格值 */
  setCellValue(row: number, col: number, value: string | number | boolean, type?: CellValueType): void {
    const key = cellKey(row, col);
    let cell = this.cells.get(key);
    if (!cell) {
      cell = { row, column: col, type: type ?? this.inferType(value), value };
      this.cells.set(key, cell);
    } else {
      cell.value = value;
      if (type) cell.type = type;
      else cell.type = this.inferType(value);
    }

    // 如果是合并区域的主单元格, 不需要额外处理
    // 如果是合并区域的从属单元格, 值应该存储在主单元格
    const masterKey = this.mergeIndex.get(key);
    if (masterKey && masterKey !== key) {
      const masterCell = this.cells.get(masterKey);
      if (masterCell) {
        masterCell.value = value;
        masterCell.type = type ?? this.inferType(value);
      }
      // 从属单元格不存储值
      this.cells.delete(key);
    }
  }

  /** 设置单元格公式 */
  setCellFormula(row: number, col: number, formula: string): void {
    const key = cellKey(row, col);
    let cell = this.cells.get(key);
    if (!cell) {
      cell = { row, column: col, type: 'formula', value: formula, formula };
      this.cells.set(key, cell);
    } else {
      cell.type = 'formula';
      cell.formula = formula;
      cell.value = formula;
    }
  }

  /** 设置单元格样式 */
  setCellStyle(row: number, col: number, style: Partial<CellStyle>): void {
    const key = cellKey(row, col);
    let cell = this.cells.get(key);
    if (!cell) {
      cell = { row, column: col, type: 'string', value: '', style: {} };
      this.cells.set(key, cell);
    }
    cell.style = { ...cell.style, ...style };
  }

  /** 获取单元格样式 */
  getCellStyle(row: number, col: number): CellStyle {
    const masterKey = this.mergeIndex.get(cellKey(row, col));
    const key = masterKey ?? cellKey(row, col);
    const cell = this.cells.get(key);
    return cell?.style ?? {};
  }

  /** 删除单元格 */
  deleteCell(row: number, col: number): void {
    this.cells.delete(cellKey(row, col));
  }

  /** 清除单元格内容 (保留样式) */
  clearCellContent(row: number, col: number): void {
    const key = cellKey(row, col);
    const cell = this.cells.get(key);
    if (cell) {
      cell.value = '';
      cell.type = 'string';
      cell.formula = undefined;
    }
  }

  /** 清除单元格样式 (保留内容) */
  clearCellStyle(row: number, col: number): void {
    const key = cellKey(row, col);
    const cell = this.cells.get(key);
    if (cell) {
      cell.style = {};
    }
  }

  /** 获取所有单元格 */
  getAllCells(): Cell[] {
    return Array.from(this.cells.values()).sort((a, b) => {
      if (a.row !== b.row) return a.row - b.row;
      return a.column - b.column;
    });
  }

  /** 获取区域内的单元格 */
  getCellsInRange(startRow: number, startCol: number, endRow: number, endCol: number): Cell[] {
    const result: Cell[] = [];
    for (let r = startRow; r <= endRow; r++) {
      for (let c = startCol; c <= endCol; c++) {
        const cell = this.getCell(r, c);
        if (cell) result.push(cell);
      }
    }
    return result;
  }

  // ============================================================
  // 合并单元格
  // ============================================================

  /** 添加合并区域 */
  addMerge(startRow: number, startCol: number, endRow: number, endCol: number): void {
    // 确保范围正确
    const sr = Math.min(startRow, endRow);
    const er = Math.max(startRow, endRow);
    const sc = Math.min(startCol, endCol);
    const ec = Math.max(startCol, endCol);

    // 移除重叠的合并区域
    this.removeOverlappingMerges(sr, sc, er, ec);

    // 添加新合并区域
    const merge: MergedCell = { startRow: sr, startColumn: sc, endRow: er, endColumn: ec };
    this.mergedCells.push(merge);

    // 建立索引: 所有从属单元格指向主单元格
    const masterKey = cellKey(sr, sc);
    for (let r = sr; r <= er; r++) {
      for (let c = sc; c <= ec; c++) {
        if (r === sr && c === sc) continue;
        this.mergeIndex.set(cellKey(r, c), masterKey);
      }
    }

    // 确保主单元格存在
    if (!this.cells.has(masterKey)) {
      this.cells.set(masterKey, { row: sr, column: sc, type: 'string', value: '' });
    }
  }

  /** 移除合并区域 (如果指定单元格在某个合并区域内) */
  removeMerge(row: number, col: number): void {
    const key = cellKey(row, col);
    const masterKey = this.mergeIndex.get(key) ?? key;

    // 找到并移除合并区域
    this.mergedCells = this.mergedCells.filter(mc => {
      const mcMasterKey = cellKey(mc.startRow, mc.startColumn);
      if (mcMasterKey === masterKey) {
        // 清除索引
        for (let r = mc.startRow; r <= mc.endRow; r++) {
          for (let c = mc.startColumn; c <= mc.endColumn; c++) {
            this.mergeIndex.delete(cellKey(r, c));
          }
        }
        return false;
      }
      return true;
    });
  }

  /** 获取合并区域 (如果指定单元格在某个合并区域内) */
  getMerge(row: number, col: number): MergedCell | null {
    const key = cellKey(row, col);
    const masterKey = this.mergeIndex.get(key) ?? key;
    for (const mc of this.mergedCells) {
      if (cellKey(mc.startRow, mc.startColumn) === masterKey) {
        return mc;
      }
    }
    return null;
  }

  /** 判断单元格是否是合并区域的主单元格 */
  isMergeMaster(row: number, col: number): boolean {
    const key = cellKey(row, col);
    return !this.mergeIndex.has(key);
  }

  /** 获取所有合并区域 */
  getAllMerges(): MergedCell[] {
    return [...this.mergedCells];
  }

  /** 移除与新区域重叠的已有合并区域 */
  private removeOverlappingMerges(sr: number, sc: number, er: number, ec: number): void {
    this.mergedCells = this.mergedCells.filter(mc => {
      const overlap = !(mc.endRow < sr || mc.startRow > er || mc.endColumn < sc || mc.startColumn > ec);
      if (overlap) {
        // 清除索引
        for (let r = mc.startRow; r <= mc.endRow; r++) {
          for (let c = mc.startColumn; c <= mc.endColumn; c++) {
            this.mergeIndex.delete(cellKey(r, c));
          }
        }
      }
      return !overlap;
    });
  }

  // ============================================================
  // 行列操作
  // ============================================================

  /** 插入行 */
  insertRow(row: number, count: number = 1): void {
    // 移动行号 >= row 的单元格
    const entries = Array.from(this.cells.entries());
    this.cells.clear();
    for (const [key, cell] of entries) {
      if (cell.row >= row) {
        cell.row += count;
      }
      this.cells.set(cellKey(cell.row, cell.column), cell);
    }

    // 移动合并区域
    this.mergedCells = this.mergedCells.map(mc => ({
      startRow: mc.startRow >= row ? mc.startRow + count : mc.startRow,
      startColumn: mc.startColumn,
      endRow: mc.endRow >= row ? mc.endRow + count : mc.endRow,
      endColumn: mc.endColumn,
    }));

    // 重建合并索引
    this.rebuildMergeIndex();

    // 移动行高
    const newHeights = new Map<number, number>();
    for (const [r, h] of this.rowHeights) {
      newHeights.set(r >= row ? r + count : r, h);
    }
    this.rowHeights = newHeights;

    // 移动隐藏行 (>=row 的行号 +count)
    this.shiftHiddenRows(row, count);

    this.rowCount += count;
  }

  /** 插入列 */
  insertColumn(col: number, count: number = 1): void {
    const entries = Array.from(this.cells.entries());
    this.cells.clear();
    for (const [, cell] of entries) {
      if (cell.column >= col) {
        cell.column += count;
      }
      this.cells.set(cellKey(cell.row, cell.column), cell);
    }

    this.mergedCells = this.mergedCells.map(mc => ({
      startRow: mc.startRow,
      startColumn: mc.startColumn >= col ? mc.startColumn + count : mc.startColumn,
      endRow: mc.endRow,
      endColumn: mc.endColumn >= col ? mc.endColumn + count : mc.endColumn,
    }));

    this.rebuildMergeIndex();

    const newWidths = new Map<number, number>();
    for (const [c, w] of this.columnWidths) {
      newWidths.set(c >= col ? c + count : c, w);
    }
    this.columnWidths = newWidths;

    // 移动隐藏列 (>=col 的列号 +count)
    this.shiftHiddenColumns(col, count);

    this.columnCount += count;
  }

  /** 删除行 */
  deleteRow(row: number, count: number = 1): void {
    const entries = Array.from(this.cells.entries());
    this.cells.clear();
    for (const [, cell] of entries) {
      if (cell.row >= row && cell.row < row + count) {
        continue; // 删除范围内的单元格
      }
      if (cell.row >= row + count) {
        cell.row -= count;
      }
      this.cells.set(cellKey(cell.row, cell.column), cell);
    }

    // 移除被删除行覆盖的合并区域, 调整其他
    this.mergedCells = this.mergedCells
      .filter(mc => {
        // 完全在删除范围内: 移除
        if (mc.startRow >= row && mc.endRow < row + count) return false;
        return true;
      })
      .map(mc => {
        // 调整行号
        let sr = mc.startRow;
        let er = mc.endRow;
        if (sr >= row + count) sr -= count;
        else if (sr >= row) sr = row;
        if (er >= row + count) er -= count;
        else if (er >= row) er = row - 1;
        return { ...mc, startRow: sr, endRow: er };
      });

    this.rebuildMergeIndex();

    const newHeights = new Map<number, number>();
    for (const [r, h] of this.rowHeights) {
      if (r >= row && r < row + count) continue;
      newHeights.set(r >= row + count ? r - count : r, h);
    }
    this.rowHeights = newHeights;

    // 调整隐藏行: 移除被删除范围内的行, 后续行号 -count
    this.removeHiddenRowsInRange(row, row + count);
    this.shiftHiddenRows(row + count, -count);

    this.rowCount = Math.max(0, this.rowCount - count);
  }

  /** 删除列 */
  deleteColumn(col: number, count: number = 1): void {
    const entries = Array.from(this.cells.entries());
    this.cells.clear();
    for (const [, cell] of entries) {
      if (cell.column >= col && cell.column < col + count) {
        continue;
      }
      if (cell.column >= col + count) {
        cell.column -= count;
      }
      this.cells.set(cellKey(cell.row, cell.column), cell);
    }

    this.mergedCells = this.mergedCells
      .filter(mc => {
        if (mc.startColumn >= col && mc.endColumn < col + count) return false;
        return true;
      })
      .map(mc => {
        let sc = mc.startColumn;
        let ec = mc.endColumn;
        if (sc >= col + count) sc -= count;
        else if (sc >= col) sc = col;
        if (ec >= col + count) ec -= count;
        else if (ec >= col) ec = col - 1;
        return { ...mc, startColumn: sc, endColumn: ec };
      });

    this.rebuildMergeIndex();

    const newWidths = new Map<number, number>();
    for (const [c, w] of this.columnWidths) {
      if (c >= col && c < col + count) continue;
      newWidths.set(c >= col + count ? c - count : c, w);
    }
    this.columnWidths = newWidths;

    // 调整隐藏列: 移除被删除范围内的列, 后续列号 -count
    this.removeHiddenColumnsInRange(col, col + count);
    this.shiftHiddenColumns(col + count, -count);

    this.columnCount = Math.max(0, this.columnCount - count);
  }

  // ============================================================
  // 行高/列宽
  // ============================================================

  /** 获取行高 */
  getRowHeight(row: number): number {
    return this.rowHeights.get(row) ?? this.defaultRowHeight;
  }

  /** 设置行高 */
  setRowHeight(row: number, height: number): void {
    this.rowHeights.set(row, Math.max(1, height));
  }

  /** 获取列宽 */
  getColumnWidth(col: number): number {
    return this.columnWidths.get(col) ?? this.defaultColumnWidth;
  }

  /** 设置列宽 */
  setColumnWidth(col: number, width: number): void {
    this.columnWidths.set(col, Math.max(1, width));
  }

  /** 获取所有行高 */
  getAllRowHeights(): RowHeight[] {
    return Array.from(this.rowHeights.entries()).map(([row, height]) => ({ row, height }));
  }

  /** 获取所有列宽 */
  getAllColumnWidths(): ColumnWidth[] {
    return Array.from(this.columnWidths.entries()).map(([column, width]) => ({ column, width }));
  }

  // ============================================================
  // 冻结窗格
  // ============================================================

  setFreeze(rowCount: number, colCount: number): void {
    this.frozenRowCount = Math.max(0, rowCount);
    this.frozenColumnCount = Math.max(0, colCount);
  }

  // ============================================================
  // 隐藏行/列
  // ============================================================

  /** 判断指定行是否隐藏 */
  isRowHidden(row: number): boolean {
    return this.hiddenRows.has(row);
  }

  /** 判断指定列是否隐藏 */
  isColumnHidden(col: number): boolean {
    return this.hiddenColumns.has(col);
  }

  /** 隐藏指定行范围 [startRow, startRow + count) */
  hideRows(startRow: number, count: number = 1): void {
    for (let r = startRow; r < startRow + count; r++) {
      if (r >= 0 && r < this.rowCount) {
        this.hiddenRows.add(r);
      }
    }
  }

  /** 隐藏指定列范围 [startCol, startCol + count) */
  hideColumns(startCol: number, count: number = 1): void {
    for (let c = startCol; c < startCol + count; c++) {
      if (c >= 0 && c < this.columnCount) {
        this.hiddenColumns.add(c);
      }
    }
  }

  /** 显示指定行范围 [startRow, startRow + count) */
  showRows(startRow: number, count: number = 1): void {
    for (let r = startRow; r < startRow + count; r++) {
      this.hiddenRows.delete(r);
    }
  }

  /** 显示指定列范围 [startCol, startCol + count) */
  showColumns(startCol: number, count: number = 1): void {
    for (let c = startCol; c < startCol + count; c++) {
      this.hiddenColumns.delete(c);
    }
  }

  /** 获取所有隐藏行号 (升序) */
  getHiddenRows(): number[] {
    return Array.from(this.hiddenRows).sort((a, b) => a - b);
  }

  /** 获取所有隐藏列号 (升序) */
  getHiddenColumns(): number[] {
    return Array.from(this.hiddenColumns).sort((a, b) => a - b);
  }

  /** 清除所有隐藏行 */
  clearHiddenRows(): void {
    this.hiddenRows.clear();
  }

  /** 清除所有隐藏列 */
  clearHiddenColumns(): void {
    this.hiddenColumns.clear();
  }

  // ============================================================
  // 批注 (CellComment) — 存储与序列化, 业务逻辑由 CellCommentManager 负责
  // ============================================================

  /** 获取所有批注 (浅拷贝) */
  getComments(): CellComment[] {
    return this.comments.map(c => this.cloneComment(c));
  }

  /** 直接设置批注列表 (覆盖) */
  setComments(comments: CellComment[]): void {
    this.comments = comments.map(c => this.cloneComment(c));
  }

  /** 深拷贝批注 (含回复线程) */
  private cloneComment(c: CellComment): CellComment {
    return {
      id: c.id,
      row: c.row,
      col: c.col,
      author: c.author,
      text: c.text,
      timestamp: c.timestamp,
      replies: c.replies ? c.replies.map(r => this.cloneComment(r)) : [],
      resolved: c.resolved,
    };
  }

  // ============================================================
  // 分组大纲 (OutlineGroup) — 存储与序列化, 业务逻辑由 GroupOutlineManager 负责
  // ============================================================

  /** 获取所有分组大纲 (浅拷贝) */
  getOutlines(): OutlineGroup[] {
    return this.outlines.map(o => ({ ...o }));
  }

  /** 直接设置分组大纲列表 (覆盖) */
  setOutlines(outlines: OutlineGroup[]): void {
    this.outlines = outlines.map(o => ({ ...o }));
  }

  // ============================================================
  // 工作表属性
  // ============================================================

  setName(name: string): void {
    this.name = name;
  }

  setTabColor(color: string | null): void {
    this.tabColor = color;
  }

  // ============================================================

  /** 序列化为 Sheet JSON 对象 */
  toJSON(): Sheet {
    return {
      sheetId: this.sheetId,
      name: this.name,
      index: this.index,
      rowCount: this.rowCount,
      columnCount: this.columnCount,
      defaultRowHeight: this.defaultRowHeight,
      defaultColumnWidth: this.defaultColumnWidth,
      cells: this.getAllCells(),
      mergedCells: this.getAllMerges(),
      columnWidths: this.getAllColumnWidths(),
      rowHeights: this.getAllRowHeights(),
      images: [],
      charts: [],
      frozenRowCount: this.frozenRowCount,
      frozenColumnCount: this.frozenColumnCount,
      hiddenRows: this.getHiddenRows(),
      hiddenColumns: this.getHiddenColumns(),
      comments: this.getComments(),
      outlines: this.getOutlines(),
      hidden: this.hidden,
      tabColor: this.tabColor ?? undefined,
    };
  }

  /** 序列化为 JSON 字符串 */
  toJSONString(): string {
    return JSON.stringify(this.toJSON());
  }

  // ============================================================
  // 辅助方法
  // ============================================================

  /** 推断值类型 */
  private inferType(value: string | number | boolean): CellValueType {
    if (typeof value === 'number') return 'number';
    if (typeof value === 'boolean') return 'boolean';
    if (typeof value === 'string' && value.startsWith('=')) return 'formula';
    if (typeof value === 'string' && value.startsWith('#') && value.endsWith('!')) return 'error';
    return 'string';
  }

  /** 重建合并索引 */
  private rebuildMergeIndex(): void {
    this.mergeIndex.clear();
    for (const mc of this.mergedCells) {
      const masterKey = cellKey(mc.startRow, mc.startColumn);
      for (let r = mc.startRow; r <= mc.endRow; r++) {
        for (let c = mc.startColumn; c <= mc.endColumn; c++) {
          if (r === mc.startRow && c === mc.startColumn) continue;
          this.mergeIndex.set(cellKey(r, c), masterKey);
        }
      }
    }
  }

  /** 平移隐藏行: 行号 >= fromRow 的行偏移 delta (delta 可为负) */
  private shiftHiddenRows(fromRow: number, delta: number): void {
    if (delta === 0) return;
    const newSet = new Set<number>();
    for (const r of this.hiddenRows) {
      if (r >= fromRow) {
        const nr = r + delta;
        if (nr >= 0) newSet.add(nr);
      } else {
        newSet.add(r);
      }
    }
    this.hiddenRows = newSet;
  }

  /** 平移隐藏列: 列号 >= fromCol 的列偏移 delta (delta 可为负) */
  private shiftHiddenColumns(fromCol: number, delta: number): void {
    if (delta === 0) return;
    const newSet = new Set<number>();
    for (const c of this.hiddenColumns) {
      if (c >= fromCol) {
        const nc = c + delta;
        if (nc >= 0) newSet.add(nc);
      } else {
        newSet.add(c);
      }
    }
    this.hiddenColumns = newSet;
  }

  /** 移除 [startRow, endRow) 范围内的隐藏行 */
  private removeHiddenRowsInRange(startRow: number, endRow: number): void {
    const newSet = new Set<number>();
    for (const r of this.hiddenRows) {
      if (r < startRow || r >= endRow) {
        newSet.add(r);
      }
    }
    this.hiddenRows = newSet;
  }

  /** 移除 [startCol, endCol) 范围内的隐藏列 */
  private removeHiddenColumnsInRange(startCol: number, endCol: number): void {
    const newSet = new Set<number>();
    for (const c of this.hiddenColumns) {
      if (c < startCol || c >= endCol) {
        newSet.add(c);
      }
    }
    this.hiddenColumns = newSet;
  }

  /** 获取列名 (A, B, ..., Z, AA, AB, ...) */
  static columnToLetter(col: number): string {
    let result = '';
    let c = col;
    while (c >= 0) {
      result = String.fromCharCode(65 + (c % 26)) + result;
      c = Math.floor(c / 26) - 1;
    }
    return result;
  }

  /** 列名转列号 */
  static letterToColumn(letter: string): number {
    let result = 0;
    for (let i = 0; i < letter.length; i++) {
      result = result * 26 + (letter.charCodeAt(i) - 64);
    }
    return result - 1;
  }

  /** 获取单元格地址 (如 "A1", "B23") */
  static cellAddress(row: number, col: number): string {
    return `${SheetModel.columnToLetter(col)}${row + 1}`;
  }
}

// ============================================================
// 文档模型 — 管理多个工作表
// ============================================================

export class DocumentModel {
  private sheets: SheetModel[] = [];
  private activeSheetIndex: number = 0;
  definedNames: DefinedName[] = [];
  coreProperties: CoreProperties = {};

  constructor(data?: ZQSheetDocument) {
    if (data) {
      this.loadFromJSON(data);
    } else {
      // 创建默认空工作表
      this.addSheet(new SheetModel({ name: 'Sheet1', index: 0 }));
    }
  }

  /** 从 ZQ JSON 加载 */
  loadFromJSON(data: ZQSheetDocument): void {
    this.sheets = [];
    this.definedNames = data.definedNames ?? [];
    this.coreProperties = data.coreProperties ?? {};

    if (data.sheets && data.sheets.length > 0) {
      for (const sheetData of data.sheets) {
        this.sheets.push(new SheetModel(sheetData));
      }
    } else {
      this.addSheet(new SheetModel({ name: 'Sheet1', index: 0 }));
    }
    this.activeSheetIndex = 0;
  }

  /** 序列化为 ZQ JSON */
  toJSON(): ZQSheetDocument {
    return {
      sheets: this.sheets.map((s, i) => {
        s.index = i;
        return s.toJSON();
      }),
      definedNames: this.definedNames,
      coreProperties: this.coreProperties,
    };
  }

  /** 序列化为 JSON 字符串 */
  toJSONString(): string {
    return JSON.stringify(this.toJSON());
  }

  /** 获取当前活动工作表 */
  getActiveSheet(): SheetModel {
    return this.sheets[this.activeSheetIndex] ?? this.sheets[0];
  }

  /** 获取工作表数量 */
  getSheetCount(): number {
    return this.sheets.length;
  }

  /** 获取所有工作表 */
  getAllSheets(): SheetModel[] {
    return [...this.sheets];
  }

  /** 按索引获取工作表 */
  getSheet(index: number): SheetModel | null {
    return this.sheets[index] ?? null;
  }

  /** 按 ID 获取工作表 */
  getSheetById(id: string): SheetModel | null {
    return this.sheets.find(s => s.sheetId === id) ?? null;
  }

  /** 添加工作表 */
  addSheet(sheet: SheetModel): void {
    sheet.index = this.sheets.length;
    this.sheets.push(sheet);
  }

  /** 删除工作表 */
  removeSheet(index: number): void {
    if (this.sheets.length <= 1) return;
    this.sheets.splice(index, 1);
    // 重新索引
    this.sheets.forEach((s, i) => s.index = i);
    if (this.activeSheetIndex >= this.sheets.length) {
      this.activeSheetIndex = this.sheets.length - 1;
    }
  }

  /** 清空所有工作表 (用于协议加载前重置) */
  clearSheets(): void {
    this.sheets = [];
    this.activeSheetIndex = 0;
  }

  /** 设置活动工作表 */
  setActiveSheet(index: number): void {
    if (index >= 0 && index < this.sheets.length) {
      this.activeSheetIndex = index;
    }
  }

  /** 获取活动工作表索引 */
  getActiveSheetIndex(): number {
    return this.activeSheetIndex;
  }

  /** 设置核心属性 */
  setCoreProperties(props: Partial<CoreProperties>): void {
    this.coreProperties = { ...this.coreProperties, ...props };
  }
}
