/**
 * Word 文档编辑器主引擎 — 协调所有模块
 *
 * 职责:
 *   1. 创建布局 (工具栏 + 编辑区 + 状态栏)
 *   2. 协调 DocModel/RenderEngine/EditEngine/Toolbar
 *   3. 文件保存/加载
 *   4. 快捷键
 */

import { DocModel, ZQDocDocument, CoreProperties } from './doc-model';
import { DocRenderEngine } from './doc-render-engine';
import { DocEditEngine } from './doc-edit-engine';
import { DocToolbar } from './doc-toolbar';

/** 主引擎配置 */
export interface DocEditorConfig {
  /** 容器元素 */
  container: HTMLElement;
  /** 初始文档 JSON (可选) */
  initialData?: ZQDocDocument;
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

export class DocEditorEngine {
  private config: DocEditorConfig;
  private container: HTMLElement;

  // 子模块
  private docModel: DocModel;
  private renderEngine: DocRenderEngine;
  private editEngine: DocEditEngine;
  private toolbar: DocToolbar;

  // 状态
  private isDirty: boolean = false;
  private isReadOnly: boolean = false;

  // DOM 元素
  private toolbarContainer!: HTMLDivElement;
  private editorContainer!: HTMLDivElement;
  private statusBar!: HTMLDivElement;

  constructor(config: DocEditorConfig) {
    this.config = config;
    this.container = config.container;
    this.isReadOnly = config.readOnly ?? false;

    // 1. 创建数据模型
    this.docModel = new DocModel(config.initialData);

    // 2. 创建布局
    this.createLayout();

    // 3. 创建渲染引擎
    this.renderEngine = new DocRenderEngine(
      this.editorContainer,
      this.docModel,
      {
        onBlockInput: (blockId, text) => this.handleBlockInput(blockId, text),
        onSelectionChange: (blockId) => this.handleSelectionChange(blockId),
      },
    );

    // 4. 创建编辑引擎
    this.editEngine = new DocEditEngine(this.docModel, this.renderEngine, {
      onContentChange: () => this.markDirty(),
      onSelectionChange: (blockId) => this.handleSelectionChange(blockId),
    });

    // 5. 创建工具栏
    this.toolbar = new DocToolbar(
      this.toolbarContainer,
      this.docModel,
      this.renderEngine,
      this.editEngine,
      { onContentChange: () => this.markDirty() },
    );

    // 6. 应用只读模式
    if (this.isReadOnly) this.applyReadOnly();

    // 7. 设置标题
    if (config.fileName) {
      this.config.onTitleChange?.(config.fileName);
    }

    // 8. 注册快捷键
    this.attachShortcuts();

    // 9. 设置核心属性
    if (config.fileName) {
      this.docModel.setCoreProperties({ title: config.fileName });
    }

    this.updateStatus();
  }

  // ============================================================
  // 布局
  // ============================================================

  private createLayout(): void {
    this.container.innerHTML = '';
    this.container.style.cssText = `
      display: flex;
      flex-direction: column;
      width: 100%;
      height: 100%;
      background: #f5f5f7;
      overflow: hidden;
    `;

    // 工具栏容器
    this.toolbarContainer = document.createElement('div');
    this.toolbarContainer.className = 'zq-doc-toolbar-container';
    this.container.appendChild(this.toolbarContainer);

    // 编辑区
    this.editorContainer = document.createElement('div');
    this.editorContainer.className = 'zq-doc-editor-container';
    this.editorContainer.style.cssText = `
      flex: 1;
      overflow: auto;
      background: #e0e0e4;
    `;
    this.container.appendChild(this.editorContainer);

    // 状态栏
    this.statusBar = document.createElement('div');
    this.statusBar.className = 'zq-doc-status-bar';
    this.statusBar.style.cssText = `
      padding: 4px 12px;
      background: #fafafa;
      border-top: 1px solid #e0e0e0;
      font-size: 12px;
      color: #666;
      display: flex;
      justify-content: space-between;
    `;
    this.container.appendChild(this.statusBar);
  }

  // ============================================================
  // 事件处理
  // ============================================================

  private handleBlockInput(blockId: string, text: string): void {
    const block = this.docModel.getBlock(blockId);
    if (!block) return;
    if (block.paragraph) {
      this.docModel.setParagraphText(blockId, text);
    } else if (block.heading) {
      this.docModel.setHeadingText(blockId, text);
    }
    this.markDirty();
  }

  private handleSelectionChange(blockId: string | null): void {
    this.updateStatus();
  }

  private markDirty(): void {
    this.isDirty = true;
    this.config.onContentChange?.();
    this.updateStatus();
  }

  private updateStatus(): void {
    const blockId = this.renderEngine.getSelection();
    const blocks = this.docModel.getBlocks();
    const blockIdx = blockId ? blocks.findIndex(b => b.id === blockId) : -1;
    const total = blocks.length;
    const dirtyMark = this.isDirty ? '●' : '';
    const posText = blockIdx >= 0 ? `块 ${blockIdx + 1} / ${total}` : `${total} 块`;
    this.statusBar.textContent = `${dirtyMark} ${posText}`;
  }

  // ============================================================
  // 快捷键
  // ============================================================

  private attachShortcuts(): void {
    document.addEventListener('keydown', (e) => {
      // Ctrl+S 保存
      if ((e.ctrlKey || e.metaKey) && e.key === 's') {
        e.preventDefault();
        this.save();
        return;
      }
      // Ctrl+B 加粗
      if ((e.ctrlKey || e.metaKey) && e.key === 'b') {
        e.preventDefault();
        const blockId = this.editEngine.getSelectedBlockId();
        if (blockId) this.editEngine.toggleBold(blockId);
        return;
      }
      // Ctrl+I 斜体
      if ((e.ctrlKey || e.metaKey) && e.key === 'i') {
        e.preventDefault();
        const blockId = this.editEngine.getSelectedBlockId();
        if (blockId) this.editEngine.toggleItalic(blockId);
        return;
      }
      // Ctrl+U 下划线
      if ((e.ctrlKey || e.metaKey) && e.key === 'u') {
        e.preventDefault();
        const blockId = this.editEngine.getSelectedBlockId();
        if (blockId) this.editEngine.toggleUnderline(blockId);
        return;
      }
    });
  }

  // ============================================================
  // 只读模式
  // ============================================================

  private applyReadOnly(): void {
    const blocks = this.editorContainer.querySelectorAll('[contenteditable]');
    blocks.forEach(el => el.setAttribute('contenteditable', 'false'));
  }

  setReadOnly(readOnly: boolean): void {
    this.isReadOnly = readOnly;
    if (readOnly) {
      this.applyReadOnly();
    } else {
      const blocks = this.editorContainer.querySelectorAll('[contenteditable]');
      blocks.forEach(el => el.setAttribute('contenteditable', 'true'));
    }
  }

  // ============================================================
  // 保存/加载
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
  getDocumentJSON(): ZQDocDocument {
    return this.docModel.toJSON();
  }

  /** 加载文档 */
  loadDocument(data: ZQDocDocument): void {
    this.docModel = new DocModel(data);
    this.renderEngine.setModel(this.docModel);
    this.editEngine.setModel(this.docModel);
    this.toolbar.setModel(this.docModel);
    this.isDirty = false;
    this.updateStatus();
  }

  /** 设置核心属性 */
  setCoreProperties(props: Partial<CoreProperties>): void {
    this.docModel.setCoreProperties(props);
    this.markDirty();
  }

  /** 获取核心属性 */
  getCoreProperties(): CoreProperties {
    return this.docModel.getCoreProperties();
  }

  /** 是否有未保存修改 */
  isDirtyState(): boolean {
    return this.isDirty;
  }

  // ============================================================
  // 销毁
  // ============================================================

  destroy(): void {
    this.renderEngine.destroy();
    this.toolbar.destroy();
    this.container.innerHTML = '';
  }
}
