/**
 * sheet_io 集成模块 — 协议适配 + 分块加载 + Mutation + Changeset
 *
 * SheetIO 职责:
 *   1. 加载文档 (从 JSON/文件 → 内部数据模型)
 *   2. 应用 mutation 命令 (增量更新)
 *   3. 消费 changeset (协同合并)
 *   4. 暴露统一 API 给上层 (SheetEngine)
 */

import { DocumentModel, SheetModel, ZQSheetDocument, Sheet } from './data-model';
import {
  ProtocolAdapter,
  ProtocolVersion,
  NormalizedProtocolData,
  NormalizedSheet,
  NormalizedChangeset,
} from './protocol-adapter';
import {
  ChunkLoader,
  ChunkLoaderCallbacks,
  LoadTask,
} from './chunk-loader';
import {
  MutationCommands,
  DecodedCommand,
  ActionCode,
} from './mutation-commands';
import {
  ChangesetMerger,
  Changeset,
  ChangesetMergerCallbacks,
  FollowFunc,
  simpleFollowFunc,
} from './changeset-merger';

// ============================================================
// SheetIO 配置
// ============================================================

export interface SheetIOConfig {
  /** 默认协议版本 (用于序列化) */
  defaultProtocolVersion?: ProtocolVersion;
  /** 是否启用 onDemand 加载 (只加载 high 优先级) */
  onDemandOnly?: boolean;
  /** OT 变换函数 (用于 changeset 合并) */
  followFunc?: FollowFunc;
  /** 远程数据请求回调 (用于分块加载) */
  onRequestData?: (task: LoadTask) => Promise<unknown>;
  /** 远程 changeset 拉取回调 (用于补漏) */
  onFetchMiss?: (fromVersion: number, toVersion: number) => Promise<Changeset[]>;
  /** changeset 应用回调 */
  onApplyChangeset?: (cs: Changeset) => void;
}

// ============================================================
// SheetIO 集成类
// ============================================================

export class SheetIO {
  private config: SheetIOConfig;
  private protocolAdapter: ProtocolAdapter;
  private chunkLoader: ChunkLoader;
  private mutationCommands: MutationCommands;
  private changesetMerger: ChangesetMerger;

  // 当前文档
  private document: DocumentModel;
  // 当前协议数据
  private protocolData: NormalizedProtocolData | null = null;
  // 加载状态
  private isLoading = false;

  constructor(config: SheetIOConfig = {}) {
    this.config = {
      defaultProtocolVersion: 'v3',
      onDemandOnly: false,
      ...config,
    };

    this.protocolAdapter = new ProtocolAdapter();
    this.mutationCommands = new MutationCommands();
    this.document = new DocumentModel();

    // 配置 ChunkLoader
    const chunkCallbacks: ChunkLoaderCallbacks = {
      onRequestData: this.config.onRequestData,
      onTaskApplied: (task) => {
        // 任务应用完成, 更新文档
        this.applyTaskToDocument(task);
      },
      onAllComplete: () => {
        this.isLoading = false;
      },
      onProgress: (completed, total) => {
        // 进度回调 (可选, 上层可订阅)
      },
    };
    this.chunkLoader = new ChunkLoader(chunkCallbacks);
    this.chunkLoader.setOnDemandOnly(this.config.onDemandOnly ?? false);

    // 配置 ChangesetMerger
    const mergerCallbacks: ChangesetMergerCallbacks = {
      onFetchMiss: this.config.onFetchMiss,
      onApplyChangeset: (cs) => {
        this.handleApplyChangeset(cs);
        this.config.onApplyChangeset?.(cs);
      },
      onVersionUpdate: (version) => {
        // 版本号更新
      },
    };
    this.changesetMerger = new ChangesetMerger(mergerCallbacks);
    if (this.config.followFunc) {
      this.changesetMerger.setFollowFunc(this.config.followFunc);
    } else {
      this.changesetMerger.setFollowFunc(simpleFollowFunc);
    }
  }

  // ============================================================
  // 文档加载
  // ============================================================

  /**
   * 从原始协议数据加载文档
   * @param raw V2 或 V3 协议数据
   */
  async loadFromProtocol(raw: unknown): Promise<DocumentModel> {
    this.isLoading = true;

    // 1. 协议适配 (V2/V3 → 内部格式)
    this.protocolData = this.protocolAdapter.normalize(raw);

    // 2. 创建文档模型 (先填充元数据, 数据稍后分块加载)
    this.document = new DocumentModel();
    // 清除默认创建的 Sheet1 (loadFromProtocol 会从协议数据中添加 sheet)
    this.document.clearSheets();
    for (const sheet of this.protocolData.sheets) {
      const sheetModel = this.createSheetFromNormalized(sheet);
      this.document.addSheet(sheetModel);
    }

    // 3. 启动分块加载
    this.chunkLoader.clear();
    for (const sheet of this.protocolData.sheets) {
      // 元数据 + 合并 (高优先级)
      this.chunkLoader.addSheetMetaTask(sheet.sheetId);
      this.chunkLoader.addMergedCellsTask(sheet.sheetId);
      // 行数据 (分块, 头部高优先级)
      this.chunkLoader.addRowDataTasks(sheet.sheetId, sheet.rowCount);
      // 样式/列宽/行高 (低优先级)
      this.chunkLoader.addStyleTasks(sheet.sheetId);
    }

    // 4. 等待高优先级任务完成 (onDemand 模式)
    if (this.config.onDemandOnly) {
      await this.chunkLoader.start();
    } else {
      // 非阻塞加载
      void this.chunkLoader.start();
    }

    // 5. 应用初始 changeset (如果有)
    if (this.protocolData.changesets.length > 0) {
      const changesets = this.protocolData.changesets.map(c => this.toChangeset(c));
      await this.changesetMerger.receiveChangesets(changesets);
    }

    // 6. 应用初始 pending mutations (如果有)
    if (this.protocolData.pendingMutations.length > 0) {
      const commands = this.mutationCommands.decodeCommands(this.protocolData.pendingMutations);
      const firstSheet = this.document.getAllSheets()[0];
      if (firstSheet) {
        this.mutationCommands.applyCommands(firstSheet, commands);
      }
    }

    return this.document;
  }

  /**
   * 从 ZQ Sheet JSON 加载 (直接, 无协议适配)
   * @param data ZQ Sheet JSON
   */
  loadFromZQJson(data: ZQSheetDocument): DocumentModel {
    this.document = new DocumentModel(data);
    return this.document;
  }

  /** 创建工作表 (从归一化数据, 仅元数据) */
  private createSheetFromNormalized(sheet: NormalizedSheet): SheetModel {
    const sheetData: Partial<Sheet> = {
      sheetId: sheet.sheetId,
      name: sheet.name,
      index: sheet.index,
      rowCount: sheet.rowCount,
      columnCount: sheet.columnCount,
      defaultRowHeight: sheet.defaultRowHeight,
      defaultColumnWidth: sheet.defaultColumnWidth,
      frozenRowCount: sheet.frozenRowCount,
      frozenColumnCount: sheet.frozenColumnCount,
      hidden: sheet.hidden,
      tabColor: sheet.tabColor ?? undefined,
      cells: [],  // 稍后分块加载
      mergedCells: sheet.mergedCells.map(m => ({
        startRow: m.startRow,
        startColumn: m.startColumn,
        endRow: m.endRow,
        endColumn: m.endColumn,
      })),
      columnWidths: sheet.columnWidths.map(cw => ({
        column: cw.column,
        width: cw.width,
      })),
      rowHeights: sheet.rowHeights.map(rh => ({
        row: rh.row,
        height: rh.height,
      })),
      images: [],
      charts: [],
    };
    return new SheetModel(sheetData);
  }

  /** 应用任务数据到文档 */
  private applyTaskToDocument(task: LoadTask): void {
    if (!task.data) return;
    const sheet = this.document.getSheetById(task.sheetId);
    if (!sheet) return;

    switch (task.type) {
      case 'sheet_meta':
        // 元数据已在创建时填充, 这里可以更新
        break;
      case 'row_data':
        // 行数据: task.data 应该是 Cell[]
        if (Array.isArray(task.data)) {
          for (const cell of task.data) {
            const c = cell as { row: number; column: number; type: string; value: string | number | boolean; formula?: string };
            if (c.formula) {
              sheet.setCellFormula(c.row, c.column, c.formula);
            } else {
              sheet.setCellValue(c.row, c.column, c.value);
            }
          }
        }
        break;
      case 'merged_cells':
        // 合并区域已在创建时填充
        break;
      case 'cell_style':
        // 样式数据
        break;
      case 'column_widths':
        // 列宽已在创建时填充
        break;
      case 'row_heights':
        // 行高已在创建时填充
        break;
    }
  }

  // ============================================================
  // Mutation 命令应用
  // ============================================================

  /**
   * 应用 mutation 命令到指定工作表
   * @param sheetId 工作表 ID
   * @param rawCommands 原始命令数据
   * @param offsetRange 坐标偏移
   */
  applyMutations(sheetId: string, rawCommands: unknown, offsetRange?: { rowOffset?: number; colOffset?: number }): void {
    const sheet = this.document.getSheetById(sheetId);
    if (!sheet) return;
    const commands = this.mutationCommands.decodeCommands(rawCommands);
    this.mutationCommands.applyCommands(sheet, commands, offsetRange);
  }

  // ============================================================
  // Changeset 消费
  // ============================================================

  /**
   * 接收远程 changeset
   * @param changesets changeset 列表
   */
  async receiveChangesets(changesets: Changeset[]): Promise<void> {
    await this.changesetMerger.receiveChangesets(changesets);
  }

  /** 处理 changeset 应用 */
  private handleApplyChangeset(cs: Changeset): void {
    const sheet = this.document.getSheetById(cs.sheetId);
    if (!sheet) return;
    this.mutationCommands.applyCommands(sheet, cs.commands);
  }

  // ============================================================
  // 本地命令管理
  // ============================================================

  /** 添加本地未提交命令 */
  addLocalCommands(sheetId: string, commands: DecodedCommand[]): string {
    return this.changesetMerger.addLocalCommands(commands, sheetId);
  }

  /** 本地命令已提交 (服务器确认) */
  commitLocalCommands(csId: string, serverVersion: number): void {
    this.changesetMerger.commitLocalCommands(csId, serverVersion);
  }

  /** 丢弃本地未提交命令 */
  discardLocalCommands(): void {
    this.changesetMerger.discardLocalCommands();
  }

  // ============================================================
  // 序列化 (用于保存)
  // ============================================================

  /** 序列化为指定协议 */
  serialize(version?: ProtocolVersion): unknown {
    const targetVersion = version ?? this.config.defaultProtocolVersion ?? 'v3';

    // 从 DocumentModel 反向构建 NormalizedProtocolData
    const sheets = this.document.getAllSheets().map(s => this.sheetToNormalized(s));
    const data: NormalizedProtocolData = {
      version: targetVersion,
      schemaVersion: targetVersion === 'v3' ? 3 : 2,
      sheets,
      changesets: [],  // 已应用的 changeset 不再输出
      pendingMutations: [],  // 本地未提交命令 (如果有)
    };

    return this.protocolAdapter.serialize(data, targetVersion);
  }

  /** SheetModel → NormalizedSheet */
  private sheetToNormalized(sheet: SheetModel): NormalizedSheet {
    const cells = sheet.getAllCells().map(c => ({
      row: c.row,
      column: c.column,
      type: c.type,
      value: c.value,
      formula: c.formula,
      style: c.style as Record<string, unknown> | undefined,
    }));
    const merges = sheet.getAllMerges().map(m => ({
      startRow: m.startRow,
      startColumn: m.startColumn,
      endRow: m.endRow,
      endColumn: m.endColumn,
    }));

    return {
      sheetId: sheet.sheetId,
      name: sheet.name,
      index: sheet.index,
      rowCount: sheet.rowCount,
      columnCount: sheet.columnCount,
      defaultRowHeight: sheet.defaultRowHeight,
      defaultColumnWidth: sheet.defaultColumnWidth,
      frozenRowCount: sheet.frozenRowCount,
      frozenColumnCount: sheet.frozenColumnCount,
      cells,
      mergedCells: merges,
      columnWidths: [],  // 从 SheetModel 获取列宽需要额外方法
      rowHeights: [],
      hidden: sheet.hidden,
      tabColor: sheet.tabColor,
    };
  }

  // ============================================================
  // 工具方法
  // ============================================================

  /** 获取当前文档 */
  getDocument(): DocumentModel {
    return this.document;
  }

  /** 获取当前协议版本 */
  getProtocolVersion(): ProtocolVersion | null {
    return this.protocolData?.version ?? null;
  }

  /** 获取当前版本号 */
  getCurrentVersion(): number {
    return this.changesetMerger.getCurrentVersion();
  }

  /** 获取加载进度 (0-1) */
  getLoadProgress(): number {
    return this.chunkLoader.getProgress();
  }

  /** 是否正在加载 */
  isLoadingState(): boolean {
    return this.isLoading;
  }

  /** NormalizedChangeset → Changeset */
  private toChangeset(nc: NormalizedChangeset): Changeset {
    return {
      id: nc.id,
      sheetId: nc.sheetId,
      version: nc.version,
      author: nc.author,
      timestamp: nc.timestamp,
      commands: this.mutationCommands.decodeCommands(nc.mutations),
    };
  }

  // ============================================================
  // 重置
  // ============================================================

  /** 重置 (切换文档时调用) */
  reset(): void {
    this.document = new DocumentModel();
    this.protocolData = null;
    this.chunkLoader.clear();
    this.changesetMerger.reset();
    this.isLoading = false;
  }
}

// ============================================================
// 便捷工厂函数
// ============================================================

/** 创建默认配置的 SheetIO 实例 */
export function createSheetIO(config: SheetIOConfig = {}): SheetIO {
  return new SheetIO({
    defaultProtocolVersion: 'v3',
    onDemandOnly: false,
    ...config,
  });
}
