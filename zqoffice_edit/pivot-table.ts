/**
 * 透视表引擎层 — 数据透视表的创建/计算/渲染
 * 全部原创实现
 *
 * 职责:
 *   1. 透视表 CRUD: 创建/删除/更新/查询
 *   2. 数据聚合计算: 按行字段/列字段分组, 对值字段执行 sum/count/average/max/min
 *   3. 渲染到工作表: 将透视表结果写入 SheetModel 的指定区域 (含表头/总计)
 *
 * 与 SheetModel 集成约定:
 *   - 源数据区域第一行视为表头 (字段名)
 *   - 渲染目标区域由 targetRange.startRow/startCol 指定, 向右下方扩展
 *   - 渲染会覆盖目标区域的单元格内容与样式
 */

import { SheetModel, CellStyle, BorderStyle } from './data-model';

// ============================================================
// 类型定义
// ============================================================

/** 聚合函数类型 */
export type ValueFunction = 'sum' | 'count' | 'average' | 'max' | 'min';

/** 值字段配置 */
export interface PivotValueField {
  /** 字段名 (对应源数据表头) */
  field: string;
  /** 聚合函数 */
  function: ValueFunction;
}

/** 筛选条件 */
export interface PivotFilter {
  /** 字段名 */
  field: string;
  /** 允许的值列表 (在该列表中的行才参与聚合) */
  values: string[];
}

/** 区域范围 (0-based, 含两端) */
export interface PivotRange {
  startRow: number;
  startCol: number;
  endRow: number;
  endCol: number;
}

/** 透视表配置 */
export interface PivotTableConfig {
  /** 唯一 ID */
  id: string;
  /** 透视表名称 */
  name: string;
  /** 源数据区域 (第一行为表头) */
  sourceRange: PivotRange;
  /** 行字段名列表 */
  rows: string[];
  /** 列字段名列表 */
  columns: string[];
  /** 值字段列表 */
  values: PivotValueField[];
  /** 筛选条件 (可选) */
  filters?: PivotFilter[];
  /** 渲染目标起始位置 */
  targetRange: { startRow: number; startCol: number };
}

/** 透视表计算结果 */
export interface PivotTableData {
  /** 行标题 (支持多级, 每个元素是该行的字段值路径) */
  rowHeaders: string[][];
  /** 列标题 (支持多级, 每个元素是该列的字段值路径 + 值标签) */
  columnHeaders: string[][];
  /** 数据值矩阵 cells[i][j] 对应 rowHeaders[i] × columnHeaders[j] */
  cells: number[][];
  /** 总计行 (每列之和), 长度 = columnHeaders.length */
  grandTotalRow: number[];
  /** 总计列 (每行之和), 长度 = rowHeaders.length */
  grandTotalColumn: number[];
}

/** 透视表条目 (配置 + 上次计算结果) */
export interface PivotTableEntry {
  config: PivotTableConfig;
  data: PivotTableData;
}

// ============================================================
// 辅助函数
// ============================================================

/** 将单元格值转为数字, 无法转换返回 null */
function toNumber(v: string | number | boolean | null | undefined): number | null {
  if (v === null || v === undefined || v === '') return null;
  if (typeof v === 'number') return Number.isNaN(v) ? null : v;
  if (typeof v === 'boolean') return v ? 1 : 0;
  if (typeof v === 'string') {
    const n = Number(v);
    return Number.isNaN(n) ? null : n;
  }
  return null;
}

/** 字符串数组字典序比较 (用于排序行键/列键) */
function compareStringArrays(a: string[], b: string[]): number {
  const len = Math.min(a.length, b.length);
  for (let i = 0; i < len; i++) {
    if (a[i] < b[i]) return -1;
    if (a[i] > b[i]) return 1;
  }
  return a.length - b.length;
}

/** 生成唯一 ID */
function generateId(prefix: string): string {
  return `${prefix}_${Date.now()}_${Math.floor(Math.random() * 1e6)}`;
}

// ============================================================
// 透视表引擎
// ============================================================

export class PivotTableEngine {
  private sheet: SheetModel;
  /** 透视表配置表, key = id */
  private pivotTables = new Map<string, PivotTableEntry>();

  constructor(sheet: SheetModel) {
    this.sheet = sheet;
  }

  // ============================================================
  // 透视表 CRUD
  // ============================================================

  /**
   * 创建透视表
   * @param config 透视表配置 (id/name 可省略, 自动生成)
   * @returns 新建透视表的 ID
   */
  createPivotTable(config: Partial<PivotTableConfig> & {
    sourceRange: PivotRange;
    rows: string[];
    columns: string[];
    values: PivotValueField[];
    targetRange: { startRow: number; startCol: number };
  }): string {
    const id = config.id ?? generateId('pivot');
    const fullConfig: PivotTableConfig = {
      id,
      name: config.name ?? `透视表${this.pivotTables.size + 1}`,
      sourceRange: { ...config.sourceRange },
      rows: [...config.rows],
      columns: [...config.columns],
      values: config.values.map(v => ({ ...v })),
      filters: config.filters?.map(f => ({ ...f, values: [...f.values] })),
      targetRange: { ...config.targetRange },
    };
    const data = this.calculate(fullConfig);
    this.pivotTables.set(id, { config: fullConfig, data });
    return id;
  }

  /**
   * 删除透视表
   * @param id 透视表 ID
   * @returns 是否删除成功
   */
  removePivotTable(id: string): boolean {
    return this.pivotTables.delete(id);
  }

  /**
   * 更新透视表配置 (合并更新)
   * @param id 透视表 ID
   * @param configPatch 配置补丁
   * @returns 是否更新成功
   */
  updatePivotTable(id: string, configPatch: Partial<Omit<PivotTableConfig, 'id'>>): boolean {
    const entry = this.pivotTables.get(id);
    if (!entry) return false;
    const newConfig: PivotTableConfig = {
      ...entry.config,
      ...configPatch,
      id,
      sourceRange: configPatch.sourceRange ? { ...configPatch.sourceRange } : entry.config.sourceRange,
      rows: configPatch.rows ? [...configPatch.rows] : entry.config.rows,
      columns: configPatch.columns ? [...configPatch.columns] : entry.config.columns,
      values: configPatch.values ? configPatch.values.map(v => ({ ...v })) : entry.config.values,
      filters: configPatch.filters
        ? configPatch.filters.map(f => ({ ...f, values: [...f.values] }))
        : entry.config.filters,
      targetRange: configPatch.targetRange ? { ...configPatch.targetRange } : entry.config.targetRange,
    };
    const data = this.calculate(newConfig);
    this.pivotTables.set(id, { config: newConfig, data });
    return true;
  }

  /**
   * 获取透视表 (配置 + 计算结果)
   * @param id 透视表 ID
   */
  getPivotTable(id: string): PivotTableEntry | null {
    const entry = this.pivotTables.get(id);
    if (!entry) return null;
    return {
      config: this.cloneConfig(entry.config),
      data: this.cloneData(entry.data),
    };
  }

  /** 获取所有透视表 */
  getAllPivotTables(): PivotTableEntry[] {
    return Array.from(this.pivotTables.values()).map(e => ({
      config: this.cloneConfig(e.config),
      data: this.cloneData(e.data),
    }));
  }

  /** 获取透视表数量 */
  getPivotTableCount(): number {
    return this.pivotTables.size;
  }

  /** 当源数据变化时, 重新计算所有透视表 */
  recalculateAll(): void {
    for (const [id, entry] of this.pivotTables) {
      const data = this.calculate(entry.config);
      this.pivotTables.set(id, { config: entry.config, data });
    }
  }

  // ============================================================
  // 数据聚合计算 — 核心算法
  // ============================================================

  /**
   * 根据配置计算透视表数据
   * 算法:
   *   1. 从源区域读取表头, 建立字段→列号映射
   *   2. 读取数据行, 应用筛选条件
   *   3. 按行字段/列字段分组, 收集每个 (行键, 列键) 桶中各值字段的数值
   *   4. 对每个桶执行聚合函数
   *   5. 计算总计行/列
   * @param config 透视表配置
   */
  calculate(config: PivotTableConfig): PivotTableData {
    const sheet = this.sheet;
    const { sourceRange, rows, columns, values, filters } = config;

    // 空值字段时返回空结果
    if (!values || values.length === 0) {
      return {
        rowHeaders: [],
        columnHeaders: [],
        cells: [],
        grandTotalRow: [],
        grandTotalColumn: [],
      };
    }

    // 1. 读取表头, 建立字段→列号映射
    const fieldToCol = new Map<string, number>();
    for (let c = sourceRange.startCol; c <= sourceRange.endCol; c++) {
      const v = sheet.getCellValue(sourceRange.startRow, c);
      if (v !== null && v !== '') {
        fieldToCol.set(String(v), c);
      }
    }

    // 2. 读取数据行 (源区域第一行为表头, 数据从第二行开始)
    interface SourceRow {
      [field: string]: string | number | boolean | null;
    }
    const dataRows: SourceRow[] = [];
    for (let r = sourceRange.startRow + 1; r <= sourceRange.endRow; r++) {
      const row: SourceRow = {};
      let hasData = false;
      for (const [field, col] of fieldToCol) {
        const v = sheet.getCellValue(r, col);
        row[field] = v;
        if (v !== null && v !== '') hasData = true;
      }
      if (hasData) dataRows.push(row);
    }

    // 3. 应用筛选条件
    let filtered = dataRows;
    if (filters && filters.length > 0) {
      filtered = dataRows.filter(row =>
        filters.every(f => {
          const v = row[f.field];
          const vStr = v === null ? '' : String(v);
          return f.values.includes(vStr);
        })
      );
    }

    // 4. 计算行键/列键
    const rowKeyOf = (row: SourceRow): string[] =>
      rows.map(f => {
        const v = row[f];
        return v === null ? '' : String(v);
      });
    const colKeyOf = (row: SourceRow): string[] =>
      columns.map(f => {
        const v = row[f];
        return v === null ? '' : String(v);
      });

    // 收集唯一行键/列键
    const rowKeySet = new Set<string>();
    const colKeySet = new Set<string>();
    for (const row of filtered) {
      rowKeySet.add(JSON.stringify(rowKeyOf(row)));
      colKeySet.add(JSON.stringify(colKeyOf(row)));
    }

    // 排序行键/列键
    const rowKeys: string[][] = Array.from(rowKeySet)
      .map(s => JSON.parse(s) as string[])
      .sort(compareStringArrays);
    const colKeys: string[][] = Array.from(colKeySet)
      .map(s => JSON.parse(s) as string[])
      .sort(compareStringArrays);

    // 行标题: rows 为空时使用 "总计" 作为唯一行
    const rowHeaders: string[][] = rows.length > 0 ? rowKeys : [['总计']];
    // 行键 (用于聚合匹配): rows 为空时使用 [] 作为唯一键 (匹配所有行)
    const rowKeysForMatch: string[][] = rows.length > 0 ? rowKeys : [[]];
    // 列键: columns 为空时使用 [] 作为唯一键
    const colKeysForMatch: string[][] = columns.length > 0 ? colKeys : [[]];

    // 5. 构建列标题 = 列字段值路径 ++ 值标签
    const valueLabels = values.map(v => this.valueLabel(v));
    const columnHeaders: string[][] = [];
    for (const ck of colKeysForMatch) {
      for (const vl of valueLabels) {
        columnHeaders.push([...ck, vl]);
      }
    }

    // 6. 预分组: 每个 (行键索引, 列键索引) 桶收集各值字段的数值与非空计数
    const rowKeyIndex = new Map<string, number>();
    rowKeysForMatch.forEach((k, i) => rowKeyIndex.set(JSON.stringify(k), i));
    const colKeyIndex = new Map<string, number>();
    colKeysForMatch.forEach((k, i) => colKeyIndex.set(JSON.stringify(k), i));

    interface Bucket {
      /** 各值字段的数值数组 (用于 sum/average/max/min) */
      nums: number[][];
      /** 各值字段的非空值计数 (用于 count) */
      counts: number[];
    }
    const bucketCount = rowKeysForMatch.length * colKeysForMatch.length;
    const buckets: Bucket[] = [];
    for (let i = 0; i < bucketCount; i++) {
      buckets.push({ nums: values.map(() => []), counts: values.map(() => 0) });
    }

    for (const row of filtered) {
      const rk = JSON.stringify(rowKeyOf(row));
      const ck = JSON.stringify(colKeyOf(row));
      const ri = rowKeyIndex.get(rk);
      const ci = colKeyIndex.get(ck);
      if (ri === undefined || ci === undefined) continue;
      const bucket = buckets[ri * colKeysForMatch.length + ci];
      for (let vi = 0; vi < values.length; vi++) {
        const raw = row[values[vi].field];
        if (raw !== null && raw !== '') {
          bucket.counts[vi]++;
        }
        const n = toNumber(raw);
        if (n !== null) {
          bucket.nums[vi].push(n);
        }
      }
    }

    // 7. 聚合, 填充 cells 矩阵
    // cells[ri][colIdx], colIdx = ci * values.length + vi
    const cells: number[][] = [];
    for (let ri = 0; ri < rowKeysForMatch.length; ri++) {
      cells.push(new Array(columnHeaders.length).fill(0));
    }
    for (let ri = 0; ri < rowKeysForMatch.length; ri++) {
      for (let ci = 0; ci < colKeysForMatch.length; ci++) {
        const bucket = buckets[ri * colKeysForMatch.length + ci];
        for (let vi = 0; vi < values.length; vi++) {
          const colIdx = ci * values.length + vi;
          cells[ri][colIdx] = this.aggregate(bucket.nums[vi], bucket.counts[vi], values[vi].function);
        }
      }
    }

    // 8. 计算总计行/列
    // grandTotalRow[j] = 第 j 列的所有行之和
    // grandTotalColumn[i] = 第 i 行的所有列之和
    // 注: 对 sum/count 为精确值; 对 average/max/min 为显示用汇总值
    const grandTotalRow = new Array(columnHeaders.length).fill(0);
    const grandTotalColumn = new Array(rowHeaders.length).fill(0);
    for (let i = 0; i < rowHeaders.length; i++) {
      for (let j = 0; j < columnHeaders.length; j++) {
        const v = cells[i][j];
        grandTotalRow[j] += v;
        grandTotalColumn[i] += v;
      }
    }

    return { rowHeaders, columnHeaders, cells, grandTotalRow, grandTotalColumn };
  }

  // ============================================================
  // 渲染到工作表
  // ============================================================

  /**
   * 将指定透视表渲染到目标区域
   * @param id 透视表 ID
   * @param targetStart 目标起始位置 (可选, 默认使用配置中的 targetRange)
   * @returns 实际写入的区域范围; 若透视表不存在返回 null
   */
  renderTo(
    id: string,
    targetStart?: { startRow: number; startCol: number }
  ): PivotRange | null {
    const entry = this.pivotTables.get(id);
    if (!entry) return null;
    const start = targetStart ?? entry.config.targetRange;
    return this.renderData(entry.data, entry.config, start);
  }

  /**
   * 渲染透视表数据到指定起始位置
   * 布局:
   *   - 左上角: 行字段名 + 透视表名
   *   - 顶部: 列标题 (多级)
   *   - 左侧: 行标题 (多级)
   *   - 中间: 数据单元格
   *   - 底部: 总计行
   *   - 右侧: 总计列
   */
  private renderData(
    data: PivotTableData,
    config: PivotTableConfig,
    start: { startRow: number; startCol: number }
  ): PivotRange {
    const sheet = this.sheet;
    const { rowHeaders, columnHeaders, cells, grandTotalRow, grandTotalColumn } = data;

    const rowLevels = Math.max(config.rows.length, 1);          // 行标题占的列数
    const colLevels = config.columns.length + 1;                // 列标题占的行数 (列字段 + 值标签)
    const totalCols = rowLevels + columnHeaders.length + 1;     // +1 为总计列
    const totalRows = colLevels + rowHeaders.length + 1;        // +1 为总计行

    const baseRow = start.startRow;
    const baseCol = start.startCol;

    // ---- 1. 左上角区域: 透视表名 (顶部) + 行字段名 (底部) ----
    sheet.setCellValue(baseRow, baseCol, config.name);
    sheet.setCellStyle(baseRow, baseCol, this.headerStyle());
    this.applyBorder(baseRow, baseCol, baseRow, baseCol);

    // 行字段名写在列标题最后一行 (紧贴数据行)
    const fieldNameRow = baseRow + colLevels - 1;
    if (config.rows.length > 0) {
      for (let i = 0; i < config.rows.length; i++) {
        sheet.setCellValue(fieldNameRow, baseCol + i, config.rows[i]);
        sheet.setCellStyle(fieldNameRow, baseCol + i, this.headerStyle());
        this.applyBorder(fieldNameRow, baseCol + i, fieldNameRow, baseCol + i);
      }
    } else {
      sheet.setCellValue(fieldNameRow, baseCol, '行标签');
      sheet.setCellStyle(fieldNameRow, baseCol, this.headerStyle());
      this.applyBorder(fieldNameRow, baseCol, fieldNameRow, baseCol);
    }

    // ---- 2. 列标题 (多级) ----
    for (let l = 0; l < colLevels; l++) {
      for (let j = 0; j < columnHeaders.length; j++) {
        const r = baseRow + l;
        const c = baseCol + rowLevels + j;
        // columnHeaders[j] 长度 = colLevels; 若不足则填空
        const val = l < columnHeaders[j].length ? columnHeaders[j][l] : '';
        sheet.setCellValue(r, c, val);
        sheet.setCellStyle(r, c, this.headerStyle());
        this.applyBorder(r, c, r, c);
      }
    }
    // 总计列的列标题
    const totalColHeaderRow = baseRow + colLevels - 1;
    sheet.setCellValue(totalColHeaderRow, baseCol + rowLevels + columnHeaders.length, '总计');
    sheet.setCellStyle(totalColHeaderRow, baseCol + rowLevels + columnHeaders.length, this.headerStyle());
    this.applyBorder(
      totalColHeaderRow,
      baseCol + rowLevels + columnHeaders.length,
      totalColHeaderRow,
      baseCol + rowLevels + columnHeaders.length
    );

    // ---- 3. 数据行 ----
    for (let i = 0; i < rowHeaders.length; i++) {
      const r = baseRow + colLevels + i;
      // 行标题
      const rh = rowHeaders[i];
      for (let k = 0; k < rowLevels; k++) {
        const v = k < rh.length ? rh[k] : '';
        sheet.setCellValue(r, baseCol + k, v);
        sheet.setCellStyle(r, baseCol + k, this.rowHeaderStyle());
        this.applyBorder(r, baseCol + k, r, baseCol + k);
      }
      // 数据单元格
      for (let j = 0; j < columnHeaders.length; j++) {
        const c = baseCol + rowLevels + j;
        const num = cells[i][j];
        sheet.setCellValue(r, c, num, 'number');
        sheet.setCellStyle(r, c, this.dataStyle());
        this.applyBorder(r, c, r, c);
      }
      // 总计列
      const gtCol = baseCol + rowLevels + columnHeaders.length;
      sheet.setCellValue(r, gtCol, grandTotalColumn[i], 'number');
      sheet.setCellStyle(r, gtCol, this.totalStyle());
      this.applyBorder(r, gtCol, r, gtCol);
    }

    // ---- 4. 总计行 ----
    const totalRow = baseRow + colLevels + rowHeaders.length;
    sheet.setCellValue(totalRow, baseCol, '总计');
    sheet.setCellStyle(totalRow, baseCol, this.totalStyle());
    this.applyBorder(totalRow, baseCol, totalRow, baseCol);
    // 总计行其余行标题列留空 (带样式)
    for (let k = 1; k < rowLevels; k++) {
      sheet.setCellValue(totalRow, baseCol + k, '');
      sheet.setCellStyle(totalRow, baseCol + k, this.totalStyle());
      this.applyBorder(totalRow, baseCol + k, totalRow, baseCol + k);
    }
    // 总计行的列合计
    for (let j = 0; j < columnHeaders.length; j++) {
      const c = baseCol + rowLevels + j;
      sheet.setCellValue(totalRow, c, grandTotalRow[j], 'number');
      sheet.setCellStyle(totalRow, c, this.totalStyle());
      this.applyBorder(totalRow, c, totalRow, c);
    }
    // 总计行 × 总计列 = 全表总计
    const grandCellCol = baseCol + rowLevels + columnHeaders.length;
    const grandTotal = grandTotalRow.reduce((a, b) => a + b, 0);
    sheet.setCellValue(totalRow, grandCellCol, grandTotal, 'number');
    sheet.setCellStyle(totalRow, grandCellCol, this.totalStyle());
    this.applyBorder(totalRow, grandCellCol, totalRow, grandCellCol);

    return {
      startRow: baseRow,
      startCol: baseCol,
      endRow: baseRow + totalRows - 1,
      endCol: baseCol + totalCols - 1,
    };
  }

  // ============================================================
  // 聚合与样式辅助
  // ============================================================

  /**
   * 执行聚合函数
   * @param nums 数值数组
   * @param count 非空值计数 (用于 count 函数)
   * @param fn 聚合函数
   */
  private aggregate(nums: number[], count: number, fn: ValueFunction): number {
    switch (fn) {
      case 'sum':
        return nums.reduce((a, b) => a + b, 0);
      case 'count':
        return count;
      case 'average':
        return nums.length > 0 ? nums.reduce((a, b) => a + b, 0) / nums.length : 0;
      case 'max':
        return nums.length > 0 ? Math.max(...nums) : 0;
      case 'min':
        return nums.length > 0 ? Math.min(...nums) : 0;
      default:
        return 0;
    }
  }

  /** 生成值字段的显示标签 (如 "求和项:销售额") */
  private valueLabel(v: PivotValueField): string {
    const fnName =
      v.function === 'sum' ? '求和项' :
      v.function === 'count' ? '计数' :
      v.function === 'average' ? '平均值' :
      v.function === 'max' ? '最大值' :
      v.function === 'min' ? '最小值' :
      v.function;
    return `${fnName}:${v.field}`;
  }

  /** 表头样式 */
  private headerStyle(): Partial<CellStyle> {
    return {
      bold: true,
      backgroundColor: '#f5f5f5',
      horizontalAlignment: 'center',
      verticalAlignment: 'middle',
    };
  }

  /** 行标题样式 */
  private rowHeaderStyle(): Partial<CellStyle> {
    return {
      bold: true,
      backgroundColor: '#fafafa',
      horizontalAlignment: 'left',
      verticalAlignment: 'middle',
    };
  }

  /** 数据单元格样式 */
  private dataStyle(): Partial<CellStyle> {
    return {
      horizontalAlignment: 'right',
      verticalAlignment: 'middle',
      numberFormat: '#,##0.00',
    };
  }

  /** 总计样式 */
  private totalStyle(): Partial<CellStyle> {
    return {
      bold: true,
      backgroundColor: '#eee',
      horizontalAlignment: 'right',
      verticalAlignment: 'middle',
      numberFormat: '#,##0.00',
    };
  }

  /** 为区域内每个单元格设置细边框 */
  private applyBorder(startRow: number, startCol: number, endRow: number, endCol: number): void {
    const border: BorderStyle = { style: 'thin', color: '#d0d0d0' };
    for (let r = startRow; r <= endRow; r++) {
      for (let c = startCol; c <= endCol; c++) {
        const existing = this.sheet.getCellStyle(r, c);
        this.sheet.setCellStyle(r, c, {
          ...existing,
          borderTop: border,
          borderRight: border,
          borderBottom: border,
          borderLeft: border,
        });
      }
    }
  }

  // ============================================================
  // 序列化辅助
  // ============================================================

  /** 深拷贝配置 */
  private cloneConfig(config: PivotTableConfig): PivotTableConfig {
    return {
      id: config.id,
      name: config.name,
      sourceRange: { ...config.sourceRange },
      rows: [...config.rows],
      columns: [...config.columns],
      values: config.values.map(v => ({ ...v })),
      filters: config.filters?.map(f => ({ ...f, values: [...f.values] })),
      targetRange: { ...config.targetRange },
    };
  }

  /** 深拷贝计算结果 */
  private cloneData(data: PivotTableData): PivotTableData {
    return {
      rowHeaders: data.rowHeaders.map(h => [...h]),
      columnHeaders: data.columnHeaders.map(h => [...h]),
      cells: data.cells.map(row => [...row]),
      grandTotalRow: [...data.grandTotalRow],
      grandTotalColumn: [...data.grandTotalColumn],
    };
  }

  /** 序列化为 JSON (用于持久化) */
  toJSON(): PivotTableConfig[] {
    return Array.from(this.pivotTables.values()).map(e => this.cloneConfig(e.config));
  }

  /** 从 JSON 恢复 (追加到当前引擎) */
  fromJSON(configs: PivotTableConfig[]): void {
    for (const config of configs) {
      const data = this.calculate(config);
      this.pivotTables.set(config.id, { config: this.cloneConfig(config), data });
    }
  }
}
