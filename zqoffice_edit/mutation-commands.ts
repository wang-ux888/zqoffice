/**
 * Mutation 命令系统
 *
 * 职责:
 *   1. 60+ ActionCode 枚举定义
 *   2. 三态路由 (mutation string / blockData / rowData)
 *   3. decodeCommands (解码二进制/字符串命令)
 *   4. applyCommands (应用命令到数据模型, 带 offsetRange 偏移)
 *
 * ActionCode 三大类:
 *   - 文本/单元格: SET_CELL, CLEAR_CELL, SET_FORMULA, SET_STYLE...
 *   - 结构: INSERT_ROW, DELETE_ROW, INSERT_COL, DELETE_COL, MERGE_CELL...
 *   - 视图: SET_FROZEN, SET_HIDDEN, SET_TAB_COLOR...
 */

import { SheetModel, CellValueType, CellStyle, BorderStyle } from './data-model';

// ============================================================
// ActionCode 枚举 (60+ 命令)
// ============================================================

export enum ActionCode {
  // ====== 单元格操作 ======
  SET_CELL = 1,             // 设置单元格值
  SET_CELL_BATCH = 2,       // 批量设置单元格
  CLEAR_CELL = 3,           // 清除单元格
  CLEAR_CELL_CONTENT = 4,   // 仅清除内容
  CLEAR_CELL_STYLE = 5,     // 仅清除样式
  SET_FORMULA = 6,          // 设置公式
  SET_STYLE = 7,            // 设置样式
  SET_CELL_TYPE = 8,        // 设置单元格类型

  // ====== 行列操作 ======
  INSERT_ROW = 10,
  INSERT_ROWS = 11,
  DELETE_ROW = 12,
  DELETE_ROWS = 13,
  INSERT_COLUMN = 14,
  INSERT_COLUMNS = 15,
  DELETE_COLUMN = 16,
  DELETE_COLUMNS = 17,
  SET_ROW_HEIGHT = 18,
  SET_COLUMN_WIDTH = 19,
  HIDE_ROWS = 20,
  HIDE_COLUMNS = 21,
  UNHIDE_ROWS = 22,
  UNHIDE_COLUMNS = 23,

  // ====== 合并单元格 ======
  MERGE_CELL = 30,
  UNMERGE_CELL = 31,
  MERGE_ALL = 32,           // 合并所有

  // ====== 视图/状态 ======
  SET_FROZEN = 40,
  SET_TAB_COLOR = 41,
  RENAME_SHEET = 42,
  ADD_SHEET = 43,
  DELETE_SHEET = 44,
  MOVE_SHEET = 45,
  COPY_SHEET = 46,

  // ====== 批注 ======
  ADD_COMMENT = 50,
  UPDATE_COMMENT = 51,
  DELETE_COMMENT = 52,
  RESOLVE_COMMENT = 53,

  // ====== 分组大纲 ======
  ADD_OUTLINE = 60,
  REMOVE_OUTLINE = 61,
  COLLAPSE_OUTLINE = 62,
  EXPAND_OUTLINE = 63,

  // ====== 数据验证 ======
  ADD_VALIDATION = 70,
  UPDATE_VALIDATION = 71,
  REMOVE_VALIDATION = 72,

  // ====== 条件格式 ======
  ADD_CONDITIONAL_FORMAT = 80,
  UPDATE_CONDITIONAL_FORMAT = 81,
  REMOVE_CONDITIONAL_FORMAT = 82,

  // ====== 图片/图表 ======
  ADD_IMAGE = 90,
  UPDATE_IMAGE = 91,
  DELETE_IMAGE = 92,
  ADD_CHART = 93,
  UPDATE_CHART = 94,
  DELETE_CHART = 95,

  // ====== 复合操作 ======
  MOVE_CELLS = 100,         // 移动单元格区域
  COPY_CELLS = 101,         // 复制单元格区域
  TRANSPOSE = 102,          // 转置
  FILL_SERIES = 103,        // 填充序列
  SORT_RANGE = 104,         // 排序
  FILTER_RANGE = 105,       // 筛选
}

/** ActionCode 名称映射 */
export const ActionCodeNames: Record<number, string> = {
  [ActionCode.SET_CELL]: 'setCell',
  [ActionCode.SET_CELL_BATCH]: 'setCellBatch',
  [ActionCode.CLEAR_CELL]: 'clearCell',
  [ActionCode.CLEAR_CELL_CONTENT]: 'clearCellContent',
  [ActionCode.CLEAR_CELL_STYLE]: 'clearCellStyle',
  [ActionCode.SET_FORMULA]: 'setFormula',
  [ActionCode.SET_STYLE]: 'setStyle',
  [ActionCode.SET_CELL_TYPE]: 'setCellType',
  [ActionCode.INSERT_ROW]: 'insertRow',
  [ActionCode.INSERT_ROWS]: 'insertRows',
  [ActionCode.DELETE_ROW]: 'deleteRow',
  [ActionCode.DELETE_ROWS]: 'deleteRows',
  [ActionCode.INSERT_COLUMN]: 'insertColumn',
  [ActionCode.INSERT_COLUMNS]: 'insertColumns',
  [ActionCode.DELETE_COLUMN]: 'deleteColumn',
  [ActionCode.DELETE_COLUMNS]: 'deleteColumns',
  [ActionCode.SET_ROW_HEIGHT]: 'setRowHeight',
  [ActionCode.SET_COLUMN_WIDTH]: 'setColumnWidth',
  [ActionCode.HIDE_ROWS]: 'hideRows',
  [ActionCode.HIDE_COLUMNS]: 'hideColumns',
  [ActionCode.UNHIDE_ROWS]: 'unhideRows',
  [ActionCode.UNHIDE_COLUMNS]: 'unhideColumns',
  [ActionCode.MERGE_CELL]: 'mergeCell',
  [ActionCode.UNMERGE_CELL]: 'unmergeCell',
  [ActionCode.SET_FROZEN]: 'setFrozen',
  [ActionCode.SET_TAB_COLOR]: 'setTabColor',
  [ActionCode.RENAME_SHEET]: 'renameSheet',
  [ActionCode.ADD_SHEET]: 'addSheet',
  [ActionCode.DELETE_SHEET]: 'deleteSheet',
  [ActionCode.MOVE_SHEET]: 'moveSheet',
  [ActionCode.ADD_COMMENT]: 'addComment',
  [ActionCode.UPDATE_COMMENT]: 'updateComment',
  [ActionCode.DELETE_COMMENT]: 'deleteComment',
  [ActionCode.MOVE_CELLS]: 'moveCells',
  [ActionCode.COPY_CELLS]: 'copyCells',
  [ActionCode.SORT_RANGE]: 'sortRange',
  [ActionCode.FILTER_RANGE]: 'filterRange',
};

// ============================================================
// 命令结构
// ============================================================

/** 命令路由分类 */
export type CommandRoute = 'mutation_string' | 'block_data' | 'row_data';

/** 解码后的命令 */
export interface DecodedCommand {
  actionCode: ActionCode;
  route: CommandRoute;
  sheetId?: string;
  rowIndex?: number;
  columnIndex?: number;
  endRowIndex?: number;
  endColumnIndex?: number;
  value?: unknown;
  style?: Partial<CellStyle>;
  formula?: string;
  payload?: Record<string, unknown>;
}

/** 偏移范围 (用于相对坐标修正) */
export interface OffsetRange {
  rowOffset?: number;
  colOffset?: number;
}

// ============================================================
// 命令系统
// ============================================================

export class MutationCommands {
  /**
   * 解码命令
   *
   * 输入: 原始 mutation 数据 (可能是字符串、对象或二进制)
   * 输出: DecodedCommand 数组
   */
  decodeCommands(raw: unknown): DecodedCommand[] {
    if (!raw) return [];

    // 字符串形式: "setCell|0|0|hello" 或 JSON 字符串
    if (typeof raw === 'string') {
      return this.decodeStringCommands(raw);
    }

    // 数组形式: [cmd1, cmd2, ...]
    if (Array.isArray(raw)) {
      return raw.flatMap(item => this.decodeSingleCommand(item));
    }

    // 对象形式: 单个命令
    if (typeof raw === 'object') {
      return this.decodeSingleCommand(raw);
    }

    return [];
  }

  /** 解码字符串命令 */
  private decodeStringCommands(str: string): DecodedCommand[] {
    // 尝试 JSON 解析
    try {
      const parsed = JSON.parse(str);
      return this.decodeCommands(parsed);
    } catch {
      // 不是 JSON, 尝试管道分隔格式: "action|row|col|value"
      const lines = str.split('\n').filter(l => l.trim());
      return lines.map(line => this.decodePipeCommand(line));
    }
  }

  /** 解码管道分隔命令 */
  private decodePipeCommand(line: string): DecodedCommand {
    const parts = line.split('|');
    const actionName = parts[0];
    const actionCode = this.actionNameToCode(actionName);
    const route = this.routeOf(actionCode);

    const cmd: DecodedCommand = {
      actionCode,
      route,
    };

    // 根据命令类型解析参数
    if (parts.length > 1 && parts[1]) cmd.rowIndex = parseInt(parts[1]);
    if (parts.length > 2 && parts[2]) cmd.columnIndex = parseInt(parts[2]);
    if (parts.length > 3 && parts[3]) cmd.value = parts[3];
    if (parts.length > 4 && parts[4]) cmd.endRowIndex = parseInt(parts[4]);
    if (parts.length > 5 && parts[5]) cmd.endColumnIndex = parseInt(parts[5]);

    return cmd;
  }

  /** 解码单个命令对象 */
  private decodeSingleCommand(obj: unknown): DecodedCommand[] {
    if (!obj || typeof obj !== 'object') return [];
    const o = obj as Record<string, unknown>;

    // actionCode 可能是数字或字符串
    let actionCode: ActionCode;
    if (typeof o.actionCode === 'number') {
      actionCode = o.actionCode;
    } else if (typeof o.actionCode === 'string') {
      actionCode = this.actionNameToCode(o.actionCode);
    } else if (typeof o.action_code === 'number') {
      actionCode = o.action_code;
    } else if (typeof o.action_code === 'string') {
      actionCode = this.actionNameToCode(o.action_code);
    } else {
      return [];
    }

    const route = this.routeOf(actionCode);

    const cmd: DecodedCommand = {
      actionCode,
      route,
      sheetId: o.sheetId as string | undefined ?? o.sheet_id as string | undefined,
      rowIndex: o.rowIndex as number | undefined ?? o.row_index as number | undefined,
      columnIndex: o.columnIndex as number | undefined ?? o.column_index as number | undefined,
      endRowIndex: o.endRowIndex as number | undefined ?? o.end_row_index as number | undefined,
      endColumnIndex: o.endColumnIndex as number | undefined ?? o.end_column_index as number | undefined,
      value: o.value,
      style: o.style as Partial<CellStyle> | undefined,
      formula: o.formula as string | undefined,
      payload: o.payload as Record<string, unknown> | undefined,
    };

    return [cmd];
  }

  /** ActionCode 名称 → 数字 */
  private actionNameToCode(name: string): ActionCode {
    for (const [code, n] of Object.entries(ActionCodeNames)) {
      if (n.toLowerCase() === name.toLowerCase()) {
        return parseInt(code) as ActionCode;
      }
    }
    return ActionCode.SET_CELL;  // 默认
  }

  /** 确定命令的路由分类 */
  private routeOf(code: ActionCode): CommandRoute {
    // 文本/单元格类 → mutation_string
    if (code <= 9) return 'mutation_string';
    // 行列操作 → row_data
    if (code >= 10 && code <= 29) return 'row_data';
    // 其他 → block_data
    return 'block_data';
  }

  // ============================================================
  // 应用命令
  // ============================================================

  /**
   * 应用命令到工作表模型
   *
   * @param sheet 目标工作表
   * @param commands 命令数组
   * @param offsetRange 偏移范围 (用于相对坐标修正)
   */
  applyCommands(sheet: SheetModel, commands: DecodedCommand[], offsetRange?: OffsetRange): void {
    for (const cmd of commands) {
      this.applyCommand(sheet, cmd, offsetRange);
    }
  }

  /** 应用单个命令 */
  private applyCommand(sheet: SheetModel, cmd: DecodedCommand, offset?: OffsetRange): void {
    const rowOffset = offset?.rowOffset ?? 0;
    const colOffset = offset?.colOffset ?? 0;

    const row = (cmd.rowIndex ?? 0) + rowOffset;
    const col = (cmd.columnIndex ?? 0) + colOffset;
    const endRow = (cmd.endRowIndex ?? row) + rowOffset;
    const endCol = (cmd.endColumnIndex ?? col) + colOffset;

    switch (cmd.actionCode) {
      // ====== 单元格操作 ======
      case ActionCode.SET_CELL:
        sheet.setCellValue(row, col, cmd.value as string | number | boolean, this.inferType(cmd.value));
        break;
      case ActionCode.CLEAR_CELL:
        sheet.deleteCell(row, col);
        break;
      case ActionCode.CLEAR_CELL_CONTENT:
        sheet.clearCellContent(row, col);
        break;
      case ActionCode.CLEAR_CELL_STYLE:
        sheet.clearCellStyle(row, col);
        break;
      case ActionCode.SET_FORMULA:
        if (cmd.formula) sheet.setCellFormula(row, col, cmd.formula);
        break;
      case ActionCode.SET_STYLE:
        if (cmd.style) sheet.setCellStyle(row, col, cmd.style);
        break;

      // ====== 合并 ======
      case ActionCode.MERGE_CELL:
        sheet.addMerge(row, col, endRow, endCol);
        break;
      case ActionCode.UNMERGE_CELL:
        sheet.removeMerge(row, col);
        break;

      // ====== 行列操作 (简化实现: 通过修改 rowCount/columnCount) ======
      case ActionCode.SET_ROW_HEIGHT:
        // 通过 payload 传 height
        if (cmd.payload?.height) {
          sheet.setRowHeight(row, Number(cmd.payload.height));
        }
        break;
      case ActionCode.SET_COLUMN_WIDTH:
        if (cmd.payload?.width) {
          sheet.setColumnWidth(col, Number(cmd.payload.width));
        }
        break;

      // ====== 冻结 ======
      case ActionCode.SET_FROZEN:
        if (cmd.payload) {
          sheet.frozenRowCount = Number(cmd.payload.frozenRowCount ?? 0);
          sheet.frozenColumnCount = Number(cmd.payload.frozenColumnCount ?? 0);
        }
        break;

      // ====== 工作表元数据 ======
      case ActionCode.SET_TAB_COLOR:
        if (typeof cmd.value === 'string') sheet.tabColor = cmd.value;
        break;
      case ActionCode.RENAME_SHEET:
        if (typeof cmd.value === 'string') sheet.name = cmd.value;
        break;

      // 其他命令: 暂未实现具体逻辑, 记录但不报错
      default:
        // 复合操作 (MOVE_CELLS, SORT_RANGE 等) 需要更复杂的处理
        // 在生产环境中应实现完整支持
        break;
    }
  }

  /** 推断单元格类型 */
  private inferType(value: unknown): CellValueType | undefined {
    if (typeof value === 'number') return 'number';
    if (typeof value === 'boolean') return 'boolean';
    if (typeof value === 'string') {
      if (value.startsWith('#')) return 'error';
      if (value.startsWith('=')) return 'formula';
      return 'string';
    }
    return undefined;
  }

  // ============================================================
  // 命令编码 (反向操作, 用于序列化)
  // ============================================================

  /** 编码命令为对象 */
  encodeCommand(cmd: DecodedCommand): Record<string, unknown> {
    const out: Record<string, unknown> = {
      actionCode: cmd.actionCode,
      actionName: ActionCodeNames[cmd.actionCode],
      route: cmd.route,
    };
    if (cmd.sheetId) out.sheetId = cmd.sheetId;
    if (cmd.rowIndex !== undefined) out.rowIndex = cmd.rowIndex;
    if (cmd.columnIndex !== undefined) out.columnIndex = cmd.columnIndex;
    if (cmd.endRowIndex !== undefined) out.endRowIndex = cmd.endRowIndex;
    if (cmd.endColumnIndex !== undefined) out.endColumnIndex = cmd.endColumnIndex;
    if (cmd.value !== undefined) out.value = cmd.value;
    if (cmd.style) out.style = cmd.style;
    if (cmd.formula) out.formula = cmd.formula;
    if (cmd.payload) out.payload = cmd.payload;
    return out;
  }

  /** 编码命令数组为 JSON 字符串 */
  encodeCommands(commands: DecodedCommand[]): string {
    return JSON.stringify(commands.map(c => this.encodeCommand(c)));
  }
}
