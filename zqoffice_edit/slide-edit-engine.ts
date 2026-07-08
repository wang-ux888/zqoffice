/**
 * 演示文稿编辑引擎 — 拖拽 + 调整大小 + 旋转 + 文本编辑 + 撤销重做
 *
 * 职责:
 *   1. 形状拖拽移动
 *   2. 形状调整大小 (8 个手柄)
 *   3. 形状旋转
 *   4. 文本编辑 (双击进入编辑模式)
 *   5. 键盘操作 (Delete/方向键/Ctrl+D 复制/Ctrl+Z 撤销等)
 *   6. 撤销/重做栈
 */

import { SlideModel, Shape, Geometry, emuToPixel, pixelToEmu, angleToDegree, degreeToAngle } from './slide-model';
import { SlideRenderEngine, HandleType } from './slide-render-engine';

/** 编辑操作类型 */
export type SlideEditType =
  | 'addShape' | 'removeShape' | 'moveShape' | 'resizeShape'
  | 'rotateShape' | 'editText' | 'setStyle' | 'reorder'
  | 'addSlide' | 'removeSlide' | 'moveSlide';

/** 编辑操作记录 */
export interface SlideEditOperation {
  type: SlideEditType;
  shapeId?: string;
  oldValue?: unknown;
  newValue?: unknown;
}

/** 编辑引擎回调 */
export interface SlideEditCallbacks {
  onContentChange?: () => void;
  onSelectionChange?: (shapeId: string | null) => void;
  onUndoRedo?: (canUndo: boolean, canRedo: boolean) => void;
}

export class SlideEditEngine {
  private container: HTMLElement;
  private slide: SlideModel;
  private renderEngine: SlideRenderEngine;
  private callbacks: SlideEditCallbacks;

  // 拖拽状态
  private dragging: boolean = false;
  private dragStart: { x: number; y: number } | null = null;
  private dragShape: Shape | null = null;
  private dragOriginalGeo: Geometry | null = null;

  // 调整大小状态
  private resizing: { handle: HandleType; startRect: Geometry; startPos: { x: number; y: number } } | null = null;

  // 旋转状态
  private rotating: { startAngle: number; startRotation: number; center: { x: number; y: number } } | null = null;

  // 文本编辑状态
  private editing: boolean = false;
  private editingShapeId: string | null = null;
  private editorEl: HTMLTextAreaElement | null = null;

  // 撤销/重做栈
  private undoStack: SlideEditOperation[] = [];
  private redoStack: SlideEditOperation[] = [];
  private readonly MAX_UNDO = 100;

  constructor(
    container: HTMLElement,
    slide: SlideModel,
    renderEngine: SlideRenderEngine,
    callbacks: SlideEditCallbacks = {},
  ) {
    this.container = container;
    this.slide = slide;
    this.renderEngine = renderEngine;
    this.callbacks = callbacks;

    this.setupStyles();
    this.bindEvents();
  }

  private setupStyles(): void {
    if (document.getElementById('zq-slide-edit-styles')) return;
    const style = document.createElement('style');
    style.id = 'zq-slide-edit-styles';
    style.textContent = `
      .zq-slide-text-editor {
        position: absolute;
        z-index: 2000;
        box-sizing: border-box;
        border: 2px solid #4787f5;
        padding: 4px 8px;
        margin: 0;
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
        background: #fff;
        outline: none;
        overflow: auto;
        resize: none;
        box-shadow: 0 2px 8px rgba(0,0,0,0.2);
      }
    `;
    document.head.appendChild(style);
  }

  private bindEvents(): void {
    // 键盘事件
    this.container.addEventListener('keydown', (e) => this.handleKeyDown(e));

    // 全局鼠标事件 (用于拖拽和调整大小)
    document.addEventListener('mousemove', (e) => this.handleMouseMove(e));
    document.addEventListener('mouseup', () => this.handleMouseUp());

    // 监听选区层的手柄事件
    const selectionLayer = this.container.querySelector('.zq-slide-selection-handle');
    if (selectionLayer) {
      const handles = this.container.querySelectorAll('.zq-slide-selection-handle');
      handles.forEach(h => {
        h.addEventListener('mousedown', (e) => {
          e.stopPropagation();
          e.preventDefault();
          this.startResizeOrRotate(e as MouseEvent, h.className);
        });
      });
    }

    // 监听形状的拖拽
    this.container.addEventListener('mousedown', (e) => {
      const target = e.target as HTMLElement;
      const shapeEl = target.closest('.zq-slide-shape') as HTMLElement | null;
      if (shapeEl && !target.classList.contains('zq-slide-selection-handle')) {
        const shapeId = shapeEl.dataset.shapeId;
        if (shapeId) {
          this.startDrag(e, shapeId);
        }
      }
    });
  }

  // ============================================================
  // 形状拖拽
  // ============================================================

  private startDrag(e: MouseEvent, shapeId: string): void {
    if (this.editing) return;

    const shape = this.slide.getShape(shapeId);
    if (!shape || shape.locked) return;

    this.dragging = true;
    this.dragShape = shape;
    this.dragStart = { x: e.clientX, y: e.clientY };
    this.dragOriginalGeo = { ...shape.geometry };
  }

  private handleDragMove(e: MouseEvent): void {
    if (!this.dragging || !this.dragShape || !this.dragStart || !this.dragOriginalGeo) return;

    const scale = this.renderEngine.getRenderScale();
    const dx = (e.clientX - this.dragStart.x) / scale;
    const dy = (e.clientY - this.dragStart.y) / scale;

    // 转换为 EMU
    const dxEmu = pixelToEmu(dx);
    const dyEmu = pixelToEmu(dy);

    this.slide.setShapePosition(
      this.dragShape.id,
      this.dragOriginalGeo.x + dxEmu,
      this.dragOriginalGeo.y + dyEmu,
    );
    this.renderEngine.refresh();
  }

  // ============================================================
  // 调整大小
  // ============================================================

  private startResizeOrRotate(e: MouseEvent, className: string): void {
    const selection = this.renderEngine.getSelection();
    if (!selection.shapeId) return;

    const shape = this.slide.getShape(selection.shapeId);
    if (!shape) return;

    // 从 className 提取手柄类型
    const handleMatch = className.match(/(?:^|\s)(tl|tr|bl|br|t|b|l|r|rot)(?:\s|$)/);
    if (!handleMatch) return;
    const handle = handleMatch[1] as HandleType;

    if (handle === 'rot') {
      // 旋转
      const el = this.container.querySelector(`[data-shape-id="${selection.shapeId}"]`) as HTMLElement;
      if (!el) return;
      const rect = el.getBoundingClientRect();
      const center = {
        x: rect.left + rect.width / 2,
        y: rect.top + rect.height / 2,
      };
      const startAngle = Math.atan2(e.clientY - center.y, e.clientX - center.x) * 180 / Math.PI;
      this.rotating = {
        startAngle,
        startRotation: shape.rotation ?? 0,
        center,
      };
    } else {
      // 调整大小
      this.resizing = {
        handle,
        startRect: { ...shape.geometry },
        startPos: { x: e.clientX, y: e.clientY },
      };
    }
  }

  private handleResizeMove(e: MouseEvent): void {
    if (!this.resizing) return;

    const selection = this.renderEngine.getSelection();
    if (!selection.shapeId) return;

    const scale = this.renderEngine.getRenderScale();
    const dx = (e.clientX - this.resizing.startPos.x) / scale;
    const dy = (e.clientY - this.resizing.startPos.y) / scale;
    const dxEmu = pixelToEmu(dx);
    const dyEmu = pixelToEmu(dy);

    const { handle, startRect } = this.resizing;
    let { x, y, width, height } = startRect;

    switch (handle) {
      case 'tl':
        x = startRect.x + dxEmu;
        y = startRect.y + dyEmu;
        width = startRect.width - dxEmu;
        height = startRect.height - dyEmu;
        break;
      case 'tr':
        y = startRect.y + dyEmu;
        width = startRect.width + dxEmu;
        height = startRect.height - dyEmu;
        break;
      case 'bl':
        x = startRect.x + dxEmu;
        width = startRect.width - dxEmu;
        height = startRect.height + dyEmu;
        break;
      case 'br':
        width = startRect.width + dxEmu;
        height = startRect.height + dyEmu;
        break;
      case 't':
        y = startRect.y + dyEmu;
        height = startRect.height - dyEmu;
        break;
      case 'b':
        height = startRect.height + dyEmu;
        break;
      case 'l':
        x = startRect.x + dxEmu;
        width = startRect.width - dxEmu;
        break;
      case 'r':
        width = startRect.width + dxEmu;
        break;
    }

    // 最小尺寸限制
    const minWidth = pixelToEmu(20);
    const minHeight = pixelToEmu(20);
    if (width < minWidth) {
      if (handle.includes('l')) x = startRect.x + startRect.width - minWidth;
      width = minWidth;
    }
    if (height < minHeight) {
      if (handle.includes('t')) y = startRect.y + startRect.height - minHeight;
      height = minHeight;
    }

    this.slide.updateShape(selection.shapeId, {
      geometry: { x, y, width, height },
    });
    this.renderEngine.refresh();
  }

  // ============================================================
  // 旋转
  // ============================================================

  private handleRotateMove(e: MouseEvent): void {
    if (!this.rotating) return;

    const selection = this.renderEngine.getSelection();
    if (!selection.shapeId) return;

    const currentAngle = Math.atan2(
      e.clientY - this.rotating.center.y,
      e.clientX - this.rotating.center.x,
    ) * 180 / Math.PI;
    const deltaAngle = currentAngle - this.rotating.startAngle;
    const newRotation = degreeToAngle(angleToDegree(this.rotating.startRotation) + deltaAngle);

    this.slide.setShapeRotation(selection.shapeId, newRotation);
    this.renderEngine.refresh();
  }

  // ============================================================
  // 鼠标事件处理
  // ============================================================

  private handleMouseMove(e: MouseEvent): void {
    if (this.dragging) {
      this.handleDragMove(e);
    } else if (this.resizing) {
      this.handleResizeMove(e);
    } else if (this.rotating) {
      this.handleRotateMove(e);
    }
  }

  private handleMouseUp(): void {
    if (this.dragging) {
      this.dragging = false;
      this.dragShape = null;
      this.dragStart = null;
      this.dragOriginalGeo = null;
      this.markDirty();
    }
    if (this.resizing) {
      this.resizing = null;
      this.markDirty();
    }
    if (this.rotating) {
      this.rotating = null;
      this.markDirty();
    }
  }

  // ============================================================
  // 文本编辑
  // ============================================================

  /** 开始文本编辑 */
  startEditText(shapeId: string): void {
    if (this.editing) this.commitEditText();

    const shape = this.slide.getShape(shapeId);
    if (!shape || (shape.type !== 'text' && shape.type !== 'autoshape')) return;

    this.editing = true;
    this.editingShapeId = shapeId;

    const el = this.container.querySelector(`[data-shape-id="${shapeId}"]`) as HTMLElement;
    if (!el) return;

    const rect = el.getBoundingClientRect();
    const containerRect = this.container.getBoundingClientRect();

    this.editorEl = document.createElement('textarea');
    this.editorEl.className = 'zq-slide-text-editor';
    this.editorEl.style.left = `${rect.left - containerRect.left}px`;
    this.editorEl.style.top = `${rect.top - containerRect.top}px`;
    this.editorEl.style.width = `${rect.width}px`;
    this.editorEl.style.height = `${rect.height}px`;

    // 提取当前文本
    const texts = shape.text?.paragraphs.map(p => p.runs.map(r => r.text).join('')).join('\n') ?? '';
    this.editorEl.value = texts;

    // 应用字体样式
    if (shape.text?.paragraphs[0]?.runs[0]?.font) {
      const font = shape.text.paragraphs[0].runs[0].font;
      if (font.family) this.editorEl.style.fontFamily = font.family;
      if (font.size) this.editorEl.style.fontSize = `${font.size / 100}pt`;
      if (font.bold) this.editorEl.style.fontWeight = 'bold';
      if (font.color) this.editorEl.style.color = `#${font.color}`;
    }

    this.container.appendChild(this.editorEl);

    setTimeout(() => {
      this.editorEl?.focus();
      this.editorEl?.select();
    }, 0);

    this.editorEl.addEventListener('blur', () => this.commitEditText());
    this.editorEl.addEventListener('keydown', (e) => {
      if (e.key === 'Escape') {
        e.preventDefault();
        this.cancelEditText();
      } else if (e.key === 'Enter' && (e.ctrlKey || e.metaKey)) {
        e.preventDefault();
        this.commitEditText();
      }
    });
  }

  /** 确认文本编辑 */
  commitEditText(): void {
    if (!this.editing || !this.editingShapeId || !this.editorEl) return;

    const text = this.editorEl.value;
    const lines = text.split('\n');

    const paragraphs = lines.map(line => ({
      alignment: 'left' as const,
      runs: [{ text: line }],
    }));

    this.slide.updateShape(this.editingShapeId, {
      text: { paragraphs },
    });

    this.removeEditor();
    this.renderEngine.refresh();
    this.markDirty();
  }

  /** 取消文本编辑 */
  cancelEditText(): void {
    this.removeEditor();
    this.renderEngine.refresh();
  }

  private removeEditor(): void {
    if (this.editorEl) {
      this.editorEl.remove();
      this.editorEl = null;
    }
    this.editing = false;
    this.editingShapeId = null;
  }

  // ============================================================
  // 键盘事件
  // ============================================================

  private handleKeyDown(e: KeyboardEvent): void {
    if (this.editing) return;  // 编辑模式下不处理快捷键

    const selection = this.renderEngine.getSelection();
    if (!selection.shapeId) return;

    switch (e.key) {
      case 'Delete':
      case 'Backspace':
        e.preventDefault();
        this.deleteSelected();
        break;
      case 'ArrowUp':
        e.preventDefault();
        this.nudgeShape(0, pixelToEmu(-1));
        break;
      case 'ArrowDown':
        e.preventDefault();
        this.nudgeShape(0, pixelToEmu(1));
        break;
      case 'ArrowLeft':
        e.preventDefault();
        this.nudgeShape(pixelToEmu(-1), 0);
        break;
      case 'ArrowRight':
        e.preventDefault();
        this.nudgeShape(pixelToEmu(1), 0);
        break;
      case 'd':
      case 'D':
        if (e.ctrlKey || e.metaKey) {
          e.preventDefault();
          this.duplicateSelected();
        }
        break;
      case 'z':
      case 'Z':
        if (e.ctrlKey || e.metaKey) {
          e.preventDefault();
          if (e.shiftKey) this.redo();
          else this.undo();
        }
        break;
    }
  }

  /** 微调位置 */
  private nudgeShape(dx: number, dy: number): void {
    const sel = this.renderEngine.getSelection();
    if (!sel.shapeId) return;
    this.slide.moveShape(sel.shapeId, dx, dy);
    this.renderEngine.refresh();
    this.markDirty();
  }

  // ============================================================
  // 形状操作
  // ============================================================

  /** 添加形状 */
  addShape(shape: Shape): void {
    this.slide.addShape(shape);
    this.renderEngine.setSelection(shape.id);
    this.renderEngine.refresh();
    this.pushUndo({ type: 'addShape', shapeId: shape.id, newValue: shape });
    this.markDirty();
  }

  /** 删除选中形状 */
  deleteSelected(): void {
    const sel = this.renderEngine.getSelection();
    if (!sel.shapeId) return;
    const shape = this.slide.getShape(sel.shapeId);
    if (!shape) return;
    this.slide.removeShape(sel.shapeId);
    this.renderEngine.setSelection(null);
    this.renderEngine.refresh();
    this.pushUndo({ type: 'removeShape', shapeId: sel.shapeId, oldValue: shape });
    this.markDirty();
  }

  /** 复制选中形状 */
  duplicateSelected(): void {
    const sel = this.renderEngine.getSelection();
    if (!sel.shapeId) return;
    const shape = this.slide.getShape(sel.shapeId);
    if (!shape) return;

    const cloned: Shape = JSON.parse(JSON.stringify(shape));
    cloned.id = `shape_${Date.now()}`;
    cloned.name = `${shape.name} 副本`;
    cloned.geometry = {
      ...shape.geometry,
      x: shape.geometry.x + pixelToEmu(20),
      y: shape.geometry.y + pixelToEmu(20),
    };

    this.slide.addShape(cloned);
    this.renderEngine.setSelection(cloned.id);
    this.renderEngine.refresh();
    this.markDirty();
  }

  /** 移到顶层 */
  bringToFront(): void {
    const sel = this.renderEngine.getSelection();
    if (!sel.shapeId) return;
    this.slide.bringToFront(sel.shapeId);
    this.renderEngine.refresh();
    this.markDirty();
  }

  /** 移到底层 */
  sendToBack(): void {
    const sel = this.renderEngine.getSelection();
    if (!sel.shapeId) return;
    this.slide.sendToBack(sel.shapeId);
    this.renderEngine.refresh();
    this.markDirty();
  }

  // ============================================================
  // 撤销/重做
  // ============================================================

  private pushUndo(op: SlideEditOperation): void {
    this.undoStack.push(op);
    if (this.undoStack.length > this.MAX_UNDO) {
      this.undoStack.shift();
    }
    this.redoStack = [];
    this.callbacks.onUndoRedo?.(this.undoStack.length > 0, this.redoStack.length > 0);
  }

  undo(): void {
    const op = this.undoStack.pop();
    if (!op) return;
    this.redoStack.push(op);
    // 简化实现: 重新渲染
    this.renderEngine.refresh();
    this.callbacks.onUndoRedo?.(this.undoStack.length > 0, this.redoStack.length > 0);
  }

  redo(): void {
    const op = this.redoStack.pop();
    if (!op) return;
    this.undoStack.push(op);
    this.renderEngine.refresh();
    this.callbacks.onUndoRedo?.(this.undoStack.length > 0, this.redoStack.length > 0);
  }

  /** 清空历史 */
  clearHistory(): void {
    this.undoStack = [];
    this.redoStack = [];
  }

  private markDirty(): void {
    this.callbacks.onContentChange?.();
  }

  // ============================================================
  // 公开 API
  // ============================================================

  /** 更新幻灯片引用 */
  setSlide(slide: SlideModel): void {
    this.slide = slide;
    this.clearHistory();
    if (this.editing) this.cancelEditText();
  }

  /** 是否正在编辑 */
  isEditing(): boolean {
    return this.editing;
  }

  /** 销毁 */
  destroy(): void {
    if (this.editing) this.cancelEditText();
    this.clearHistory();
  }
}
