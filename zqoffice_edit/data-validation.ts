/**
 * 数据验证层 — 单元格输入合法性校验 + 下拉列表 + 错误提示 + 输入提示
 * 全部原创实现
 *
 * 职责:
 *   1. 管理数据验证规则 (增删查)
 *   2. 校验单元格值是否合法 (list/number/text/date/custom 五种类型)
 *   3. 用户输入时校验并返回结构化结果
 *   4. 下拉列表 DOM 交互 (箭头 + 选项菜单)
 *   5. 非法值错误提示对话框 (stop/warning/information 三种样式)
 *   6. 鼠标悬停输入提示 (inputTitle/inputMessage)
 *
 * 与 EditEngine 的集成: EditEngine 在 commitEdit 前调用 validateInput,
 *   若返回 invalid, 调用 showErrorDialog 让用户决定 (重试/取消/继续)。
 */

import { SheetModel } from './data-model';
import { RenderEngine, SelectionRange } from './render-engine';

// ============================================================
// 类型定义
// ============================================================

/** 验证类型 */
export type ValidationType = 'list' | 'number' | 'text' | 'date' | 'custom';

/** 数值比较运算符 */
export type ValidationOperator =
  | 'between'
  | 'notBetween'
  | 'equal'
  | 'notEqual'
  | 'greaterThan'
  | 'lessThan'
  | 'greaterThanOrEqual'
  | 'lessThanOrEqual';

/** 错误样式: stop=阻止, warning=警告可继续, information=仅提示 */
export type ErrorStyle = 'stop' | 'warning' | 'information';

/** 数据验证规则 */
export interface DataValidationRule {
  id: string;
  type: ValidationType;
  range: SelectionRange;
  // list 类型
  listValues?: string[];      // 下拉选项 (与 listFormula 二选一)
  listFormula?: string;       // 引用公式, 如 "A1:A10" 或 "=Sheet1!A1:A10"
  // number 类型
  operator?: ValidationOperator;
  min?: number;
  max?: number;
  // text 类型
  textMin?: number;
  textMax?: number;
  // date 类型
  dateMin?: string;
  dateMax?: string;
  // custom 类型
  customFormula?: string;     // 如 "=AND(A1>0, A1<100)"
  // 通用
  allowBlank: boolean;        // 允许空值
  showDropdown: boolean;      // 显示下拉箭头
  errorTitle: string;         // 错误提示标题
  errorMessage: string;       // 错误提示内容
  errorStyle: ErrorStyle;     // 错误样式
  inputTitle?: string;        // 输入提示标题
  inputMessage?: string;      // 输入提示内容
}

/** 验证结果 */
export interface ValidationResult {
  valid: boolean;
  error?: string;             // 错误描述 (valid=false 时有值)
  errorStyle?: ErrorStyle;    // 错误样式 (来自规则)
}

/** 错误对话框用户选择 */
export type ErrorDialogAction = 'retry' | 'cancel' | 'continue';

/** 单元格值解析器 (用于 custom 公式中引用单元格取值) */
type CellResolver = (row: number, col: number) => number | string | boolean | null;

/** 下拉值应用回调 (选中下拉项时调用, 默认写入 sheet) */
export type DropdownSelectCallback = (row: number, col: number, value: string) => void;

// ============================================================
// 自定义公式求值器 — 支持 比较运算/算术/常用函数/单元格引用
// ============================================================

/** AST 节点 */
type AstNode =
  | { type: 'num'; value: number }
  | { type: 'str'; value: string }
  | { type: 'bool'; value: boolean }
  | { type: 'cell'; row: number; col: number }
  | { type: 'unary'; op: '-'; operand: AstNode }
  | { type: 'binary'; op: string; left: AstNode; right: AstNode }
  | { type: 'call'; name: string; args: AstNode[] };

/** Token 类型 */
type TokenType = 'num' | 'str' | 'ident' | 'cell' | 'op' | 'lparen' | 'rparen' | 'comma' | 'eof';

interface Token {
  type: TokenType;
  value: string;
}

/** 单元格值类型 */
type typeCellValue = number | string | boolean | null;

/**
 * 自定义公式求值器 (递归下降)
 * 支持:
 *   - 数字 / 字符串 / 布尔 (TRUE/FALSE)
 *   - 单元格引用 (A1, $A$1)
 *   - 算术: + - * / %
 *   - 比较: > < >= <= = <>
 *   - 括号, 一元负号
 *   - 函数: AND OR NOT ISNUMBER ISTEXT ISBLANK LEN LEFT RIGHT MOD ABS IF
 */
class CustomFormulaEvaluator {
  private resolver: CellResolver;
  private tokens: Token[] = [];
  private pos: number = 0;

  constructor(resolver: CellResolver) {
    this.resolver = resolver;
  }

  /** 求值: 返回布尔结果 (公式是否成立) */
  evaluate(formula: string): boolean {
    const expr = formula.trim();
    const f = expr.startsWith('=') ? expr.slice(1) : expr;
    if (f.length === 0) return true;
    this.tokens = this.tokenize(f);
    this.pos = 0;
    const ast = this.parseExpression();
    if (this.peek().type !== 'eof') {
      throw new Error('公式存在未解析的尾部字符');
    }
    const result = this.evalNode(ast);
    return this.toBool(result);
  }

  // -------- 词法分析 --------

  private tokenize(s: string): Token[] {
    const tokens: Token[] = [];
    let i = 0;
    const n = s.length;
    while (i < n) {
      const ch = s[i];
      // 空白
      if (ch === ' ' || ch === '\t' || ch === '\n' || ch === '\r') {
        i++;
        continue;
      }
      // 字符串
      if (ch === '"') {
        let j = i + 1;
        let str = '';
        while (j < n) {
          if (s[j] === '"') {
            if (s[j + 1] === '"') { str += '"'; j += 2; continue; }
            break;
          }
          str += s[j];
          j++;
        }
        if (j >= n) throw new Error('字符串未闭合');
        tokens.push({ type: 'str', value: str });
        i = j + 1;
        continue;
      }
      // 数字
      if ((ch >= '0' && ch <= '9') || (ch === '.' && s[i + 1] >= '0' && s[i + 1] <= '9')) {
        let j = i;
        let num = '';
        while (j < n && ((s[j] >= '0' && s[j] <= '9') || s[j] === '.')) {
          num += s[j];
          j++;
        }
        // 科学计数法
        if (j < n && (s[j] === 'e' || s[j] === 'E')) {
          num += s[j]; j++;
          if (j < n && (s[j] === '+' || s[j] === '-')) { num += s[j]; j++; }
          while (j < n && s[j] >= '0' && s[j] <= '9') { num += s[j]; j++; }
        }
        tokens.push({ type: 'num', value: num });
        i = j;
        continue;
      }
      // 单元格引用或标识符: [$]字母[$]数字
      if (ch === '$' || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch === '_') {
        // 先尝试匹配单元格引用: \$?[A-Za-z]+\$?\d+ (字母后紧跟数字, 不会与函数名冲突)
        const cellMatch = /^(\$?)[A-Za-z]+(\$?)(\d+)/.exec(s.slice(i));
        if (cellMatch) {
          const fullMatch = cellMatch[0];
          tokens.push({ type: 'cell', value: fullMatch });
          i += fullMatch.length;
          continue;
        }
        // 标识符 (函数名或 TRUE/FALSE)
        let j = i;
        let ident = '';
        while (j < n && ((s[j] >= 'A' && s[j] <= 'Z') || (s[j] >= 'a' && s[j] <= 'z') || (s[j] >= '0' && s[j] <= '9') || s[j] === '_')) {
          ident += s[j];
          j++;
        }
        tokens.push({ type: 'ident', value: ident });
        i = j;
        continue;
      }
      // 比较运算符 (双字符优先)
      const two = s.slice(i, i + 2);
      if (two === '>=' || two === '<=' || two === '<>') {
        tokens.push({ type: 'op', value: two });
        i += 2;
        continue;
      }
      if (ch === '>' || ch === '<' || ch === '=') {
        tokens.push({ type: 'op', value: ch });
        i++;
        continue;
      }
      // 算术运算符
      if (ch === '+' || ch === '-' || ch === '*' || ch === '/' || ch === '%') {
        tokens.push({ type: 'op', value: ch });
        i++;
        continue;
      }
      if (ch === '(') { tokens.push({ type: 'lparen', value: ch }); i++; continue; }
      if (ch === ')') { tokens.push({ type: 'rparen', value: ch }); i++; continue; }
      if (ch === ',') { tokens.push({ type: 'comma', value: ch }); i++; continue; }
      throw new Error(`无法识别的字符: ${ch}`);
    }
    tokens.push({ type: 'eof', value: '' });
    return tokens;
  }

  // -------- 语法分析 (递归下降) --------

  private peek(): Token {
    return this.tokens[this.pos];
  }

  private next(): Token {
    return this.tokens[this.pos++];
  }

  /** 表达式 (比较级) */
  private parseExpression(): AstNode {
    const left = this.parseAdditive();
    const t = this.peek();
    if (t.type === 'op' && (t.value === '>' || t.value === '<' || t.value === '>=' || t.value === '<=' || t.value === '=' || t.value === '<>')) {
      this.next();
      const right = this.parseAdditive();
      return { type: 'binary', op: t.value, left, right };
    }
    return left;
  }

  /** 加减级 */
  private parseAdditive(): AstNode {
    let left = this.parseMultiplicative();
    while (true) {
      const t = this.peek();
      if (t.type === 'op' && (t.value === '+' || t.value === '-')) {
        this.next();
        const right = this.parseMultiplicative();
        left = { type: 'binary', op: t.value, left, right };
      } else {
        break;
      }
    }
    return left;
  }

  /** 乘除模级 */
  private parseMultiplicative(): AstNode {
    let left = this.parseUnary();
    while (true) {
      const t = this.peek();
      if (t.type === 'op' && (t.value === '*' || t.value === '/' || t.value === '%')) {
        this.next();
        const right = this.parseUnary();
        left = { type: 'binary', op: t.value, left, right };
      } else {
        break;
      }
    }
    return left;
  }

  /** 一元负号 */
  private parseUnary(): AstNode {
    const t = this.peek();
    if (t.type === 'op' && t.value === '-') {
      this.next();
      const operand = this.parseUnary();
      return { type: 'unary', op: '-', operand };
    }
    return this.parsePrimary();
  }

  /** 基础元素 */
  private parsePrimary(): AstNode {
    const t = this.next();
    switch (t.type) {
      case 'num': {
        const v = Number(t.value);
        if (isNaN(v)) throw new Error(`无效数字: ${t.value}`);
        return { type: 'num', value: v };
      }
      case 'str':
        return { type: 'str', value: t.value };
      case 'cell': {
        const addr = this.parseCellAddress(t.value);
        if (!addr) throw new Error(`无效单元格引用: ${t.value}`);
        return { type: 'cell', row: addr.row, col: addr.col };
      }
      case 'ident': {
        const upper = t.value.toUpperCase();
        // 布尔常量
        if (upper === 'TRUE') return { type: 'bool', value: true };
        if (upper === 'FALSE') return { type: 'bool', value: false };
        // 函数调用
        const next = this.peek();
        if (next.type === 'lparen') {
          this.next(); // consume '('
          const args: AstNode[] = [];
          if (this.peek().type !== 'rparen') {
            args.push(this.parseExpression());
            while (this.peek().type === 'comma') {
              this.next();
              args.push(this.parseExpression());
            }
          }
          if (this.peek().type !== 'rparen') throw new Error('函数缺少右括号');
          this.next(); // consume ')'
          return { type: 'call', name: upper, args };
        }
        throw new Error(`未知标识符: ${t.value}`);
      }
      case 'lparen': {
        const inner = this.parseExpression();
        if (this.peek().type !== 'rparen') throw new Error('缺少右括号');
        this.next();
        return inner;
      }
      default:
        throw new Error(`意外的 token: ${t.value || t.type}`);
    }
  }

  /** 解析单元格地址 (A1 / $A$1) -> {row, col} (0-based) */
  private parseCellAddress(addr: string): { row: number; col: number } | null {
    const m = /^\$?([A-Za-z]+)\$?(\d+)$/.exec(addr);
    if (!m) return null;
    const col = SheetModel.letterToColumn(m[1].toUpperCase());
    const row = parseInt(m[2], 10) - 1;
    if (row < 0 || col < 0) return null;
    return { row, col };
  }

  // -------- 求值 --------

  private evalNode(node: AstNode): typeCellValue {
    switch (node.type) {
      case 'num':
        return node.value;
      case 'str':
        return node.value;
      case 'bool':
        return node.value;
      case 'cell':
        return this.resolver(node.row, node.col);
      case 'unary': {
        const v = this.toNumber(this.evalNode(node.operand));
        return -v;
      }
      case 'binary':
        return this.evalBinary(node);
      case 'call':
        return this.evalCall(node);
    }
  }

  private evalBinary(node: { type: 'binary'; op: string; left: AstNode; right: AstNode }): typeCellValue {
    const a = this.evalNode(node.left);
    const b = this.evalNode(node.right);
    const op = node.op;
    switch (op) {
      case '+': return this.toNumber(a) + this.toNumber(b);
      case '-': return this.toNumber(a) - this.toNumber(b);
      case '*': return this.toNumber(a) * this.toNumber(b);
      case '/': {
        const denom = this.toNumber(b);
        if (denom === 0) throw new Error('除以零');
        return this.toNumber(a) / denom;
      }
      case '%': {
        const denom = this.toNumber(b);
        if (denom === 0) throw new Error('模零');
        return this.toNumber(a) % denom;
      }
      case '>': return this.compare(a, b) > 0;
      case '<': return this.compare(a, b) < 0;
      case '>=': return this.compare(a, b) >= 0;
      case '<=': return this.compare(a, b) <= 0;
      case '=': return this.compare(a, b) === 0;
      case '<>': return this.compare(a, b) !== 0;
      default:
        throw new Error(`未知运算符: ${op}`);
    }
  }

  /** 比较 a 和 b, 返回 -1/0/1 */
  private compare(a: typeCellValue, b: typeCellValue): number {
    // null 视为最小
    if (a === null && b === null) return 0;
    if (a === null) return -1;
    if (b === null) return 1;
    // 两者都是数字
    if (typeof a === 'number' && typeof b === 'number') {
      return a < b ? -1 : a > b ? 1 : 0;
    }
    // 两者都是布尔
    if (typeof a === 'boolean' && typeof b === 'boolean') {
      const na = a ? 1 : 0;
      const nb = b ? 1 : 0;
      return na < nb ? -1 : na > nb ? 1 : 0;
    }
    // 字符串比较 (统一转字符串)
    const sa = typeof a === 'string' ? a : String(a);
    const sb = typeof b === 'string' ? b : String(b);
    return sa < sb ? -1 : sa > sb ? 1 : 0;
  }

  private evalCall(node: { type: 'call'; name: string; args: AstNode[] }): typeCellValue {
    const name = node.name;
    const args = node.args;
    switch (name) {
      case 'AND': {
        if (args.length === 0) return true;
        for (const arg of args) {
          if (!this.toBool(this.evalNode(arg))) return false;
        }
        return true;
      }
      case 'OR': {
        for (const arg of args) {
          if (this.toBool(this.evalNode(arg))) return true;
        }
        return false;
      }
      case 'NOT': {
        if (args.length !== 1) throw new Error('NOT 需要 1 个参数');
        return !this.toBool(this.evalNode(args[0]));
      }
      case 'IF': {
        if (args.length < 2 || args.length > 3) throw new Error('IF 需要 2 或 3 个参数');
        const cond = this.toBool(this.evalNode(args[0]));
        if (cond) return this.evalNode(args[1]);
        return args.length === 3 ? this.evalNode(args[2]) : false;
      }
      case 'ISNUMBER': {
        if (args.length !== 1) throw new Error('ISNUMBER 需要 1 个参数');
        return typeof this.evalNode(args[0]) === 'number';
      }
      case 'ISTEXT': {
        if (args.length !== 1) throw new Error('ISTEXT 需要 1 个参数');
        return typeof this.evalNode(args[0]) === 'string';
      }
      case 'ISBLANK': {
        if (args.length !== 1) throw new Error('ISBLANK 需要 1 个参数');
        const v = this.evalNode(args[0]);
        return v === null || v === '';
      }
      case 'LEN': {
        if (args.length !== 1) throw new Error('LEN 需要 1 个参数');
        const v = this.evalNode(args[0]);
        return String(v === null ? '' : v).length;
      }
      case 'LEFT': {
        if (args.length < 1 || args.length > 2) throw new Error('LEFT 需要 1 或 2 个参数');
        const s = String(this.evalNode(args[0]) ?? '');
        const n = args.length === 2 ? this.toNumber(this.evalNode(args[1])) : 1;
        return s.slice(0, Math.max(0, Math.floor(n)));
      }
      case 'RIGHT': {
        if (args.length < 1 || args.length > 2) throw new Error('RIGHT 需要 1 或 2 个参数');
        const s = String(this.evalNode(args[0]) ?? '');
        const n = args.length === 2 ? this.toNumber(this.evalNode(args[1])) : 1;
        const cnt = Math.max(0, Math.floor(n));
        return cnt === 0 ? '' : s.slice(-cnt);
      }
      case 'MOD': {
        if (args.length !== 2) throw new Error('MOD 需要 2 个参数');
        const a = this.toNumber(this.evalNode(args[0]));
        const b = this.toNumber(this.evalNode(args[1]));
        if (b === 0) throw new Error('MOD 除以零');
        return a % b;
      }
      case 'ABS': {
        if (args.length !== 1) throw new Error('ABS 需要 1 个参数');
        return Math.abs(this.toNumber(this.evalNode(args[0])));
      }
      case 'TRUE': return true;
      case 'FALSE': return false;
      default:
        throw new Error(`未知函数: ${name}`);
    }
  }

  /** 转为数字 */
  private toNumber(v: typeCellValue): number {
    if (typeof v === 'number') return v;
    if (typeof v === 'boolean') return v ? 1 : 0;
    if (v === null || v === '') return 0;
    const n = Number(v);
    return isNaN(n) ? 0 : n;
  }

  /** 转为布尔 */
  private toBool(v: typeCellValue): boolean {
    if (typeof v === 'boolean') return v;
    if (typeof v === 'number') return v !== 0;
    if (typeof v === 'string') return v.toUpperCase() === 'TRUE';
    return false;
  }
}

// ============================================================
// 数据验证管理器
// ============================================================

/** 数据验证管理器配置回调 */
export interface DataValidationCallbacks {
  /** 下拉项被选中时调用 (未设置则直接写入 sheet) */
  onSelectDropdownValue?: DropdownSelectCallback;
  /** 内容变化时调用 (默认写入后刷新渲染, 若设置则由调用方负责) */
  onContentChange?: () => void;
}

export class DataValidationManager {
  private container: HTMLElement;
  private sheet: SheetModel;
  private renderEngine: RenderEngine;
  private callbacks: DataValidationCallbacks;

  /** 所有验证规则 (按 id 索引) */
  private rules = new Map<string, DataValidationRule>();
  /** 规则插入顺序 */
  private ruleOrder: string[] = [];
  /** 单元格 -> 规则 的索引 (懒构建, 规则变更时重建) */
  private cellIndex = new Map<string, DataValidationRule>();
  private indexDirty: boolean = true;

  // DOM 元素
  private dropdownArrows = new Map<string, HTMLDivElement>(); // key = "row:col"
  private dropdownMenu: HTMLDivElement | null = null;
  private errorDialog: HTMLDivElement | null = null;
  private inputTooltip: HTMLDivElement | null = null;

  /** 当前下拉菜单对应的单元格 */
  private dropdownCell: { row: number; col: number } | null = null;
  /** 当前输入提示对应的单元格 */
  private tooltipCell: { row: number; col: number } | null = null;

  /** 滚动重绘节流句柄 */
  private scrollRafId: number | null = null;

  /** 已绑定的事件监听器引用 (用于销毁时移除) */
  private boundScrollHandler: (() => void) | null = null;
  private boundArrowClickHandler: ((e: MouseEvent) => void) | null = null;
  private boundHoverHandler: ((e: MouseEvent) => void) | null = null;
  private boundDocClickHandler: ((e: MouseEvent) => void) | null = null;

  constructor(
    container: HTMLElement,
    sheet: SheetModel,
    renderEngine: RenderEngine,
    callbacks: DataValidationCallbacks = {}
  ) {
    this.container = container;
    this.sheet = sheet;
    this.renderEngine = renderEngine;
    this.callbacks = callbacks;

    this.setupStyles();
    this.bindDomEvents();
  }

  // ============================================================
  // 样式注入
  // ============================================================

  private setupStyles(): void {
    if (document.getElementById('zq-dv-styles')) return;
    const style = document.createElement('style');
    style.id = 'zq-dv-styles';
    style.textContent = `
      .zq-dv-arrow {
        position: absolute;
        width: 14px;
        height: 100%;
        right: 0;
        top: 0;
        display: flex;
        align-items: center;
        justify-content: center;
        cursor: pointer;
        background: linear-gradient(to right, rgba(255,255,255,0) 0%, rgba(255,255,255,0.85) 50%, #fff 100%);
        font-size: 9px;
        color: #666;
        z-index: 9;
        user-select: none;
      }
      .zq-dv-arrow:hover {
        color: #4787f5;
      }
      .zq-dv-menu {
        position: absolute;
        z-index: 200;
        background: #fff;
        border: 1px solid #d0d0d0;
        box-shadow: 0 4px 12px rgba(0,0,0,0.18);
        max-height: 240px;
        overflow-y: auto;
        min-width: 100px;
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
        font-size: 13px;
      }
      .zq-dv-menu-item {
        padding: 6px 12px;
        cursor: pointer;
        white-space: nowrap;
      }
      .zq-dv-menu-item:hover {
        background: #e8f0fe;
        color: #4787f5;
      }
      .zq-dv-menu-empty {
        padding: 8px 12px;
        color: #999;
        font-style: italic;
      }
      .zq-dv-dialog-mask {
        position: fixed;
        top: 0; left: 0; right: 0; bottom: 0;
        background: rgba(0,0,0,0.25);
        z-index: 1000;
        display: flex;
        align-items: center;
        justify-content: center;
      }
      .zq-dv-dialog {
        background: #fff;
        border-radius: 6px;
        box-shadow: 0 8px 24px rgba(0,0,0,0.25);
        width: 360px;
        max-width: 90vw;
        overflow: hidden;
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
      }
      .zq-dv-dialog-icon {
        width: 44px;
        height: 44px;
        border-radius: 50%;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 24px;
        margin-right: 12px;
        flex-shrink: 0;
      }
      .zq-dv-dialog-icon.stop { background: #ffe4e4; color: #e53935; }
      .zq-dv-dialog-icon.warning { background: #fff4e0; color: #f5a623; }
      .zq-dv-dialog-icon.information { background: #e3f2fd; color: #4787f5; }
      .zq-dv-dialog-header {
        display: flex;
        align-items: center;
        padding: 18px 20px 8px;
      }
      .zq-dv-dialog-title {
        font-size: 15px;
        font-weight: 600;
        color: #222;
        word-break: break-all;
      }
      .zq-dv-dialog-body {
        padding: 4px 20px 18px 76px;
        font-size: 13px;
        color: #444;
        line-height: 1.5;
        word-break: break-all;
      }
      .zq-dv-dialog-footer {
        padding: 10px 20px 16px;
        display: flex;
        justify-content: flex-end;
        gap: 8px;
      }
      .zq-dv-btn {
        padding: 6px 16px;
        border: 1px solid #d0d0d0;
        background: #fff;
        border-radius: 4px;
        cursor: pointer;
        font-size: 13px;
        color: #333;
        font-family: inherit;
      }
      .zq-dv-btn:hover { background: #f5f5f5; }
      .zq-dv-btn-primary {
        background: #4787f5;
        border-color: #4787f5;
        color: #fff;
      }
      .zq-dv-btn-primary:hover { background: #3676e0; }
      .zq-dv-tooltip {
        position: absolute;
        z-index: 250;
        background: #fffbe6;
        border: 1px solid #f5d76e;
        border-radius: 4px;
        padding: 8px 10px;
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
        font-size: 12px;
        color: #5a4500;
        max-width: 280px;
        box-shadow: 0 2px 8px rgba(0,0,0,0.12);
        pointer-events: none;
      }
      .zq-dv-tooltip-title {
        font-weight: 600;
        margin-bottom: 4px;
        color: #8a6d00;
      }
    `;
    document.head.appendChild(style);
  }

  // ============================================================
  // DOM 事件绑定
  // ============================================================

  private bindDomEvents(): void {
    // 滚动时重绘下拉箭头并隐藏菜单/提示
    const canvas = this.container.querySelector('.zq-sheet-canvas') as HTMLElement | null;
    if (canvas) {
      this.boundScrollHandler = () => {
        this.hideDropdownMenu();
        this.hideInputTooltip();
        if (this.scrollRafId !== null) return;
        this.scrollRafId = window.requestAnimationFrame(() => {
          this.scrollRafId = null;
          this.refreshDropdownArrows();
        });
      };
      canvas.addEventListener('scroll', this.boundScrollHandler);
    }

    // 点击箭头 (事件委托)
    this.boundArrowClickHandler = (e: MouseEvent) => {
      const target = e.target as HTMLElement;
      if (target.classList.contains('zq-dv-arrow')) {
        e.preventDefault();
        e.stopPropagation();
        const cellEl = target.parentElement;
        if (!cellEl) return;
        const row = parseInt(cellEl.dataset.row || '-1', 10);
        const col = parseInt(cellEl.dataset.col || '-1', 10);
        if (row < 0 || col < 0) return;
        this.toggleDropdownMenu(row, col);
      }
    };
    this.container.addEventListener('click', this.boundArrowClickHandler, true);

    // 鼠标悬停显示输入提示 (用 closest 兼容单元格内的子元素, 如下拉箭头)
    this.boundHoverHandler = (e: MouseEvent) => {
      const target = e.target as HTMLElement;
      const cellEl = target.closest ? (target.closest('.zq-cell') as HTMLDivElement | null) : null;
      if (cellEl) {
        const row = parseInt(cellEl.dataset.row || '-1', 10);
        const col = parseInt(cellEl.dataset.col || '-1', 10);
        if (row < 0 || col < 0) {
          this.hideInputTooltip();
          return;
        }
        const rule = this.getRuleForCell(row, col);
        if (rule && (rule.inputTitle || rule.inputMessage)) {
          this.showInputTooltip(row, col, rule);
        } else {
          this.hideInputTooltip();
        }
      } else if (!target.closest('.zq-dv-tooltip')) {
        this.hideInputTooltip();
      }
    };
    this.container.addEventListener('mouseover', this.boundHoverHandler);

    // 点击外部关闭下拉菜单
    this.boundDocClickHandler = (e: MouseEvent) => {
      if (this.dropdownMenu && !this.dropdownMenu.contains(e.target as Node)) {
        const target = e.target as HTMLElement;
        if (!target.classList.contains('zq-dv-arrow')) {
          this.hideDropdownMenu();
        }
      }
    };
    document.addEventListener('mousedown', this.boundDocClickHandler);
  }

  // ============================================================
  // 规则管理
  // ============================================================

  /** 添加验证规则 (若 id 已存在则覆盖) */
  addRule(rule: DataValidationRule): void {
    // 基本校验
    if (!rule.id) throw new Error('规则 id 不能为空');
    if (!this.rules.has(rule.id)) {
      this.ruleOrder.push(rule.id);
    }
    this.rules.set(rule.id, { ...rule });
    this.indexDirty = true;
    this.refreshDropdownArrows();
  }

  /** 删除规则 */
  removeRule(id: string): boolean {
    const existed = this.rules.delete(id);
    if (existed) {
      this.ruleOrder = this.ruleOrder.filter(rid => rid !== id);
      this.indexDirty = true;
      this.refreshDropdownArrows();
    }
    return existed;
  }

  /** 按 id 获取规则 */
  getRuleById(id: string): DataValidationRule | null {
    return this.rules.get(id) ?? null;
  }

  /** 获取所有规则 (按添加顺序) */
  getRules(): DataValidationRule[] {
    return this.ruleOrder.map(id => this.rules.get(id)!).filter(r => r !== undefined);
  }

  /** 清空所有规则 */
  clearRules(): void {
    this.rules.clear();
    this.ruleOrder = [];
    this.cellIndex.clear();
    this.indexDirty = false;
    this.clearArrows();
    this.hideDropdownMenu();
    this.hideInputTooltip();
  }

  /** 获取单元格的验证规则 (一个单元格只取第一条匹配规则) */
  getRuleForCell(row: number, col: number): DataValidationRule | null {
    this.ensureIndex();
    return this.cellIndex.get(`${row}:${col}`) ?? null;
  }

  /** 该单元格是否具有验证规则 */
  hasValidation(row: number, col: number): boolean {
    return this.getRuleForCell(row, col) !== null;
  }

  /** 该单元格是否具有下拉列表 */
  hasDropdown(row: number, col: number): boolean {
    const rule = this.getRuleForCell(row, col);
    return rule !== null && rule.type === 'list' && rule.showDropdown;
  }

  /** 重建单元格索引 */
  private ensureIndex(): void {
    if (!this.indexDirty) return;
    this.cellIndex.clear();
    for (const id of this.ruleOrder) {
      const rule = this.rules.get(id);
      if (!rule) continue;
      const sr = Math.min(rule.range.startRow, rule.range.endRow);
      const er = Math.max(rule.range.startRow, rule.range.endRow);
      const sc = Math.min(rule.range.startCol, rule.range.endCol);
      const ec = Math.max(rule.range.startCol, rule.range.endCol);
      for (let r = sr; r <= er; r++) {
        for (let c = sc; c <= ec; c++) {
          // 仅在尚未有规则时设置 (先添加的规则优先)
          const key = `${r}:${c}`;
          if (!this.cellIndex.has(key)) {
            this.cellIndex.set(key, rule);
          }
        }
      }
    }
    this.indexDirty = false;
  }

  // ============================================================
  // 验证 — 核心逻辑
  // ============================================================

  /**
   * 验证单元格值是否合法
   * @param row 行
   * @param col 列
   * @param value 待验证的值 (string | number | boolean | null)
   */
  validateCell(row: number, col: number, value: string | number | boolean | null): ValidationResult {
    const rule = this.getRuleForCell(row, col);
    if (!rule) return { valid: true };
    return this.runValidation(rule, row, col, value);
  }

  /**
   * 验证用户输入 (字符串形式)
   * @param row 行
   * @param col 列
   * @param input 用户输入的原始字符串
   */
  validateInput(row: number, col: number, input: string): ValidationResult {
    const rule = this.getRuleForCell(row, col);
    if (!rule) return { valid: true };
    // 空输入: 由 allowBlank 决定
    if (input === '' || input == null) {
      if (rule.allowBlank) return { valid: true };
      return {
        valid: false,
        error: rule.errorMessage || '该单元格不能为空',
        errorStyle: rule.errorStyle,
      };
    }
    // list 类型: 直接按字符串比对 (不做类型推断)
    if (rule.type === 'list') {
      return this.runValidation(rule, row, col, input);
    }
    // number / date 类型: 推断值类型
    if (rule.type === 'number') {
      const num = Number(input.trim());
      if (input.trim() !== '' && isNaN(num)) {
        return {
          valid: false,
          error: rule.errorMessage || `请输入有效数字`,
          errorStyle: rule.errorStyle,
        };
      }
      return this.runValidation(rule, row, col, num);
    }
    if (rule.type === 'date') {
      const parsed = parseDate(input);
      if (parsed === null) {
        return {
          valid: false,
          error: rule.errorMessage || `请输入有效日期`,
          errorStyle: rule.errorStyle,
        };
      }
      return this.runValidation(rule, row, col, input);
    }
    // text / custom: 按字符串处理
    return this.runValidation(rule, row, col, input);
  }

  /** 执行具体验证 (分类型) */
  private runValidation(
    rule: DataValidationRule,
    row: number,
    col: number,
    value: string | number | boolean | null
  ): ValidationResult {
    // 空值
    if (value === null || value === '') {
      if (rule.allowBlank) return { valid: true };
      return {
        valid: false,
        error: rule.errorMessage || '该单元格不能为空',
        errorStyle: rule.errorStyle,
      };
    }

    let valid = false;
    let reason = '';
    switch (rule.type) {
      case 'list':
        ({ valid, reason } = this.validateList(rule, value));
        break;
      case 'number':
        ({ valid, reason } = this.validateNumber(rule, value));
        break;
      case 'text':
        ({ valid, reason } = this.validateText(rule, value));
        break;
      case 'date':
        ({ valid, reason } = this.validateDate(rule, value));
        break;
      case 'custom':
        ({ valid, reason } = this.validateCustom(rule, row, col, value));
        break;
    }

    if (valid) return { valid: true };
    return {
      valid: false,
      error: rule.errorMessage || reason || '输入值不合法',
      errorStyle: rule.errorStyle,
    };
  }

  /** list 验证: 值必须在选项列表中 */
  private validateList(rule: DataValidationRule, value: string | number | boolean | null): { valid: boolean; reason: string } {
    const options = this.resolveListValues(rule);
    const str = String(value);
    if (options.length === 0) {
      return { valid: false, reason: '下拉选项为空' };
    }
    if (options.indexOf(str) >= 0) return { valid: true, reason: '' };
    return { valid: false, reason: `值 "${str}" 不在下拉列表中` };
  }

  /** number 验证: 按运算符比较 */
  private validateNumber(rule: DataValidationRule, value: string | number | boolean | null): { valid: boolean; reason: string } {
    if (typeof value === 'boolean') {
      // 布尔不视为数字
      return { valid: false, reason: '需要数字类型' };
    }
    const num = typeof value === 'number' ? value : Number(value);
    if (isNaN(num)) {
      return { valid: false, reason: '需要有效数字' };
    }
    const op = rule.operator ?? 'between';
    const min = rule.min;
    const max = rule.max;
    switch (op) {
      case 'between':
        if (min !== undefined && num < min) return { valid: false, reason: `数值必须 ≥ ${min}` };
        if (max !== undefined && num > max) return { valid: false, reason: `数值必须 ≤ ${max}` };
        return { valid: true, reason: '' };
      case 'notBetween':
        if (min !== undefined && max !== undefined && num >= min && num <= max) {
          return { valid: false, reason: `数值不能在 ${min} 与 ${max} 之间` };
        }
        return { valid: true, reason: '' };
      case 'equal':
        if (min !== undefined && num !== min) return { valid: false, reason: `数值必须等于 ${min}` };
        return { valid: true, reason: '' };
      case 'notEqual':
        if (min !== undefined && num === min) return { valid: false, reason: `数值不能等于 ${min}` };
        return { valid: true, reason: '' };
      case 'greaterThan':
        if (min !== undefined && !(num > min)) return { valid: false, reason: `数值必须大于 ${min}` };
        return { valid: true, reason: '' };
      case 'lessThan':
        if (max !== undefined && !(num < max)) return { valid: false, reason: `数值必须小于 ${max}` };
        return { valid: true, reason: '' };
      case 'greaterThanOrEqual':
        if (min !== undefined && !(num >= min)) return { valid: false, reason: `数值必须大于等于 ${min}` };
        return { valid: true, reason: '' };
      case 'lessThanOrEqual':
        if (max !== undefined && !(num <= max)) return { valid: false, reason: `数值必须小于等于 ${max}` };
        return { valid: true, reason: '' };
      default:
        return { valid: false, reason: `未知运算符: ${op}` };
    }
  }

  /** text 验证: 文本长度范围 (textMin ~ textMax, between 语义) */
  private validateText(rule: DataValidationRule, value: string | number | boolean | null): { valid: boolean; reason: string } {
    const len = String(value).length;
    if (rule.textMin !== undefined && len < rule.textMin) {
      return { valid: false, reason: `文本长度必须 ≥ ${rule.textMin}` };
    }
    if (rule.textMax !== undefined && len > rule.textMax) {
      return { valid: false, reason: `文本长度必须 ≤ ${rule.textMax}` };
    }
    return { valid: true, reason: '' };
  }

  /** date 验证: 日期范围 (dateMin ~ dateMax, between 语义) */
  private validateDate(rule: DataValidationRule, value: string | number | boolean | null): { valid: boolean; reason: string } {
    const v = parseDate(value);
    if (v === null) {
      return { valid: false, reason: '需要有效日期' };
    }
    if (rule.dateMin) {
      const dmin = parseDate(rule.dateMin);
      if (dmin === null) return { valid: false, reason: `规则的起始日期无效: ${rule.dateMin}` };
      if (v.getTime() < dmin.getTime()) {
        return { valid: false, reason: `日期必须 ≥ ${rule.dateMin}` };
      }
    }
    if (rule.dateMax) {
      const dmax = parseDate(rule.dateMax);
      if (dmax === null) return { valid: false, reason: `规则的结束日期无效: ${rule.dateMax}` };
      if (v.getTime() > dmax.getTime()) {
        return { valid: false, reason: `日期必须 ≤ ${rule.dateMax}` };
      }
    }
    return { valid: true, reason: '' };
  }

  /** custom 验证: 自定义公式求值为布尔 */
  private validateCustom(
    rule: DataValidationRule,
    row: number,
    col: number,
    value: string | number | boolean | null
  ): { valid: boolean; reason: string } {
    if (!rule.customFormula) {
      return { valid: false, reason: 'custom 类型缺少 customFormula' };
    }
    // 构造单元格解析器: 当前单元格使用待验证值, 其余从 sheet 取
    const resolver: CellResolver = (r, c) => {
      if (r === row && c === col) {
        return value;
      }
      return this.sheet.getCellValue(r, c);
    };
    try {
      const evaluator = new CustomFormulaEvaluator(resolver);
      const ok = evaluator.evaluate(rule.customFormula);
      if (ok) return { valid: true, reason: '' };
      return { valid: false, reason: `自定义公式 "${rule.customFormula}" 验证未通过` };
    } catch (err) {
      const msg = err instanceof Error ? err.message : String(err);
      return { valid: false, reason: `公式求值错误: ${msg}` };
    }
  }

  // ============================================================
  // 下拉列表值解析
  // ============================================================

  /** 获取下拉选项 (供外部调用) */
  getDropdownValues(row: number, col: number): string[] | null {
    const rule = this.getRuleForCell(row, col);
    if (!rule || rule.type !== 'list') return null;
    return this.resolveListValues(rule);
  }

  /** 解析下拉选项: 优先 listValues, 其次 listFormula 引用范围 */
  private resolveListValues(rule: DataValidationRule): string[] {
    if (rule.listValues && rule.listValues.length > 0) {
      return rule.listValues.slice();
    }
    if (rule.listFormula) {
      return this.resolveListFormula(rule.listFormula);
    }
    return [];
  }

  /** 解析引用公式为选项列表 (支持 "A1:A10" / "=A1:A10" / "Sheet1!A1:A10") */
  private resolveListFormula(formula: string): string[] {
    let f = formula.trim();
    if (f.startsWith('=')) f = f.slice(1);
    // 去除工作表名前缀
    const bangIdx = f.indexOf('!');
    if (bangIdx >= 0) f = f.slice(bangIdx + 1);
    f = f.trim();
    // 范围 "A1:B2" 或单个 "A1"
    const parts = f.split(':').map(p => p.trim());
    const start = parseCellAddress(parts[0]);
    if (!start) return [];
    let endRow = start.row;
    let endCol = start.col;
    if (parts.length >= 2) {
      const end = parseCellAddress(parts[1]);
      if (!end) return [];
      endRow = end.row;
      endCol = end.col;
    }
    const sr = Math.min(start.row, endRow);
    const er = Math.max(start.row, endRow);
    const sc = Math.min(start.col, endCol);
    const ec = Math.max(start.col, endCol);
    const values: string[] = [];
    for (let r = sr; r <= er; r++) {
      for (let c = sc; c <= ec; c++) {
        const v = this.sheet.getCellValue(r, c);
        if (v !== null && v !== '') {
          values.push(String(v));
        }
      }
    }
    return values;
  }

  // ============================================================
  // DOM 交互 — 下拉箭头
  // ============================================================

  /** 重绘所有可见单元格的下拉箭头 */
  refreshDropdownArrows(): void {
    this.clearArrows();
    this.ensureIndex();

    // 遍历当前渲染的单元格 DOM (虚拟滚动下仅可见单元格在 DOM 中)
    const cellEls = this.container.querySelectorAll('.zq-cell');
    cellEls.forEach(el => {
      const cellEl = el as HTMLDivElement;
      const row = parseInt(cellEl.dataset.row || '-1', 10);
      const col = parseInt(cellEl.dataset.col || '-1', 10);
      if (row < 0 || col < 0) return;
      const rule = this.cellIndex.get(`${row}:${col}`);
      if (!rule || rule.type !== 'list' || !rule.showDropdown) return;
      this.attachArrow(cellEl, row, col);
    });
  }

  /** 给单元格附加下拉箭头 */
  private attachArrow(cellEl: HTMLDivElement, row: number, col: number): void {
    const key = `${row}:${col}`;
    // 已存在则跳过
    if (this.dropdownArrows.has(key)) return;
    // 确保单元格是相对定位容器
    const pos = getComputedStyle(cellEl).position;
    if (pos === 'static') {
      cellEl.style.position = 'relative';
    }
    const arrow = document.createElement('div');
    arrow.className = 'zq-dv-arrow';
    arrow.innerHTML = '&#9660;'; // ▼
    arrow.title = '点击查看下拉选项';
    cellEl.appendChild(arrow);
    this.dropdownArrows.set(key, arrow);
  }

  /** 清除所有下拉箭头 */
  private clearArrows(): void {
    this.dropdownArrows.forEach(arrow => arrow.remove());
    this.dropdownArrows.clear();
  }

  // ============================================================
  // DOM 交互 — 下拉选项菜单
  // ============================================================

  /** 切换下拉菜单显示/隐藏 */
  private toggleDropdownMenu(row: number, col: number): void {
    if (this.dropdownCell && this.dropdownCell.row === row && this.dropdownCell.col === col && this.dropdownMenu) {
      this.hideDropdownMenu();
      return;
    }
    this.showDropdownMenu(row, col);
  }

  /** 显示下拉菜单 */
  private showDropdownMenu(row: number, col: number): void {
    this.hideDropdownMenu();
    const values = this.getDropdownValues(row, col);
    if (values === null) return; // 非下拉单元格

    const menu = document.createElement('div');
    menu.className = 'zq-dv-menu';

    if (values.length === 0) {
      const empty = document.createElement('div');
      empty.className = 'zq-dv-menu-empty';
      empty.textContent = '(无选项)';
      menu.appendChild(empty);
    } else {
      values.forEach(v => {
        const item = document.createElement('div');
        item.className = 'zq-dv-menu-item';
        item.textContent = v;
        item.addEventListener('mousedown', (e) => {
          e.preventDefault();
          e.stopPropagation();
          this.applyDropdownValue(row, col, v);
          this.hideDropdownMenu();
        });
        menu.appendChild(item);
      });
    }

    this.container.appendChild(menu);
    this.dropdownMenu = menu;
    this.dropdownCell = { row, col };

    // 定位菜单到单元格下方
    const cellEl = this.renderEngine.getRenderedCell(row, col);
    if (cellEl) {
      const rect = cellEl.getBoundingClientRect();
      const containerRect = this.container.getBoundingClientRect();
      const left = rect.left - containerRect.left;
      const top = rect.bottom - containerRect.top;
      menu.style.left = `${left}px`;
      menu.style.top = `${top}px`;
      // 菜单宽度至少与单元格同宽
      const minW = rect.width;
      const menuWidth = menu.offsetWidth;
      if (menuWidth < minW) {
        menu.style.minWidth = `${minW}px`;
      }
      // 超出容器底部时向上展示
      const menuHeight = menu.offsetHeight;
      if (top + menuHeight > containerRect.height) {
        menu.style.top = `${rect.top - containerRect.top - menuHeight}px`;
      }
    }
  }

  /** 隐藏下拉菜单 */
  private hideDropdownMenu(): void {
    if (this.dropdownMenu) {
      this.dropdownMenu.remove();
      this.dropdownMenu = null;
    }
    this.dropdownCell = null;
  }

  /** 应用下拉选中的值 */
  private applyDropdownValue(row: number, col: number, value: string): void {
    if (this.callbacks.onSelectDropdownValue) {
      this.callbacks.onSelectDropdownValue(row, col, value);
    } else {
      // 默认行为: 写入 sheet 并刷新渲染
      this.sheet.setCellValue(row, col, value);
      this.renderEngine.refreshCell(row, col);
    }
    this.callbacks.onContentChange?.();
  }

  // ============================================================
  // DOM 交互 — 错误提示对话框
  // ============================================================

  /**
   * 显示错误提示对话框 (返回用户选择)
   * stop: 重试 / 取消
   * warning: 是 (继续) / 否 (取消)
   * information: 确定 (继续)
   */
  showErrorDialog(
    rule: DataValidationRule,
    row: number,
    col: number,
    result: ValidationResult
  ): Promise<ErrorDialogAction> {
    return new Promise<ErrorDialogAction>((resolve) => {
      this.hideErrorDialog();

      const mask = document.createElement('div');
      mask.className = 'zq-dv-dialog-mask';

      const dialog = document.createElement('div');
      dialog.className = 'zq-dv-dialog';

      const errorStyle = result.errorStyle ?? rule.errorStyle ?? 'stop';
      const title = rule.errorTitle || '输入无效';
      const message = result.error || rule.errorMessage || '您输入的值不合法。';
      const cellAddr = SheetModel.cellAddress(row, col);

      // 头部 (图标 + 标题)
      const header = document.createElement('div');
      header.className = 'zq-dv-dialog-header';
      const icon = document.createElement('div');
      icon.className = `zq-dv-dialog-icon ${errorStyle}`;
      icon.textContent = errorStyle === 'stop' ? '!' : errorStyle === 'warning' ? '!' : 'i';
      const titleEl = document.createElement('div');
      titleEl.className = 'zq-dv-dialog-title';
      titleEl.textContent = title;
      header.appendChild(icon);
      header.appendChild(titleEl);
      dialog.appendChild(header);

      // 正文
      const body = document.createElement('div');
      body.className = 'zq-dv-dialog-body';
      body.textContent = `${message} (单元格 ${cellAddr})`;
      dialog.appendChild(body);

      // 按钮区
      const footer = document.createElement('div');
      footer.className = 'zq-dv-dialog-footer';

      const close = (action: ErrorDialogAction) => {
        this.hideErrorDialog();
        resolve(action);
      };

      if (errorStyle === 'stop') {
        const retryBtn = document.createElement('button');
        retryBtn.className = 'zq-dv-btn zq-dv-btn-primary';
        retryBtn.textContent = '重试';
        retryBtn.addEventListener('click', () => close('retry'));
        const cancelBtn = document.createElement('button');
        cancelBtn.className = 'zq-dv-btn';
        cancelBtn.textContent = '取消';
        cancelBtn.addEventListener('click', () => close('cancel'));
        footer.appendChild(retryBtn);
        footer.appendChild(cancelBtn);
      } else if (errorStyle === 'warning') {
        const yesBtn = document.createElement('button');
        yesBtn.className = 'zq-dv-btn zq-dv-btn-primary';
        yesBtn.textContent = '是';
        yesBtn.addEventListener('click', () => close('continue'));
        const noBtn = document.createElement('button');
        noBtn.className = 'zq-dv-btn';
        noBtn.textContent = '否';
        noBtn.addEventListener('click', () => close('cancel'));
        footer.appendChild(yesBtn);
        footer.appendChild(noBtn);
      } else {
        // information
        const okBtn = document.createElement('button');
        okBtn.className = 'zq-dv-btn zq-dv-btn-primary';
        okBtn.textContent = '确定';
        okBtn.addEventListener('click', () => close('continue'));
        footer.appendChild(okBtn);
      }
      dialog.appendChild(footer);

      // ESC 关闭 (视为取消)
      const escHandler = (e: KeyboardEvent) => {
        if (e.key === 'Escape') {
          document.removeEventListener('keydown', escHandler);
          close('cancel');
        }
      };
      document.addEventListener('keydown', escHandler);

      mask.appendChild(dialog);
      document.body.appendChild(mask);
      this.errorDialog = mask;

      // 点击遮罩不关闭 (强制选择按钮), 仅 stop 模式下点击遮罩视为取消
      if (errorStyle === 'stop') {
        mask.addEventListener('mousedown', (e) => {
          if (e.target === mask) close('cancel');
        });
      }
    });
  }

  /** 隐藏错误对话框 */
  private hideErrorDialog(): void {
    if (this.errorDialog) {
      this.errorDialog.remove();
      this.errorDialog = null;
    }
  }

  // ============================================================
  // DOM 交互 — 输入提示 (悬停)
  // ============================================================

  /** 显示输入提示 */
  private showInputTooltip(row: number, col: number, rule: DataValidationRule): void {
    if (!rule.inputTitle && !rule.inputMessage) return;
    // 同一单元格已显示则不重复创建 (避免闪烁)
    if (this.inputTooltip && this.tooltipCell && this.tooltipCell.row === row && this.tooltipCell.col === col) {
      return;
    }
    this.hideInputTooltip();

    const tooltip = document.createElement('div');
    tooltip.className = 'zq-dv-tooltip';
    if (rule.inputTitle) {
      const t = document.createElement('div');
      t.className = 'zq-dv-tooltip-title';
      t.textContent = rule.inputTitle;
      tooltip.appendChild(t);
    }
    if (rule.inputMessage) {
      const m = document.createElement('div');
      m.textContent = rule.inputMessage;
      tooltip.appendChild(m);
    }

    this.container.appendChild(tooltip);
    this.inputTooltip = tooltip;
    this.tooltipCell = { row, col };

    // 定位到单元格右下角
    const cellEl = this.renderEngine.getRenderedCell(row, col);
    if (cellEl) {
      const rect = cellEl.getBoundingClientRect();
      const containerRect = this.container.getBoundingClientRect();
      let left = rect.right - containerRect.left + 2;
      let top = rect.bottom - containerRect.top + 2;
      // 超出右侧则放左侧
      const tooltipWidth = tooltip.offsetWidth;
      if (left + tooltipWidth > containerRect.width) {
        left = rect.left - containerRect.left - tooltipWidth - 2;
        if (left < 0) left = 2;
      }
      // 超出底部则上移
      const tooltipHeight = tooltip.offsetHeight;
      if (top + tooltipHeight > containerRect.height) {
        top = Math.max(2, rect.top - containerRect.top - tooltipHeight);
      }
      tooltip.style.left = `${left}px`;
      tooltip.style.top = `${top}px`;
    }
  }

  /** 隐藏输入提示 */
  private hideInputTooltip(): void {
    if (this.inputTooltip) {
      this.inputTooltip.remove();
      this.inputTooltip = null;
    }
    this.tooltipCell = null;
  }

  // ============================================================
  // 辅助
  // ============================================================

  /** 生成规则 id */
  static generateRuleId(): string {
    return `dv_${Date.now().toString(36)}_${Math.random().toString(36).slice(2, 9)}`;
  }

  /** 更新工作表引用 (供 sheet-engine 切换工作表时调用) */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
    this.indexDirty = true;
    this.hideDropdownMenu();
    this.hideInputTooltip();
    this.refreshDropdownArrows();
  }

  /** 销毁, 移除所有 DOM 与事件 */
  destroy(): void {
    this.clearArrows();
    this.hideDropdownMenu();
    this.hideErrorDialog();
    this.hideInputTooltip();
    if (this.scrollRafId !== null) {
      window.cancelAnimationFrame(this.scrollRafId);
      this.scrollRafId = null;
    }
    const canvas = this.container.querySelector('.zq-sheet-canvas');
    if (canvas && this.boundScrollHandler) {
      canvas.removeEventListener('scroll', this.boundScrollHandler);
    }
    if (this.boundArrowClickHandler) {
      this.container.removeEventListener('click', this.boundArrowClickHandler, true);
    }
    if (this.boundHoverHandler) {
      this.container.removeEventListener('mouseover', this.boundHoverHandler);
    }
    if (this.boundDocClickHandler) {
      document.removeEventListener('mousedown', this.boundDocClickHandler);
    }
    this.rules.clear();
    this.ruleOrder = [];
    this.cellIndex.clear();
  }
}

// ============================================================
// 辅助函数 (模块级)
// ============================================================

/** 解析单元格地址 "A1" / "$A$1" -> {row, col} (0-based) */
function parseCellAddress(addr: string): { row: number; col: number } | null {
  const m = /^\$?([A-Za-z]+)\$?(\d+)$/.exec(addr.trim());
  if (!m) return null;
  const col = SheetModel.letterToColumn(m[1].toUpperCase());
  const row = parseInt(m[2], 10) - 1;
  if (row < 0 || col < 0) return null;
  return { row, col };
}

/**
 * 解析日期: 支持
 *   - ISO: yyyy-mm-dd / yyyy-mm-ddTHH:mm:ss
 *   - 斜杠: yyyy/mm/dd
 *   - 中文: yyyy年mm月dd日
 *   - 时间戳数字 (毫秒)
 * 返回 Date 或 null
 */
function parseDate(value: string | number | boolean | null): Date | null {
  if (value === null) return null;
  if (typeof value === 'boolean') return null;
  if (typeof value === 'number') {
    // 视为毫秒时间戳
    if (!isFinite(value)) return null;
    const d = new Date(value);
    return isNaN(d.getTime()) ? null : d;
  }
  const s = value.trim();
  if (s === '') return null;

  // 中文格式
  const cn = /^(\d{4})年(\d{1,2})月(\d{1,2})日(?:\s+(\d{1,2}):(\d{1,2})(?::(\d{1,2}))?)?$/.exec(s);
  if (cn) {
    const d = new Date(
      parseInt(cn[1], 10),
      parseInt(cn[2], 10) - 1,
      parseInt(cn[3], 10),
      cn[4] ? parseInt(cn[4], 10) : 0,
      cn[5] ? parseInt(cn[5], 10) : 0,
      cn[6] ? parseInt(cn[6], 10) : 0
    );
    return isNaN(d.getTime()) ? null : d;
  }

  // 标准化: 把 / 替换为 -
  const normalized = s.replace(/\//g, '-');
  // 仅日期 yyyy-mm-dd
  const dateOnly = /^(\d{4})-(\d{1,2})-(\d{1,2})$/.exec(normalized);
  if (dateOnly) {
    const d = new Date(
      parseInt(dateOnly[1], 10),
      parseInt(dateOnly[2], 10) - 1,
      parseInt(dateOnly[3], 10)
    );
    return isNaN(d.getTime()) ? null : d;
  }
  // 完整 ISO
  const full = /^(\d{4})-(\d{1,2})-(\d{1,2})[T ](\d{1,2}):(\d{1,2})(?::(\d{1,2}))?$/.exec(normalized);
  if (full) {
    const d = new Date(
      parseInt(full[1], 10),
      parseInt(full[2], 10) - 1,
      parseInt(full[3], 10),
      parseInt(full[4], 10),
      parseInt(full[5], 10),
      full[6] ? parseInt(full[6], 10) : 0
    );
    return isNaN(d.getTime()) ? null : d;
  }
  // 兜底: Date 解析
  const fallback = new Date(s);
  return isNaN(fallback.getTime()) ? null : fallback;
}
