/**
 * Changeset 增量合并
 *
 * 职责:
 *   1. 按版本号顺序消费 changeset
 *   2. followFunc 外部注入式 OT 算法 (Operational Transformation)
 *   3. receiveChangesetsMap 批量接收
 *   4. fetchMiss 补漏机制 (发现版本号缺口时主动拉取)
 *
 * OT (Operational Transformation) 核心思想:
 *   - 当两个用户同时编辑同一区域时, 通过变换操作使其在对方视图上"等效"
 *   - follow(A, B) 返回 B' 使得 A ∘ B' = B ∘ A'
 *   - 这里采用简化实现: 由调用方注入 followFunc
 */

import { DecodedCommand } from './mutation-commands';

// ============================================================
// 类型定义
// ============================================================

/** Changeset (与 protocol-adapter 中的 NormalizedChangeset 一致) */
export interface Changeset {
  id: string;
  sheetId: string;
  /** 版本号 (单调递增) */
  version: number;
  /** 作者 ID */
  author: string;
  /** 时间戳 (毫秒) */
  timestamp: number;
  /** 命令列表 */
  commands: DecodedCommand[];
  /** 父版本 (基于哪个版本派生) */
  parentVersion?: number;
}

/** OT 变换函数类型 */
export type FollowFunc = (local: DecodedCommand[], remote: DecodedCommand[]) => DecodedCommand[];

/** Changeset 合并器回调 */
export interface ChangesetMergerCallbacks {
  /** 发现版本号缺口, 请求补漏 */
  onFetchMiss?: (fromVersion: number, toVersion: number) => Promise<Changeset[]>;
  /** 应用 changeset 到本地模型 */
  onApplyChangeset?: (cs: Changeset) => void;
  /** 状态变化 */
  onVersionUpdate?: (currentVersion: number) => void;
}

// ============================================================
// ChangesetMerger
// ============================================================

export class ChangesetMerger {
  private callbacks: ChangesetMergerCallbacks;
  private followFunc: FollowFunc | null = null;

  /** 已应用的 changeset (按版本号索引) */
  private appliedChangesets = new Map<number, Changeset>();
  /** 待应用的 changeset (按版本号排序) */
  private pendingChangesets: Changeset[] = [];
  /** 当前已应用版本号 */
  private currentVersion = 0;
  /** 本地未提交的 commands */
  private localPendingCommands: DecodedCommand[] = [];

  constructor(callbacks: ChangesetMergerCallbacks = {}) {
    this.callbacks = callbacks;
  }

  // ============================================================
  // OT 函数注入
  // ============================================================

  /** 注入 OT 变换函数 */
  setFollowFunc(fn: FollowFunc): void {
    this.followFunc = fn;
  }

  // ============================================================
  // 接收 changeset
  // ============================================================

  /**
   * 批量接收 changeset
   *
   * @param changesets 远程 changeset 列表
   */
  async receiveChangesets(changesets: Changeset[]): Promise<void> {
    // 1. 加入待应用队列
    for (const cs of changesets) {
      this.pendingChangesets.push(cs);
    }

    // 2. 按版本号排序
    this.pendingChangesets.sort((a, b) => a.version - b.version);

    // 3. 处理队列
    await this.processPending();
  }

  /** 处理待应用队列 */
  private async processPending(): Promise<void> {
    while (this.pendingChangesets.length > 0) {
      const cs = this.pendingChangesets[0];

      // 检查版本号连续性
      if (cs.version > this.currentVersion + 1) {
        // 版本号缺口: 触发 fetchMiss
        await this.fetchMiss(this.currentVersion + 1, cs.version - 1);
        // fetchMiss 后重新检查
        continue;
      }

      // 版本号 <= currentVersion 的跳过 (已应用)
      if (cs.version <= this.currentVersion) {
        this.pendingChangesets.shift();
        continue;
      }

      // 应用 changeset
      this.pendingChangesets.shift();
      this.applyChangeset(cs);
    }
  }

  /** 应用单个 changeset */
  private applyChangeset(cs: Changeset): void {
    // 如果有本地未提交的 commands, 进行 OT 变换
    let commandsToApply = cs.commands;
    if (this.localPendingCommands.length > 0 && this.followFunc) {
      // OT 变换: 将远程命令针对本地未提交命令进行变换
      commandsToApply = this.followFunc(this.localPendingCommands, cs.commands);
      // 同时变换本地命令 (针对远程)
      this.localPendingCommands = this.followFunc(cs.commands, this.localPendingCommands);
    }

    // 应用变换后的命令
    this.callbacks.onApplyChangeset?.({
      ...cs,
      commands: commandsToApply,
    });

    // 更新版本号
    this.appliedChangesets.set(cs.version, cs);
    this.currentVersion = cs.version;
    this.callbacks.onVersionUpdate?.(this.currentVersion);
  }

  // ============================================================
  // 补漏机制
  // ============================================================

  /**
   * 发现版本号缺口时主动拉取
   */
  private async fetchMiss(fromVersion: number, toVersion: number): Promise<void> {
    if (!this.callbacks.onFetchMiss) {
      // 没有补漏回调, 跳过缺口 (可能丢数据)
      console.warn(`[ChangesetMerger] Version gap ${fromVersion}-${toVersion}, no fetchMiss callback`);
      this.currentVersion = toVersion;
      return;
    }

    try {
      const missed = await this.callbacks.onFetchMiss(fromVersion, toVersion);
      // 将补漏的 changeset 插入到队列头部 (按版本号排序)
      this.pendingChangesets.unshift(...missed);
      this.pendingChangesets.sort((a, b) => a.version - b.version);
    } catch (e) {
      console.error('[ChangesetMerger] fetchMiss failed:', e);
      // 失败时跳过缺口
      this.currentVersion = toVersion;
    }
  }

  // ============================================================
  // 本地命令管理
  // ============================================================

  /**
   * 添加本地未提交命令
   * @returns 临时 changeset ID (用于后续提交)
   */
  addLocalCommands(commands: DecodedCommand[], sheetId: string): string {
    this.localPendingCommands.push(...commands);
    const csId = `local_${Date.now()}_${Math.random().toString(36).slice(2, 8)}`;
    return csId;
  }

  /**
   * 本地命令已提交 (服务器确认)
   * @param csId addLocalCommands 返回的 ID
   * @param serverVersion 服务器分配的版本号
   */
  commitLocalCommands(csId: string, serverVersion: number): void {
    // 从本地待提交队列移除
    // (实际实现中需要根据 csId 关联到具体命令, 这里简化处理)
    this.localPendingCommands = [];

    // 创建本地 changeset 并标记为已应用
    const cs: Changeset = {
      id: csId,
      sheetId: '',
      version: serverVersion,
      author: 'local',
      timestamp: Date.now(),
      commands: [],
    };
    this.appliedChangesets.set(serverVersion, cs);
    this.currentVersion = Math.max(this.currentVersion, serverVersion);
    this.callbacks.onVersionUpdate?.(this.currentVersion);
  }

  /** 丢弃本地未提交命令 (回滚) */
  discardLocalCommands(): void {
    this.localPendingCommands = [];
  }

  // ============================================================
  // 状态查询
  // ============================================================

  /** 获取当前已应用版本号 */
  getCurrentVersion(): number {
    return this.currentVersion;
  }

  /** 获取本地未提交命令数 */
  getLocalPendingCount(): number {
    return this.localPendingCommands.length;
  }

  /** 获取待应用的 changeset 数 */
  getPendingCount(): number {
    return this.pendingChangesets.length;
  }

  /** 获取已应用的 changeset */
  getAppliedChangeset(version: number): Changeset | null {
    return this.appliedChangesets.get(version) ?? null;
  }

  /** 获取所有已应用版本号 */
  getAppliedVersions(): number[] {
    return Array.from(this.appliedChangesets.keys()).sort((a, b) => a - b);
  }

  /** 重置 (用于切换文档) */
  reset(): void {
    this.appliedChangesets.clear();
    this.pendingChangesets = [];
    this.localPendingCommands = [];
    this.currentVersion = 0;
  }
}

// ============================================================
// 内置 OT 变换函数 (简化版)
// ============================================================

/**
 * 简化版 OT 变换
 *   - 文本编辑: 位置偏移修正
 *   - 行列插入: 索引修正
 *   - 其他: 不变换 (直接通过)
 */
export function simpleFollowFunc(local: DecodedCommand[], remote: DecodedCommand[]): DecodedCommand[] {
  // 简化实现: 不做变换, 直接返回 remote (适用于无冲突场景)
  // 生产环境应实现完整的 OT 算法
  return remote.map(cmd => {
    const transformed = { ...cmd };

    // 如果本地有 INSERT_ROW 操作, 远程的 rowIndex 需要修正
    for (const lcmd of local) {
      if (lcmd.actionCode === 10 /* INSERT_ROW */ &&
          lcmd.rowIndex !== undefined &&
          cmd.rowIndex !== undefined &&
          lcmd.rowIndex <= cmd.rowIndex) {
        transformed.rowIndex = cmd.rowIndex + 1;
      }
      if (lcmd.actionCode === 14 /* INSERT_COLUMN */ &&
          lcmd.columnIndex !== undefined &&
          cmd.columnIndex !== undefined &&
          lcmd.columnIndex <= cmd.columnIndex) {
        transformed.columnIndex = cmd.columnIndex + 1;
      }
    }

    return transformed;
  });
}
