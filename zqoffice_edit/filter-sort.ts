/**
 * 筛选排序引擎层 — 自动筛选 + 多列排序
 * 全部原创实现
 *
 * 职责:
 *   1. 自动筛选: 设置筛选范围、按列设置筛选条件、清除筛选、应用筛选(隐藏不匹配行)
 *   2. 排序: 单列排序、多列排序、获取行索引映射、应用排序(重排行数据)
 *   3. UI 辅助: 获取列唯一值列表(用于筛选下拉菜单)
 *
 * 与 SheetModel 集成约定:
 *   - 筛选不修改原始数据, 只控制行可见性(通过 hiddenRows 集合)
 *   - 排序直接修改 SheetModel 的行数据顺序(单元格数据 + 行高 + 合并区域一起交换)
 *   - 筛选范围的第一行视为表头, 不参与筛选匹配
 */

import { SheetModel, Cell, MergedCell } from './data-model';
import { SelectionRange } from './render-engine';

// ============================================================
// 类型定义
// ============================================================

/** 筛选条件类型 */
export type FilterConditionType =
  | 'contains'      // 文本包含
  | 'equals'        // 等于(文本/数字)
  | 'startsWith'    // 文本开头匹配
  | 'endsWith'      // 文本结尾匹配
  | 'greaterThan'   // 数字大于
  | 'lessThan'      // 数字小于
  | 'between'       // 数字区间 [value, value2]
  | 'notEqual';     // 不等于

/** 单列筛选条件 */
export interface FilterCondition {
  /** 列号 (0-based) */
  column: number;
  /** 条件类型 */
  type: FilterConditionType;
  /** 比较值 (文本或数字) */
  value: string | number;
  /** between 条件的第二个值 (仅 type='between' 时使用) */
  value2?: number;
}

/** 单列排序条件 */
export interface SortCondition {
  /** 列号 (0-based) */
  column: number;
  /** true=升序, false=降序 */
  ascending: boolean;
}

// ============================================================
// 筛选排序引擎
// ============================================================

export class FilterSortEngine {
  private sheet: SheetModel;

  /** 筛选范围 (null 表示未启用筛选) */
  private filterRange: SelectionRange | null = null;
  /** 各列的筛选条件, key = 列号 */
  private filterConditions = new Map<number, FilterCondition>();
  /** 被筛选隐藏的行号集合 */
  private hiddenRows = new Set<number>();

  /** 当前排序条件 (空数组表示未排序) */
  private sortConditions: SortCondition[] = [];
  /** 排序后的行索引映射: rowMapping[新行号] = 旧行号; null 表示未排序 */
  private rowMapping: number[] | null = null;

  constructor(sheet: SheetModel) {
    this.sheet = sheet;
  }

  // ============================================================
  // 筛选 — 范围管理
  // ============================================================

  /**
   * 设置筛选范围
   * 范围第一行视为表头, 不参与筛选匹配
   */
  setFilterRange(range: SelectionRange): void {
    // 规范化 (确保 start <= end)
    const startRow = Math.min(range.startRow, range.endRow);
    const endRow = Math.max(range.startRow, range.endRow);
    const startCol = Math.min(range.startCol, range.endCol);
    const endCol = Math.max(range.startCol, range.endCol);

    this.filterRange = { startRow, endRow, startCol, endCol };

    // 范围变化时清除超出新范围的列条件
    for (const col of Array.from(this.filterConditions.keys())) {
      if (col < startCol || col > endCol) {
        this.filterConditions.delete(col);
      }
    }

    // 重新计算隐藏行
    this.applyFilter();
  }

  /** 获取当前筛选范围 */
  getFilterRange(): SelectionRange | null {
    return this.filterRange ? { ...this.filterRange } : null;
  }

  /** 是否已启用筛选 */
  hasFilter(): boolean {
    return this.filterRange !== null;
  }

  // ============================================================
  // 筛选 — 条件管理
  // ============================================================

  /**
   * 设置某列的筛选条件
   * @param condition 筛选条件 (column 必须在当前筛选范围内)
   */
  setColumnFilter(condition: FilterCondition): void {
    if (!this.filterRange) {
      throw new Error('请先调用 setFilterRange 设置筛选范围');
    }
    if (condition.column < this.filterRange.startCol || condition.column > this.filterRange.endCol) {
      throw new Error(`列 ${condition.column} 不在筛选范围内`);
    }
    // between 类型必须提供 value2
    if (condition.type === 'between' && condition.value2 === undefined) {
      throw new Error("between 条件必须提供 value2");
    }

    this.filterConditions.set(condition.column, { ...condition });
    this.applyFilter();
  }

  /**
   * 清除指定列的筛选条件
   */
  clearColumnFilter(column: number): void {
    if (this.filterConditions.delete(column)) {
      this.applyFilter();
    }
  }

  /**
   * 清除所有筛选条件
   */
  clearFilter(): void {
    this.filterConditions.clear();
    this.hiddenRows.clear();
  }

  /** 移除筛选 (清除条件 + 范围 + 隐藏状态) */
  removeFilter(): void {
    this.filterRange = null;
    this.filterConditions.clear();
    this.hiddenRows.clear();
  }

  /** 获取所有筛选条件 */
  getFilterConditions(): FilterCondition[] {
    return Array.from(this.filterConditions.values()).map(c => ({ ...c }));
  }

  /** 获取指定列的筛选条件 */
  getColumnFilter(column: number): FilterCondition | null {
    const cond = this.filterConditions.get(column);
    return cond ? { ...cond } : null;
  }

  // ============================================================
  // 筛选 — 应用与查询
  // ============================================================

  /**
   * 应用筛选 — 重新计算隐藏行集合
   * 遍历筛选范围内的每一行 (除表头), 不满足任一列条件的行加入 hiddenRows
   * 范围外的行不受影响 (始终可见)
   */
  applyFilter(): void {
    this.hiddenRows.clear();

    if (!this.filterRange) return;
    // 没有条件, 无需隐藏
    if (this.filterConditions.size === 0) return;

    const { startRow, endRow } = this.filterRange;
    // 第一行为表头, 从第二行开始匹配
    const dataStartRow = startRow + 1;

    for (let row = dataStartRow; row <= endRow; row++) {
      if (!this.matchAllConditions(row)) {
        this.hiddenRows.add(row);
      }
    }
  }

  /**
   * 获取筛选后可见的数据行号列表
   * @returns 行号数组 (按原顺序)
   */
  getFilteredRows(): number[] {
    if (!this.filterRange) return [];
    const { startRow, endRow } = this.filterRange;
    const dataStartRow = startRow + 1;
    const result: number[] = [];
    for (let row = dataStartRow; row <= endRow; row++) {
      if (!this.hiddenRows.has(row)) {
        result.push(row);
      }
    }
    return result;
  }

  /** 判断指定行是否被筛选隐藏 */
  isRowHidden(row: number): boolean {
    return this.hiddenRows.has(row);
  }

  /** 获取所有被隐藏的行号 */
  getHiddenRows(): number[] {
    return Array.from(this.hiddenRows).sort((a, b) => a - b);
  }

  /** 判断某行是否在筛选范围内 */
  isRowInFilterRange(row: number): boolean {
    if (!this.filterRange) return false;
    return row >= this.filterRange.startRow && row <= this.filterRange.endRow;
  }

  /**
   * 检查某行是否满足所有筛选条件
   * 多列条件之间为 AND 关系 (所有列都必须满足)
   */
  private matchAllConditions(row: number): boolean {
    for (const condition of this.filterConditions.values()) {
      if (!this.matchCondition(row, condition)) {
        return false;
      }
    }
    return true;
  }

  /**
   * 检查某行是否满足单个筛选条件
   */
  private matchCondition(row: number, condition: FilterCondition): boolean {
    const cellValue = this.sheet.getCellValue(row, condition.column);
    // 空单元格: 文本类条件不匹配 (除 notEqual 外), 数值类条件视为 0/空字符串
    if (cellValue === null || cellValue === undefined || cellValue === '') {
      // notEqual: 空值视为不等于任何非空值, 匹配
      if (condition.type === 'notEqual') return true;
      // 其他条件: 空值不匹配
      return false;
    }

    switch (condition.type) {
      case 'contains':
        return String(cellValue).includes(String(condition.value));
      case 'equals':
        return this.equalsValue(cellValue, condition.value);
      case 'startsWith':
        return String(cellValue).startsWith(String(condition.value));
      case 'endsWith':
        return String(cellValue).endsWith(String(condition.value));
      case 'greaterThan':
        return this.toNumber(cellValue) > this.toNumber(condition.value);
      case 'lessThan':
        return this.toNumber(cellValue) < this.toNumber(condition.value);
      case 'between': {
        const num = this.toNumber(cellValue);
        const min = this.toNumber(condition.value);
        const max = condition.value2 !== undefined ? this.toNumber(condition.value2) : num;
        return num >= Math.min(min, max) && num <= Math.max(min, max);
      }
      case 'notEqual':
        return !this.equalsValue(cellValue, condition.value);
      default:
        return true;
    }
  }

  /** 值相等比较 (区分数字与字符串) */
  private equalsValue(cellValue: string | number | boolean, target: string | number): boolean {
    if (typeof cellValue === 'number' && typeof target === 'number') {
      return cellValue === target;
    }
    // 数字单元格与数字字符串比较: 尝试数值比较
    if (typeof cellValue === 'number' && typeof target === 'string') {
      const targetNum = Number(target);
      if (!Number.isNaN(targetNum)) return cellValue === targetNum;
    }
    return String(cellValue) === String(target);
  }

  /** 转换为数字 (无法转换返回 NaN) */
  private toNumber(value: string | number | boolean): number {
    if (typeof value === 'number') return value;
    if (typeof value === 'boolean') return value ? 1 : 0;
    return Number(String(value).trim());
  }

  // ============================================================
  // UI 辅助 — 列唯一值 (用于筛选下拉菜单)
  // ============================================================

  /**
   * 获取指定列在筛选范围内的唯一值列表
   * @param column 列号 (0-based)
   * @returns 唯一值数组 (已去重, 排除空值; 数字优先按数值排序, 文本按字典序)
   */
  getUniqueValues(column: number): Array<string | number> {
    if (!this.filterRange) return [];

    const { startRow, endRow } = this.filterRange;
    const dataStartRow = startRow + 1;
    const seen = new Set<string>();
    const values: Array<string | number> = [];

    for (let row = dataStartRow; row <= endRow; row++) {
      // 隐藏行也参与唯一值统计
      const v = this.sheet.getCellValue(row, column);
      if (v === null || v === undefined || v === '') continue;

      const key = String(v);
      if (!seen.has(key)) {
        seen.add(key);
        // 保留原始类型: 数字存为 number, 其他存为 string
        if (typeof v === 'number') {
          values.push(v);
        } else if (typeof v === 'boolean') {
          values.push(v ? 'TRUE' : 'FALSE');
        } else {
          values.push(String(v));
        }
      }
    }

    // 排序: 全是数字按数值升序, 否则按字符串字典序
    const allNumeric = values.every(v => typeof v === 'number');
    if (allNumeric) {
      (values as number[]).sort((a, b) => (a as number) - (b as number));
    } else {
      (values as string[]).sort((a, b) => String(a).localeCompare(String(b), 'zh-CN'));
    }

    return values;
  }

  // ============================================================
  // 排序 — 单列 / 多列
  // ============================================================

  /**
   * 按单列排序
   * @param column 排序依据列 (0-based)
   * @param ascending true=升序, false=降序
   * @param range 排序范围 (可选, 默认使用筛选范围或整个数据区域)
   */
  sortByColumn(column: number, ascending: boolean, range?: SelectionRange): void {
    this.sortByColumns([{ column, ascending }], range);
  }

  /**
   * 按多列排序
   * @param conditions 排序条件数组 (按优先级从高到低)
   * @param range 排序范围 (可选, 默认使用筛选范围或整个数据区域)
   */
  sortByColumns(conditions: SortCondition[], range?: SelectionRange): void {
    if (conditions.length === 0) {
      this.clearSort();
      return;
    }

    const sortRange = this.resolveSortRange(range);
    this.sortConditions = conditions.map(c => ({ ...c }));

    // 检查合并单元格: 排序范围内不允许有合并区域跨越 (会破坏数据完整性)
    const conflictingMerge = this.findMergeConflict(sortRange);
    if (conflictingMerge) {
      throw new Error(
        `排序范围内存在合并单元格 (${conflictingMerge.startRow},${conflictingMerge.startColumn})-(${conflictingMerge.endRow},${conflictingMerge.endColumn}), 请先取消合并再排序`
      );
    }

    // 生成旧行索引列表 (不含表头)
    const dataStartRow = sortRange.startRow + 1;
    const dataEndRow = sortRange.endRow;
    const oldRows: number[] = [];
    for (let r = dataStartRow; r <= dataEndRow; r++) {
      oldRows.push(r);
    }

    // 排序: 多条件比较
    oldRows.sort((rowA, rowB) => this.compareRows(rowA, rowB, conditions));

    // 构建 rowMapping: 新行号 -> 旧行号
    // 表头行 (startRow) 保持不变, 数据行按排序结果重排
    const mapping: number[] = [];
    for (let r = 0; r < this.sheet.rowCount; r++) {
      if (r < dataStartRow) {
        // 表头及之前的行: 不动
        mapping.push(r);
      } else if (r <= dataEndRow) {
        // 数据行: 按排序结果
        mapping.push(oldRows[r - dataStartRow]);
      } else {
        // 范围后的行: 不动
        mapping.push(r);
      }
    }

    this.rowMapping = mapping;
  }

  /**
   * 应用排序 — 实际修改 SheetModel 的行数据顺序
   * 注意: 此操作会改变原始数据, 不可通过 clearSort 撤销
   */
  applySort(): void {
    if (!this.rowMapping) {
      throw new Error('当前没有排序映射, 请先调用 sortByColumn/sortByColumns');
    }

    const mapping = this.rowMapping;
    this.rowMapping = null; // 应用后清除映射 (数据已重排)

    // 1. 收集所有单元格的深拷贝 (含样式和公式)
    const allCells = this.sheet.getAllCells();
    const cellCopies: Cell[] = allCells.map(c => ({
      ...c,
      style: c.style ? { ...c.style } : undefined,
    }));

    // 2. 收集所有行高的深拷贝
    const rowHeights = new Map<number, number>();
    for (let r = 0; r < this.sheet.rowCount; r++) {
      rowHeights.set(r, this.sheet.getRowHeight(r));
    }

    // 3. 收集所有合并区域 (排序范围外的不动, 范围内的应已在 sortByColumns 检查过无冲突)
    const allMerges = this.sheet.getAllMerges();

    // 4. 构建反向映射: 旧行号 -> 新行号
    const oldToNew = new Map<number, number>();
    for (let newRow = 0; newRow < mapping.length; newRow++) {
      oldToNew.set(mapping[newRow], newRow);
    }

    // 5. 删除所有原单元格
    for (const cell of allCells) {
      this.sheet.deleteCell(cell.row, cell.column);
    }

    // 6. 按新行号重写所有单元格
    for (const cell of cellCopies) {
      const newRow = oldToNew.get(cell.row) ?? cell.row;
      if (cell.formula) {
        this.sheet.setCellFormula(newRow, cell.column, cell.formula);
      } else {
        this.sheet.setCellValue(newRow, cell.column, cell.value, cell.type);
      }
      if (cell.style) {
        this.sheet.setCellStyle(newRow, cell.column, cell.style);
      }
    }

    // 7. 行高跟随行数据交换 (旧行高度写到新行位置)
    for (let r = 0; r < this.sheet.rowCount; r++) {
      const oldRow = mapping[r];
      const oldHeight = rowHeights.get(oldRow);
      if (oldHeight !== undefined) {
        this.sheet.setRowHeight(r, oldHeight);
      }
    }

    // 8. 合并区域: 范围内的按映射重排行号, 范围外的不动
    //    (sortByColumns 已确保范围内无合并区域, 这里仅处理跨越范围边界的情况)
    for (const mc of allMerges) {
      const newStartRow = oldToNew.get(mc.startRow);
      const newEndRow = oldToNew.get(mc.endRow);
      if (newStartRow !== undefined && newEndRow !== undefined) {
        this.sheet.addMerge(
          Math.min(newStartRow, newEndRow),
          mc.startColumn,
          Math.max(newStartRow, newEndRow),
          mc.endColumn
        );
      }
    }

    // 9. 排序后清除筛选状态 (行号已变化, 旧筛选映射失效)
    this.clearFilter();
  }

  /** 清除排序 (仅清除映射, 不回滚已应用的数据) */
  clearSort(): void {
    this.sortConditions = [];
    this.rowMapping = null;
  }

  /** 获取当前排序条件 */
  getSortConditions(): SortCondition[] {
    return this.sortConditions.map(c => ({ ...c }));
  }

  /**
   * 获取排序后的行索引映射
   * @returns 映射数组 (rowMapping[新行号] = 旧行号); 未排序时返回 null
   */
  getSortMapping(): number[] | null {
    return this.rowMapping ? [...this.rowMapping] : null;
  }

  /** 是否有待应用的排序 */
  hasPendingSort(): boolean {
    return this.rowMapping !== null;
  }

  // ============================================================
  // 排序 — 内部辅助
  // ============================================================

  /** 解析排序范围: 优先用传入参数, 其次用筛选范围, 最后用整个数据区域 */
  private resolveSortRange(range?: SelectionRange): SelectionRange {
    if (range) {
      return {
        startRow: Math.min(range.startRow, range.endRow),
        endRow: Math.max(range.startRow, range.endRow),
        startCol: Math.min(range.startCol, range.endCol),
        endCol: Math.max(range.startCol, range.endCol),
      };
    }
    if (this.filterRange) {
      return { ...this.filterRange };
    }
    // 默认整个数据区域 (第 0 行为表头)
    return {
      startRow: 0,
      endRow: this.sheet.rowCount - 1,
      startCol: 0,
      endCol: this.sheet.columnCount - 1,
    };
  }

  /**
   * 查找排序范围内的合并单元格冲突
   * 冲突定义: 合并区域部分落在排序数据行范围内 (跨越表头与数据行, 或跨越范围边界)
   * 完全在数据行范围内的合并区域也算冲突 (排序会破坏)
   */
  private findMergeConflict(range: SelectionRange): MergedCell | null {
    const dataStartRow = range.startRow + 1;
    const dataEndRow = range.endRow;
    const merges = this.sheet.getAllMerges();
    for (const mc of merges) {
      // 合并区域与数据行范围有交集
      const overlapsDataRows = !(mc.endRow < dataStartRow || mc.startRow > dataEndRow);
      // 合并区域与排序列范围有交集
      const overlapsCols = !(mc.endColumn < range.startCol || mc.startColumn > range.endCol);
      if (overlapsDataRows && overlapsCols) {
        return mc;
      }
    }
    return null;
  }

  /**
   * 多条件行比较
   * @returns 负数 a<b, 0 相等, 正数 a>b (按条件升序/降序调整)
   */
  private compareRows(rowA: number, rowB: number, conditions: SortCondition[]): number {
    for (const cond of conditions) {
      const cmp = this.compareCells(rowA, rowB, cond.column);
      if (cmp !== 0) {
        return cond.ascending ? cmp : -cmp;
      }
    }
    return 0; // 所有条件都相等, 保持原顺序 (sort 稳定)
  }

  /**
   * 比较两行指定列的单元格值
   * 排序规则:
   *   - 空值排最后 (无论升降序)
   *   - 数字按数值比较
   *   - 文本按字典序 (localeCompare 支持中文)
   *   - 数字 < 文本 (类型不同时数字优先)
   */
  private compareCells(rowA: number, rowB: number, column: number): number {
    const valA = this.sheet.getCellValue(rowA, column);
    const valB = this.sheet.getCellValue(rowB, column);

    const aEmpty = valA === null || valA === undefined || valA === '';
    const bEmpty = valB === null || valB === undefined || valB === '';

    // 空值排最后
    if (aEmpty && bEmpty) return 0;
    if (aEmpty) return 1;
    if (bEmpty) return -1;

    const aIsNum = typeof valA === 'number' || this.isNumericString(valA);
    const bIsNum = typeof valB === 'number' || this.isNumericString(valB);

    if (aIsNum && bIsNum) {
      const numA = this.toNumber(valA);
      const numB = this.toNumber(valB);
      if (numA < numB) return -1;
      if (numA > numB) return 1;
      return 0;
    }
    if (aIsNum && !bIsNum) return -1; // 数字排在文本前
    if (!aIsNum && bIsNum) return 1;

    // 都是文本: localeCompare 支持中文拼音排序
    return String(valA).localeCompare(String(valB), 'zh-CN');
  }

  /** 判断值是否为数值字符串 */
  private isNumericString(value: string | number | boolean): boolean {
    if (typeof value === 'number') return true;
    if (typeof value === 'boolean') return false;
    if (typeof value === 'string') {
      const trimmed = value.trim();
      if (trimmed === '') return false;
      return !Number.isNaN(Number(trimmed));
    }
    return false;
  }

  // ============================================================
  // 工作表切换
  // ============================================================

  /** 切换工作表时重置引擎状态 */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
    this.filterRange = null;
    this.filterConditions.clear();
    this.hiddenRows.clear();
    this.sortConditions = [];
    this.rowMapping = null;
  }

  /** 获取当前关联的工作表 */
  getSheet(): SheetModel {
    return this.sheet;
  }
}
