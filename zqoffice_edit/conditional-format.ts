/**
 * 条件格式引擎 — 根据规则动态计算单元格样式
 * 全部原创实现
 *
 * 支持的规则类型:
 *   - cellValue: 基于单元格值 (大于/小于/等于/介于/包含文本)
 *   - colorScale: 色阶 (三色渐变: 最小值-中值-最大值)
 *   - dataBar: 数据条 (在单元格内绘制比例条)
 *   - topBottom: 前 N / 后 N / 前 10% / 后 10%
 *   - duplicateValues: 重复值/唯一值高亮
 *   - formula: 自定义公式决定格式
 */

import { SheetModel, CellStyle } from './data-model';
import { SelectionRange } from './render-engine';

// ============================================================
// 类型定义
// ============================================================

/** 条件格式规则类型 */
export type ConditionalFormatType =
  | 'cellValue'
  | 'colorScale'
  | 'dataBar'
  | 'topBottom'
  | 'duplicateValues'
  | 'formula';

/** 比较运算符 (cellValue 类型) */
export type ConditionalFormatOperator =
  | 'greaterThan'
  | 'lessThan'
  | 'equal'
  | 'between'
  | 'notEqual'
  | 'containsText'
  | 'notContainsText';

/** 条件格式规则 */
export interface ConditionalFormatRule {
  id: string;
  type: ConditionalFormatType;
  range: SelectionRange;
  priority: number; // 数字越小优先级越高 (1 = 最高), 与 Excel 一致
  // cellValue 类型
  operator?: ConditionalFormatOperator;
  value?: number | string;
  value2?: number;
  // 通用格式 (cellValue/topBottom/duplicateValues/formula 使用)
  style: Partial<CellStyle>;
  // colorScale 类型
  minColor?: string;
  midColor?: string;
  maxColor?: string;
  // dataBar 类型
  barColor?: string;
  // topBottom 类型
  rank?: number;
  isPercent?: boolean;
  isTop?: boolean;
  // duplicateValues 类型 (true = 高亮唯一值, false/缺省 = 高亮重复值)
  unique?: boolean;
  // formula 类型
  formula?: string;
}

/** 自定义公式求值器: 返回布尔值表示是否应用格式 */
export type FormulaEvaluator = (
  formula: string,
  row: number,
  col: number,
  sheet: SheetModel,
) => boolean;

/** 范围内数值统计 */
interface RangeStats {
  min: number;
  max: number;
  median: number;
  values: number[]; // 升序排列
}

/** RGB 颜色 */
interface RGB {
  r: number;
  g: number;
  b: number;
}

// ============================================================
// 颜色工具函数
// ============================================================

/** 将十六进制颜色字符串解析为 RGB 对象, 支持 #RGB / #RRGGBB / #RRGGBBAA */
function hexToRgb(hex: string): RGB | null {
  let h = hex.trim().replace('#', '');
  if (h.length === 3) {
    // #RGB → #RRGGBB
    h = h[0] + h[0] + h[1] + h[1] + h[2] + h[2];
  }
  if (h.length !== 6 && h.length !== 8) return null;
  const r = parseInt(h.substring(0, 2), 16);
  const g = parseInt(h.substring(2, 4), 16);
  const b = parseInt(h.substring(4, 6), 16);
  if (isNaN(r) || isNaN(g) || isNaN(b)) return null;
  return { r, g, b };
}

/** 将 RGB 对象转换为十六进制颜色字符串 #RRGGBB */
function rgbToHex(rgb: RGB): string {
  const toHex2 = (n: number): string => {
    const clamped = Math.max(0, Math.min(255, Math.round(n)));
    return clamped.toString(16).padStart(2, '0');
  };
  return `#${toHex2(rgb.r)}${toHex2(rgb.g)}${toHex2(rgb.b)}`;
}

/**
 * 在两个颜色之间线性插值
 * @param color1 起始颜色 (#RRGGBB)
 * @param color2 结束颜色 (#RRGGBB)
 * @param t 插值因子, 0 = color1, 1 = color2
 */
function interpolateColor(color1: string, color2: string, t: number): string {
  const c1 = hexToRgb(color1);
  const c2 = hexToRgb(color2);
  // 颜色解析失败时回退到第二个颜色
  if (!c1 || !c2) return color2;
  const clampedT = Math.max(0, Math.min(1, t));
  return rgbToHex({
    r: c1.r + (c2.r - c1.r) * clampedT,
    g: c1.g + (c2.g - c1.g) * clampedT,
    b: c1.b + (c2.b - c1.b) * clampedT,
  });
}

// ============================================================
// 条件格式引擎
// ============================================================

export class ConditionalFormatEngine {
  private sheet: SheetModel;
  private rules: ConditionalFormatRule[] = [];

  /** 自定义公式求值器 (可通过 setFormulaEvaluator 注入, 通常连接 FormulaEngine) */
  private formulaEvaluator: FormulaEvaluator | null = null;

  /** 范围数值统计缓存, key = rule.id */
  private statsCache = new Map<string, RangeStats | null>();

  /** 范围值计数缓存 (用于重复值判断), key = rule.id */
  private valueCountsCache = new Map<string, Map<string, number>>();

  constructor(sheet: SheetModel) {
    this.sheet = sheet;
  }

  // ============================================================
  // 规则管理
  // ============================================================

  /**
   * 添加条件格式规则
   * @param rule 规则 (id 可省略, 自动生成)
   * @returns 带有完整 id 的规则
   */
  addRule(rule: Omit<ConditionalFormatRule, 'id'> & { id?: string }): ConditionalFormatRule {
    const id = rule.id ?? ConditionalFormatEngine.generateId();
    const fullRule: ConditionalFormatRule = { ...rule, id } as ConditionalFormatRule;
    this.rules.push(fullRule);
    this.invalidateCache();
    return fullRule;
  }

  /**
   * 删除指定 id 的规则
   * @param id 规则 id
   * @returns 是否删除成功
   */
  removeRule(id: string): boolean {
    const before = this.rules.length;
    this.rules = this.rules.filter(r => r.id !== id);
    const removed = this.rules.length < before;
    if (removed) this.invalidateCache();
    return removed;
  }

  /**
   * 获取所有规则 (按优先级升序: 优先级数字越小越靠前)
   */
  getRules(): ConditionalFormatRule[] {
    return [...this.rules].sort((a, b) => a.priority - b.priority);
  }

  /**
   * 获取影响指定单元格的规则 (按优先级升序)
   * @param row 行号 (0-based)
   * @param col 列号 (0-based)
   */
  getRulesForCell(row: number, col: number): ConditionalFormatRule[] {
    return this.rules
      .filter(r => this.isInRange(row, col, r.range))
      .sort((a, b) => a.priority - b.priority);
  }

  /**
   * 清除指定范围内的所有规则 (规则的 range 与给定 range 相交即移除)
   * @param range 选区范围
   */
  clearRange(range: SelectionRange): void {
    const sr = Math.min(range.startRow, range.endRow);
    const er = Math.max(range.startRow, range.endRow);
    const sc = Math.min(range.startCol, range.endCol);
    const ec = Math.max(range.startCol, range.endCol);

    this.rules = this.rules.filter(rule => {
      const rsr = Math.min(rule.range.startRow, rule.range.endRow);
      const rer = Math.max(rule.range.startRow, rule.range.endRow);
      const rsc = Math.min(rule.range.startCol, rule.range.endCol);
      const rec = Math.max(rule.range.startCol, rule.range.endCol);
      // 不相交则保留
      const intersects = !(rer < sr || rsr > er || rec < sc || rsc > ec);
      return !intersects;
    });
    this.invalidateCache();
  }

  /**
   * 清除所有规则
   */
  clearAll(): void {
    this.rules = [];
    this.invalidateCache();
  }

  /**
   * 更新关联的工作表 (切换 sheet 时调用)
   */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
    this.invalidateCache();
  }

  /**
   * 注入自定义公式求值器 (通常连接 FormulaEngine)
   * 若未注入, formula 类型规则使用内置的简化求值器
   */
  setFormulaEvaluator(fn: FormulaEvaluator | null): void {
    this.formulaEvaluator = fn;
  }

  // ============================================================
  // 样式计算
  // ============================================================

  /**
   * 计算单元格的条件格式样式
   * 多条规则命中时, 高优先级 (priority 数字小) 的属性覆盖低优先级
   * @param row 行号 (0-based)
   * @param col 列号 (0-based)
   * @returns 合并后的条件格式样式, 由渲染层叠加到单元格基础样式上
   */
  evaluateCell(row: number, col: number): Partial<CellStyle> {
    const cellRules = this.getRulesForCell(row, col);
    // cellRules 已按 priority 升序排列 (priority 1 = 最高优先级在前)
    // 从最低优先级开始合并, 高优先级后合并以覆盖同名属性
    let result: Partial<CellStyle> = {};
    for (let i = cellRules.length - 1; i >= 0; i--) {
      const style = this.evaluateRule(cellRules[i], row, col);
      if (style) {
        result = { ...result, ...style };
      }
    }
    return result;
  }

  /**
   * 对所有受条件格式影响的单元格计算样式
   * 在渲染时调用, 返回 cellKey ("row:col") → 样式 的映射
   */
  applyAll(): Map<string, Partial<CellStyle>> {
    const result = new Map<string, Partial<CellStyle>>();
    // 收集所有受影响的单元格
    const affectedCells = new Set<string>();
    for (const rule of this.rules) {
      this.iterRangeCells(rule.range, (row, col) => {
        affectedCells.add(`${row}:${col}`);
      });
    }
    // 逐个计算
    for (const key of affectedCells) {
      const sepIdx = key.indexOf(':');
      const row = parseInt(key.substring(0, sepIdx), 10);
      const col = parseInt(key.substring(sepIdx + 1), 10);
      const style = this.evaluateCell(row, col);
      if (Object.keys(style).length > 0) {
        result.set(key, style);
      }
    }
    return result;
  }

  // ============================================================
  // 规则求值 (各类型)
  // ============================================================

  /**
   * 求值单条规则, 返回应应用的样式 (不匹配则返回 null)
   */
  private evaluateRule(
    rule: ConditionalFormatRule,
    row: number,
    col: number,
  ): Partial<CellStyle> | null {
    switch (rule.type) {
      case 'cellValue':
        return this.evalCellValue(rule, row, col);
      case 'colorScale':
        return this.evalColorScale(rule, row, col);
      case 'dataBar':
        return this.evalDataBar(rule, row, col);
      case 'topBottom':
        return this.evalTopBottom(rule, row, col);
      case 'duplicateValues':
        return this.evalDuplicateValues(rule, row, col);
      case 'formula':
        return this.evalFormula(rule, row, col);
      default:
        return null;
    }
  }

  /**
   * cellValue 类型: 基于单元格值与阈值比较
   */
  private evalCellValue(
    rule: ConditionalFormatRule,
    row: number,
    col: number,
  ): Partial<CellStyle> | null {
    const cellVal = this.sheet.getCellValue(row, col);
    if (cellVal === null || cellVal === undefined) return null;

    const op = rule.operator;
    if (!op) return null;

    let matched = false;
    switch (op) {
      case 'greaterThan': {
        const num = this.toNumber(cellVal);
        const target = this.toNumber(rule.value);
        matched = num !== null && target !== null && num > target;
        break;
      }
      case 'lessThan': {
        const num = this.toNumber(cellVal);
        const target = this.toNumber(rule.value);
        matched = num !== null && target !== null && num < target;
        break;
      }
      case 'equal': {
        const num = this.toNumber(cellVal);
        const target = this.toNumber(rule.value);
        if (num !== null && target !== null) {
          matched = num === target;
        } else {
          matched = String(cellVal) === String(rule.value);
        }
        break;
      }
      case 'notEqual': {
        const num = this.toNumber(cellVal);
        const target = this.toNumber(rule.value);
        if (num !== null && target !== null) {
          matched = num !== target;
        } else {
          matched = String(cellVal) !== String(rule.value);
        }
        break;
      }
      case 'between': {
        const num = this.toNumber(cellVal);
        const lo = this.toNumber(rule.value);
        const hi = rule.value2 !== undefined ? this.toNumber(rule.value2) : null;
        if (num !== null && lo !== null && hi !== null) {
          matched = num >= Math.min(lo, hi) && num <= Math.max(lo, hi);
        }
        break;
      }
      case 'containsText': {
        const text = String(rule.value ?? '');
        matched = text.length > 0 && String(cellVal).includes(text);
        break;
      }
      case 'notContainsText': {
        const text = String(rule.value ?? '');
        matched = text.length === 0 || !String(cellVal).includes(text);
        break;
      }
      default:
        return null;
    }

    return matched ? rule.style : null;
  }

  /**
   * colorScale 类型: 三色色阶渐变
   * 根据当前值在 [min, max] 区间的位置, 在 minColor / midColor / maxColor 之间插值
   */
  private evalColorScale(
    rule: ConditionalFormatRule,
    row: number,
    col: number,
  ): Partial<CellStyle> | null {
    const value = this.getNumericValue(row, col);
    if (value === null) return null;

    const stats = this.getRangeStats(rule);
    if (!stats || stats.values.length === 0) return null;

    // 默认色阶: 绿-黄-红
    const minColor = rule.minColor ?? '#63BE7B';
    const midColor = rule.midColor ?? '#FFEB84';
    const maxColor = rule.maxColor ?? '#F8696B';

    // 所有人值相同: 使用中间色
    if (stats.min === stats.max) {
      return { backgroundColor: midColor };
    }

    let color: string;
    if (value <= stats.min) {
      color = minColor;
    } else if (value >= stats.max) {
      color = maxColor;
    } else if (value <= stats.median) {
      // 在 [min, median] 区间: minColor → midColor
      const span = stats.median - stats.min;
      const t = span > 0 ? (value - stats.min) / span : 0;
      color = interpolateColor(minColor, midColor, t);
    } else {
      // 在 (median, max) 区间: midColor → maxColor
      const span = stats.max - stats.median;
      const t = span > 0 ? (value - stats.median) / span : 1;
      color = interpolateColor(midColor, maxColor, t);
    }

    return { backgroundColor: color };
  }

  /**
   * dataBar 类型: 数据条
   * 根据当前值在 [min, max] 区间的比例, 用 CSS 线性渐变在单元格背景绘制比例条
   */
  private evalDataBar(
    rule: ConditionalFormatRule,
    row: number,
    col: number,
  ): Partial<CellStyle> | null {
    const value = this.getNumericValue(row, col);
    if (value === null) return null;

    const stats = this.getRangeStats(rule);
    if (!stats || stats.min === stats.max) return null;

    const barColor = rule.barColor ?? '#638EC6';
    const span = stats.max - stats.min;
    const ratio = span > 0 ? (value - stats.min) / span : 0;
    const percent = Math.max(0, Math.min(1, ratio)) * 100;

    // 用线性渐变模拟数据条: barColor 填充 0~percent%, 之后透明
    return {
      backgroundColor: `linear-gradient(to right, ${barColor} ${percent}%, transparent ${percent}%)`,
    };
  }

  /**
   * topBottom 类型: 前 N / 后 N / 前 N% / 后 N%
   */
  private evalTopBottom(
    rule: ConditionalFormatRule,
    row: number,
    col: number,
  ): Partial<CellStyle> | null {
    const value = this.getNumericValue(row, col);
    if (value === null) return null;

    const stats = this.getRangeStats(rule);
    if (!stats || stats.values.length === 0) return null;

    const rank = Math.max(1, rule.rank ?? 10);
    const isTop = rule.isTop ?? true;
    const isPercent = rule.isPercent ?? false;

    // 已排序 (升序)
    const sorted = stats.values;
    let count: number;
    if (isPercent) {
      count = Math.max(1, Math.ceil((sorted.length * rank) / 100));
    } else {
      count = Math.min(rank, sorted.length);
    }

    if (isTop) {
      // 前 N: 值 >= sorted[len - count] 即命中
      const threshold = sorted[sorted.length - count];
      return value >= threshold ? rule.style : null;
    } else {
      // 后 N: 值 <= sorted[count - 1] 即命中
      const threshold = sorted[count - 1];
      return value <= threshold ? rule.style : null;
    }
  }

  /**
   * duplicateValues 类型: 重复值/唯一值高亮
   * rule.unique === true 时高亮唯一值, 否则高亮重复值
   */
  private evalDuplicateValues(
    rule: ConditionalFormatRule,
    row: number,
    col: number,
  ): Partial<CellStyle> | null {
    const value = this.sheet.getCellValue(row, col);
    if (value === null || value === undefined || value === '') return null;

    const counts = this.getValueCounts(rule);
    const key = String(value);
    const count = counts.get(key) ?? 0;

    if (rule.unique === true) {
      return count === 1 ? rule.style : null;
    }
    return count > 1 ? rule.style : null;
  }

  /**
   * formula 类型: 自定义公式决定格式
   * 优先使用注入的 formulaEvaluator, 否则使用内置简化求值器
   */
  private evalFormula(
    rule: ConditionalFormatRule,
    row: number,
    col: number,
  ): Partial<CellStyle> | null {
    const formula = rule.formula ?? (typeof rule.value === 'string' ? rule.value : null);
    if (!formula) return null;

    let result: boolean;
    if (this.formulaEvaluator) {
      result = this.formulaEvaluator(formula, row, col, this.sheet);
    } else {
      result = this.defaultFormulaEvaluator(formula, row, col);
    }

    return result ? rule.style : null;
  }

  // ============================================================
  // 内置公式求值器 (简化版)
  // 支持: 单元格引用 (A1/$A$1), 比较运算符 (>, <, >=, <=, =, <>), AND(), OR()
  // ============================================================

  /**
   * 默认公式求值器
   * 将公式中的单元格引用替换为实际值后, 求值布尔表达式
   */
  private defaultFormulaEvaluator(formula: string, row: number, col: number): boolean {
    // row/col 参数保留以备扩展 (如相对引用偏移), 当前未使用
    void row;
    void col;
    let expr = formula.trim();
    if (expr.startsWith('=')) expr = expr.substring(1).trim();
    if (!expr) return false;
    return this.evalBooleanExpr(expr);
  }

  /**
   * 求值布尔表达式
   * 支持: AND(...), OR(...), 以及单个比较表达式
   */
  private evalBooleanExpr(expr: string): boolean {
    expr = expr.trim();
    if (!expr) return false;

    // AND(...) / OR(...)
    const funcMatch = expr.match(/^(AND|OR)\s*\(([\s\S]*)\)$/i);
    if (funcMatch) {
      const func = funcMatch[1].toUpperCase();
      const args = this.splitArgs(funcMatch[2]);
      const results = args.map(a => this.evalBooleanExpr(a));
      return func === 'AND' ? results.every(r => r) : results.some(r => r);
    }

    // 比较表达式: left OP right
    // 按运算符长度优先匹配 (>=, <=, <>, != 优先于 >, <, =)
    const match = expr.match(/^(.+?)(>=|<=|<>|!=|=|>|<)(.+)$/);
    if (match) {
      const left = this.evalValueExpr(match[1].trim());
      const op = match[2];
      const right = this.evalValueExpr(match[3].trim());
      return this.applyComparison(left, op, right);
    }

    // 无比较运算符: 当作布尔值
    const val = this.evalValueExpr(expr);
    return val === true || val === 'TRUE' || val === 'true' || val === 1;
  }

  /**
   * 求值标量表达式 (单元格引用 / 字符串 / 数字 / 布尔)
   */
  private evalValueExpr(expr: string): number | string | boolean {
    expr = expr.trim();

    // 空表达式
    if (!expr) return 0;

    // 带引号的字符串
    if (expr.length >= 2 && expr.startsWith('"') && expr.endsWith('"')) {
      return expr.slice(1, -1);
    }

    // 布尔字面量
    const upper = expr.toUpperCase();
    if (upper === 'TRUE') return true;
    if (upper === 'FALSE') return false;

    // 单元格引用 (A1, $A$1, B23, $AA$10)
    const cellRef = expr.match(/^\$?([A-Za-z]+)\$?(\d+)$/);
    if (cellRef) {
      const refCol = SheetModel.letterToColumn(cellRef[1].toUpperCase());
      const refRow = parseInt(cellRef[2], 10) - 1;
      const val = this.sheet.getCellValue(refRow, refCol);
      if (val === null || val === undefined) return 0;
      if (typeof val === 'number') return val;
      if (typeof val === 'boolean') return val;
      return val;
    }

    // 数字字面量
    if (/^-?\d+(\.\d+)?$/.test(expr)) {
      return parseFloat(expr);
    }

    // 其他: 当作字符串
    return expr;
  }

  /**
   * 应用比较运算
   */
  private applyComparison(
    left: number | string | boolean,
    op: string,
    right: number | string | boolean,
  ): boolean {
    // 布尔值转为数字
    const l: number | string = typeof left === 'boolean' ? (left ? 1 : 0) : left;
    const r: number | string = typeof right === 'boolean' ? (right ? 1 : 0) : right;

    const cmp = this.compareValues(l, r);
    switch (op) {
      case '>': return cmp > 0;
      case '<': return cmp < 0;
      case '>=': return cmp >= 0;
      case '<=': return cmp <= 0;
      case '=': return cmp === 0;
      case '<>':
      case '!=': return cmp !== 0;
      default: return false;
    }
  }

  /**
   * 比较两个值 (数字按数值比较, 否则按字符串比较)
   */
  private compareValues(a: number | string, b: number | string): number {
    const na = typeof a === 'number' ? a : Number(a);
    const nb = typeof b === 'number' ? b : Number(b);
    if (!isNaN(na) && !isNaN(nb)) {
      return na < nb ? -1 : na > nb ? 1 : 0;
    }
    const sa = String(a);
    const sb = String(b);
    return sa < sb ? -1 : sa > sb ? 1 : 0;
  }

  /**
   * 按顶层逗号分割参数 (尊重括号嵌套)
   */
  private splitArgs(s: string): string[] {
    const args: string[] = [];
    let depth = 0;
    let current = '';
    for (let i = 0; i < s.length; i++) {
      const ch = s[i];
      if (ch === '(') depth++;
      else if (ch === ')') depth--;
      if (ch === ',' && depth === 0) {
        args.push(current);
        current = '';
      } else {
        current += ch;
      }
    }
    if (current.trim()) args.push(current);
    return args;
  }

  // ============================================================
  // 范围统计 (带缓存)
  // ============================================================

  /**
   * 获取规则范围内的数值统计 (min/max/median/排序后值列表)
   */
  private getRangeStats(rule: ConditionalFormatRule): RangeStats | null {
    const cacheKey = rule.id;
    if (this.statsCache.has(cacheKey)) {
      return this.statsCache.get(cacheKey) ?? null;
    }

    const values: number[] = [];
    this.iterRangeCells(rule.range, (row, col) => {
      const v = this.getNumericValue(row, col);
      if (v !== null) values.push(v);
    });

    if (values.length === 0) {
      this.statsCache.set(cacheKey, null);
      return null;
    }

    values.sort((a, b) => a - b);
    const min = values[0];
    const max = values[values.length - 1];
    const mid = Math.floor(values.length / 2);
    const median =
      values.length % 2 === 0
        ? (values[mid - 1] + values[mid]) / 2
        : values[mid];

    const stats: RangeStats = { min, max, median, values };
    this.statsCache.set(cacheKey, stats);
    return stats;
  }

  /**
   * 获取规则范围内的值计数 (用于重复值判断)
   */
  private getValueCounts(rule: ConditionalFormatRule): Map<string, number> {
    const cacheKey = rule.id;
    if (this.valueCountsCache.has(cacheKey)) {
      return this.valueCountsCache.get(cacheKey)!;
    }

    const counts = new Map<string, number>();
    this.iterRangeCells(rule.range, (row, col) => {
      const v = this.sheet.getCellValue(row, col);
      if (v !== null && v !== undefined && v !== '') {
        const key = String(v);
        counts.set(key, (counts.get(key) ?? 0) + 1);
      }
    });

    this.valueCountsCache.set(cacheKey, counts);
    return counts;
  }

  // ============================================================
  // 辅助方法
  // ============================================================

  /** 生成唯一规则 id */
  private static generateId(): string {
    return `cf_${Date.now()}_${Math.random().toString(36).substring(2, 9)}`;
  }

  /** 失效所有缓存 */
  private invalidateCache(): void {
    this.statsCache.clear();
    this.valueCountsCache.clear();
  }

  /** 判断单元格是否在选区内 */
  private isInRange(row: number, col: number, range: SelectionRange): boolean {
    const sr = Math.min(range.startRow, range.endRow);
    const er = Math.max(range.startRow, range.endRow);
    const sc = Math.min(range.startCol, range.endCol);
    const ec = Math.max(range.startCol, range.endCol);
    return row >= sr && row <= er && col >= sc && col <= ec;
  }

  /** 遍历选区内的所有单元格 */
  private iterRangeCells(
    range: SelectionRange,
    fn: (row: number, col: number) => void,
  ): void {
    const sr = Math.min(range.startRow, range.endRow);
    const er = Math.max(range.startRow, range.endRow);
    const sc = Math.min(range.startCol, range.endCol);
    const ec = Math.max(range.startCol, range.endCol);
    for (let r = sr; r <= er; r++) {
      for (let c = sc; c <= ec; c++) {
        fn(r, c);
      }
    }
  }

  /** 获取单元格数值 (非数值返回 null) */
  private getNumericValue(row: number, col: number): number | null {
    const val = this.sheet.getCellValue(row, col);
    return this.toNumber(val);
  }

  /** 将任意值转为数字 (无法转换返回 null) */
  private toNumber(val: unknown): number | null {
    if (val === null || val === undefined) return null;
    if (typeof val === 'number') return isNaN(val) ? null : val;
    if (typeof val === 'boolean') return val ? 1 : 0;
    if (typeof val === 'string') {
      const trimmed = val.trim();
      if (trimmed === '') return null;
      const n = Number(trimmed);
      return isNaN(n) ? null : n;
    }
    return null;
  }
}
