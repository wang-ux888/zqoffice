/**
 * 公式引擎 — 解析和计算单元格公式
 * 全部原创实现
 * 支持: SUM/AVERAGE/MAX/MIN/COUNT/IF/AND/OR/CONCATENATE/ROUND/ABS/SQRT/POWER/MOD 等
 */

import { SheetModel } from './data-model';

/** 公式值类型 */
type FormulaValue = number | string | boolean | null;

/** 令牌类型 */
type TokenType = 'number' | 'string' | 'boolean' | 'cell' | 'range' | 'function' | 'operator' | 'lparen' | 'rparen' | 'comma' | 'colon';

/** 令牌 */
interface Token {
  type: TokenType;
  value: string;
  position: number;
}

/** AST 节点 */
type ASTNode =
  | { type: 'number'; value: number }
  | { type: 'string'; value: string }
  | { type: 'boolean'; value: boolean }
  | { type: 'cell'; row: number; col: number }
  | { type: 'range'; startRow: number; startCol: number; endRow: number; endCol: number }
  | { type: 'function'; name: string; args: ASTNode[] }
  | { type: 'binop'; op: string; left: ASTNode; right: ASTNode }
  | { type: 'unaryop'; op: string; operand: ASTNode };

export class FormulaEngine {
  private sheet: SheetModel;

  // 公式函数注册表
  private functions = new Map<string, (args: FormulaValue[][], sheet: SheetModel, argNodes?: ASTNode[]) => FormulaValue>();

  constructor(sheet: SheetModel) {
    this.sheet = sheet;
    this.registerBuiltins();
  }

  /** 更新工作表引用 */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
  }

  // ============================================================
  // 公开 API
  // ============================================================

  /**
   * 计算公式
   * @param formula 公式文本 (如 "SUM(A1:A10)" 或 "=SUM(A1:A10)")
   * @returns 计算结果
   */
  calculate(formula: string): FormulaValue {
    // 去除前导 =
    let expr = formula.trim();
    if (expr.startsWith('=')) expr = expr.slice(1);

    try {
      const tokens = this.tokenize(expr);
      const ast = this.parse(tokens);
      const result = this.evaluate(ast);
      return result;
    } catch (e) {
      return `#${(e as Error).message?.toUpperCase() || 'ERROR'}!`;
    }
  }

  /**
   * 计算所有公式单元格
   */
  calculateAll(): Map<string, FormulaValue> {
    const results = new Map<string, FormulaValue>();
    const cells = this.sheet.getAllCells();

    for (const cell of cells) {
      if (cell.formula) {
        const result = this.calculate(cell.formula);
        const key = `${cell.row}:${cell.column}`;
        results.set(key, result);
        // 更新单元格显示值
        this.sheet.setCellValue(cell.row, cell.column, this.valueToDisplay(result), 'string');
      }
    }

    return results;
  }

  // ============================================================
  // 词法分析 (Tokenizer)
  // ============================================================

  private tokenize(expr: string): Token[] {
    const tokens: Token[] = [];
    let i = 0;

    while (i < expr.length) {
      // 跳过空白
      while (i < expr.length && /\s/.test(expr[i])) i++;

      if (i >= expr.length) break;

      const ch = expr[i];
      const start = i;

      // 数字
      if (/\d/.test(ch) || (ch === '.' && i + 1 < expr.length && /\d/.test(expr[i + 1]))) {
        let num = '';
        while (i < expr.length && /[\d.]/.test(expr[i])) {
          num += expr[i];
          i++;
        }
        // 科学计数法
        if (i < expr.length && (expr[i] === 'e' || expr[i] === 'E')) {
          num += expr[i];
          i++;
          if (i < expr.length && (expr[i] === '+' || expr[i] === '-')) {
            num += expr[i];
            i++;
          }
          while (i < expr.length && /\d/.test(expr[i])) {
            num += expr[i];
            i++;
          }
        }
        tokens.push({ type: 'number', value: num, position: start });
        continue;
      }

      // 字符串
      if (ch === '"') {
        let str = '';
        i++; // 跳过开始引号
        while (i < expr.length && expr[i] !== '"') {
          str += expr[i];
          i++;
        }
        i++; // 跳过结束引号
        tokens.push({ type: 'string', value: str, position: start });
        continue;
      }

      // 布尔值
      if (expr.substring(i, i + 4).toUpperCase() === 'TRUE') {
        tokens.push({ type: 'boolean', value: 'true', position: start });
        i += 4;
        continue;
      }
      if (expr.substring(i, i + 5).toUpperCase() === 'FALSE') {
        tokens.push({ type: 'boolean', value: 'false', position: start });
        i += 5;
        continue;
      }

      // 单元格引用 或 范围 或 函数名
      if (/[A-Za-z_]/.test(ch)) {
        let name = '';
        while (i < expr.length && /[A-Za-z0-9_]/.test(expr[i])) {
          name += expr[i];
          i++;
        }

        // 跳过空白
        let j = i;
        while (j < expr.length && /\s/.test(expr[j])) j++;

        // 如果后面跟 ( 则是函数
        if (j < expr.length && expr[j] === '(') {
          tokens.push({ type: 'function', value: name.toUpperCase(), position: start });
          continue;
        }

        // 否则是单元格引用
        const cellRef = this.parseCellRef(name);
        if (cellRef) {
          // 检查是否是范围 (A1:B2)
          let k = i;
          while (k < expr.length && /\s/.test(expr[k])) k++;
          if (k < expr.length && expr[k] === ':') {
            // 范围
            k++; // 跳过冒号
            while (k < expr.length && /\s/.test(expr[k])) k++;
            let name2 = '';
            while (k < expr.length && /[A-Za-z0-9]/.test(expr[k])) {
              name2 += expr[k];
              k++;
            }
            const cellRef2 = this.parseCellRef(name2);
            if (cellRef2) {
              tokens.push({
                type: 'range',
                value: `${name}:${name2}`,
                position: start,
              });
              i = k;
              continue;
            }
          }
          tokens.push({ type: 'cell', value: name, position: start });
          continue;
        }

        // 未知标识符, 当作错误处理
        throw new Error(`Unknown identifier: ${name}`);
      }

      // 运算符 (包括比较运算符 < > <= >= <> !=)
      if ('+-*/^&='.includes(ch)) {
        tokens.push({ type: 'operator', value: ch, position: start });
        i++;
        continue;
      }
      // 比较运算符 < > <= >= <> !=
      if (ch === '<' || ch === '>') {
        let op = ch;
        i++;
        if (i < expr.length && (expr[i] === '=' || (ch === '<' && expr[i] === '>'))) {
          op += expr[i];
          i++;
        }
        tokens.push({ type: 'operator', value: op, position: start });
        continue;
      }
      if (ch === '!' && i + 1 < expr.length && expr[i + 1] === '=') {
        tokens.push({ type: 'operator', value: '!=', position: start });
        i += 2;
        continue;
      }

      // 括号
      if (ch === '(') {
        tokens.push({ type: 'lparen', value: ch, position: start });
        i++;
        continue;
      }
      if (ch === ')') {
        tokens.push({ type: 'rparen', value: ch, position: start });
        i++;
        continue;
      }

      // 逗号
      if (ch === ',') {
        tokens.push({ type: 'comma', value: ch, position: start });
        i++;
        continue;
      }

      // 冒号 (范围分隔符, 已在单元格引用中处理)
      if (ch === ':') {
        tokens.push({ type: 'colon', value: ch, position: start });
        i++;
        continue;
      }

      throw new Error(`Unexpected character: ${ch} at position ${start}`);
    }

    return tokens;
  }

  /** 解析单元格引用 (如 "A1" -> {row:0, col:0}) */
  private parseCellRef(ref: string): { row: number; col: number } | null {
    const match = ref.match(/^([A-Za-z]+)(\d+)$/);
    if (!match) return null;
    const col = SheetModel.letterToColumn(match[1].toUpperCase());
    const row = parseInt(match[2]) - 1;
    if (row < 0 || col < 0) return null;
    return { row, col };
  }

  // ============================================================
  // 语法分析 (Parser) — 递归下降解析器
  // ============================================================

  private tokens: Token[] = [];
  private pos: number = 0;

  private parse(tokens: Token[]): ASTNode {
    this.tokens = tokens;
    this.pos = 0;
    const ast = this.parseExpression();
    if (this.pos < this.tokens.length) {
      throw new Error(`Unexpected token at position ${this.tokens[this.pos].position}`);
    }
    return ast;
  }

  private peek(): Token | null {
    return this.pos < this.tokens.length ? this.tokens[this.pos] : null;
  }

  private consume(): Token {
    return this.tokens[this.pos++];
  }

  private expect(type: TokenType): Token {
    const token = this.peek();
    if (!token || token.type !== type) {
      throw new Error(`Expected ${type}, got ${token?.type ?? 'EOF'}`);
    }
    return this.consume();
  }

  /** 表达式: 字符串连接 (&) — 最低优先级 */
  private parseExpression(): ASTNode {
    let left = this.parseComparison();
    while (this.peek()?.type === 'operator' && this.peek()!.value === '&') {
      const op = this.consume().value;
      const right = this.parseComparison();
      left = { type: 'binop', op, left, right };
    }
    return left;
  }

  /** 比较运算 (= < > <= >= != <>) */
  private parseComparison(): ASTNode {
    let left = this.parseAddSub();
    while (this.peek()?.type === 'operator' &&
           ['=', '<', '>', '<=', '>=', '!=', '<>'].includes(this.peek()!.value)) {
      const op = this.consume().value;
      const right = this.parseAddSub();
      left = { type: 'binop', op, left, right };
    }
    return left;
  }

  /** 加减法 */
  private parseAddSub(): ASTNode {
    let left = this.parseMulDiv();
    while (this.peek()?.type === 'operator' && '+-'.includes(this.peek()!.value)) {
      const op = this.consume().value;
      const right = this.parseMulDiv();
      left = { type: 'binop', op, left, right };
    }
    return left;
  }

  /** 乘除法 */
  private parseMulDiv(): ASTNode {
    let left = this.parsePower();
    while (this.peek()?.type === 'operator' && '*/'.includes(this.peek()!.value)) {
      const op = this.consume().value;
      const right = this.parsePower();
      left = { type: 'binop', op, left, right };
    }
    return left;
  }

  /** 幂运算 */
  private parsePower(): ASTNode {
    let left = this.parseUnary();
    while (this.peek()?.type === 'operator' && this.peek()!.value === '^') {
      const op = this.consume().value;
      const right = this.parseUnary();
      left = { type: 'binop', op, left, right };
    }
    return left;
  }

  /** 一元运算 (负号) */
  private parseUnary(): ASTNode {
    if (this.peek()?.type === 'operator' && this.peek()!.value === '-') {
      this.consume();
      const operand = this.parseUnary();
      return { type: 'unaryop', op: '-', operand };
    }
    if (this.peek()?.type === 'operator' && this.peek()!.value === '+') {
      this.consume();
      return this.parseUnary();
    }
    return this.parsePrimary();
  }

  /** 基本元素 */
  private parsePrimary(): ASTNode {
    const token = this.peek();
    if (!token) throw new Error('Unexpected end of formula');

    switch (token.type) {
      case 'number': {
        this.consume();
        return { type: 'number', value: parseFloat(token.value) };
      }
      case 'string': {
        this.consume();
        return { type: 'string', value: token.value };
      }
      case 'boolean': {
        this.consume();
        return { type: 'boolean', value: token.value === 'true' };
      }
      case 'cell': {
        this.consume();
        const ref = this.parseCellRef(token.value)!;
        return { type: 'cell', row: ref.row, col: ref.col };
      }
      case 'range': {
        this.consume();
        const [start, end] = token.value.split(':');
        const startRef = this.parseCellRef(start)!;
        const endRef = this.parseCellRef(end)!;
        return {
          type: 'range',
          startRow: Math.min(startRef.row, endRef.row),
          startCol: Math.min(startRef.col, endRef.col),
          endRow: Math.max(startRef.row, endRef.row),
          endCol: Math.max(startRef.col, endRef.col),
        };
      }
      case 'function': {
        this.consume();
        this.expect('lparen');
        const args: ASTNode[] = [];
        if (this.peek()?.type !== 'rparen') {
          args.push(this.parseExpression());
          while (this.peek()?.type === 'comma') {
            this.consume();
            args.push(this.parseExpression());
          }
        }
        this.expect('rparen');
        return { type: 'function', name: token.value, args };
      }
      case 'lparen': {
        this.consume();
        const expr = this.parseExpression();
        this.expect('rparen');
        return expr;
      }
      default:
        throw new Error(`Unexpected token type: ${token.type}`);
    }
  }

  // ============================================================
  // 求值 (Evaluator)
  // ============================================================

  private evaluate(node: ASTNode): FormulaValue {
    switch (node.type) {
      case 'number':
        return node.value;
      case 'string':
        return node.value;
      case 'boolean':
        return node.value;
      case 'cell':
        return this.getCellValue(node.row, node.col);
      case 'range':
        // 范围在函数参数中处理, 直接返回第一个值
        return this.getCellValue(node.startRow, node.startCol);
      case 'binop':
        return this.evaluateBinop(node.op, this.evaluate(node.left), this.evaluate(node.right));
      case 'unaryop':
        const val = this.evaluate(node.operand);
        if (node.op === '-') {
          if (typeof val === 'number') return -val;
          throw new Error('VALUE');
        }
        return val;
      case 'function':
        return this.evaluateFunction(node.name, node.args);
      default:
        throw new Error('Unknown AST node');
    }
  }

  /** 获取单元格值 */
  private getCellValue(row: number, col: number): FormulaValue {
    const cell = this.sheet.getCell(row, col);
    if (!cell) return null;
    if (cell.formula) {
      // 递归计算
      const result = this.calculate(cell.formula);
      return this.toFormulaValue(result);
    }
    return this.toFormulaValue(cell.value);
  }

  /** 转换为公式值 */
  private toFormulaValue(value: string | number | boolean | FormulaValue): FormulaValue {
    if (typeof value === 'number') return value;
    if (typeof value === 'boolean') return value;
    // 尝试解析数字
    const num = Number(value);
    if (!isNaN(num) && value !== '') return num;
    return value;
  }

  /** 二元运算 */
  private evaluateBinop(op: string, left: FormulaValue, right: FormulaValue): FormulaValue {
    // 字符串连接
    if (op === '&') {
      return this.valueToString(left) + this.valueToString(right);
    }

    // 比较运算
    if (op === '=' || op === '<>' || op === '!=' ||
        op === '<' || op === '>' || op === '<=' || op === '>=') {
      return this.compareValuesOp(left, right, op);
    }

    // 算术运算
    const l = this.toNumber(left);
    const r = this.toNumber(right);

    switch (op) {
      case '+': return l + r;
      case '-': return l - r;
      case '*': return l * r;
      case '/': return r === 0 ? '#DIV/0!' : l / r;
      case '^': return Math.pow(l, r);
      default: throw new Error(`Unknown operator: ${op}`);
    }
  }

  /** 比较两个值 (用于比较运算符 = <> != < > <= >=) */
  private compareValuesOp(left: FormulaValue, right: FormulaValue, op: string): boolean {
    // 处理 null
    if (left === null) left = 0;
    if (right === null) right = 0;

    // 数字比较
    if (typeof left === 'number' && typeof right === 'number') {
      switch (op) {
        case '=': return left === right;
        case '<>':
        case '!=': return left !== right;
        case '<': return left < right;
        case '>': return left > right;
        case '<=': return left <= right;
        case '>=': return left >= right;
      }
    }

    // 字符串比较
    const ls = String(left);
    const rs = String(right);
    switch (op) {
      case '=': return ls === rs;
      case '<>':
      case '!=': return ls !== rs;
      case '<': return ls < rs;
      case '>': return ls > rs;
      case '<=': return ls <= rs;
      case '>=': return ls >= rs;
    }
    return false;
  }

  /** 转为数字 */
  private toNumber(val: FormulaValue): number {
    if (val === null) return 0;
    if (typeof val === 'number') return val;
    if (typeof val === 'boolean') return val ? 1 : 0;
    const num = Number(val);
    return isNaN(num) ? 0 : num;
  }

  /** 转为字符串 */
  private valueToString(val: FormulaValue): string {
    if (val === null) return '';
    if (typeof val === 'boolean') return val ? 'TRUE' : 'FALSE';
    return String(val);
  }

  /** 转为显示值 */
  private valueToDisplay(val: FormulaValue): string | number | boolean {
    if (val === null) return '';
    if (typeof val === 'string' && val.startsWith('#')) return val; // 错误值
    if (typeof val === 'number') return val;
    if (typeof val === 'boolean') return val;
    return String(val);
  }

  // ============================================================
  // 函数求值
  // ============================================================

  private evaluateFunction(name: string, args: ASTNode[]): FormulaValue {
    const fn = this.functions.get(name);
    if (!fn) throw new Error(`Unknown function: ${name}`);

    // 收集参数值 (范围参数展开为值数组)
    const argValues: FormulaValue[][] = [];
    for (const arg of args) {
      if (arg.type === 'range') {
        const values: FormulaValue[] = [];
        for (let r = arg.startRow; r <= arg.endRow; r++) {
          for (let c = arg.startCol; c <= arg.endCol; c++) {
            values.push(this.getCellValue(r, c));
          }
        }
        argValues.push(values);
      } else {
        argValues.push([this.evaluate(arg)]);
      }
    }

    return fn(argValues, this.sheet, args);
  }

  // ============================================================
  // 内置函数注册
  // ============================================================

  private registerBuiltins(): void {
    // 数学函数
    this.register('SUM', (args) => {
      let sum = 0;
      for (const argList of args) {
        for (const v of argList) {
          sum += this.toNumber(v);
        }
      }
      return sum;
    });

    this.register('AVERAGE', (args) => {
      let sum = 0, count = 0;
      for (const argList of args) {
        for (const v of argList) {
          if (v !== null && v !== '') {
            sum += this.toNumber(v);
            count++;
          }
        }
      }
      return count === 0 ? '#DIV/0!' : sum / count;
    });

    this.register('MAX', (args) => {
      let max = -Infinity;
      let hasValue = false;
      for (const argList of args) {
        for (const v of argList) {
          if (v !== null && v !== '') {
            max = Math.max(max, this.toNumber(v));
            hasValue = true;
          }
        }
      }
      return hasValue ? max : 0;
    });

    this.register('MIN', (args) => {
      let min = Infinity;
      let hasValue = false;
      for (const argList of args) {
        for (const v of argList) {
          if (v !== null && v !== '') {
            min = Math.min(min, this.toNumber(v));
            hasValue = true;
          }
        }
      }
      return hasValue ? min : 0;
    });

    this.register('COUNT', (args) => {
      let count = 0;
      for (const argList of args) {
        for (const v of argList) {
          if (typeof v === 'number') count++;
        }
      }
      return count;
    });

    this.register('COUNTA', (args) => {
      let count = 0;
      for (const argList of args) {
        for (const v of argList) {
          if (v !== null && v !== '') count++;
        }
      }
      return count;
    });

    this.register('ROUND', (args) => {
      const val = this.toNumber(args[0]?.[0] ?? 0);
      const digits = this.toNumber(args[1]?.[0] ?? 0);
      const factor = Math.pow(10, digits);
      return Math.round(val * factor) / factor;
    });

    this.register('ABS', (args) => Math.abs(this.toNumber(args[0]?.[0] ?? 0)));

    this.register('SQRT', (args) => {
      const val = this.toNumber(args[0]?.[0] ?? 0);
      return val < 0 ? '#NUM!' : Math.sqrt(val);
    });

    this.register('POWER', (args) => Math.pow(this.toNumber(args[0]?.[0] ?? 0), this.toNumber(args[1]?.[0] ?? 0)));

    this.register('MOD', (args) => {
      const a = this.toNumber(args[0]?.[0] ?? 0);
      const b = this.toNumber(args[1]?.[0] ?? 0);
      return b === 0 ? '#DIV/0!' : a - Math.floor(a / b) * b;
    });

    this.register('INT', (args) => Math.floor(this.toNumber(args[0]?.[0] ?? 0)));

    this.register('PI', () => Math.PI);

    this.register('RAND', () => Math.random());

    // 逻辑函数
    this.register('IF', (args) => {
      const cond = args[0]?.[0];
      const isTrue = typeof cond === 'boolean' ? cond : this.toNumber(cond) !== 0;
      return isTrue ? (args[1]?.[0] ?? false) : (args[2]?.[0] ?? false);
    });

    this.register('AND', (args) => {
      for (const argList of args) {
        for (const v of argList) {
          const b = typeof v === 'boolean' ? v : this.toNumber(v) !== 0;
          if (!b) return false;
        }
      }
      return true;
    });

    this.register('OR', (args) => {
      for (const argList of args) {
        for (const v of argList) {
          const b = typeof v === 'boolean' ? v : this.toNumber(v) !== 0;
          if (b) return true;
        }
      }
      return false;
    });

    this.register('NOT', (args) => {
      const v = args[0]?.[0];
      const b = typeof v === 'boolean' ? v : this.toNumber(v) !== 0;
      return !b;
    });

    // 文本函数
    this.register('CONCATENATE', (args) => {
      let result = '';
      for (const argList of args) {
        for (const v of argList) {
          result += this.valueToString(v);
        }
      }
      return result;
    });

    this.register('LEFT', (args) => {
      const str = this.valueToString(args[0]?.[0] ?? '');
      const n = this.toNumber(args[1]?.[0] ?? 1);
      return str.substring(0, n);
    });

    this.register('RIGHT', (args) => {
      const str = this.valueToString(args[0]?.[0] ?? '');
      const n = this.toNumber(args[1]?.[0] ?? 1);
      return str.substring(str.length - n);
    });

    this.register('MID', (args) => {
      const str = this.valueToString(args[0]?.[0] ?? '');
      const start = this.toNumber(args[1]?.[0] ?? 1);
      const len = this.toNumber(args[2]?.[0] ?? 0);
      return str.substring(start - 1, start - 1 + len);
    });

    this.register('LEN', (args) => this.valueToString(args[0]?.[0] ?? '').length);

    this.register('UPPER', (args) => this.valueToString(args[0]?.[0] ?? '').toUpperCase());

    this.register('LOWER', (args) => this.valueToString(args[0]?.[0] ?? '').toLowerCase());

    this.register('TRIM', (args) => this.valueToString(args[0]?.[0] ?? '').trim());

    this.register('REPLACE', (args) => {
      const str = this.valueToString(args[0]?.[0] ?? '');
      const start = this.toNumber(args[1]?.[0] ?? 1);
      const len = this.toNumber(args[2]?.[0] ?? 0);
      const replacement = this.valueToString(args[3]?.[0] ?? '');
      return str.substring(0, start - 1) + replacement + str.substring(start - 1 + len);
    });

    // 日期函数
    this.register('TODAY', () => {
      const today = new Date();
      const epoch = new Date(Date.UTC(1899, 11, 31));
      const diff = Math.floor((today.getTime() - epoch.getTime()) / 86400000);
      return diff + 1; // Excel 日期序列号
    });

    this.register('NOW', () => {
      const now = new Date();
      const epoch = new Date(Date.UTC(1899, 11, 31));
      return (now.getTime() - epoch.getTime()) / 86400000;
    });

    this.register('YEAR', (args) => {
      const serial = this.toNumber(args[0]?.[0] ?? 0);
      const date = this.serialToDate(serial);
      return date.getUTCFullYear();
    });

    this.register('MONTH', (args) => {
      const serial = this.toNumber(args[0]?.[0] ?? 0);
      const date = this.serialToDate(serial);
      return date.getUTCMonth() + 1;
    });

    this.register('DAY', (args) => {
      const serial = this.toNumber(args[0]?.[0] ?? 0);
      const date = this.serialToDate(serial);
      return date.getUTCDate();
    });

    // 查找函数
    this.register('VLOOKUP', (args, _sheet, argNodes) => {
      const lookupValue = args[0]?.[0];
      const colIndex = Math.floor(this.toNumber(args[2]?.[0] ?? 1));
      const exactMatch = args[3]?.[0];
      const isExact = exactMatch === undefined || exactMatch === null || this.toNumber(exactMatch) !== 0;

      // 通过 AST 节点获取 range 的行列信息
      const rangeNode = argNodes?.[1];
      if (!rangeNode || rangeNode.type !== 'range') {
        return '#REF!';
      }

      const { startRow, startCol, endRow, endCol } = rangeNode;
      const colCount = endCol - startCol + 1;
      if (colIndex < 1 || colIndex > colCount) return '#REF!';

      // 逐行查找
      for (let r = startRow; r <= endRow; r++) {
        const cellVal = this.getCellValue(r, startCol);
        if (this.valuesEqual(cellVal, lookupValue)) {
          return this.getCellValue(r, startCol + colIndex - 1);
        }
        // 近似匹配: 找到第一个大于查找值的行, 返回上一行
        if (!isExact && this.compareValues(cellVal, lookupValue) > 0) {
          if (r > startRow) {
            return this.getCellValue(r - 1, startCol + colIndex - 1);
          }
          return '#N/A';
        }
      }

      return '#N/A';
    });

    this.register('HLOOKUP', (args, _sheet, argNodes) => {
      const lookupValue = args[0]?.[0];
      const rowIndex = Math.floor(this.toNumber(args[2]?.[0] ?? 1));
      const rangeNode = argNodes?.[1];
      if (!rangeNode || rangeNode.type !== 'range') return '#REF!';

      const { startRow, startCol, endRow, endCol } = rangeNode;
      const rowCount = endRow - startRow + 1;
      if (rowIndex < 1 || rowIndex > rowCount) return '#REF!';

      for (let c = startCol; c <= endCol; c++) {
        const cellVal = this.getCellValue(startRow, c);
        if (this.valuesEqual(cellVal, lookupValue)) {
          return this.getCellValue(startRow + rowIndex - 1, c);
        }
      }
      return '#N/A';
    });

    this.register('MATCH', (args, _sheet, argNodes) => {
      const lookupValue = args[0]?.[0];
      const rangeNode = argNodes?.[1];
      if (!rangeNode || rangeNode.type !== 'range') return '#REF!';
      const { startRow, startCol, endRow, endCol } = rangeNode;

      const matchType = this.toNumber(args[2]?.[0] ?? 1);
      let index = 0;
      if (startRow === endRow) {
        // 水平范围
        for (let c = startCol; c <= endCol; c++) {
          index++;
          if (this.valuesEqual(this.getCellValue(startRow, c), lookupValue)) return index;
        }
      } else {
        // 垂直范围
        for (let r = startRow; r <= endRow; r++) {
          index++;
          if (this.valuesEqual(this.getCellValue(r, startCol), lookupValue)) return index;
        }
      }
      return '#N/A';
    });

    this.register('INDEX', (args, _sheet, argNodes) => {
      const rangeNode = argNodes?.[0];
      if (!rangeNode || rangeNode.type !== 'range') return '#REF!';
      const { startRow, startCol, endRow, endCol } = rangeNode;
      const rowNum = Math.floor(this.toNumber(args[1]?.[0] ?? 1));
      const colNum = args.length > 2 ? Math.floor(this.toNumber(args[2]?.[0] ?? 1)) : 1;
      const r = startRow + rowNum - 1;
      const c = startCol + colNum - 1;
      if (r < startRow || r > endRow || c < startCol || c > endCol) return '#REF!';
      return this.getCellValue(r, c);
    });

    this.register('IFERROR', (args) => {
      const val = args[0]?.[0];
      if (typeof val === 'string' && val.startsWith('#')) {
        return args[1]?.[0] ?? '';
      }
      return val ?? '';
    });
  }

  /** 注册函数 */
  private register(name: string, fn: (args: FormulaValue[][], sheet: SheetModel, argNodes?: ASTNode[]) => FormulaValue): void {
    this.functions.set(name.toUpperCase(), fn);
  }

  /** 日期序列号转 Date */
  private serialToDate(serial: number): Date {
    const adjusted = serial >= 60 ? serial - 1 : serial;
    const epoch = new Date(Date.UTC(1899, 11, 31));
    return new Date(epoch.getTime() + adjusted * 86400000);
  }

  /** 判断两个值是否相等 (供 VLOOKUP/MATCH 使用) */
  private valuesEqual(a: FormulaValue, b: FormulaValue): boolean {
    if (a === b) return true;
    if (a === null || b === null) return false;
    // 数字比较
    if (typeof a === 'number' && typeof b === 'number') return a === b;
    // 字符串比较 (忽略大小写)
    if (typeof a === 'string' && typeof b === 'string') return a.toLowerCase() === b.toLowerCase();
    return false;
  }

  /** 比较两个值的大小 (供 VLOOKUP 近似匹配使用) */
  private compareValues(a: FormulaValue, b: FormulaValue): number {
    if (a === null && b === null) return 0;
    if (a === null) return -1;
    if (b === null) return 1;
    if (typeof a === 'number' && typeof b === 'number') return a - b;
    const sa = String(a);
    const sb = String(b);
    return sa < sb ? -1 : sa > sb ? 1 : 0;
  }

  /** 获取所有已注册函数名 */
  getFunctionNames(): string[] {
    return Array.from(this.functions.keys()).sort();
  }
}
