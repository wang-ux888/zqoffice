/**
 * 批注评论模块 — 单元格批注的增删改查与 DOM 渲染
 * 全部原创实现
 *
 * 职责:
 *   1. 添加批注 addComment(row, col, text, author)
 *   2. 回复批注 replyComment(row, col, replyId, text, author)
 *   3. 编辑批注 editComment(row, col, commentId, text)
 *   4. 删除批注 deleteComment(row, col, commentId?)
 *   5. 获取批注 getComment(row, col) / getAllComments()
 *   6. DOM 渲染:
 *      - 有批注的单元格右上角显示红色三角标记
 *      - 悬停时显示批注气泡
 *      - 批注气泡内可编辑/回复/删除/解决
 *   7. 序列化到 SheetModel (通过 Sheet.comments 字段)
 *
 * 批注语义:
 *   - 每个单元格可有多个顶级批注, 每个批注可有回复线程
 *   - getComment(row, col) 返回该单元格第一个顶级批注 (含回复)
 *   - 已解决的批注在标记上显示为绿色三角
 */

import { SheetModel, CellComment } from './data-model';
import { RenderEngine } from './render-engine';

/** 批注管理器回调 */
export interface CellCommentCallbacks {
  /** 批注内容变化时调用 (默认刷新渲染, 若设置则由调用方负责) */
  onContentChange?: () => void;
  /** 默认作者名 (未传入 author 时使用) */
  defaultAuthor?: string;
}

export class CellCommentManager {
  private container: HTMLElement;
  private sheet: SheetModel;
  private renderEngine: RenderEngine;
  private callbacks: CellCommentCallbacks;

  /** DOM 元素 */
  private markerLayerEl: HTMLDivElement;    // 红色三角标记层
  private bubbleEl: HTMLDivElement;          // 批注气泡 (单例, 悬停时显示)

  /** 当前气泡对应的单元格 */
  private bubbleCell: { row: number; col: number } | null = null;
  /** ID 自增计数器 */
  private idCounter: number = 0;

  /** 滚动重绘节流句柄 */
  private scrollRafId: number | null = null;
  /** 已绑定的事件监听器引用 (用于销毁时移除) */
  private boundScrollHandler: (() => void) | null = null;
  private boundCellHoverHandler: ((e: MouseEvent) => void) | null = null;
  private boundDocClickHandler: ((e: MouseEvent) => void) | null = null;

  constructor(
    container: HTMLElement,
    sheet: SheetModel,
    renderEngine: RenderEngine,
    callbacks: CellCommentCallbacks = {}
  ) {
    this.container = container;
    this.sheet = sheet;
    this.renderEngine = renderEngine;
    this.callbacks = callbacks;

    // 创建标记层 (覆盖在 canvas 上方, 不拦截鼠标事件)
    this.markerLayerEl = document.createElement('div');
    this.markerLayerEl.className = 'zq-comment-marker-layer';
    this.container.appendChild(this.markerLayerEl);

    // 创建批注气泡 (单例, 默认隐藏)
    this.bubbleEl = document.createElement('div');
    this.bubbleEl.className = 'zq-comment-bubble';
    this.bubbleEl.style.display = 'none';
    this.container.appendChild(this.bubbleEl);

    this.setupStyles();
    this.bindDomEvents();
    this.renderMarkers();
  }

  // ============================================================
  // 样式注入
  // ============================================================

  private setupStyles(): void {
    if (document.getElementById('zq-comment-styles')) return;
    const style = document.createElement('style');
    style.id = 'zq-comment-styles';
    style.textContent = `
      .zq-comment-marker-layer {
        position: absolute;
        top: 24px;
        left: 40px;
        right: 0;
        bottom: 0;
        overflow: hidden;
        pointer-events: none;
        z-index: 8;
      }
      .zq-comment-marker {
        position: absolute;
        width: 0;
        height: 0;
        border-style: solid;
        border-width: 0 6px 6px 0;
        border-color: transparent #ff3b30 transparent transparent;
        pointer-events: auto;
        cursor: pointer;
      }
      .zq-comment-marker.resolved {
        border-color: transparent #34c759 transparent transparent;
      }
      .zq-comment-bubble {
        position: absolute;
        z-index: 200;
        background: #fff;
        border: 1px solid #d0d0d0;
        border-radius: 6px;
        box-shadow: 0 4px 16px rgba(0,0,0,0.15);
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
        font-size: 13px;
        min-width: 240px;
        max-width: 360px;
        max-height: 400px;
        overflow-y: auto;
        pointer-events: auto;
      }
      .zq-comment-bubble-header {
        display: flex;
        align-items: center;
        justify-content: space-between;
        padding: 8px 12px;
        border-bottom: 1px solid #eee;
        background: #f9f9f9;
        border-radius: 6px 6px 0 0;
      }
      .zq-comment-bubble-title {
        font-weight: 600;
        color: #333;
        font-size: 12px;
      }
      .zq-comment-bubble-close {
        cursor: pointer;
        color: #999;
        font-size: 16px;
        padding: 0 4px;
        line-height: 1;
      }
      .zq-comment-bubble-close:hover {
        color: #333;
      }
      .zq-comment-thread {
        padding: 8px 12px;
      }
      .zq-comment-item {
        margin-bottom: 8px;
      }
      .zq-comment-item:last-child {
        margin-bottom: 0;
      }
      .zq-comment-item-meta {
        display: flex;
        align-items: center;
        justify-content: space-between;
        margin-bottom: 2px;
      }
      .zq-comment-item-author {
        font-weight: 600;
        color: #4787f5;
        font-size: 12px;
      }
      .zq-comment-item-time {
        color: #999;
        font-size: 11px;
      }
      .zq-comment-item-text {
        color: #333;
        line-height: 1.5;
        word-break: break-word;
        white-space: pre-wrap;
      }
      .zq-comment-item.resolved .zq-comment-item-text {
        color: #999;
        text-decoration: line-through;
      }
      .zq-comment-reply {
        margin-left: 16px;
        padding-left: 8px;
        border-left: 2px solid #e0e0e0;
        margin-top: 6px;
      }
      .zq-comment-actions {
        display: flex;
        gap: 4px;
        margin-top: 4px;
      }
      .zq-comment-action-btn {
        font-size: 11px;
        color: #4787f5;
        cursor: pointer;
        padding: 2px 6px;
        border-radius: 3px;
        background: transparent;
        border: none;
        font-family: inherit;
      }
      .zq-comment-action-btn:hover {
        background: #e8f0fe;
      }
      .zq-comment-action-btn.danger {
        color: #ff3b30;
      }
      .zq-comment-action-btn.danger:hover {
        background: #ffeaea;
      }
      .zq-comment-edit-area {
        margin-top: 4px;
      }
      .zq-comment-edit-textarea {
        width: 100%;
        min-height: 48px;
        border: 1px solid #d0d0d0;
        border-radius: 4px;
        padding: 4px 6px;
        font-family: inherit;
        font-size: 12px;
        resize: vertical;
        box-sizing: border-box;
      }
      .zq-comment-edit-buttons {
        display: flex;
        gap: 4px;
        margin-top: 4px;
        justify-content: flex-end;
      }
      .zq-comment-save-btn,
      .zq-comment-cancel-btn {
        font-size: 11px;
        padding: 3px 10px;
        border-radius: 3px;
        cursor: pointer;
        border: 1px solid #d0d0d0;
        font-family: inherit;
      }
      .zq-comment-save-btn {
        background: #4787f5;
        color: #fff;
        border-color: #4787f5;
      }
      .zq-comment-save-btn:hover {
        background: #3a7be0;
      }
      .zq-comment-cancel-btn {
        background: #fff;
        color: #666;
      }
      .zq-comment-reply-area {
        padding: 8px 12px;
        border-top: 1px solid #eee;
        display: flex;
        gap: 6px;
      }
      .zq-comment-reply-input {
        flex: 1;
        min-height: 28px;
        border: 1px solid #d0d0d0;
        border-radius: 4px;
        padding: 4px 8px;
        font-family: inherit;
        font-size: 12px;
        resize: none;
        box-sizing: border-box;
      }
      .zq-comment-reply-btn {
        font-size: 12px;
        padding: 4px 12px;
        border-radius: 4px;
        cursor: pointer;
        background: #4787f5;
        color: #fff;
        border: none;
        font-family: inherit;
      }
      .zq-comment-reply-btn:hover {
        background: #3a7be0;
      }
      .zq-comment-resolve-bar {
        padding: 6px 12px;
        border-top: 1px solid #eee;
        display: flex;
        justify-content: flex-end;
        gap: 8px;
      }
      .zq-comment-resolve-btn {
        font-size: 11px;
        padding: 3px 10px;
        border-radius: 3px;
        cursor: pointer;
        border: 1px solid #34c759;
        color: #34c759;
        background: #fff;
        font-family: inherit;
      }
      .zq-comment-resolve-btn:hover {
        background: #f0fff4;
      }
      .zq-comment-resolve-btn.resolved {
        background: #34c759;
        color: #fff;
      }
    `;
    document.head.appendChild(style);
  }

  // ============================================================
  // DOM 事件绑定
  // ============================================================

  private bindDomEvents(): void {
    // 监听 canvas 滚动 — 重新定位标记
    const canvas = this.container.querySelector('.zq-sheet-canvas') as HTMLElement | null;
    if (canvas) {
      this.boundScrollHandler = (): void => {
        if (this.scrollRafId !== null) cancelAnimationFrame(this.scrollRafId);
        this.scrollRafId = requestAnimationFrame(() => {
          this.renderMarkers();
          // 如果气泡可见, 同步更新位置
          if (this.bubbleCell) {
            this.positionBubble(this.bubbleCell.row, this.bubbleCell.col);
          }
        });
      };
      canvas.addEventListener('scroll', this.boundScrollHandler);
    }

    // 监听单元格悬停 — 显示批注气泡
    this.boundCellHoverHandler = (e: MouseEvent): void => {
      const target = e.target as HTMLElement;
      // 检查是否悬停在标记三角上
      if (target.classList.contains('zq-comment-marker')) {
        const row = parseInt(target.dataset.row || '0');
        const col = parseInt(target.dataset.col || '0');
        if (this.bubbleCell?.row !== row || this.bubbleCell?.col !== col) {
          this.showBubble(row, col);
        }
        return;
      }
      // 检查是否悬停在有批注的单元格上
      if (target.classList.contains('zq-cell')) {
        const row = parseInt(target.dataset.row || '0');
        const col = parseInt(target.dataset.col || '0');
        if (this.hasComment(row, col)) {
          if (this.bubbleCell?.row !== row || this.bubbleCell?.col !== col) {
            this.showBubble(row, col);
          }
        }
      }
    };
    this.container.addEventListener('mouseover', this.boundCellHoverHandler);

    // 鼠标离开单元格 — 不立即隐藏 (气泡需要交互), 由文档点击关闭
    this.boundDocClickHandler = (e: MouseEvent): void => {
      if (this.bubbleEl.style.display === 'none') return;
      const target = e.target as HTMLElement;
      // 点击气泡内部不关闭
      if (this.bubbleEl.contains(target)) return;
      // 点击标记三角不关闭
      if (target.classList.contains('zq-comment-marker')) return;
      // 点击其他区域关闭气泡
      this.hideBubble();
    };
    document.addEventListener('mousedown', this.boundDocClickHandler);
  }

  // ============================================================
  // 批注 CRUD
  // ============================================================

  /**
   * 添加批注
   * @param row 行号 (0-based)
   * @param col 列号 (0-based)
   * @param text 批注正文
   * @param author 作者 (可选, 默认使用 callbacks.defaultAuthor)
   * @returns 新建的批注对象
   */
  addComment(row: number, col: number, text: string, author?: string): CellComment {
    const comment: CellComment = {
      id: this.generateId(),
      row,
      col,
      author: author ?? this.callbacks.defaultAuthor ?? '匿名',
      text,
      timestamp: Date.now(),
      replies: [],
      resolved: false,
    };

    const comments = this.sheet.getComments();
    comments.push(comment);
    this.sheet.setComments(comments);

    this.afterChange();
    return comment;
  }

  /**
   * 回复批注
   * @param row 行号 (0-based)
   * @param col 列号 (0-based)
   * @param replyId 要回复的父批注 ID
   * @param text 回复正文
   * @param author 作者 (可选)
   * @returns 新建的回复对象, 若父批注不存在返回 null
   */
  replyComment(
    row: number,
    col: number,
    replyId: string,
    text: string,
    author?: string
  ): CellComment | null {
    const comments = this.sheet.getComments();
    const parent = this.findCommentById(comments, replyId);
    if (!parent) return null;

    const reply: CellComment = {
      id: this.generateId(),
      row,
      col,
      author: author ?? this.callbacks.defaultAuthor ?? '匿名',
      text,
      timestamp: Date.now(),
      replies: [],
      resolved: false,
    };
    parent.replies.push(reply);
    this.sheet.setComments(comments);

    this.afterChange();
    return reply;
  }

  /**
   * 编辑批注
   * @param row 行号 (0-based)
   * @param col 列号 (0-based)
   * @param commentId 要编辑的批注 ID
   * @param text 新正文
   * @returns 是否编辑成功
   */
  editComment(row: number, col: number, commentId: string, text: string): boolean {
    const comments = this.sheet.getComments();
    const target = this.findCommentById(comments, commentId);
    if (!target) return false;

    target.text = text;
    target.timestamp = Date.now();
    this.sheet.setComments(comments);

    this.afterChange();
    return true;
  }

  /**
   * 删除批注
   * @param row 行号 (0-based)
   * @param col 列号 (0-based)
   * @param commentId 要删除的批注 ID (可选, 不传则删除该单元格所有批注)
   * @returns 是否删除成功
   */
  deleteComment(row: number, col: number, commentId?: string): boolean {
    const comments = this.sheet.getComments();

    if (commentId === undefined) {
      // 删除该单元格的所有顶级批注
      const before = comments.length;
      const filtered = comments.filter(c => !(c.row === row && c.col === col));
      if (filtered.length === before) return false;
      this.sheet.setComments(filtered);
      this.afterChange();
      return true;
    }

    // 删除指定 ID 的批注 (可能是顶级或回复)
    const removed = this.removeCommentById(comments, commentId);
    if (!removed) return false;

    this.sheet.setComments(comments);
    this.afterChange();
    return true;
  }

  /**
   * 获取单元格的第一个顶级批注 (含回复)
   * @param row 行号 (0-based)
   * @param col 列号 (0-based)
   * @returns 批注对象 (深拷贝), 若无返回 null
   */
  getComment(row: number, col: number): CellComment | null {
    const comments = this.sheet.getComments();
    const found = comments.find(c => c.row === row && c.col === col);
    return found ?? null;
  }

  /**
   * 获取单元格的所有顶级批注
   * @param row 行号 (0-based)
   * @param col 列号 (0-based)
   * @returns 批注列表 (深拷贝)
   */
  getCommentsAt(row: number, col: number): CellComment[] {
    const comments = this.sheet.getComments();
    return comments.filter(c => c.row === row && c.col === col);
  }

  /**
   * 获取所有批注 (顶级, 含回复)
   * @returns 批注列表 (深拷贝)
   */
  getAllComments(): CellComment[] {
    return this.sheet.getComments();
  }

  /**
   * 切换批注的解决状态
   * @param commentId 批注 ID
   * @param resolved 是否解决 (不传则切换)
   * @returns 是否操作成功
   */
  toggleResolve(commentId: string, resolved?: boolean): boolean {
    const comments = this.sheet.getComments();
    const target = this.findCommentById(comments, commentId);
    if (!target) return false;

    target.resolved = resolved ?? !target.resolved;
    this.sheet.setComments(comments);

    this.afterChange();
    return true;
  }

  /**
   * 判断单元格是否有批注
   */
  hasComment(row: number, col: number): boolean {
    const comments = this.sheet.getComments();
    return comments.some(c => c.row === row && c.col === col);
  }

  /**
   * 获取批注数量
   */
  getCommentCount(): number {
    return this.sheet.getComments().length;
  }

  // ============================================================
  // DOM 渲染
  // ============================================================

  /**
   * 渲染红色三角标记 — 在有批注的单元格右上角显示
   * 只渲染可见区域内的标记 (虚拟滚动)
   */
  renderMarkers(): void {
    this.markerLayerEl.innerHTML = '';

    const comments = this.sheet.getComments();
    if (comments.length === 0) return;

    // 获取当前滚动位置
    const canvas = this.container.querySelector('.zq-sheet-canvas') as HTMLElement | null;
    if (!canvas) return;
    const scrollTop = canvas.scrollTop;
    const scrollLeft = canvas.scrollLeft;
    const viewW = canvas.clientWidth;
    const viewH = canvas.clientHeight;

    // 计算行/列偏移量 (与 RenderEngine 一致的逻辑)
    const rowOffsets = this.computeRowOffsets();
    const colOffsets = this.computeColOffsets();

    // 收集有批注的单元格 (去重, 取第一个批注的 resolved 状态)
    const cellMap = new Map<string, boolean>();
    for (const c of comments) {
      const key = `${c.row}:${c.col}`;
      if (!cellMap.has(key)) {
        cellMap.set(key, c.resolved ?? false);
      } else {
        // 只要有任意一个未解决, 标记为未解决
        if (!c.resolved) cellMap.set(key, false);
      }
    }

    for (const [key, resolved] of cellMap) {
      const [row, col] = key.split(':').map(Number);
      if (row >= this.sheet.rowCount || col >= this.sheet.columnCount) continue;

      const x = colOffsets[col] - scrollLeft;
      const y = rowOffsets[row] - scrollTop;
      const cellW = this.sheet.getColumnWidth(col);
      const cellH = this.sheet.getRowHeight(row);

      // 跳过不可见的单元格
      if (x + cellW < 0 || x > viewW || y + cellH < 0 || y > viewH) continue;
      // 跳过隐藏的行列
      if (this.sheet.isRowHidden(row) || this.sheet.isColumnHidden(col)) continue;

      const marker = document.createElement('div');
      marker.className = 'zq-comment-marker';
      if (resolved) marker.classList.add('resolved');
      marker.dataset.row = String(row);
      marker.dataset.col = String(col);
      // 三角定位在单元格右上角
      marker.style.left = `${x + cellW - 6}px`;
      marker.style.top = `${y}px`;
      this.markerLayerEl.appendChild(marker);
    }
  }

  /**
   * 显示批注气泡
   */
  private showBubble(row: number, col: number): void {
    this.bubbleCell = { row, col };
    this.renderBubbleContent(row, col);
    this.bubbleEl.style.display = 'block';
    this.positionBubble(row, col);
  }

  /**
   * 隐藏批注气泡
   */
  private hideBubble(): void {
    this.bubbleEl.style.display = 'none';
    this.bubbleCell = null;
  }

  /**
   * 定位气泡到单元格附近
   */
  private positionBubble(row: number, col: number): void {
    const canvas = this.container.querySelector('.zq-sheet-canvas') as HTMLElement | null;
    if (!canvas) return;
    const scrollTop = canvas.scrollTop;
    const scrollLeft = canvas.scrollLeft;

    const rowOffsets = this.computeRowOffsets();
    const colOffsets = this.computeColOffsets();

    // 单元格相对于 grid 的位置 (canvas 在 grid 内偏移 40, 24)
    const cellX = 40 + colOffsets[col] - scrollLeft;
    const cellY = 24 + rowOffsets[row] - scrollTop;
    const cellW = this.sheet.getColumnWidth(col);
    const cellH = this.sheet.getRowHeight(row);

    const containerRect = this.container.getBoundingClientRect();
    const bubbleW = Math.min(360, containerRect.width - cellX - 20);
    const bubbleH = this.bubbleEl.offsetHeight || 200;

    // 默认显示在单元格右下方
    let bubbleX = cellX + cellW + 4;
    let bubbleY = cellY;

    // 如果右侧空间不足, 显示在左侧
    if (bubbleX + bubbleW > this.container.clientWidth) {
      bubbleX = cellX - bubbleW - 4;
      if (bubbleX < 0) bubbleX = cellX + cellW + 4; // 无空间则保持右侧
    }

    // 如果下方空间不足, 向上偏移
    if (bubbleY + bubbleH > this.container.clientHeight) {
      bubbleY = Math.max(24, this.container.clientHeight - bubbleH - 4);
    }

    this.bubbleEl.style.left = `${bubbleX}px`;
    this.bubbleEl.style.top = `${bubbleY}px`;
    this.bubbleEl.style.maxWidth = `${bubbleW}px`;
  }

  /**
   * 渲染气泡内容 (批注列表 + 回复区)
   */
  private renderBubbleContent(row: number, col: number): void {
    const comments = this.getCommentsAt(row, col);
    const cellAddr = SheetModel.cellAddress(row, col);

    this.bubbleEl.innerHTML = '';

    // 头部
    const header = document.createElement('div');
    header.className = 'zq-comment-bubble-header';
    const title = document.createElement('span');
    title.className = 'zq-comment-bubble-title';
    title.textContent = `${cellAddr} 的批注`;
    const closeBtn = document.createElement('span');
    closeBtn.className = 'zq-comment-bubble-close';
    closeBtn.textContent = '×';
    closeBtn.addEventListener('click', () => this.hideBubble());
    header.appendChild(title);
    header.appendChild(closeBtn);
    this.bubbleEl.appendChild(header);

    // 批注线程
    const thread = document.createElement('div');
    thread.className = 'zq-comment-thread';
    for (const comment of comments) {
      thread.appendChild(this.renderCommentItem(comment, row, col));
    }
    this.bubbleEl.appendChild(thread);

    // 回复输入区
    const replyArea = document.createElement('div');
    replyArea.className = 'zq-comment-reply-area';
    const replyInput = document.createElement('textarea');
    replyInput.className = 'zq-comment-reply-input';
    replyInput.placeholder = '回复...';
    const replyBtn = document.createElement('button');
    replyBtn.className = 'zq-comment-reply-btn';
    replyBtn.textContent = '回复';
    replyBtn.addEventListener('click', () => {
      const text = replyInput.value.trim();
      if (!text) return;
      // 回复该单元格的第一个顶级批注
      if (comments.length > 0) {
        this.replyComment(row, col, comments[0].id, text);
        replyInput.value = '';
        // 刷新气泡内容
        this.renderBubbleContent(row, col);
        this.positionBubble(row, col);
      }
    });
    replyArea.appendChild(replyInput);
    replyArea.appendChild(replyBtn);
    this.bubbleEl.appendChild(replyArea);

    // 解决/重新打开按钮栏
    if (comments.length > 0) {
      const resolveBar = document.createElement('div');
      resolveBar.className = 'zq-comment-resolve-bar';
      const resolveBtn = document.createElement('button');
      resolveBtn.className = 'zq-comment-resolve-btn';
      const allResolved = comments.every(c => c.resolved);
      resolveBtn.textContent = allResolved ? '重新打开' : '标记为已解决';
      if (allResolved) resolveBtn.classList.add('resolved');
      resolveBtn.addEventListener('click', () => {
        const newState = !allResolved;
        for (const c of comments) {
          this.toggleResolve(c.id, newState);
        }
        this.renderBubbleContent(row, col);
        this.positionBubble(row, col);
      });
      resolveBar.appendChild(resolveBtn);
      this.bubbleEl.appendChild(resolveBar);
    }
  }

  /**
   * 渲染单条批注 (含回复线程)
   */
  private renderCommentItem(comment: CellComment, row: number, col: number): HTMLDivElement {
    const item = document.createElement('div');
    item.className = 'zq-comment-item';
    if (comment.resolved) item.classList.add('resolved');
    item.dataset.commentId = comment.id;

    // 元信息行
    const meta = document.createElement('div');
    meta.className = 'zq-comment-item-meta';
    const author = document.createElement('span');
    author.className = 'zq-comment-item-author';
    author.textContent = comment.author;
    const time = document.createElement('span');
    time.className = 'zq-comment-item-time';
    time.textContent = this.formatTime(comment.timestamp);
    meta.appendChild(author);
    meta.appendChild(time);
    item.appendChild(meta);

    // 正文
    const textEl = document.createElement('div');
    textEl.className = 'zq-comment-item-text';
    textEl.textContent = comment.text;
    item.appendChild(textEl);

    // 操作按钮
    const actions = document.createElement('div');
    actions.className = 'zq-comment-actions';
    const editBtn = document.createElement('button');
    editBtn.className = 'zq-comment-action-btn';
    editBtn.textContent = '编辑';
    editBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      this.enterEditMode(item, comment, row, col);
    });
    const deleteBtn = document.createElement('button');
    deleteBtn.className = 'zq-comment-action-btn danger';
    deleteBtn.textContent = '删除';
    deleteBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      this.deleteComment(row, col, comment.id);
      this.renderBubbleContent(row, col);
      this.positionBubble(row, col);
      if (this.getCommentsAt(row, col).length === 0) {
        this.hideBubble();
      }
    });
    actions.appendChild(editBtn);
    actions.appendChild(deleteBtn);
    item.appendChild(actions);

    // 渲染回复
    for (const reply of comment.replies) {
      const replyEl = this.renderCommentItem(reply, row, col);
      replyEl.classList.add('zq-comment-reply');
      item.appendChild(replyEl);
    }

    return item;
  }

  /**
   * 进入编辑模式
   */
  private enterEditMode(
    itemEl: HTMLDivElement,
    comment: CellComment,
    row: number,
    col: number
  ): void {
    // 隐藏正文和操作按钮
    const textEl = itemEl.querySelector('.zq-comment-item-text') as HTMLElement | null;
    const actionsEl = itemEl.querySelector('.zq-comment-actions') as HTMLElement | null;
    if (textEl) textEl.style.display = 'none';
    if (actionsEl) actionsEl.style.display = 'none';

    // 创建编辑区
    const editArea = document.createElement('div');
    editArea.className = 'zq-comment-edit-area';
    const textarea = document.createElement('textarea');
    textarea.className = 'zq-comment-edit-textarea';
    textarea.value = comment.text;
    const btns = document.createElement('div');
    btns.className = 'zq-comment-edit-buttons';
    const saveBtn = document.createElement('button');
    saveBtn.className = 'zq-comment-save-btn';
    saveBtn.textContent = '保存';
    const cancelBtn = document.createElement('button');
    cancelBtn.className = 'zq-comment-cancel-btn';
    cancelBtn.textContent = '取消';
    cancelBtn.addEventListener('click', () => {
      editArea.remove();
      if (textEl) textEl.style.display = '';
      if (actionsEl) actionsEl.style.display = '';
    });
    saveBtn.addEventListener('click', () => {
      const newText = textarea.value.trim();
      if (newText) {
        this.editComment(row, col, comment.id, newText);
      }
      this.renderBubbleContent(row, col);
      this.positionBubble(row, col);
    });
    btns.appendChild(cancelBtn);
    btns.appendChild(saveBtn);
    editArea.appendChild(textarea);
    editArea.appendChild(btns);
    itemEl.appendChild(editArea);
    textarea.focus();
  }

  // ============================================================
  // 内部辅助
  // ============================================================

  /** 生成唯一 ID */
  private generateId(): string {
    this.idCounter++;
    return `cmt_${Date.now()}_${this.idCounter}`;
  }

  /** 在批注列表中递归查找指定 ID 的批注 */
  private findCommentById(comments: CellComment[], id: string): CellComment | null {
    for (const c of comments) {
      if (c.id === id) return c;
      const found = this.findCommentById(c.replies, id);
      if (found) return found;
    }
    return null;
  }

  /** 递归删除指定 ID 的批注, 返回是否删除成功 */
  private removeCommentById(comments: CellComment[], id: string): boolean {
    for (let i = 0; i < comments.length; i++) {
      if (comments[i].id === id) {
        comments.splice(i, 1);
        return true;
      }
      if (this.removeCommentById(comments[i].replies, id)) {
        return true;
      }
    }
    return false;
  }

  /** 格式化时间戳 */
  private formatTime(ts: number): string {
    const d = new Date(ts);
    const y = d.getFullYear();
    const m = String(d.getMonth() + 1).padStart(2, '0');
    const day = String(d.getDate()).padStart(2, '0');
    const h = String(d.getHours()).padStart(2, '0');
    const min = String(d.getMinutes()).padStart(2, '0');
    return `${y}-${m}-${day} ${h}:${min}`;
  }

  /**
   * 计算行偏移量数组 (与 RenderEngine 一致的逻辑)
   * rowOffsets[i] = 前 i 行的累计高度 (隐藏行高度为 0)
   */
  private computeRowOffsets(): number[] {
    const offsets = new Array(this.sheet.rowCount + 1);
    offsets[0] = 0;
    for (let i = 0; i < this.sheet.rowCount; i++) {
      const h = this.sheet.isRowHidden(i) ? 0 : this.sheet.getRowHeight(i);
      offsets[i + 1] = offsets[i] + h;
    }
    return offsets;
  }

  /**
   * 计算列偏移量数组 (与 RenderEngine 一致的逻辑)
   * colOffsets[i] = 前 i 列的累计宽度 (隐藏列宽度为 0)
   */
  private computeColOffsets(): number[] {
    const offsets = new Array(this.sheet.columnCount + 1);
    offsets[0] = 0;
    for (let i = 0; i < this.sheet.columnCount; i++) {
      const w = this.sheet.isColumnHidden(i) ? 0 : this.sheet.getColumnWidth(i);
      offsets[i + 1] = offsets[i] + w;
    }
    return offsets;
  }

  /** 数据变更后的统一处理: 刷新渲染 + 通知回调 */
  private afterChange(): void {
    this.renderMarkers();
    if (this.callbacks.onContentChange) {
      this.callbacks.onContentChange();
    } else {
      this.renderEngine.refresh();
      this.renderMarkers();
    }
  }

  // ============================================================
  // 公开方法
  // ============================================================

  /** 刷新标记渲染 (外部渲染变化后调用) */
  refresh(): void {
    this.renderMarkers();
  }

  /** 更新工作表引用 (切换工作表时调用) */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
    this.hideBubble();
    this.renderMarkers();
  }

  /** 销毁 — 移除 DOM 和事件监听 */
  destroy(): void {
    // 移除事件监听
    const canvas = this.container.querySelector('.zq-sheet-canvas') as HTMLElement | null;
    if (canvas && this.boundScrollHandler) {
      canvas.removeEventListener('scroll', this.boundScrollHandler);
    }
    if (this.boundCellHoverHandler) {
      this.container.removeEventListener('mouseover', this.boundCellHoverHandler);
    }
    if (this.boundDocClickHandler) {
      document.removeEventListener('mousedown', this.boundDocClickHandler);
    }
    if (this.scrollRafId !== null) {
      cancelAnimationFrame(this.scrollRafId);
    }
    // 移除 DOM
    this.markerLayerEl.remove();
    this.bubbleEl.remove();
  }
}
