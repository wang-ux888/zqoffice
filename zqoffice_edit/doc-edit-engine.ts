/**
 * Word 文档编辑引擎 — 文本编辑 + 命令 + 撤销重做
 *
 * 职责:
 *   1. 文本输入 (基于 contentEditable)
 *   2. 命令系统 (格式化/插入/删除/对齐等)
 *   3. 撤销/重做栈
 *   4. 键盘操作 (Enter/Backspace/Delete/方向键等)
 */

import { DocModel, Block, ParagraphAlignment, Font, DEFAULT_FONT } from './doc-model';
import { DocRenderEngine } from './doc-render-engine';

/** 编辑操作类型 */
export type DocEditType =
  | 'insertBlock' | 'deleteBlock' | 'moveBlock'
  | 'updateText' | 'updateAlignment' | 'updateFont'
  | 'updateHeadingLevel' | 'updateHeadingText'
  | 'insertTableRow' | 'insertTableColumn'
  | 'deleteTableRow' | 'deleteTableColumn'
  | 'updateTableCell';

/** 编辑操作 */
export interface DocEditOperation {
  type: DocEditType;
  blockId?: string;
  oldValue?: unknown;
  newValue?: unknown;
}

/** 编辑回调 */
export interface DocEditCallbacks {
  onContentChange?: () => void;
  onSelectionChange?: (blockId: string | null) => void;
  onUndoRedo?: (canUndo: boolean, canRedo: boolean) => void;
}

export class DocEditEngine {
  private docModel: DocModel;
  private renderEngine: DocRenderEngine;
  private callbacks: DocEditCallbacks;

  // 撤销/重做栈
  private undoStack: DocEditOperation[] = [];
  private redoStack: DocEditOperation[] = [];
  private readonly maxUndo = 100;

  constructor(
    docModel: DocModel,
    renderEngine: DocRenderEngine,
    callbacks: DocEditCallbacks = {},
  ) {
    this.docModel = docModel;
    this.renderEngine = renderEngine;
    this.callbacks = callbacks;
    this.attachKeyboardEvents();
  }

  // ============================================================
  // 键盘事件
  // ============================================================

  private attachKeyboardEvents(): void {
    const contentEl = this.renderEngine.getContentEl();
    if (!contentEl) return;

    contentEl.addEventListener('keydown', (e: KeyboardEvent) => {
      // 撤销/重做
      if ((e.ctrlKey || e.metaKey) && e.key === 'z' && !e.shiftKey) {
        e.preventDefault();
        this.undo();
        return;
      }
      if ((e.ctrlKey || e.metaKey) && (e.key === 'y' || (e.key === 'z' && e.shiftKey))) {
        e.preventDefault();
        this.redo();
        return;
      }
      // 复制
      if ((e.ctrlKey || e.metaKey) && e.key === 'd') {
        e.preventDefault();
        this.duplicateBlock();
        return;
      }
    });
  }

  // ============================================================
  // Block 操作
  // ============================================================

  /** 在指定块后插入段落 */
  insertParagraphAfter(afterId: string, text: string = ''): string {
    const block = this.docModel.createParagraphBlock(text);
    this.docModel.insertBlockAfter(afterId, block);
    this.renderEngine.insertBlockAfter(afterId, block);
    this.recordOp({ type: 'insertBlock', blockId: block.id, newValue: block });
    this.markDirty();
    return block.id;
  }

  /** 在文档末尾插入段落 */
  appendParagraph(text: string = ''): string {
    const blocks = this.docModel.getBlocks();
    const lastBlock = blocks[blocks.length - 1];
    return this.insertParagraphAfter(lastBlock.id, text);
  }

  /** 在指定块后插入标题 */
  insertHeadingAfter(afterId: string, level: number, text: string = ''): string {
    const block = this.docModel.createHeadingBlock(level, text);
    this.docModel.insertBlockAfter(afterId, block);
    this.renderEngine.insertBlockAfter(afterId, block);
    this.recordOp({ type: 'insertBlock', blockId: block.id, newValue: block });
    this.markDirty();
    return block.id;
  }

  /** 在指定块后插入表格 */
  insertTableAfter(afterId: string, rows: number, columns: number): string {
    const block = this.docModel.createTableBlock(rows, columns);
    this.docModel.insertBlockAfter(afterId, block);
    this.renderEngine.insertBlockAfter(afterId, block);
    this.recordOp({ type: 'insertBlock', blockId: block.id, newValue: block });
    this.markDirty();
    return block.id;
  }

  /** 在指定块后插入图片 */
  insertImageAfter(afterId: string, path: string, width: number, height: number): string {
    const block = this.docModel.createImageBlock(path, width, height);
    this.docModel.insertBlockAfter(afterId, block);
    this.renderEngine.insertBlockAfter(afterId, block);
    this.recordOp({ type: 'insertBlock', blockId: block.id, newValue: block });
    this.markDirty();
    return block.id;
  }

  /** 删除当前选中块 */
  deleteBlock(blockId: string): void {
    const block = this.docModel.getBlock(blockId);
    if (!block) return;
    this.docModel.deleteBlock(blockId);
    this.renderEngine.removeBlockDom(blockId);
    this.recordOp({ type: 'deleteBlock', blockId, oldValue: block });
    this.markDirty();
  }

  /** 复制当前选中块 */
  duplicateBlock(): void {
    const blockId = this.renderEngine.getSelection();
    if (!blockId) return;
    const block = this.docModel.getBlock(blockId);
    if (!block) return;
    // 深拷贝
    const newBlock: Block = JSON.parse(JSON.stringify(block));
    newBlock.id = `block_${Date.now().toString(36)}_${Math.random().toString(36).slice(2, 8)}`;
    this.docModel.insertBlockAfter(blockId, newBlock);
    this.renderEngine.insertBlockAfter(blockId, newBlock);
    this.recordOp({ type: 'insertBlock', blockId: newBlock.id, newValue: newBlock });
    this.markDirty();
  }

  /** 上移块 */
  moveBlockUp(blockId: string): void {
    this.docModel.moveBlock(blockId, 'up');
    this.renderEngine.refresh();
    this.markDirty();
  }

  /** 下移块 */
  moveBlockDown(blockId: string): void {
    this.docModel.moveBlock(blockId, 'down');
    this.renderEngine.refresh();
    this.markDirty();
  }

  // ============================================================
  // 段落格式
  // ============================================================

  /** 设置段落对齐 */
  setAlignment(blockId: string, alignment: ParagraphAlignment): void {
    const block = this.docModel.getBlock(blockId);
    if (!block?.paragraph) return;
    const oldAlignment = block.paragraph.alignment;
    this.docModel.setParagraphAlignment(blockId, alignment);
    this.renderEngine.refreshBlock(blockId);
    this.recordOp({ type: 'updateAlignment', blockId, oldValue: oldAlignment, newValue: alignment });
    this.markDirty();
  }

  // ============================================================
  // 文本格式
  // ============================================================

  /** 设置当前选中块的字体属性 (应用到整个段落) */
  setFont(blockId: string, font: Partial<Font>): void {
    const block = this.docModel.getBlock(blockId);
    if (!block?.paragraph) return;
    const oldRuns = JSON.parse(JSON.stringify(block.paragraph.runs));
    // 应用到所有 runs
    for (const run of block.paragraph.runs) {
      run.font = { ...run.font, ...DEFAULT_FONT, ...font };
    }
    // 如果段落没有 runs，创建一个
    if (block.paragraph.runs.length === 0) {
      block.paragraph.runs.push({ text: '', font: { ...DEFAULT_FONT, ...font } });
    }
    this.renderEngine.refreshBlock(blockId);
    this.recordOp({ type: 'updateFont', blockId, oldValue: oldRuns, newValue: block.paragraph.runs });
    this.markDirty();
  }

  /** 加粗 */
  toggleBold(blockId: string): void {
    const block = this.docModel.getBlock(blockId);
    if (!block?.paragraph) return;
    const allBold = block.paragraph.runs.every(r => r.font?.bold);
    this.setFont(blockId, { bold: !allBold });
  }

  /** 斜体 */
  toggleItalic(blockId: string): void {
    const block = this.docModel.getBlock(blockId);
    if (!block?.paragraph) return;
    const allItalic = block.paragraph.runs.every(r => r.font?.italic);
    this.setFont(blockId, { italic: !allItalic });
  }

  /** 下划线 */
  toggleUnderline(blockId: string): void {
    const block = this.docModel.getBlock(blockId);
    if (!block?.paragraph) return;
    const allUnderline = block.paragraph.runs.every(r => r.font?.underline);
    this.setFont(blockId, { underline: !allUnderline });
  }

  /** 设置字号 */
  setFontSize(blockId: string, size: number): void {
    this.setFont(blockId, { size });
  }

  /** 设置字体族 */
  setFontFamily(blockId: string, family: string): void {
    this.setFont(blockId, { family });
  }

  /** 设置文字颜色 */
  setColor(blockId: string, color: string): void {
    this.setFont(blockId, { color });
  }

  // ============================================================
  // 标题操作
  // ============================================================

  /** 转换为标题 (paragraph → heading) */
  convertToHeading(blockId: string, level: number): void {
    const block = this.docModel.getBlock(blockId);
    if (!block) return;
    const text = block.paragraph
      ? block.paragraph.runs.map(r => r.text).join('')
      : (block.heading?.text ?? '');
    const oldBlock = JSON.parse(JSON.stringify(block));
    // 修改为 heading
    block.type = 'heading';
    block.heading = { level, text };
    block.paragraph = undefined;
    this.renderEngine.refreshBlock(blockId);
    this.recordOp({ type: 'updateHeadingLevel', blockId, oldValue: oldBlock, newValue: block });
    this.markDirty();
  }

  /** 转换为段落 (heading → paragraph) */
  convertToParagraph(blockId: string): void {
    const block = this.docModel.getBlock(blockId);
    if (!block) return;
    const text = block.heading?.text ?? '';
    const oldBlock = JSON.parse(JSON.stringify(block));
    block.type = 'paragraph';
    block.paragraph = {
      style: 'Normal',
      alignment: 'left',
      runs: text ? [{ text, font: { ...DEFAULT_FONT } }] : [],
    };
    block.heading = undefined;
    this.renderEngine.refreshBlock(blockId);
    this.recordOp({ type: 'updateHeadingLevel', blockId, oldValue: oldBlock, newValue: block });
    this.markDirty();
  }

  // ============================================================
  // 表格操作
  // ============================================================

  addTableRow(blockId: string, atIndex?: number): void {
    this.docModel.addTableRow(blockId, atIndex);
    this.renderEngine.refreshBlock(blockId);
    this.recordOp({ type: 'insertTableRow', blockId });
    this.markDirty();
  }

  addTableColumn(blockId: string, atIndex?: number): void {
    this.docModel.addTableColumn(blockId, atIndex);
    this.renderEngine.refreshBlock(blockId);
    this.recordOp({ type: 'insertTableColumn', blockId });
    this.markDirty();
  }

  deleteTableRow(blockId: string, row: number): void {
    this.docModel.deleteTableRow(blockId, row);
    this.renderEngine.refreshBlock(blockId);
    this.recordOp({ type: 'deleteTableRow', blockId, oldValue: row });
    this.markDirty();
  }

  deleteTableColumn(blockId: string, col: number): void {
    this.docModel.deleteTableColumn(blockId, col);
    this.renderEngine.refreshBlock(blockId);
    this.recordOp({ type: 'deleteTableColumn', blockId, oldValue: col });
    this.markDirty();
  }

  // ============================================================
  // 撤销/重做
  // ============================================================

  private recordOp(op: DocEditOperation): void {
    this.undoStack.push(op);
    if (this.undoStack.length > this.maxUndo) this.undoStack.shift();
    this.redoStack = [];
    this.notifyUndoRedo();
  }

  undo(): void {
    const op = this.undoStack.pop();
    if (!op) return;
    this.applyOpReverse(op);
    this.redoStack.push(op);
    this.notifyUndoRedo();
    this.markDirty();
  }

  redo(): void {
    const op = this.redoStack.pop();
    if (!op) return;
    this.applyOp(op);
    this.undoStack.push(op);
    this.notifyUndoRedo();
    this.markDirty();
  }

  canUndo(): boolean { return this.undoStack.length > 0; }
  canRedo(): boolean { return this.redoStack.length > 0; }

  private applyOp(op: DocEditOperation): void {
    // 简化实现：直接刷新
    this.renderEngine.refresh();
  }

  private applyOpReverse(op: DocEditOperation): void {
    // 简化实现：直接刷新
    this.renderEngine.refresh();
  }

  private notifyUndoRedo(): void {
    this.callbacks.onUndoRedo?.(this.canUndo(), this.canRedo());
  }

  // ============================================================
  // 工具
  // ============================================================

  private markDirty(): void {
    this.callbacks.onContentChange?.();
  }

  /** 获取选区块 ID */
  getSelectedBlockId(): string | null {
    return this.renderEngine.getSelection();
  }

  /** 设置新模型 (用于加载新文档) */
  setModel(docModel: DocModel): void {
    this.docModel = docModel;
  }
}
