/**
 * 分块懒加载调度器
 *
 * 职责:
 *   1. 优先级队列管理加载任务 (High/Normal/Low)
 *   2. onDemand 过滤 (只取 High 优先级任务)
 *   3. appliedRowCountStatus 计数器追踪完成
 *   4. fiberTaskScheduler 分片应用 (避免阻塞主线程)
 *
 * 加载任务类型:
 *   - sheet_meta: 工作表元数据 (优先级 High)
 *   - row_data: 行数据 (按需加载, 可分块)
 *   - cell_style: 单元格样式 (优先级 Normal)
 *   - merged_cells: 合并区域 (优先级 High)
 *   - column_widths: 列宽 (优先级 Low)
 *   - row_heights: 行高 (优先级 Low)
 */

// ============================================================
// 类型定义
// ============================================================

export type LoadPriority = 'high' | 'normal' | 'low';

export type LoadTaskType =
  | 'sheet_meta'
  | 'row_data'
  | 'cell_style'
  | 'merged_cells'
  | 'column_widths'
  | 'row_heights';

/** 加载任务 */
export interface LoadTask {
  id: string;
  type: LoadTaskType;
  sheetId: string;
  priority: LoadPriority;
  /** 行范围 (row_data 类型用) */
  rowRange?: { start: number; end: number };
  /** 列范围 (row_data 类型用) */
  colRange?: { start: number; end: number };
  /** 任务数据 (加载完成后填充) */
  data?: unknown;
  /** 是否已完成 */
  done: boolean;
  /** 时间戳 */
  createdAt: number;
}

/** 加载回调 */
export interface ChunkLoaderCallbacks {
  /** 请求加载任务数据 (从远程/文件) */
  onRequestData?: (task: LoadTask) => Promise<unknown>;
  /** 任务应用完成 */
  onTaskApplied?: (task: LoadTask) => void;
  /** 全部任务完成 */
  onAllComplete?: () => void;
  /** 进度更新 */
  onProgress?: (completed: number, total: number) => void;
}

// ============================================================
// 分块加载调度器
// ============================================================

export class ChunkLoader {
  private callbacks: ChunkLoaderCallbacks;
  private tasks: LoadTask[] = [];
  private appliedCount = 0;
  private isRunning = false;
  private onDemandOnly = false;

  // 分片参数
  private readonly CHUNK_SIZE = 100;        // 每块 100 行
  private readonly TIME_SLICE_MS = 16;       // 每个时间片 16ms (60fps)
  private readonly MAX_TASKS_PER_SLICE = 5;  // 每个时间片最多处理 5 个任务

  constructor(callbacks: ChunkLoaderCallbacks = {}) {
    this.callbacks = callbacks;
  }

  // ============================================================
  // 任务管理
  // ============================================================

  /** 添加任务 */
  addTask(task: Omit<LoadTask, 'id' | 'done' | 'createdAt'>): string {
    const id = `task_${Date.now()}_${Math.random().toString(36).slice(2, 8)}`;
    this.tasks.push({
      ...task,
      id,
      done: false,
      createdAt: Date.now(),
    });
    return id;
  }

  /** 添加工作表元数据加载任务 */
  addSheetMetaTask(sheetId: string): string {
    return this.addTask({
      type: 'sheet_meta',
      sheetId,
      priority: 'high',
    });
  }

  /** 添加行数据加载任务 (按范围分块) */
  addRowDataTasks(sheetId: string, totalRows: number, startRow = 0): string[] {
    const ids: string[] = [];
    for (let i = startRow; i < totalRows; i += this.CHUNK_SIZE) {
      const end = Math.min(i + this.CHUNK_SIZE - 1, totalRows - 1);
      // 头部行优先级 high, 中部 normal, 尾部 low
      let priority: LoadPriority = 'normal';
      if (i < 50) priority = 'high';
      else if (i > totalRows * 0.8) priority = 'low';

      const id = this.addTask({
        type: 'row_data',
        sheetId,
        priority,
        rowRange: { start: i, end },
      });
      ids.push(id);
    }
    return ids;
  }

  /** 添加合并区域任务 */
  addMergedCellsTask(sheetId: string): string {
    return this.addTask({
      type: 'merged_cells',
      sheetId,
      priority: 'high',
    });
  }

  /** 添加样式/列宽/行高任务 (低优先级) */
  addStyleTasks(sheetId: string): void {
    this.addTask({ type: 'cell_style', sheetId, priority: 'normal' });
    this.addTask({ type: 'column_widths', sheetId, priority: 'low' });
    this.addTask({ type: 'row_heights', sheetId, priority: 'low' });
  }

  /** 移除任务 */
  removeTask(id: string): void {
    const idx = this.tasks.findIndex(t => t.id === id);
    if (idx !== -1) {
      if (this.tasks[idx].done) this.appliedCount--;
      this.tasks.splice(idx, 1);
    }
  }

  /** 清空所有任务 */
  clear(): void {
    this.tasks = [];
    this.appliedCount = 0;
    this.isRunning = false;
  }

  // ============================================================
  // 调度
  // ============================================================

  /** 设置 onDemand 模式 (只处理 high 优先级) */
  setOnDemandOnly(onDemand: boolean): void {
    this.onDemandOnly = onDemand;
  }

  /** 启动调度器 */
  async start(): Promise<void> {
    if (this.isRunning) return;
    this.isRunning = true;

    // 无 onRequestData 回调时, 所有任务可同步完成, 无需时间分片
    if (!this.callbacks.onRequestData) {
      this.runSync();
      return;
    }

    // 有 onRequestData 回调时, 使用时间分片调度
    return new Promise<void>((resolve) => {
      this.runSlice(resolve);
    });
  }

  /** 同步执行所有任务 (无异步加载场景) */
  private runSync(): void {
    while (this.isRunning) {
      const task = this.pickNextTask();
      if (!task) {
        this.isRunning = false;
        this.callbacks.onAllComplete?.();
        return;
      }
      this.markTaskDone(task);
      this.callbacks.onProgress?.(this.appliedCount, this.tasks.length);
    }
  }

  /** 运行一个时间片 (有异步加载时使用) */
  private runSlice(resolve: () => void): void {
    if (!this.isRunning) {
      resolve();
      return;
    }

    const startTime = Date.now();
    let processed = 0;

    while (processed < this.MAX_TASKS_PER_SLICE &&
           (Date.now() - startTime) < this.TIME_SLICE_MS) {
      const task = this.pickNextTask();
      if (!task) {
        // 没有任务了
        this.isRunning = false;
        this.callbacks.onAllComplete?.();
        resolve();
        return;
      }
      this.processTaskSync(task);
      processed++;
    }

    // 更新进度
    this.callbacks.onProgress?.(this.appliedCount, this.tasks.length);

    // 让出主线程, 下一帧继续
    if (typeof requestAnimationFrame === 'function') {
      requestAnimationFrame(() => this.runSlice(resolve));
    } else {
      setTimeout(() => this.runSlice(resolve), 0);
    }
  }

  /** 停止调度器 (清理 pending 定时器) */
  stop(): void {
    this.isRunning = false;
  }

  /** 选择下一个待处理任务 (按优先级) */
  private pickNextTask(): LoadTask | null {
    const pending = this.tasks.filter(t => !t.done);
    if (pending.length === 0) return null;

    // onDemand 模式: 只取 high
    let candidates = pending;
    if (this.onDemandOnly) {
      candidates = pending.filter(t => t.priority === 'high');
      if (candidates.length === 0) return null;
    }

    // 优先级排序: high > normal > low
    const priorityOrder: Record<LoadPriority, number> = { high: 0, normal: 1, low: 2 };
    candidates.sort((a, b) => {
      // 1. 优先级
      const pd = priorityOrder[a.priority] - priorityOrder[b.priority];
      if (pd !== 0) return pd;
      // 2. 创建时间 (FIFO)
      return a.createdAt - b.createdAt;
    });

    return candidates[0];
  }

  /** 同步处理任务 (异步加载, 同步应用) */
  private processTaskSync(task: LoadTask): void {
    // 异步加载数据
    if (this.callbacks.onRequestData) {
      // 异步加载 (这里用伪同步, 实际由调用方提供)
      this.callbacks.onRequestData(task).then(data => {
        task.data = data;
        this.markTaskDone(task);
      }).catch(() => {
        // 加载失败也标记完成 (避免阻塞)
        this.markTaskDone(task);
      });
    } else {
      // 无回调, 直接标记完成
      this.markTaskDone(task);
    }
  }

  /** 标记任务完成 */
  private markTaskDone(task: LoadTask): void {
    if (task.done) return;
    task.done = true;
    this.appliedCount++;
    this.callbacks.onTaskApplied?.(task);
    this.callbacks.onProgress?.(this.appliedCount, this.tasks.length);
  }

  // ============================================================
  // 状态查询
  // ============================================================

  /** 获取总任务数 */
  getTotalCount(): number {
    return this.tasks.length;
  }

  /** 获取已完成任务数 */
  getCompletedCount(): number {
    return this.appliedCount;
  }

  /** 获取完成进度 (0-1) */
  getProgress(): number {
    if (this.tasks.length === 0) return 1;
    return this.appliedCount / this.tasks.length;
  }

  /** 是否正在运行 */
  isRunningState(): boolean {
    return this.isRunning;
  }

  /** 是否全部完成 */
  isComplete(): boolean {
    return this.tasks.length > 0 && this.appliedCount >= this.tasks.length;
  }

  /** 获取待处理任务 (按优先级排序) */
  getPendingTasks(): LoadTask[] {
    return this.tasks
      .filter(t => !t.done)
      .sort((a, b) => {
        const priorityOrder: Record<LoadPriority, number> = { high: 0, normal: 1, low: 2 };
        return priorityOrder[a.priority] - priorityOrder[b.priority];
      });
  }
}
