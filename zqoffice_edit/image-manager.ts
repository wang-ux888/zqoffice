/**
 * 图片管理层 — 浮动图片与嵌入图片的 CRUD 与 DOM 渲染
 * 全部原创实现, 支持拖拽移动和 8 向调整大小
 */

import { SheetModel } from './data-model';

// ============================================================
// 类型定义
// ============================================================

/** 浮动图片 (浮在单元格上方, 可自由移动和调整大小) */
export interface FloatImage {
  id: string;
  src: string;          // base64 或 URL
  row: number;          // 锚定单元格行号 (0-based)
  col: number;          // 锚定单元格列号 (0-based)
  offsetX: number;      // 在锚定单元格内的水平偏移 (像素)
  offsetY: number;      // 在锚定单元格内的垂直偏移 (像素)
  width: number;        // 图片显示宽度 (像素)
  height: number;       // 图片显示高度 (像素)
  rotation?: number;    // 旋转角度 (0-360, 顺时针)
}

/** 嵌入图片 (嵌入单元格内, 随单元格移动) */
export interface CellImage {
  id: string;
  src: string;
  row: number;
  col: number;
}

/** 图片类型联合 */
export type SheetImage = FloatImage | CellImage;

/** 图片管理器事件回调 */
export interface ImageManagerCallbacks {
  /** 选中图片变化时触发 (null 表示取消选中) */
  onImageSelect?: (id: string | null) => void;
  /** 图片数据变化 (增删改) 时触发 */
  onImageChange?: () => void;
}

/** 调整手柄方向 */
type HandleDirection = 'nw' | 'n' | 'ne' | 'w' | 'e' | 'sw' | 's' | 'se';

/** 拖拽状态 */
interface DragState {
  type: 'move' | 'resize';
  imageId: string;
  handle: HandleDirection;
  startX: number;        // 鼠标起始 clientX
  startY: number;        // 鼠标起始 clientY
  origRow: number;
  origCol: number;
  origOffsetX: number;
  origOffsetY: number;
  origWidth: number;
  origHeight: number;
}

// ============================================================
// 图片管理器
// ============================================================

export class ImageManager {
  private sheet: SheetModel;
  private container: HTMLElement;
  private callbacks: ImageManagerCallbacks;

  /** 图片渲染层 (覆盖在单元格上方, 随内容滚动) */
  private layer: HTMLDivElement;
  /** 选中边框和手柄容器 */
  private selectionOverlay: HTMLDivElement;
  /** 8 个调整手柄 */
  private handles = new Map<HandleDirection, HTMLDivElement>();

  /** 浮动图片数据 */
  private floatImages = new Map<string, FloatImage>();
  /** 嵌入图片数据 */
  private cellImages = new Map<string, CellImage>();

  /** 浮动图片 DOM 元素映射 */
  private floatImageEls = new Map<string, HTMLDivElement>();
  /** 嵌入图片 DOM 元素映射 */
  private cellImageEls = new Map<string, HTMLImageElement>();

  /** 当前选中的图片 ID */
  private selectedId: string | null = null;

  /** 拖拽/调整状态 */
  private dragState: DragState | null = null;

  /** 全局事件监听是否已安装 */
  private globalListenersInstalled = false;

  /** 绑定的全局事件处理函数引用 (用于销毁时移除) */
  private boundMouseMove: (e: MouseEvent) => void;
  private boundMouseUp: () => void;

  constructor(
    container: HTMLElement,
    sheet: SheetModel,
    callbacks: ImageManagerCallbacks = {}
  ) {
    this.container = container;
    this.sheet = sheet;
    this.callbacks = callbacks;

    // 预绑定全局事件处理函数
    this.boundMouseMove = (e) => this.onMouseMove(e);
    this.boundMouseUp = () => this.onMouseUp();

    // 创建图片渲染层
    this.layer = document.createElement('div');
    this.layer.className = 'zq-sheet-image-layer';
    this.container.appendChild(this.layer);

    // 创建选中覆盖层 (边框 + 手柄)
    this.selectionOverlay = document.createElement('div');
    this.selectionOverlay.className = 'zq-sheet-image-selection';
    this.selectionOverlay.style.display = 'none';
    this.layer.appendChild(this.selectionOverlay);

    this.setupStyles();
    this.setupGlobalListeners();
  }

  // ============================================================
  // 样式设置
  // ============================================================

  private setupStyles(): void {
    if (document.getElementById('zq-sheet-image-styles')) return;
    const style = document.createElement('style');
    style.id = 'zq-sheet-image-styles';
    style.textContent = `
      .zq-sheet-image-layer {
        position: absolute;
        top: 0;
        left: 0;
        pointer-events: none;
        z-index: 20;
      }
      .zq-sheet-float-image {
        position: absolute;
        pointer-events: auto;
        cursor: move;
        box-sizing: border-box;
      }
      .zq-sheet-float-image img {
        width: 100%;
        height: 100%;
        display: block;
        pointer-events: none;
        user-select: none;
        -webkit-user-drag: none;
      }
      .zq-sheet-float-image.selected {
        outline: 2px solid #4787f5;
        outline-offset: 0;
      }
      .zq-sheet-cell-image {
        pointer-events: auto;
        cursor: pointer;
        display: block;
        user-select: none;
        -webkit-user-drag: none;
      }
      .zq-sheet-image-selection {
        position: absolute;
        pointer-events: none;
        z-index: 21;
        box-sizing: border-box;
      }
      .zq-sheet-image-handle {
        position: absolute;
        width: 8px;
        height: 8px;
        background: #fff;
        border: 1px solid #4787f5;
        border-radius: 1px;
        pointer-events: auto;
        z-index: 22;
        box-sizing: border-box;
      }
      .zq-sheet-image-handle.nw { cursor: nw-resize; }
      .zq-sheet-image-handle.n  { cursor: n-resize; }
      .zq-sheet-image-handle.ne { cursor: ne-resize; }
      .zq-sheet-image-handle.w  { cursor: w-resize; }
      .zq-sheet-image-handle.e  { cursor: e-resize; }
      .zq-sheet-image-handle.sw { cursor: sw-resize; }
      .zq-sheet-image-handle.s  { cursor: s-resize; }
      .zq-sheet-image-handle.se { cursor: se-resize; }
    `;
    document.head.appendChild(style);
  }

  // ============================================================
  // 全局事件监听 (拖拽和调整大小)
  // ============================================================

  private setupGlobalListeners(): void {
    if (this.globalListenersInstalled) return;
    this.globalListenersInstalled = true;
    document.addEventListener('mousemove', this.boundMouseMove);
    document.addEventListener('mouseup', this.boundMouseUp);
  }

  /** 鼠标移动处理 (拖拽移动 / 调整大小) */
  private onMouseMove(e: MouseEvent): void {
    if (!this.dragState) return;

    const img = this.floatImages.get(this.dragState.imageId);
    if (!img) return;

    const dx = e.clientX - this.dragState.startX;
    const dy = e.clientY - this.dragState.startY;

    if (this.dragState.type === 'move') {
      // —— 移动图片: 调整 offset, 超出单元格时切换锚点 ——
      let newOffsetX = this.dragState.origOffsetX + dx;
      let newOffsetY = this.dragState.origOffsetY + dy;
      let newRow = this.dragState.origRow;
      let newCol = this.dragState.origCol;

      // 水平方向: 向右溢出则推进到下一列
      while (newOffsetX >= this.sheet.getColumnWidth(newCol) && newCol < this.sheet.columnCount - 1) {
        newOffsetX -= this.sheet.getColumnWidth(newCol);
        newCol++;
      }
      // 向左溢出则退回上一列
      while (newOffsetX < 0 && newCol > 0) {
        newCol--;
        newOffsetX += this.sheet.getColumnWidth(newCol);
      }
      if (newOffsetX < 0) newOffsetX = 0;

      // 垂直方向: 向下溢出则推进到下一行
      while (newOffsetY >= this.sheet.getRowHeight(newRow) && newRow < this.sheet.rowCount - 1) {
        newOffsetY -= this.sheet.getRowHeight(newRow);
        newRow++;
      }
      // 向上溢出则退回上一行
      while (newOffsetY < 0 && newRow > 0) {
        newRow--;
        newOffsetY += this.sheet.getRowHeight(newRow);
      }
      if (newOffsetY < 0) newOffsetY = 0;

      img.row = newRow;
      img.col = newCol;
      img.offsetX = newOffsetX;
      img.offsetY = newOffsetY;
    } else if (this.dragState.type === 'resize') {
      // —— 调整大小: 根据手柄方向调整宽高 ——
      const handle = this.dragState.handle;
      let newWidth = this.dragState.origWidth;
      let newHeight = this.dragState.origHeight;
      let newOffsetX = this.dragState.origOffsetX;
      let newOffsetY = this.dragState.origOffsetY;

      // 水平方向
      if (handle.includes('e')) {
        newWidth = this.dragState.origWidth + dx;
      } else if (handle.includes('w')) {
        newWidth = this.dragState.origWidth - dx;
        newOffsetX = this.dragState.origOffsetX + dx;
      }

      // 垂直方向
      if (handle.includes('s')) {
        newHeight = this.dragState.origHeight + dy;
      } else if (handle.includes('n')) {
        newHeight = this.dragState.origHeight - dy;
        newOffsetY = this.dragState.origOffsetY + dy;
      }

      // 限制最小尺寸
      const minSize = 20;
      if (newWidth < minSize) {
        // w/n 手柄导致宽度过小时, 修正 offset
        if (handle.includes('w')) {
          newOffsetX = this.dragState.origOffsetX + (this.dragState.origWidth - minSize);
        }
        newWidth = minSize;
      }
      if (newHeight < minSize) {
        if (handle.includes('n')) {
          newOffsetY = this.dragState.origOffsetY + (this.dragState.origHeight - minSize);
        }
        newHeight = minSize;
      }

      img.width = newWidth;
      img.height = newHeight;
      img.offsetX = newOffsetX;
      img.offsetY = newOffsetY;

      // 处理 offset 为负时推进锚点单元格
      this.normalizeAnchor(img);
    }

    this.renderFloatImage(img);
    this.updateSelectionOverlay();
    this.callbacks.onImageChange?.();
  }

  /** 鼠标松开: 结束拖拽 */
  private onMouseUp(): void {
    this.dragState = null;
  }

  /** 当 offset 为负或超出单元格时, 推进/回退锚点单元格 */
  private normalizeAnchor(img: FloatImage): void {
    // 水平方向: offset 为负则回退到前一列
    while (img.offsetX < 0 && img.col > 0) {
      img.col--;
      img.offsetX += this.sheet.getColumnWidth(img.col);
    }
    // offset 超出当前列宽则推进到下一列
    while (img.offsetX >= this.sheet.getColumnWidth(img.col) && img.col < this.sheet.columnCount - 1) {
      img.offsetX -= this.sheet.getColumnWidth(img.col);
      img.col++;
    }
    if (img.offsetX < 0) img.offsetX = 0;

    // 垂直方向
    while (img.offsetY < 0 && img.row > 0) {
      img.row--;
      img.offsetY += this.sheet.getRowHeight(img.row);
    }
    while (img.offsetY >= this.sheet.getRowHeight(img.row) && img.row < this.sheet.rowCount - 1) {
      img.offsetY -= this.sheet.getRowHeight(img.row);
      img.row++;
    }
    if (img.offsetY < 0) img.offsetY = 0;
  }

  // ============================================================
  // 单元格像素位置计算
  // ============================================================

  /** 计算列的像素偏移 (累计宽度, 跳过隐藏列) */
  private getCellOffsetX(col: number): number {
    let x = 0;
    for (let c = 0; c < col; c++) {
      if (!this.sheet.isColumnHidden(c)) {
        x += this.sheet.getColumnWidth(c);
      }
    }
    return x;
  }

  /** 计算行的像素偏移 (累计高度, 跳过隐藏行) */
  private getCellOffsetY(row: number): number {
    let y = 0;
    for (let r = 0; r < row; r++) {
      if (!this.sheet.isRowHidden(r)) {
        y += this.sheet.getRowHeight(r);
      }
    }
    return y;
  }

  // ============================================================
  // 图片 CRUD
  // ============================================================

  /** 生成唯一 ID */
  private generateId(): string {
    return `img_${Date.now()}_${Math.random().toString(36).slice(2, 8)}`;
  }

  /**
   * 添加浮动图片 — 图片浮在单元格上方
   * @param row 锚定单元格行号 (0-based)
   * @param col 锚定单元格列号 (0-based)
   * @param src 图片源 (base64 或 URL)
   * @param width 显示宽度 (像素)
   * @param height 显示高度 (像素)
   * @returns 图片 ID
   */
  addFloatImage(row: number, col: number, src: string, width: number, height: number): string {
    const id = this.generateId();
    const image: FloatImage = {
      id,
      src,
      row,
      col,
      offsetX: 0,
      offsetY: 0,
      width: Math.max(20, width),
      height: Math.max(20, height),
      rotation: 0,
    };
    this.floatImages.set(id, image);
    this.renderFloatImage(image);
    this.callbacks.onImageChange?.();
    return id;
  }

  /**
   * 添加嵌入图片 — 图片嵌入单元格内, 随单元格移动
   * @param row 单元格行号 (0-based)
   * @param col 单元格列号 (0-based)
   * @param src 图片源
   * @returns 图片 ID
   */
  addCellImage(row: number, col: number, src: string): string {
    const id = this.generateId();
    const image: CellImage = { id, src, row, col };
    this.cellImages.set(id, image);
    this.renderCellImage(image);
    this.callbacks.onImageChange?.();
    return id;
  }

  /**
   * 删除图片
   * @param id 图片 ID
   */
  removeImage(id: string): void {
    // 从浮动图片中删除
    if (this.floatImages.has(id)) {
      this.floatImages.delete(id);
      const el = this.floatImageEls.get(id);
      if (el) {
        el.remove();
        this.floatImageEls.delete(id);
      }
      if (this.selectedId === id) {
        this.deselect();
      }
      this.callbacks.onImageChange?.();
      return;
    }
    // 从嵌入图片中删除
    if (this.cellImages.has(id)) {
      this.cellImages.delete(id);
      const el = this.cellImageEls.get(id);
      if (el) {
        el.remove();
        this.cellImageEls.delete(id);
      }
      if (this.selectedId === id) {
        this.deselect();
      }
      this.callbacks.onImageChange?.();
    }
  }

  /**
   * 移动图片到新位置
   * @param id 图片 ID
   * @param newRow 新行号
   * @param newCol 新列号
   */
  moveImage(id: string, newRow: number, newCol: number): void {
    const floatImg = this.floatImages.get(id);
    if (floatImg) {
      floatImg.row = newRow;
      floatImg.col = newCol;
      this.renderFloatImage(floatImg);
      if (this.selectedId === id) {
        this.updateSelectionOverlay();
      }
      this.callbacks.onImageChange?.();
      return;
    }
    const cellImg = this.cellImages.get(id);
    if (cellImg) {
      cellImg.row = newRow;
      cellImg.col = newCol;
      this.renderCellImage(cellImg);
      this.callbacks.onImageChange?.();
    }
  }

  /**
   * 调整图片大小 (仅浮动图片支持)
   * @param id 图片 ID
   * @param width 新宽度
   * @param height 新高度
   */
  resizeImage(id: string, width: number, height: number): void {
    const img = this.floatImages.get(id);
    if (!img) return;
    img.width = Math.max(20, width);
    img.height = Math.max(20, height);
    this.renderFloatImage(img);
    if (this.selectedId === id) {
      this.updateSelectionOverlay();
    }
    this.callbacks.onImageChange?.();
  }

  /**
   * 获取图片
   * @param id 图片 ID
   */
  getImage(id: string): SheetImage | null {
    return this.floatImages.get(id) ?? this.cellImages.get(id) ?? null;
  }

  /** 获取所有图片 */
  getAllImages(): SheetImage[] {
    return [...this.floatImages.values(), ...this.cellImages.values()];
  }

  /** 获取所有浮动图片 */
  getAllFloatImages(): FloatImage[] {
    return Array.from(this.floatImages.values());
  }

  /** 获取所有嵌入图片 */
  getAllCellImages(): CellImage[] {
    return Array.from(this.cellImages.values());
  }

  // ============================================================
  // 选中管理
  // ============================================================

  /** 选中图片 (显示边框和 8 个调整手柄) */
  selectImage(id: string): void {
    this.selectedId = id;
    // 更新 DOM 选中状态
    this.floatImageEls.forEach((el, imgId) => {
      el.classList.toggle('selected', imgId === id);
    });
    this.updateSelectionOverlay();
    this.callbacks.onImageSelect?.(id);
  }

  /** 取消选中 */
  deselect(): void {
    this.selectedId = null;
    this.floatImageEls.forEach(el => el.classList.remove('selected'));
    this.selectionOverlay.style.display = 'none';
    this.handles.forEach(h => h.style.display = 'none');
    this.callbacks.onImageSelect?.(null);
  }

  /** 获取当前选中的图片 ID */
  getSelectedId(): string | null {
    return this.selectedId;
  }

  // ============================================================
  // DOM 渲染
  // ============================================================

  /** 渲染浮动图片 (创建或更新 DOM) */
  private renderFloatImage(img: FloatImage): void {
    let el = this.floatImageEls.get(img.id);
    if (!el) {
      el = document.createElement('div');
      el.className = 'zq-sheet-float-image';
      el.dataset.imageId = img.id;

      const inner = document.createElement('img');
      inner.draggable = false;
      el.appendChild(inner);

      // 鼠标按下: 选中并启动拖拽
      el.addEventListener('mousedown', (e) => {
        e.stopPropagation();
        this.selectImage(img.id);
        this.startDrag(img.id, 'move', e);
      });

      this.layer.appendChild(el);
      this.floatImageEls.set(img.id, el);
    }

    const x = this.getCellOffsetX(img.col) + img.offsetX;
    const y = this.getCellOffsetY(img.row) + img.offsetY;

    el.style.left = `${x}px`;
    el.style.top = `${y}px`;
    el.style.width = `${img.width}px`;
    el.style.height = `${img.height}px`;
    if (img.rotation) {
      el.style.transform = `rotate(${img.rotation}deg)`;
    } else {
      el.style.transform = '';
    }

    const innerImg = el.querySelector('img') as HTMLImageElement;
    if (innerImg.src !== img.src) {
      innerImg.src = img.src;
    }

    if (this.selectedId === img.id) {
      el.classList.add('selected');
      this.updateSelectionOverlay();
    }
  }

  /** 渲染嵌入图片 (创建或更新 DOM) */
  private renderCellImage(img: CellImage): void {
    let el = this.cellImageEls.get(img.id);
    if (!el) {
      el = document.createElement('img');
      el.className = 'zq-sheet-cell-image';
      el.draggable = false;
      el.dataset.imageId = img.id;

      el.addEventListener('mousedown', (e) => {
        e.stopPropagation();
        // 嵌入图片不支持调整大小, 仅高亮显示
      });

      this.layer.appendChild(el);
      this.cellImageEls.set(img.id, el);
    }

    const x = this.getCellOffsetX(img.col);
    const y = this.getCellOffsetY(img.row);
    const w = this.sheet.getColumnWidth(img.col);
    const h = this.sheet.getRowHeight(img.row);

    el.src = img.src;
    el.style.position = 'absolute';
    el.style.left = `${x + 2}px`;
    el.style.top = `${y + 2}px`;
    el.style.maxWidth = `${w - 4}px`;
    el.style.maxHeight = `${h - 4}px`;
  }

  /** 更新选中覆盖层 (边框 + 8 个调整手柄) */
  private updateSelectionOverlay(): void {
    if (!this.selectedId) {
      this.selectionOverlay.style.display = 'none';
      this.handles.forEach(h => h.style.display = 'none');
      return;
    }

    const img = this.floatImages.get(this.selectedId);
    if (!img) {
      this.selectionOverlay.style.display = 'none';
      this.handles.forEach(h => h.style.display = 'none');
      return;
    }

    const x = this.getCellOffsetX(img.col) + img.offsetX;
    const y = this.getCellOffsetY(img.row) + img.offsetY;

    // 更新边框位置
    this.selectionOverlay.style.display = 'block';
    this.selectionOverlay.style.left = `${x}px`;
    this.selectionOverlay.style.top = `${y}px`;
    this.selectionOverlay.style.width = `${img.width}px`;
    this.selectionOverlay.style.height = `${img.height}px`;
    this.selectionOverlay.style.border = '2px solid #4787f5';

    // 创建/更新 8 个调整手柄
    const directions: HandleDirection[] = ['nw', 'n', 'ne', 'w', 'e', 'sw', 's', 'se'];
    const handlePositions: Record<HandleDirection, { left: string; top: string }> = {
      nw: { left: '-4px', top: '-4px' },
      n:  { left: 'calc(50% - 4px)', top: '-4px' },
      ne: { left: 'calc(100% - 4px)', top: '-4px' },
      w:  { left: '-4px', top: 'calc(50% - 4px)' },
      e:  { left: 'calc(100% - 4px)', top: 'calc(50% - 4px)' },
      sw: { left: '-4px', top: 'calc(100% - 4px)' },
      s:  { left: 'calc(50% - 4px)', top: 'calc(100% - 4px)' },
      se: { left: 'calc(100% - 4px)', top: 'calc(100% - 4px)' },
    };

    for (const dir of directions) {
      let handle = this.handles.get(dir);
      if (!handle) {
        handle = document.createElement('div');
        handle.className = `zq-sheet-image-handle ${dir}`;
        handle.dataset.handle = dir;
        handle.addEventListener('mousedown', (e) => {
          e.stopPropagation();
          e.preventDefault();
          this.selectImage(img.id);
          this.startDrag(img.id, 'resize', e, dir);
        });
        this.selectionOverlay.appendChild(handle);
        this.handles.set(dir, handle);
      }
      handle.style.display = 'block';
      handle.style.left = handlePositions[dir].left;
      handle.style.top = handlePositions[dir].top;
    }
  }

  // ============================================================
  // 拖拽启动
  // ============================================================

  /** 启动拖拽 (移动或调整大小) */
  private startDrag(imageId: string, type: 'move' | 'resize', e: MouseEvent, handle?: HandleDirection): void {
    const img = this.floatImages.get(imageId);
    if (!img) return;

    this.dragState = {
      type,
      imageId,
      handle: handle ?? 'se',
      startX: e.clientX,
      startY: e.clientY,
      origRow: img.row,
      origCol: img.col,
      origOffsetX: img.offsetX,
      origOffsetY: img.offsetY,
      origWidth: img.width,
      origHeight: img.height,
    };
  }

  // ============================================================
  // 序列化
  // ============================================================

  /** 从 JSON 加载图片数据 (覆盖现有数据) */
  loadFromJSON(images: unknown[]): void {
    this.clear();
    for (const item of images) {
      const obj = item as Record<string, unknown>;
      const type = obj.type as string | undefined;
      if (type === 'cell') {
        // 嵌入图片
        this.cellImages.set(obj.id as string, {
          id: obj.id as string,
          src: obj.src as string,
          row: obj.row as number,
          col: obj.col as number,
        });
      } else {
        // 默认为浮动图片
        this.floatImages.set(obj.id as string, {
          id: obj.id as string,
          src: obj.src as string,
          row: obj.row as number,
          col: obj.col as number,
          offsetX: (obj.offsetX as number) ?? 0,
          offsetY: (obj.offsetY as number) ?? 0,
          width: obj.width as number,
          height: obj.height as number,
          rotation: (obj.rotation as number) ?? 0,
        });
      }
    }
    this.renderAll();
  }

  /** 序列化为 JSON 数组 (存入 Sheet.images 字段) */
  toJSON(): unknown[] {
    const result: unknown[] = [];
    for (const img of this.floatImages.values()) {
      result.push({ type: 'float', ...img });
    }
    for (const img of this.cellImages.values()) {
      result.push({ type: 'cell', ...img });
    }
    return result;
  }

  // ============================================================
  // 刷新与销毁
  // ============================================================

  /** 清除所有图片 */
  clear(): void {
    this.floatImages.clear();
    this.cellImages.clear();
    this.floatImageEls.forEach(el => el.remove());
    this.cellImageEls.forEach(el => el.remove());
    this.floatImageEls.clear();
    this.cellImageEls.clear();
    this.deselect();
  }

  /** 重新渲染所有图片 (行列尺寸变化时调用) */
  renderAll(): void {
    // 清除旧 DOM
    this.floatImageEls.forEach(el => el.remove());
    this.cellImageEls.forEach(el => el.remove());
    this.floatImageEls.clear();
    this.cellImageEls.clear();
    this.deselect();

    for (const img of this.floatImages.values()) {
      this.renderFloatImage(img);
    }
    for (const img of this.cellImages.values()) {
      this.renderCellImage(img);
    }
  }

  /** 刷新所有图片位置 (行列尺寸变化时调用, 等同于 renderAll) */
  refresh(): void {
    this.renderAll();
  }

  /** 更新工作表引用 (切换工作表时调用) */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
    this.renderAll();
  }

  /** 销毁, 移除所有 DOM 和事件监听 */
  destroy(): void {
    this.clear();
    this.selectionOverlay.remove();
    this.layer.remove();
    if (this.globalListenersInstalled) {
      document.removeEventListener('mousemove', this.boundMouseMove);
      document.removeEventListener('mouseup', this.boundMouseUp);
      this.globalListenersInstalled = false;
    }
  }
}
