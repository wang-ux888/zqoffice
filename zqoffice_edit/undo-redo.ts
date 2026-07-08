/**
 * 撤销/重做模块 — 命令模式 + 批量操作 + 合并
 * 全部原创实现
 *
 * 职责:
 *   1. 命令模式 (Command pattern): 每个操作封装为可逆命令
 *   2. 批量操作: beginBatch/endBatch 将多个操作合并为一个撤销项
 *   3. 合并 (coalescing): 连续相同类型操作合并 (如连续输入)
 *   4. 事件通知: canUndo/canRedo 状态变化
 *   5. 内存控制: 最大栈深度限制
 *
 * 注意: edit-engine.ts 内置了简单栈, 这个模块是增强版,
 * 可在 sheet-engine.ts 中替换 edit-engine 的简单栈
 */

/** 命令接口 */
export interface ICommand {
  /** 正向执行 */
  execute(): void;
  /** 逆向执行 (撤销) */
  undo(): void;
  /** 操作类型 (用于合并判断) */
  type: string;
  /** 时间戳 (用于合并窗口判断) */
  timestamp: number;
}

/** 撤销/重做管理器 */
export class UndoRedoManager {
  private undoStack: ICommand[] = [];
  private redoStack: ICommand[] = [];
  private maxStack: number;

  // 批量操作
  private batchMode: boolean = false;
  private batchCommands: ICommand[] = [];

  // 合并窗口 (毫秒)
  private readonly COALESCE_WINDOW = 800;

  // 事件监听
  private listeners: Array<(canUndo: boolean, canRedo: boolean) => void> = [];

  constructor(maxStack: number = 100) {
    this.maxStack = maxStack;
  }

  // ============================================================
  // 执行命令
  // ============================================================

  /**
   * 执行命令并推入撤销栈
   */
  execute(command: ICommand): void {
    // 如果在批量模式, 先缓存
    if (this.batchMode) {
      command.execute();
      this.batchCommands.push(command);
      return;
    }

    // 合并判断: 如果上一个命令与当前命令类型相同且时间间隔小于窗口, 则合并
    if (this.shouldCoalesce(command)) {
      const last = this.undoStack.pop();
      if (last) {
        // 合并: 保留新命令的 execute 结果, 但 undo 时回退到合并前的状态
        command.execute();
        this.undoStack.push(this.mergeCommands(last, command));
        this.notifyListeners();
        return;
      }
    }

    command.execute();
    this.undoStack.push(command);
    this.redoStack = []; // 清空重做栈

    // 限制栈深度
    if (this.undoStack.length > this.maxStack) {
      this.undoStack.shift();
    }

    this.notifyListeners();
  }

  // ============================================================
  // 撤销/重做
  // ============================================================

  /**
   * 撤销
   */
  undo(): void {
    const command = this.undoStack.pop();
    if (!command) return;
    command.undo();
    this.redoStack.push(command);
    this.notifyListeners();
  }

  /**
   * 重做
   */
  redo(): void {
    const command = this.redoStack.pop();
    if (!command) return;
    command.execute();
    this.undoStack.push(command);
    this.notifyListeners();
  }

  canUndo(): boolean {
    return this.undoStack.length > 0;
  }

  canRedo(): boolean {
    return this.redoStack.length > 0;
  }

  // ============================================================
  // 批量操作
  // ============================================================

  /**
   * 开始批量操作
   * 在 beginBatch/endBatch 之间的所有命令会合并为一个撤销项
   */
  beginBatch(): void {
    this.batchMode = true;
    this.batchCommands = [];
  }

  /**
   * 结束批量操作, 将缓存的命令合并为一个
   */
  endBatch(type: string = 'batch'): void {
    if (!this.batchMode) return;
    this.batchMode = false;

    if (this.batchCommands.length === 0) return;

    const commands = [...this.batchCommands];
    const batchCommand: ICommand = {
      type,
      timestamp: Date.now(),
      execute: () => {
        for (const cmd of commands) cmd.execute();
      },
      undo: () => {
        // 逆序撤销
        for (let i = commands.length - 1; i >= 0; i--) {
          commands[i].undo();
        }
      },
    };

    this.undoStack.push(batchCommand);
    this.redoStack = [];
    if (this.undoStack.length > this.maxStack) {
      this.undoStack.shift();
    }
    this.notifyListeners();
  }

  // ============================================================
  // 合并 (coalescing)
  // ============================================================

  /**
   * 判断是否应该与上一个命令合并
   */
  private shouldCoalesce(command: ICommand): boolean {
    if (this.undoStack.length === 0) return false;
    const last = this.undoStack[this.undoStack.length - 1];
    // 类型相同且时间间隔在窗口内
    return last.type === command.type &&
           (command.timestamp - last.timestamp) < this.COALESCE_WINDOW;
  }

  /**
   * 合并两个命令
   */
  private mergeCommands(old: ICommand, cur: ICommand): ICommand {
    return {
      type: cur.type,
      timestamp: cur.timestamp,
      execute: cur.execute,
      undo: old.undo,  // 撤销时回退到合并前的状态
    };
  }

  // ============================================================
  // 事件监听
  // ============================================================

  /**
   * 添加状态变化监听器
   */
  addListener(fn: (canUndo: boolean, canRedo: boolean) => void): void {
    this.listeners.push(fn);
  }

  /**
   * 移除监听器
   */
  removeListener(fn: (canUndo: boolean, canRedo: boolean) => void): void {
    this.listeners = this.listeners.filter(l => l !== fn);
  }

  private notifyListeners(): void {
    const canU = this.canUndo();
    const canR = this.canRedo();
    for (const listener of this.listeners) {
      listener(canU, canR);
    }
  }

  // ============================================================
  // 其他
  // ============================================================

  /**
   * 清空所有历史
   */
  clear(): void {
    this.undoStack = [];
    this.redoStack = [];
    this.batchCommands = [];
    this.batchMode = false;
    this.notifyListeners();
  }

  /**
   * 获取撤销栈深度
   */
  getUndoCount(): number {
    return this.undoStack.length;
  }

  /**
   * 获取重做栈深度
   */
  getRedoCount(): number {
    return this.redoStack.length;
  }
}

// ============================================================
// 预定义命令工厂 — 创建常用操作的命令
// ============================================================

import { SheetModel, CellStyle, CellValueType } from './data-model';

/** 命令工厂 */
export class CommandFactory {
  /**
   * 设置单元格值命令
   */
  static setCellValue(
    sheet: SheetModel,
    row: number,
    col: number,
    newValue: string | number | boolean,
    type: CellValueType
  ): ICommand {
    const cell = sheet.getCell(row, col);
    const oldValue = cell ? { value: cell.value, type: cell.type, formula: cell.formula } : null;

    return {
      type: 'setCellValue',
      timestamp: Date.now(),
      execute: () => {
        sheet.setCellValue(row, col, newValue, type);
      },
      undo: () => {
        if (oldValue) {
          if (oldValue.formula) {
            sheet.setCellFormula(row, col, oldValue.formula);
            const updated = sheet.getCell(row, col);
            if (updated) updated.value = oldValue.value;
          } else {
            sheet.setCellValue(row, col, oldValue.value, oldValue.type);
          }
        } else {
          sheet.clearCellContent(row, col);
        }
      },
    };
  }

  /**
   * 设置样式命令
   */
  static setStyle(
    sheet: SheetModel,
    row: number,
    col: number,
    newStyle: Partial<CellStyle>
  ): ICommand {
    const oldStyle = sheet.getCellStyle(row, col);
    return {
      type: 'setStyle',
      timestamp: Date.now(),
      execute: () => {
        sheet.setCellStyle(row, col, newStyle);
      },
      undo: () => {
        sheet.setCellStyle(row, col, oldStyle);
      },
    };
  }

  /**
   * 插入行命令
   */
  static insertRow(sheet: SheetModel, row: number, count: number = 1): ICommand {
    return {
      type: 'insertRow',
      timestamp: Date.now(),
      execute: () => {
        sheet.insertRow(row, count);
      },
      undo: () => {
        sheet.deleteRow(row, count);
      },
    };
  }

  /**
   * 删除行命令
   */
  static deleteRow(sheet: SheetModel, row: number, count: number = 1): ICommand {
    // 记录被删除的单元格
    const deletedCells: Array<{ row: number; col: number; cell: any }> = [];
    for (let r = row; r < row + count; r++) {
      for (let c = 0; c < sheet.columnCount; c++) {
        const cell = sheet.getCell(r, c);
        if (cell) deletedCells.push({ row: r, col: c, cell: { ...cell } });
      }
    }

    return {
      type: 'deleteRow',
      timestamp: Date.now(),
      execute: () => {
        sheet.deleteRow(row, count);
      },
      undo: () => {
        sheet.insertRow(row, count);
        // 恢复单元格
        for (const { row: r, col: c, cell } of deletedCells) {
          if (cell.formula) {
            sheet.setCellFormula(r, c, cell.formula);
          } else {
            sheet.setCellValue(r, c, cell.value, cell.type);
          }
          if (cell.style) sheet.setCellStyle(r, c, cell.style);
        }
      },
    };
  }

  /**
   * 设置行高命令
   */
  static setRowHeight(sheet: SheetModel, row: number, newHeight: number): ICommand {
    const oldHeight = sheet.getRowHeight(row);
    return {
      type: 'resizeRow',
      timestamp: Date.now(),
      execute: () => {
        sheet.setRowHeight(row, newHeight);
      },
      undo: () => {
        sheet.setRowHeight(row, oldHeight);
      },
    };
  }

  /**
   * 设置列宽命令
   */
  static setColumnWidth(sheet: SheetModel, col: number, newWidth: number): ICommand {
    const oldWidth = sheet.getColumnWidth(col);
    return {
      type: 'resizeColumn',
      timestamp: Date.now(),
      execute: () => {
        sheet.setColumnWidth(col, newWidth);
      },
      undo: () => {
        sheet.setColumnWidth(col, oldWidth);
      },
    };
  }
}
