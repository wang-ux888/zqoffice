/**
 * 查找替换引擎层 — 查找/替换 + 查找栏 UI
 * 全部原创实现
 *
 * 职责:
 *   1. 查找: findAll / findNext / findPrevious, 支持区分大小写/全字匹配/正则/匹配公式/范围搜索
 *   2. 替换: replaceCurrent / replaceAll
 *   3. UI: 查找栏 (输入框 + 选项 + 上一个/下一个/替换/全部替换按钮 + 计数器)
 *   4. 高亮: 匹配单元格高亮, 当前匹配项特殊高亮
 *   5. 快捷键: Ctrl+F 查找 / Ctrl+H 替换 / Enter 下一个 / Shift+Enter 上一个 / Escape 关闭
 */

import { SheetModel, Cell, CellValueType } from './data-model';
import { RenderEngine, SelectionRange } from './render-engine';
import { EditEngine } from './edit-engine';

// ============================================================
// 类型定义
// ============================================================

/** 查找选项 */
export interface FindOptions {
  caseSensitive: boolean;    // 区分大小写
  wholeWord: boolean;        // 全字匹配
  useRegex: boolean;         // 正则表达式
  searchFormula: boolean;    // 搜索公式
  searchInRange?: SelectionRange; // 范围内搜索
}

/** 单个匹配结果 */
export interface FindMatch {
  row: number;
  col: number;
  value: string;       // 匹配所在单元格的文本 (查找时的搜索文本)
  matchStart: number;   // 匹配起始位置
  matchEnd: number;     // 匹配结束位置
}

/** 查找引擎回调 */
export interface FindReplaceCallbacks {
  /** 当前匹配序号与总数变化 (1-based; 0 表示无当前匹配) */
  onMatchChange?: (current: number, total: number) => void;
  /** 查找栏关闭 */
  onClose?: () => void;
  /** 内容被替换 (用于标记文档已修改) */
  onContentChange?: () => void;
}

/** 查找栏模式 */
export type FindBarMode = 'find' | 'replace';

// ============================================================
// 查找替换引擎
// ============================================================

export class FindReplaceEngine {
  private container: HTMLElement;
  private sheet: SheetModel;
  private renderEngine: RenderEngine;
  private editEngine: EditEngine;
  private callbacks: FindReplaceCallbacks;

  // 查找状态
  private matches: FindMatch[] = [];
  private currentIndex: number = -1;          // 当前匹配在 matches 中的下标, -1 表示无
  private currentKeyword: string | null = null; // 当前查找关键字
  private currentOptions: FindOptions | null = null; // 当前查找选项
  private currentReplacement: string = '';

  // 默认选项
  private options: FindOptions = {
    caseSensitive: false,
    wholeWord: false,
    useRegex: false,
    searchFormula: false,
  };

  // 查找栏 DOM
  private findBarEl: HTMLDivElement | null = null;
  private findInputEl: HTMLInputElement | null = null;
  private replaceInputEl: HTMLInputElement | null = null;
  private counterEl: HTMLSpanElement | null = null;
  private replaceRowEl: HTMLDivElement | null = null;
  private optionsRowEl: HTMLDivElement | null = null;
  private optionsToggleEl: HTMLButtonElement | null = null;
  private barMode: FindBarMode = 'find';
  private optionsExpanded: boolean = false;

  // 高亮观察器 — 监听渲染引擎重建单元格 DOM 时重新应用高亮
  private highlightObserver: MutationObserver | null = null;

  // 输入防抖句柄
  private inputDebounceId: number | null = null;

  constructor(
    container: HTMLElement,
    sheet: SheetModel,
    renderEngine: RenderEngine,
    editEngine: EditEngine,
    callbacks: FindReplaceCallbacks = {}
  ) {
    this.container = container;
    this.sheet = sheet;
    this.renderEngine = renderEngine;
    this.editEngine = editEngine;
    this.callbacks = callbacks;
  }

  // ============================================================

  /**
   * 根据关键字与选项构造查找正则表达式
   * - useRegex=false 时对关键字中的正则特殊字符做转义
   * - caseSensitive=false 时添加 'i' 标志
   * - 始终使用 'g' 标志以支持全局匹配
   * 全字匹配在 findInText 中通过边界检查实现 (兼容非 ASCII 字符)
   */
  private createFindRegExp(keyword: string, options: FindOptions): RegExp | null {
    if (keyword === '') return null;
    let pattern: string;
    if (options.useRegex) {
      pattern = keyword;
    } else {
      // 转义正则特殊字符
      pattern = keyword.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
    }
    const flags = options.caseSensitive ? 'g' : 'gi';
    try {
      return new RegExp(pattern, flags);
    } catch {
      // 非法正则
      return null;
    }
  }

  /**
   * 在文本中查找所有匹配区间
   * 返回 [matchStart, matchEnd] 数组
   * 若 wholeWord=true, 额外检查匹配两侧是否为非单词字符边界
   */
  private findInText(text: string, regex: RegExp, options: FindOptions): Array<[number, number]> {
    if (text === '') return [];
    const results: Array<[number, number]> = [];
    // 重置 lastIndex, 因为带 g 标志的 RegExp 会被复用
    regex.lastIndex = 0;
    let match: RegExpExecArray | null;
    while ((match = regex.exec(text)) !== null) {
      const start = match.index;
      const end = start + match[0].length;
      // 零宽匹配保护: 避免死循环
      if (match[0].length === 0) {
        regex.lastIndex++;
        continue;
      }
      // 全字匹配边界检查
      if (options.wholeWord) {
        const before = start > 0 ? text.charAt(start - 1) : '';
        const after = end < text.length ? text.charAt(end) : '';
        // 单词字符 (字母/数字/下划线) 视为非边界
        if (/\w/.test(before) || /\w/.test(after)) {
          continue;
        }
      }
      results.push([start, end]);
    }
    return results;
  }

  // ============================================================
  // 查找核心
  // ============================================================

  /**
   * 获取单元格的搜索文本
   * - searchFormula=true 且单元格有公式时, 搜索公式文本 (带 '=' 前缀)
   * - 否则搜索值的字符串形式
   */
  private getCellSearchText(cell: Cell, options: FindOptions): string {
    if (options.searchFormula && cell.formula) {
      return `=${cell.formula}`;
    }
    return this.cellValueToString(cell.value);
  }

  /** 单元格值转字符串 */
  private cellValueToString(value: string | number | boolean): string {
    if (typeof value === 'boolean') return value ? 'TRUE' : 'FALSE';
    if (value === null || value === undefined) return '';
    return String(value);
  }

  /**
   * 获取参与搜索的单元格列表
   * - searchInRange 指定时只取该范围内的单元格
   * - 仅返回主单元格 (合并区域的从属单元格不存储, 自然被排除)
   */
  private getSearchCells(options: FindOptions): Cell[] {
    if (options.searchInRange) {
      const r = options.searchInRange;
      return this.sheet.getCellsInRange(r.startRow, r.startCol, r.endRow, r.endCol);
    }
    return this.sheet.getAllCells();
  }

  /**
   * 查找全部匹配
   */
  findAll(keyword: string, options: FindOptions): FindMatch[] {
    const regex = this.createFindRegExp(keyword, options);
    if (!regex) return [];
    const matches: FindMatch[] = [];
    const cells = this.getSearchCells(options);
    for (const cell of cells) {
      const text = this.getCellSearchText(cell, options);
      const positions = this.findInText(text, regex, options);
      for (const [start, end] of positions) {
        matches.push({
          row: cell.row,
          col: cell.column,
          value: text,
          matchStart: start,
          matchEnd: end,
        });
      }
    }
    return matches;
  }

  /**
   * 执行搜索 — 计算 matches, 定位当前匹配, 更新高亮与计数
   * 输入框每次变化时调用
   */
  performSearch(keyword: string, options: FindOptions): FindMatch[] {
    this.currentKeyword = keyword;
    this.currentOptions = { ...options };
    this.matches = this.findAll(keyword, options);

    if (this.matches.length === 0) {
      this.currentIndex = -1;
    } else {
      // 定位到当前选区之后的第一个匹配 (行优先)
      const sel = this.renderEngine.getSelection();
      if (sel) {
        this.currentIndex = this.indexOfNextMatchAfter(this.matches, sel.startRow, sel.startCol);
      } else {
        this.currentIndex = 0;
      }
    }

    this.updateHighlights();
    this.updateCounter();
    if (this.currentIndex >= 0) {
      this.navigateToMatch(this.currentIndex);
    }
    return this.matches;
  }

  /**
   * 找到 matches 中第一个位于 (row, col) 之后 (含当前) 的下标
   * 若无则回绕到 0
   */
  private indexOfNextMatchAfter(matches: FindMatch[], row: number, col: number): number {
    for (let i = 0; i < matches.length; i++) {
      const m = matches[i];
      if (m.row > row || (m.row === row && m.col >= col)) {
        return i;
      }
    }
    return 0; // 回绕
  }

  /**
   * 查找下一个
   */
  findNext(): FindMatch | null {
    if (this.matches.length === 0) {
      return null;
    }
    this.currentIndex = (this.currentIndex + 1) % this.matches.length;
    this.navigateToMatch(this.currentIndex);
    return this.matches[this.currentIndex] ?? null;
  }

  /**
   * 查找上一个
   */
  findPrevious(): FindMatch | null {
    if (this.matches.length === 0) {
      return null;
    }
    this.currentIndex = (this.currentIndex - 1 + this.matches.length) % this.matches.length;
    this.navigateToMatch(this.currentIndex);
    return this.matches[this.currentIndex] ?? null;
  }

  /** 跳转到指定匹配: 设置选区并滚动到该单元格 */
  private navigateToMatch(index: number): void {
    const m = this.matches[index];
    if (!m) return;
    this.renderEngine.setSelection({
      startRow: m.row,
      startCol: m.col,
      endRow: m.row,
      endCol: m.col,
    });
    this.renderEngine.scrollToCell(m.row, m.col);
    this.updateHighlights();
    this.updateCounter();
  }

  // ============================================================

  /**
   * 替换文本中指定区间的子串
   */
  private replaceCore(text: string, matchStart: number, matchEnd: number, replacement: string): string {
    return text.slice(0, matchStart) + replacement + text.slice(matchEnd);
  }

  /**
   * 替换当前匹配
   * 替换后重新搜索并定位到下一处匹配
   */
  replaceCurrent(replacement: string): boolean {
    if (this.currentIndex < 0 || !this.currentOptions || this.currentKeyword === null) {
      return false;
    }
    const m = this.matches[this.currentIndex];
    if (!m) return false;
    const cell = this.sheet.getCell(m.row, m.col);
    if (!cell) return false;

    const options = this.currentOptions;
    const replaced = this.applyReplaceToCell(cell, m, replacement, options);
    if (!replaced) return false;

    // 刷新公式重算与渲染, 通知文档已修改
    this.editEngine.recalcAndRefresh();
    this.callbacks.onContentChange?.();

    // 重新搜索: 从当前单元格位置开始找下一处
    this.currentReplacement = replacement;
    this.matches = this.findAll(this.currentKeyword, options);
    if (this.matches.length === 0) {
      this.currentIndex = -1;
    } else {
      // 定位到当前单元格之后 (替换后该处通常已不匹配)
      this.currentIndex = this.indexOfNextMatchAfter(this.matches, m.row, m.col);
    }
    this.updateHighlights();
    this.updateCounter();
    if (this.currentIndex >= 0) {
      this.navigateToMatch(this.currentIndex);
    }
    return true;
  }

  /**
   * 全部替换
   * @returns 替换次数
   */
  replaceAll(replacement: string): number {
    if (!this.currentOptions || this.currentKeyword === null) return 0;
    const options = this.currentOptions;
    const keyword = this.currentKeyword;
    const regex = this.createFindRegExp(keyword, options);
    if (!regex) return 0;

    const cells = this.getSearchCells(options);
    let count = 0;

    for (const cell of cells) {
      const text = this.getCellSearchText(cell, options);
      const positions = this.findInText(text, regex, options);
      if (positions.length === 0) continue;

      // 从后向前替换以保持前面索引不变
      let newText = text;
      for (let i = positions.length - 1; i >= 0; i--) {
        const [start, end] = positions[i];
        newText = this.replaceCore(newText, start, end, replacement);
        count++;
      }

      this.writeCellText(cell, newText, options);
    }

    this.editEngine.recalcAndRefresh();
    this.callbacks.onContentChange?.();

    // 重新搜索以更新计数与高亮
    this.currentReplacement = replacement;
    this.matches = this.findAll(keyword, options);
    if (this.matches.length === 0) {
      this.currentIndex = -1;
    } else {
      this.currentIndex = 0;
    }
    this.updateHighlights();
    this.updateCounter();
    return count;
  }

  /**
   * 对单个单元格应用一次替换 (用于 replaceCurrent)
   * @returns 是否成功写入
   */
  private applyReplaceToCell(cell: Cell, m: FindMatch, replacement: string, options: FindOptions): boolean {
    const text = this.getCellSearchText(cell, options);
    const newText = this.replaceCore(text, m.matchStart, m.matchEnd, replacement);
    this.writeCellText(cell, newText, options);
    return true;
  }

  /**
   * 将新文本写回单元格
   * - searchFormula 且单元格有公式: 写回公式 (去 '=' 前缀)
   * - 否则按值写入, 若原为公式单元格则清除公式
   */
  private writeCellText(cell: Cell, newText: string, options: FindOptions): void {
    if (options.searchFormula && cell.formula) {
      const newFormula = newText.startsWith('=') ? newText.slice(1) : newText;
      this.sheet.setCellFormula(cell.row, cell.column, newFormula);
      return;
    }
    const typed = this.parseValue(newText);
    this.sheet.setCellValue(cell.row, cell.column, typed.value, typed.type);
    // 若原来是公式单元格, 替换为静态值后应清除公式标记
    const updated = this.sheet.getCell(cell.row, cell.column);
    if (updated && updated.formula && !options.searchFormula) {
      updated.formula = undefined;
      updated.type = typed.type;
    }
  }

  /**
   * 解析字符串为单元格值 (推断类型)
   * 与 edit-engine 的 parseInputValue 行为对齐
   */
  private parseValue(text: string): { value: string | number | boolean; type: CellValueType } {
    const trimmed = text.trim();
    if (trimmed === '') return { value: '', type: 'string' };
    if (trimmed.toLowerCase() === 'true') return { value: true, type: 'boolean' };
    if (trimmed.toLowerCase() === 'false') return { value: false, type: 'boolean' };
    const num = Number(trimmed);
    if (trimmed !== '' && !isNaN(num) && isFinite(num)) {
      return { value: num, type: 'number' };
    }
    return { value: trimmed, type: 'string' };
  }

  // ============================================================
  // 高亮 — 在 render-engine 渲染的单元格上添加高亮 class
  // ============================================================

  /** 查找渲染引擎的 canvas 容器 */
  private getCanvasEl(): HTMLElement | null {
    return this.container.querySelector('.zq-sheet-canvas') as HTMLElement | null;
  }

  /**
   * 更新所有匹配单元格的高亮
   * - zq-find-highlight: 所有匹配单元格
   * - zq-find-current: 当前匹配单元格 (特殊颜色)
   */
  private updateHighlights(): void {
    const canvas = this.getCanvasEl();
    if (!canvas) return;
    const allCells = canvas.querySelectorAll('.zq-cell');
    // 先清除
    allCells.forEach(el => {
      el.classList.remove('zq-find-highlight', 'zq-find-current');
    });
    if (this.matches.length === 0) return;

    // 收集匹配单元格 key
    const matchKeys = new Set<string>();
    for (const m of this.matches) {
      matchKeys.add(`${m.row}:${m.col}`);
    }
    let currentKey = '';
    if (this.currentIndex >= 0 && this.matches[this.currentIndex]) {
      const c = this.matches[this.currentIndex];
      currentKey = `${c.row}:${c.col}`;
    }
    allCells.forEach(el => {
      const r = el.getAttribute('data-row');
      const c = el.getAttribute('data-col');
      if (r === null || c === null) return;
      const key = `${r}:${c}`;
      if (matchKeys.has(key)) {
        el.classList.add('zq-find-highlight');
      }
      if (key === currentKey) {
        el.classList.add('zq-find-current');
      }
    });
  }

  /**
   * 设置 MutationObserver — 滚动时 render-engine 会重建单元格 DOM,
   * 此刻重新应用高亮
   */
  private setupHighlightObserver(): void {
    if (this.highlightObserver) return;
    const canvas = this.getCanvasEl();
    if (!canvas) return;
    this.highlightObserver = new MutationObserver(() => {
      if (this.matches.length > 0) {
        this.updateHighlights();
      }
    });
    this.highlightObserver.observe(canvas, { childList: true, subtree: false });
  }

  // ============================================================
  // 计数器
  // ============================================================

  /** 更新计数显示并通知回调, 格式 "current/total" (current 1-based) */
  private updateCounter(): void {
    const total = this.matches.length;
    const current = this.currentIndex >= 0 ? this.currentIndex + 1 : 0;
    if (this.counterEl) {
      this.counterEl.textContent = total > 0 ? `${current}/${total}` : '0/0';
    }
    this.callbacks.onMatchChange?.(current, total);
  }

  // ============================================================
  // 查找栏 UI
  // ============================================================

  /**
   * 打开查找栏
   * @param mode 'find' 仅查找, 'replace' 查找+替换
   */
  showFindBar(mode: FindBarMode = 'find'): void {
    if (!this.findBarEl) {
      this.injectStyles();
      this.createFindBarDOM();
      this.setupHighlightObserver();
    }
    this.barMode = mode;
    this.applyMode();
    this.findBarEl!.style.display = 'flex';
    // 聚焦查找输入框
    setTimeout(() => {
      this.findInputEl?.focus();
      this.findInputEl?.select();
    }, 0);
  }

  /** 关闭查找栏 */
  hideFindBar(): void {
    if (!this.findBarEl) return;
    this.findBarEl.style.display = 'none';
    // 清除高亮
    this.matches = [];
    this.currentIndex = -1;
    this.currentKeyword = null;
    this.currentOptions = null;
    this.updateHighlights();
    this.updateCounter();
    this.callbacks.onClose?.();
  }

  /** 切换查找/替换模式时显示/隐藏替换行 */
  private applyMode(): void {
    if (!this.replaceRowEl) return;
    if (this.barMode === 'replace') {
      this.replaceRowEl.style.display = 'flex';
    } else {
      this.replaceRowEl.style.display = 'none';
    }
  }

  /** 注入样式 (仅一次) */
  private injectStyles(): void {
    if (document.getElementById('zq-find-styles')) return;
    const style = document.createElement('style');
    style.id = 'zq-find-styles';
    style.textContent = `
      .zq-find-bar {
        position: absolute;
        top: 8px;
        right: 16px;
        z-index: 200;
        display: flex;
        flex-direction: column;
        gap: 6px;
        padding: 8px;
        width: 320px;
        background: #fff;
        border: 1px solid #d0d0d0;
        border-radius: 6px;
        box-shadow: 0 4px 16px rgba(0,0,0,0.12);
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
        font-size: 13px;
        color: #333;
      }
      .zq-find-row {
        display: flex;
        align-items: center;
        gap: 4px;
      }
      .zq-find-input,
      .zq-replace-input {
        flex: 1;
        min-width: 0;
        height: 26px;
        border: 1px solid #d0d0d0;
        border-radius: 3px;
        padding: 0 6px;
        font-size: 13px;
        outline: none;
        box-sizing: border-box;
      }
      .zq-find-input:focus,
      .zq-replace-input:focus {
        border-color: #4787f5;
      }
      .zq-find-counter {
        min-width: 48px;
        text-align: center;
        font-size: 12px;
        color: #666;
        flex-shrink: 0;
      }
      .zq-find-btn {
        height: 26px;
        min-width: 28px;
        padding: 0 6px;
        border: 1px solid #d0d0d0;
        border-radius: 3px;
        background: #fff;
        cursor: pointer;
        font-size: 12px;
        color: #333;
        display: inline-flex;
        align-items: center;
        justify-content: center;
      }
      .zq-find-btn:hover {
        background: #f0f0f0;
      }
      .zq-find-btn:active {
        background: #e0e0e0;
      }
      .zq-find-btn.primary {
        background: #4787f5;
        border-color: #4787f5;
        color: #fff;
      }
      .zq-find-btn.primary:hover {
        background: #2f74e8;
      }
      .zq-find-btn.icon {
        font-size: 14px;
      }
      .zq-find-btn.close {
        border: none;
        background: transparent;
        font-size: 16px;
        color: #999;
      }
      .zq-find-btn.close:hover {
        color: #333;
        background: #f0f0f0;
      }
      .zq-replace-row {
        display: none;
      }
      .zq-find-options {
        display: none;
        flex-wrap: wrap;
        gap: 8px 12px;
        padding: 4px 2px 0;
        border-top: 1px solid #ececec;
        font-size: 12px;
      }
      .zq-find-options.expanded {
        display: flex;
      }
      .zq-find-options label {
        display: inline-flex;
        align-items: center;
        gap: 4px;
        cursor: pointer;
        color: #555;
        user-select: none;
      }
      .zq-find-options input[type="checkbox"] {
        cursor: pointer;
        margin: 0;
      }
      /* 单元格高亮 */
      .zq-cell.zq-find-highlight {
        background-color: #fff3a8 !important;
      }
      .zq-cell.zq-find-current {
        background-color: #ff9a3c !important;
        outline: 2px solid #f56a00;
        outline-offset: -2px;
      }
    `;
    document.head.appendChild(style);
  }

  /** 创建查找栏 DOM */
  private createFindBarDOM(): void {
    // 确保容器可定位 (find bar 使用 absolute 定位)
    const computedPos = getComputedStyle(this.container).position;
    if (computedPos === 'static') {
      this.container.style.position = 'relative';
    }

    const bar = document.createElement('div');
    bar.className = 'zq-find-bar';
    bar.style.display = 'none';

    // 第一行: 查找输入 + 计数 + 上一个/下一个/选项/关闭
    const findRow = document.createElement('div');
    findRow.className = 'zq-find-row';

    this.findInputEl = document.createElement('input');
    this.findInputEl.type = 'text';
    this.findInputEl.className = 'zq-find-input';
    this.findInputEl.placeholder = '查找内容';

    this.counterEl = document.createElement('span');
    this.counterEl.className = 'zq-find-counter';
    this.counterEl.textContent = '0/0';

    const prevBtn = document.createElement('button');
    prevBtn.type = 'button';
    prevBtn.className = 'zq-find-btn icon';
    prevBtn.title = '上一个 (Shift+Enter)';
    prevBtn.textContent = '▲';

    const nextBtn = document.createElement('button');
    nextBtn.type = 'button';
    nextBtn.className = 'zq-find-btn icon';
    nextBtn.title = '下一个 (Enter)';
    nextBtn.textContent = '▼';

    this.optionsToggleEl = document.createElement('button');
    this.optionsToggleEl.type = 'button';
    this.optionsToggleEl.className = 'zq-find-btn icon';
    this.optionsToggleEl.title = '选项';
    this.optionsToggleEl.textContent = '⚙';

    const closeBtn = document.createElement('button');
    closeBtn.type = 'button';
    closeBtn.className = 'zq-find-btn close';
    closeBtn.title = '关闭 (Esc)';
    closeBtn.textContent = '✕';

    findRow.appendChild(this.findInputEl);
    findRow.appendChild(this.counterEl);
    findRow.appendChild(prevBtn);
    findRow.appendChild(nextBtn);
    findRow.appendChild(this.optionsToggleEl);
    findRow.appendChild(closeBtn);
    bar.appendChild(findRow);

    // 第二行: 替换输入 + 替换/全部替换
    this.replaceRowEl = document.createElement('div');
    this.replaceRowEl.className = 'zq-find-row zq-replace-row';

    this.replaceInputEl = document.createElement('input');
    this.replaceInputEl.type = 'text';
    this.replaceInputEl.className = 'zq-replace-input';
    this.replaceInputEl.placeholder = '替换为';

    const replaceBtn = document.createElement('button');
    replaceBtn.type = 'button';
    replaceBtn.className = 'zq-find-btn';
    replaceBtn.title = '替换当前';
    replaceBtn.textContent = '替换';

    const replaceAllBtn = document.createElement('button');
    replaceAllBtn.type = 'button';
    replaceAllBtn.className = 'zq-find-btn primary';
    replaceAllBtn.title = '全部替换';
    replaceAllBtn.textContent = '全部替换';

    this.replaceRowEl.appendChild(this.replaceInputEl);
    this.replaceRowEl.appendChild(replaceBtn);
    this.replaceRowEl.appendChild(replaceAllBtn);
    bar.appendChild(this.replaceRowEl);

    // 第三行: 选项
    this.optionsRowEl = document.createElement('div');
    this.optionsRowEl.className = 'zq-find-options';

    type BooleanOptionKey = 'caseSensitive' | 'wholeWord' | 'useRegex' | 'searchFormula';
    const mkOpt = (label: string, key: BooleanOptionKey): HTMLLabelElement => {
      const wrap = document.createElement('label');
      const cb = document.createElement('input');
      cb.type = 'checkbox';
      cb.checked = this.options[key];
      cb.addEventListener('change', () => {
        this.options[key] = cb.checked;
        this.runSearchFromUI();
      });
      wrap.appendChild(cb);
      wrap.appendChild(document.createTextNode(label));
      return wrap;
    };

    this.optionsRowEl.appendChild(mkOpt('区分大小写', 'caseSensitive'));
    this.optionsRowEl.appendChild(mkOpt('全字匹配', 'wholeWord'));
    this.optionsRowEl.appendChild(mkOpt('正则表达式', 'useRegex'));
    this.optionsRowEl.appendChild(mkOpt('匹配公式', 'searchFormula'));
    bar.appendChild(this.optionsRowEl);

    this.container.appendChild(bar);
    this.findBarEl = bar;

    // 绑定事件
    this.bindFindBarEvents({
      prevBtn, nextBtn, closeBtn,
      replaceBtn, replaceAllBtn,
    });
  }

  /** 绑定查找栏内部事件 */
  private bindFindBarEvents(els: {
    prevBtn: HTMLButtonElement;
    nextBtn: HTMLButtonElement;
    closeBtn: HTMLButtonElement;
    replaceBtn: HTMLButtonElement;
    replaceAllBtn: HTMLButtonElement;
  }): void {
    // 输入框防抖搜索
    this.findInputEl?.addEventListener('input', () => {
      if (this.inputDebounceId !== null) {
        clearTimeout(this.inputDebounceId);
      }
      this.inputDebounceId = window.setTimeout(() => {
        this.inputDebounceId = null;
        this.runSearchFromUI();
      }, 120);
    });

    // 查找输入框键盘事件
    this.findInputEl?.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') {
        e.preventDefault();
        e.stopPropagation();
        if (e.shiftKey) {
          this.findPrevious();
        } else {
          // 首次按 Enter 若未搜索过, 先搜索
          if (this.matches.length === 0) {
            this.runSearchFromUI();
          }
          this.findNext();
        }
      } else if (e.key === 'Escape') {
        e.preventDefault();
        e.stopPropagation();
        this.hideFindBar();
      }
    });

    // 替换输入框键盘事件: Enter 替换当前
    this.replaceInputEl?.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') {
        e.preventDefault();
        e.stopPropagation();
        this.doReplaceCurrentFromUI();
      } else if (e.key === 'Escape') {
        e.preventDefault();
        e.stopPropagation();
        this.hideFindBar();
      }
    });

    // 按钮
    els.prevBtn.addEventListener('click', () => {
      if (this.matches.length === 0) {
        this.runSearchFromUI();
      }
      this.findPrevious();
    });
    els.nextBtn.addEventListener('click', () => {
      if (this.matches.length === 0) {
        this.runSearchFromUI();
      }
      this.findNext();
    });
    els.closeBtn.addEventListener('click', () => this.hideFindBar());
    els.replaceBtn.addEventListener('click', () => this.doReplaceCurrentFromUI());
    els.replaceAllBtn.addEventListener('click', () => {
      const replacement = this.replaceInputEl?.value ?? '';
      const count = this.replaceAll(replacement);
      // 简要反馈: 临时显示替换次数, 1.5s 后恢复计数显示
      if (this.counterEl) {
        this.counterEl.textContent = `已替换 ${count} 处`;
        window.setTimeout(() => this.updateCounter(), 1500);
      }
    });

    // 选项展开/收起
    this.optionsToggleEl?.addEventListener('click', () => {
      this.optionsExpanded = !this.optionsExpanded;
      if (this.optionsRowEl) {
        this.optionsRowEl.classList.toggle('expanded', this.optionsExpanded);
      }
    });

    // 阻止点击查找栏时事件冒泡到表格 (避免触发表格选区/编辑)
    this.findBarEl?.addEventListener('mousedown', (e) => {
      e.stopPropagation();
    });
  }

  /** 从 UI 读取关键字并执行搜索 */
  private runSearchFromUI(): void {
    const keyword = this.findInputEl?.value ?? '';
    this.performSearch(keyword, { ...this.options });
  }

  /** 从 UI 读取替换文本并替换当前 */
  private doReplaceCurrentFromUI(): void {
    if (this.matches.length === 0) {
      this.runSearchFromUI();
    }
    const replacement = this.replaceInputEl?.value ?? '';
    this.replaceCurrent(replacement);
  }

  // ============================================================
  // 全局快捷键
  // ============================================================

  /**
   * 绑定全局快捷键 (Ctrl+F / Ctrl+H)
   * 应挂载到表格容器上, 在捕获阶段拦截以避免与 EditEngine 冲突
   */
  bindGlobalShortcuts(): void {
    this.container.addEventListener('keydown', (e) => {
      const isMod = e.ctrlKey || e.metaKey;
      if (!isMod) return;
      const key = e.key.toLowerCase();
      if (key === 'f') {
        e.preventDefault();
        e.stopPropagation();
        this.showFindBar('find');
      } else if (key === 'h') {
        e.preventDefault();
        e.stopPropagation();
        this.showFindBar('replace');
        // 自动展开选项便于操作
        if (!this.optionsExpanded) {
          this.optionsExpanded = true;
          this.optionsRowEl?.classList.add('expanded');
        }
      }
    }, true); // 捕获阶段, 优先于 EditEngine
  }

  // ============================================================
  // 工作表切换 / 销毁
  // ============================================================

  /** 切换工作表时调用, 清空状态并重新建立高亮观察器 */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
    this.matches = [];
    this.currentIndex = -1;
    this.currentKeyword = null;
    this.currentOptions = null;
    this.updateHighlights();
    this.updateCounter();
    // canvas 元素可能未变, 观察器仍有效; 若需重建则先断开
    if (this.highlightObserver) {
      this.highlightObserver.disconnect();
      this.highlightObserver = null;
    }
    this.setupHighlightObserver();
  }

  /** 销毁, 清理 DOM 与观察器 */
  destroy(): void {
    if (this.inputDebounceId !== null) {
      clearTimeout(this.inputDebounceId);
      this.inputDebounceId = null;
    }
    if (this.highlightObserver) {
      this.highlightObserver.disconnect();
      this.highlightObserver = null;
    }
    this.findBarEl?.remove();
    this.findBarEl = null;
    this.findInputEl = null;
    this.replaceInputEl = null;
    this.counterEl = null;
    this.replaceRowEl = null;
    this.optionsRowEl = null;
    this.optionsToggleEl = null;
    this.matches = [];
    this.currentIndex = -1;
  }
}
