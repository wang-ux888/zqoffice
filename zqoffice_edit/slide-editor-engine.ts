/**
 * 演示文稿主引擎 — 协调所有模块
 *
 * 职责:
 *   1. 创建和初始化所有子模块 (model/render/edit/toolbar)
 *   2. 事件路由
 *   3. 文档加载/保存 (对接 IPC)
 *   4. 幻灯片切换/管理
 *   5. 幻灯片缩略图侧边栏
 *   6. 窗口级快捷键
 */

import {
  SlideDocumentModel, SlideModel, ZQSlideDocument,
  SlideSize, DEFAULT_SLIDE_SIZE, pixelToEmu,
} from './slide-model';
import { SlideRenderEngine, SlideRenderCallbacks } from './slide-render-engine';
import { SlideEditEngine, SlideEditCallbacks } from './slide-edit-engine';
import { SlideToolbar } from './slide-toolbar';

/** 主引擎配置 */
export interface SlideEditorConfig {
  /** 容器元素 */
  container: HTMLElement;
  /** 初始文档 JSON (可选) */
  initialData?: ZQSlideDocument;
  /** 文件路径 */
  filePath?: string;
  /** 文件名 */
  fileName?: string;
  /** 是否只读 */
  readOnly?: boolean;
  /** 保存回调 */
  onSave?: (json: string, filePath?: string) => Promise<void>;
  /** 内容变化回调 */
  onContentChange?: () => void;
  /** 标题更新回调 */
  onTitleChange?: (title: string) => void;
}

export class SlideEditorEngine {
  private config: SlideEditorConfig;
  private container: HTMLElement;

  // 子模块
  private docModel: SlideDocumentModel;
  private slide: SlideModel;
  private renderEngine: SlideRenderEngine;
  private editEngine: SlideEditEngine;
  private toolbar: SlideToolbar;

  // 状态
  private isDirty: boolean = false;

  // DOM 元素
  private toolbarContainer!: HTMLDivElement;
  private sidebarContainer!: HTMLDivElement;
  private slideContainer!: HTMLDivElement;
  private statusBar!: HTMLDivElement;

  constructor(config: SlideEditorConfig) {
    this.config = config;
    this.container = config.container;

    // 1. 创建数据模型
    this.docModel = new SlideDocumentModel(config.initialData);
    this.slide = this.docModel.getActiveSlide();

    // 2. 创建布局
    const layout = this.createLayout();
    this.container.appendChild(layout);

    // 3. 创建渲染引擎
    const renderCallbacks: SlideRenderCallbacks = {
      onShapeClick: (shapeId, e) => {
        // 选区由 renderEngine 内部处理
      },
      onShapeDoubleClick: (shapeId, e) => {
        this.editEngine.startEditText(shapeId);
      },
      onBackgroundClick: (e) => {
        // 取消选中
      },
      onSelectionChange: (selection) => {
        this.toolbar.updateButtonStates();
      },
      onContextMenu: (shapeId, e) => {
        this.showContextMenu(shapeId, e);
      },
    };

    this.renderEngine = new SlideRenderEngine(
      this.slideContainer,
      this.slide,
      this.docModel.getSlideSize(),
      renderCallbacks,
    );

    // 4. 创建编辑引擎
    const editCallbacks: SlideEditCallbacks = {
      onContentChange: () => this.markDirty(),
      onSelectionChange: (shapeId) => {
        this.toolbar.updateButtonStates();
      },
      onUndoRedo: (canUndo, canRedo) => {
        this.toolbar.updateButtonStates();
      },
    };

    this.editEngine = new SlideEditEngine(
      this.slideContainer,
      this.slide,
      this.renderEngine,
      editCallbacks,
    );

    // 5. 创建工具栏
    this.toolbar = new SlideToolbar(
      this.toolbarContainer,
      this.slide,
      this.renderEngine,
      this.editEngine,
    );

    // 6. 渲染缩略图侧边栏
    this.renderSidebar();

    // 7. 绑定全局事件
    this.bindGlobalEvents();

    // 8. 设置只读模式
    if (config.readOnly) {
      this.setReadOnly(true);
    }

    // 9. 设置标题
    if (config.fileName) {
      this.config.onTitleChange?.(config.fileName);
    }
  }

  // ============================================================
  // 布局创建
  // ============================================================

  /**
   * 创建编辑器布局
   * 结构:
   *   ┌──────────────────────────────┐
   *   │         工具栏                │
   *   ├────────┬─────────────────────┤
   *   │        │                     │
   *   │  缩略图 │     幻灯片编辑区     │
   *   │  侧边栏 │                     │
   *   │        │                     │
   *   ├────────┴─────────────────────┤
   *   │  状态栏: 幻灯片 1/5 | 就绪    │
   *   └──────────────────────────────┘
   */
  private createLayout(): HTMLElement {
    this.setupLayoutStyles();

    const wrapper = document.createElement('div');
    wrapper.className = 'zq-slide-editor-wrapper';

    // 工具栏
    this.toolbarContainer = document.createElement('div');
    this.toolbarContainer.className = 'zq-slide-editor-toolbar';
    wrapper.appendChild(this.toolbarContainer);

    // 主体区域 (侧边栏 + 编辑区)
    const body = document.createElement('div');
    body.className = 'zq-slide-editor-body';

    // 缩略图侧边栏
    this.sidebarContainer = document.createElement('div');
    this.sidebarContainer.className = 'zq-slide-editor-sidebar';
    body.appendChild(this.sidebarContainer);

    // 幻灯片编辑区
    this.slideContainer = document.createElement('div');
    this.slideContainer.className = 'zq-slide-editor-canvas';
    this.slideContainer.tabIndex = 0;
    body.appendChild(this.slideContainer);

    wrapper.appendChild(body);

    // 状态栏
    this.statusBar = document.createElement('div');
    this.statusBar.className = 'zq-slide-editor-status';
    this.statusBar.textContent = '就绪';
    wrapper.appendChild(this.statusBar);

    return wrapper;
  }

  private setupLayoutStyles(): void {
    if (document.getElementById('zq-slide-editor-layout-styles')) return;
    const style = document.createElement('style');
    style.id = 'zq-slide-editor-layout-styles';
    style.textContent = `
      .zq-slide-editor-wrapper {
        display: flex;
        flex-direction: column;
        width: 100%;
        height: 100%;
        background: #fff;
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
      }
      .zq-slide-editor-toolbar {
        flex-shrink: 0;
      }
      .zq-slide-editor-body {
        display: flex;
        flex: 1;
        overflow: hidden;
      }
      .zq-slide-editor-sidebar {
        width: 180px;
        flex-shrink: 0;
        background: #f5f5f5;
        border-right: 1px solid #e0e0e0;
        overflow-y: auto;
        padding: 8px;
        display: flex;
        flex-direction: column;
        gap: 8px;
        align-items: center;
      }
      .zq-slide-editor-canvas {
        flex: 1;
        position: relative;
        overflow: hidden;
        outline: none;
        background: #e8e8e8;
      }
      .zq-slide-editor-status {
        padding: 2px 12px;
        border-top: 1px solid #e0e0e0;
        background: #fafafa;
        font-size: 12px;
        color: #666;
        flex-shrink: 0;
      }
      .zq-slide-sidebar-item {
        display: flex;
        flex-direction: column;
        align-items: center;
        gap: 4px;
      }
      .zq-slide-sidebar-number {
        font-size: 12px;
        color: #666;
      }
      .zq-slide-sidebar-add {
        width: 160px;
        height: 28px;
        border: 1px dashed #b0b0b0;
        border-radius: 3px;
        background: #fff;
        cursor: pointer;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 18px;
        color: #666;
      }
      .zq-slide-sidebar-add:hover {
        background: #e8e8e8;
        border-color: #4787f5;
        color: #4787f5;
      }
    `;
    document.head.appendChild(style);
  }

  // ============================================================
  // 侧边栏 (缩略图)
  // ============================================================

  private renderSidebar(): void {
    this.sidebarContainer.innerHTML = '';

    const slides = this.docModel.getAllSlides();
    const activeIndex = this.docModel.getActiveIndex();
    const slideSize = this.docModel.getSlideSize();

    for (let i = 0; i < slides.length; i++) {
      const item = document.createElement('div');
      item.className = 'zq-slide-sidebar-item';

      // 缩略图
      const thumb = new SlideRenderEngine(
        document.createElement('div'),
        slides[i],
        slideSize,
        {},
      );
      const thumbEl = thumb.renderThumbnail(160);
      thumbEl.style.border = i === activeIndex ? '2px solid #4787f5' : '2px solid transparent';
      thumbEl.addEventListener('click', () => this.switchToSlide(i));
      thumb.destroy();

      // 编号
      const num = document.createElement('div');
      num.className = 'zq-slide-sidebar-number';
      num.textContent = `${i + 1}`;

      item.appendChild(thumbEl);
      item.appendChild(num);
      this.sidebarContainer.appendChild(item);
    }

    // 添加按钮
    const addBtn = document.createElement('div');
    addBtn.className = 'zq-slide-sidebar-add';
    addBtn.textContent = '+';
    addBtn.title = '新建幻灯片';
    addBtn.addEventListener('click', () => this.addSlide());
    this.sidebarContainer.appendChild(addBtn);
  }

  // ============================================================
  // 幻灯片管理
  // ============================================================

  /** 切换到指定幻灯片 */
  switchToSlide(index: number): void {
    if (index < 0 || index >= this.docModel.getSlideCount()) return;
    this.docModel.setActiveIndex(index);
    this.slide = this.docModel.getActiveSlide();
    this.renderEngine.setSlide(this.slide);
    this.editEngine.setSlide(this.slide);
    this.toolbar.setSlide(this.slide);
    this.renderSidebar();
    this.updateStatus();
  }

  /** 添加幻灯片 */
  addSlide(): void {
    this.docModel.addSlide();
    this.slide = this.docModel.getActiveSlide();
    this.renderEngine.setSlide(this.slide);
    this.editEngine.setSlide(this.slide);
    this.toolbar.setSlide(this.slide);
    this.renderSidebar();
    this.markDirty();
    this.updateStatus();
  }

  /** 复制当前幻灯片 */
  duplicateSlide(): void {
    this.docModel.duplicateSlide(this.docModel.getActiveIndex());
    this.slide = this.docModel.getActiveSlide();
    this.renderEngine.setSlide(this.slide);
    this.editEngine.setSlide(this.slide);
    this.toolbar.setSlide(this.slide);
    this.renderSidebar();
    this.markDirty();
    this.updateStatus();
  }

  /** 删除当前幻灯片 */
  deleteSlide(): void {
    if (this.docModel.getSlideCount() <= 1) return;
    this.docModel.removeSlide(this.docModel.getActiveIndex());
    this.slide = this.docModel.getActiveSlide();
    this.renderEngine.setSlide(this.slide);
    this.editEngine.setSlide(this.slide);
    this.toolbar.setSlide(this.slide);
    this.renderSidebar();
    this.markDirty();
    this.updateStatus();
  }

  /** 上一张 */
  prevSlide(): void {
    const idx = this.docModel.getActiveIndex();
    if (idx > 0) this.switchToSlide(idx - 1);
  }

  /** 下一张 */
  nextSlide(): void {
    const idx = this.docModel.getActiveIndex();
    if (idx < this.docModel.getSlideCount() - 1) this.switchToSlide(idx + 1);
  }

  // ============================================================
  // 全局事件
  // ============================================================

  private bindGlobalEvents(): void {
    this.slideContainer.addEventListener('keydown', (e) => {
      // PageUp/PageDown 切换幻灯片
      if (e.key === 'PageUp') {
        e.preventDefault();
        this.prevSlide();
        return;
      }
      if (e.key === 'PageDown') {
        e.preventDefault();
        this.nextSlide();
        return;
      }

      // Ctrl+S 保存
      if ((e.ctrlKey || e.metaKey) && e.key === 's') {
        e.preventDefault();
        this.save();
        return;
      }

      // Ctrl+M 新建幻灯片
      if ((e.ctrlKey || e.metaKey) && e.key === 'm') {
        e.preventDefault();
        this.addSlide();
        return;
      }
    });
  }

  // ============================================================
  // 保存
  // ============================================================

  /** 保存文档 */
  async save(): Promise<void> {
    if (!this.config.onSave) return;
    const json = this.docModel.toJSONString();
    await this.config.onSave(json, this.config.filePath);
    this.isDirty = false;
    this.updateStatus();
  }

  /** 获取文档 JSON */
  getDocumentJSON(): ZQSlideDocument {
    return this.docModel.toJSON();
  }

  /** 加载文档 */
  loadDocument(data: ZQSlideDocument): void {
    this.docModel = new SlideDocumentModel(data);
    this.slide = this.docModel.getActiveSlide();
    this.renderEngine.setSlideSize(this.docModel.getSlideSize());
    this.renderEngine.setSlide(this.slide);
    this.editEngine.setSlide(this.slide);
    this.toolbar.setSlide(this.slide);
    this.renderSidebar();
    this.editEngine.clearHistory();
    this.isDirty = false;
    this.updateStatus();
  }

  // ============================================================
  // 状态
  // ============================================================

  private markDirty(): void {
    this.isDirty = true;
    this.config.onContentChange?.();
    this.updateStatus();
  }

  private updateStatus(): void {
    const idx = this.docModel.getActiveIndex() + 1;
    const total = this.docModel.getSlideCount();
    const dirty = this.isDirty ? ' *' : '';
    this.statusBar.textContent = `幻灯片 ${idx}/${total}${dirty} | 就绪`;
  }

  /** 设置只读模式 */
  setReadOnly(readOnly: boolean): void {
    this.slideContainer.style.pointerEvents = readOnly ? 'none' : 'auto';
  }

  // ============================================================
  // 右键菜单
  // ============================================================

  private showContextMenu(shapeId: string | null, e: MouseEvent): void {
    // 简化实现: 可扩展为完整右键菜单
    console.log('context menu', shapeId, e);
  }

  // ============================================================
  // 公开 API
  // ============================================================

  /** 获取文档模型 */
  getDocumentModel(): SlideDocumentModel {
    return this.docModel;
  }

  /** 获取当前幻灯片 */
  getCurrentSlide(): SlideModel {
    return this.slide;
  }

  /** 销毁 */
  destroy(): void {
    this.editEngine.destroy();
    this.renderEngine.destroy();
    this.container.innerHTML = '';
  }
}
