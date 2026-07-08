/**
 * V2/V3 双协议适配层
 *
 * 职责:
 *   1. 检测协议版本 (V2 snake_case / V3 camelCase + schemaVersion)
 *   2. 将外部协议数据归一化为内部统一格式
 *   3. 提供协议无关的数据访问接口
 *
 * V2 协议特征:
 *   - 字段命名: snake_case (如 changeset_list, sheet_id, row_count)
 *   - 无 schemaVersion 字段
 *   - 顶层结构: { sheets, changeset_list, mutations, ... }
 *
 * V3 协议特征:
 *   - 字段命名: camelCase (如 changesets, sheetId, rowCount)
 *   - 有 schemaVersion 字段 (>= 3)
 *   - 顶层结构: { schemaVersion, sheets, changesets, mutations, ... }
 */

// ============================================================
// 协议版本
// ============================================================

export type ProtocolVersion = 'v2' | 'v3';

// ============================================================
// 内部统一格式 (camelCase)
// ============================================================

/** 内部统一的工作表数据 */
export interface NormalizedSheet {
  sheetId: string;
  name: string;
  index: number;
  rowCount: number;
  columnCount: number;
  defaultRowHeight: number;
  defaultColumnWidth: number;
  frozenRowCount: number;
  frozenColumnCount: number;
  cells: NormalizedCell[];
  mergedCells: NormalizedMerge[];
  columnWidths: NormalizedColumnWidth[];
  rowHeights: NormalizedRowHeight[];
  hidden: boolean;
  tabColor: string | null;
}

export interface NormalizedCell {
  row: number;
  column: number;
  type: string;
  value: string | number | boolean;
  formula?: string;
  style?: Record<string, unknown>;
}

export interface NormalizedMerge {
  startRow: number;
  startColumn: number;
  endRow: number;
  endColumn: number;
}

export interface NormalizedColumnWidth {
  column: number;
  width: number;
}

export interface NormalizedRowHeight {
  row: number;
  height: number;
}

/** 内部统一的 changeset */
export interface NormalizedChangeset {
  id: string;
  sheetId: string;
  version: number;
  author: string;
  timestamp: number;
  mutations: NormalizedMutation[];
}

/** 内部统一的 mutation 命令 */
export interface NormalizedMutation {
  actionCode: string;
  sheetId?: string;
  rowIndex?: number;
  columnIndex?: number;
  payload: unknown;
}

/** 内部统一的协议数据 */
export interface NormalizedProtocolData {
  version: ProtocolVersion;
  schemaVersion: number;
  sheets: NormalizedSheet[];
  changesets: NormalizedChangeset[];
  pendingMutations: NormalizedMutation[];
}

// ============================================================
// 协议适配器
// ============================================================

export class ProtocolAdapter {
  /**
   * 检测协议版本
   *
   * 检测策略:
   *   1. 显式 schemaVersion 字段: >= 3 → V3
   *   2. 隐式字段名检测: 出现 changesets → V3, 出现 changeset_list → V2
   *   3. 默认: V2 (向后兼容)
   */
  detectVersion(raw: unknown): ProtocolVersion {
    if (!raw || typeof raw !== 'object') return 'v2';
    const obj = raw as Record<string, unknown>;

    // 1. 显式 schemaVersion
    if (typeof obj.schemaVersion === 'number') {
      return obj.schemaVersion >= 3 ? 'v3' : 'v2';
    }

    // 2. 隐式字段名检测
    if (obj.changesets !== undefined || obj.sheetId !== undefined) {
      return 'v3';
    }
    if (obj.changeset_list !== undefined || obj.sheet_id !== undefined) {
      return 'v2';
    }

    // 3. 默认 V2
    return 'v2';
  }

  /**
   * 归一化协议数据
   * 将 V2 或 V3 协议数据转换为内部统一格式
   */
  normalize(raw: unknown): NormalizedProtocolData {
    const version = this.detectVersion(raw);
    const obj = (raw ?? {}) as Record<string, unknown>;

    const schemaVersion = typeof obj.schemaVersion === 'number' ? obj.schemaVersion :
      (version === 'v3' ? 3 : 2);

    // 解析 sheets
    const rawSheets = (obj.sheets ?? []) as unknown[];
    const sheets = rawSheets.map(s => this.normalizeSheet(s, version));

    // 解析 changesets
    const changesetsField = version === 'v3' ? 'changesets' : 'changeset_list';
    const rawChangesets = (obj[changesetsField] ?? []) as unknown[];
    const changesets = rawChangesets.map(c => this.normalizeChangeset(c, version));

    // 解析 pending mutations
    const mutationsField = version === 'v3' ? 'pendingMutations' : 'pending_mutations';
    const rawMutations = (obj[mutationsField] ?? obj.mutations ?? []) as unknown[];
    const pendingMutations = rawMutations.map(m => this.normalizeMutation(m, version));

    return {
      version,
      schemaVersion,
      sheets,
      changesets,
      pendingMutations,
    };
  }

  /** 归一化工作表 */
  private normalizeSheet(raw: unknown, version: ProtocolVersion): NormalizedSheet {
    const obj = (raw ?? {}) as Record<string, unknown>;
    const g = <T,>(v2Key: string, v3Key: string, defaultValue: T): T => {
      const v = version === 'v3' ? obj[v3Key] : obj[v2Key];
      return (v ?? defaultValue) as T;
    };

    // 解析 cells
    const rawCells = (g('cells', 'cells', [])) as unknown[];
    const cells = rawCells.map(c => this.normalizeCell(c, version));

    // 解析 merges
    const rawMerges = (g('merged_cells', 'mergedCells', [])) as unknown[];
    const mergedCells = rawMerges.map(m => this.normalizeMerge(m, version));

    // 解析列宽
    const rawColWidths = (g('column_widths', 'columnWidths', [])) as unknown[];
    const columnWidths = rawColWidths.map(cw => this.normalizeColumnWidth(cw, version));

    // 解析行高
    const rawRowHeights = (g('row_heights', 'rowHeights', [])) as unknown[];
    const rowHeights = rawRowHeights.map(rh => this.normalizeRowHeight(rh, version));

    // 解析隐藏行/列
    const hiddenRows = (g('hidden_rows', 'hiddenRows', []) as unknown[]).map(Number);
    const hiddenColumns = (g('hidden_columns', 'hiddenColumns', []) as unknown[]).map(Number);

    return {
      sheetId: g('sheet_id', 'sheetId', `sheet_${Date.now()}`),
      name: g('name', 'name', 'Sheet1'),
      index: g('index', 'index', 0),
      rowCount: g('row_count', 'rowCount', 100),
      columnCount: g('column_count', 'columnCount', 26),
      defaultRowHeight: g('default_row_height', 'defaultRowHeight', 24),
      defaultColumnWidth: g('default_column_width', 'defaultColumnWidth', 88),
      frozenRowCount: g('frozen_row_count', 'frozenRowCount', 0),
      frozenColumnCount: g('frozen_column_count', 'frozenColumnCount', 0),
      cells,
      mergedCells,
      columnWidths,
      rowHeights,
      hidden: g('hidden', 'hidden', false),
      tabColor: g('tab_color', 'tabColor', null),
      // 扩展字段
      ...(hiddenRows.length ? { hiddenRows } : {}),
      ...(hiddenColumns.length ? { hiddenColumns } : {}),
    } as NormalizedSheet;
  }

  /** 归一化单元格 */
  private normalizeCell(raw: unknown, version: ProtocolVersion): NormalizedCell {
    const obj = (raw ?? {}) as Record<string, unknown>;
    return {
      row: version === 'v3' ? (obj.row as number) : (obj.row as number),
      column: version === 'v3' ? (obj.column as number) : (obj.column as number),
      type: version === 'v3' ? (obj.type as string) : (obj.type as string),
      value: version === 'v3' ? (obj.value as string | number | boolean) : (obj.value as string | number | boolean),
      formula: version === 'v3' ? (obj.formula as string) : (obj.formula as string),
      style: version === 'v3' ? (obj.style as Record<string, unknown>) : (obj.style as Record<string, unknown>),
    };
  }

  /** 归一化合并区域 */
  private normalizeMerge(raw: unknown, version: ProtocolVersion): NormalizedMerge {
    const obj = (raw ?? {}) as Record<string, unknown>;
    return {
      startRow: obj[version === 'v3' ? 'startRow' : 'start_row'] as number,
      startColumn: obj[version === 'v3' ? 'startColumn' : 'start_column'] as number,
      endRow: obj[version === 'v3' ? 'endRow' : 'end_row'] as number,
      endColumn: obj[version === 'v3' ? 'endColumn' : 'end_column'] as number,
    };
  }

  /** 归一化列宽 */
  private normalizeColumnWidth(raw: unknown, version: ProtocolVersion): NormalizedColumnWidth {
    const obj = (raw ?? {}) as Record<string, unknown>;
    return {
      column: obj[version === 'v3' ? 'column' : 'column'] as number,
      width: obj[version === 'v3' ? 'width' : 'width'] as number,
    };
  }

  /** 归一化行高 */
  private normalizeRowHeight(raw: unknown, version: ProtocolVersion): NormalizedRowHeight {
    const obj = (raw ?? {}) as Record<string, unknown>;
    return {
      row: obj[version === 'v3' ? 'row' : 'row'] as number,
      height: obj[version === 'v3' ? 'height' : 'height'] as number,
    };
  }

  /** 归一化 changeset */
  private normalizeChangeset(raw: unknown, version: ProtocolVersion): NormalizedChangeset {
    const obj = (raw ?? {}) as Record<string, unknown>;
    const g = <T,>(v2Key: string, v3Key: string, defaultValue: T): T => {
      const v = version === 'v3' ? obj[v3Key] : obj[v2Key];
      return (v ?? defaultValue) as T;
    };

    // mutations
    const mutationsField = version === 'v3' ? 'mutations' : 'mutations';
    const rawMutations = (obj[mutationsField] ?? []) as unknown[];
    const mutations = rawMutations.map(m => this.normalizeMutation(m, version));

    return {
      id: g('id', 'id', `cs_${Date.now()}_${Math.random().toString(36).slice(2, 8)}`),
      sheetId: g('sheet_id', 'sheetId', ''),
      version: g('version', 'version', 0),
      author: g('author', 'author', ''),
      timestamp: g('timestamp', 'timestamp', Date.now()),
      mutations,
    };
  }

  /** 归一化 mutation */
  private normalizeMutation(raw: unknown, version: ProtocolVersion): NormalizedMutation {
    const obj = (raw ?? {}) as Record<string, unknown>;
    return {
      actionCode: obj[version === 'v3' ? 'actionCode' : 'action_code'] as string,
      sheetId: obj[version === 'v3' ? 'sheetId' : 'sheet_id'] as string | undefined,
      rowIndex: obj[version === 'v3' ? 'rowIndex' : 'row_index'] as number | undefined,
      columnIndex: obj[version === 'v3' ? 'columnIndex' : 'column_index'] as number | undefined,
      payload: obj.payload,
    };
  }

  // ============================================================
  // 反向序列化 (内部 → 协议)
  // ============================================================

  /** 序列化为指定协议 */
  serialize(data: NormalizedProtocolData, targetVersion: ProtocolVersion): unknown {
    if (targetVersion === 'v3') {
      return this.serializeV3(data);
    }
    return this.serializeV2(data);
  }

  /** 序列化为 V3 协议 (camelCase) */
  private serializeV3(data: NormalizedProtocolData): Record<string, unknown> {
    return {
      schemaVersion: 3,
      sheets: data.sheets.map(s => ({
        sheetId: s.sheetId,
        name: s.name,
        index: s.index,
        rowCount: s.rowCount,
        columnCount: s.columnCount,
        defaultRowHeight: s.defaultRowHeight,
        defaultColumnWidth: s.defaultColumnWidth,
        frozenRowCount: s.frozenRowCount,
        frozenColumnCount: s.frozenColumnCount,
        cells: s.cells,
        mergedCells: s.mergedCells,
        columnWidths: s.columnWidths,
        rowHeights: s.rowHeights,
        hidden: s.hidden,
        tabColor: s.tabColor,
      })),
      changesets: data.changesets.map(c => ({
        id: c.id,
        sheetId: c.sheetId,
        version: c.version,
        author: c.author,
        timestamp: c.timestamp,
        mutations: c.mutations,
      })),
      pendingMutations: data.pendingMutations,
    };
  }

  /** 序列化为 V2 协议 (snake_case) */
  private serializeV2(data: NormalizedProtocolData): Record<string, unknown> {
    return {
      sheets: data.sheets.map(s => ({
        sheet_id: s.sheetId,
        name: s.name,
        index: s.index,
        row_count: s.rowCount,
        column_count: s.columnCount,
        default_row_height: s.defaultRowHeight,
        default_column_width: s.defaultColumnWidth,
        frozen_row_count: s.frozenRowCount,
        frozen_column_count: s.frozenColumnCount,
        cells: s.cells,
        merged_cells: s.mergedCells,
        column_widths: s.columnWidths,
        row_heights: s.rowHeights,
        hidden: s.hidden,
        tab_color: s.tabColor,
      })),
      changeset_list: data.changesets.map(c => ({
        id: c.id,
        sheet_id: c.sheetId,
        version: c.version,
        author: c.author,
        timestamp: c.timestamp,
        mutations: c.mutations,
      })),
      pending_mutations: data.pendingMutations,
    };
  }
}
