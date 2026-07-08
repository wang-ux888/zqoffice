/**
 * Office Worker 线程池
 *
 * 高并发核心：C++ 原生模块调用是同步阻塞的，大文件转换可能耗时数秒。
 * Worker Pool 确保多并发请求不阻塞主进程，且 Worker 崩溃不影响主进程。
 */

import { Worker } from 'worker_threads';
import { cpus } from 'os';
import { randomUUID } from 'crypto';

interface Task {
  id: string;
  method: string;
  args: any[];
  resolve: (value: any) => void;
  reject: (reason: any) => void;
  timer?: ReturnType<typeof setTimeout>;
  cancelled?: boolean;
}

export interface WorkerPoolOptions {
  maxWorkers?: number;
  maxTasksPerWorker?: number;
  taskTimeout?: number;
}

export class OfficeWorkerPool {
  private workers: Worker[] = [];
  private idleWorkers: Worker[] = [];
  private taskQueue: Task[] = [];
  private busyWorkers = new Map<Worker, Task>();
  private maxWorkers: number;
  private maxTasksPerWorker: number;
  private taskTimeout: number;
  private destroyed = false;

  constructor(options: WorkerPoolOptions = {}) {
    this.maxWorkers = options.maxWorkers ?? cpus().length;
    this.maxTasksPerWorker = options.maxTasksPerWorker ?? 1000;
    this.taskTimeout = options.taskTimeout ?? 30000;

    for (let i = 0; i < this.maxWorkers; i++) {
      this.createWorker();
    }
  }

  private createWorker(): Worker {
    const worker = new Worker(require.resolve('./worker-thread'), {
      workerData: { maxTasks: this.maxTasksPerWorker },
    });

    worker.on('message', (result) => this.handleResult(worker, result));
    worker.on('error', (err) => this.handleError(worker, err));
    worker.on('exit', (code) => this.handleExit(worker, code));

    this.workers.push(worker);
    this.idleWorkers.push(worker);
    return worker;
  }

  async exec<T = any>(method: string, args: any[]): Promise<T> {
    if (this.destroyed) {
      throw new Error('WorkerPool has been destroyed');
    }

    return new Promise((resolve, reject) => {
      const task: Task = {
        id: randomUUID(),
        method,
        args,
        resolve,
        reject,
      };

      task.timer = setTimeout(() => {
        task.cancelled = true;
        task.reject(new Error(`Task ${method} timed out after ${this.taskTimeout}ms`));
        const idx = this.taskQueue.indexOf(task);
        if (idx !== -1) this.taskQueue.splice(idx, 1);
        for (const [w, t] of this.busyWorkers.entries()) {
          if (t === task) {
            this.busyWorkers.delete(w);
            this.removeWorker(w);
            w.terminate().then(() => {
              if (!this.destroyed) this.createWorker();
            });
            break;
          }
        }
      }, this.taskTimeout);

      const worker = this.idleWorkers.pop();
      if (worker) {
        this.dispatch(worker, task);
      } else {
        this.taskQueue.push(task);
      }
    });
  }

  private dispatch(worker: Worker, task: Task) {
    this.busyWorkers.set(worker, task);
    worker.postMessage({ id: task.id, method: task.method, args: task.args });
  }

  private handleResult(worker: Worker, result: { id: string; data?: any; error?: string }) {
    // Handle worker init error
    if (result.id === '__init_error__') {
      console.error('[WorkerPool] Worker init failed:', result.error);
      this.removeWorker(worker);
      if (!this.destroyed) this.createWorker();
      return;
    }

    const task = this.busyWorkers.get(worker);
    this.busyWorkers.delete(worker);

    if (task) {
      if (task.timer) clearTimeout(task.timer);
      if (task.cancelled) return; // Already rejected by timeout
      if (result.error) {
        task.reject(new Error(result.error));
      } else {
        task.resolve(result.data);
      }
    }

    if (this.taskQueue.length > 0) {
      this.dispatch(worker, this.taskQueue.shift()!);
    } else {
      this.idleWorkers.push(worker);
    }
  }

  private handleError(worker: Worker, err: Error) {
    const task = this.busyWorkers.get(worker);
    this.busyWorkers.delete(worker);
    if (task) {
      if (task.timer) clearTimeout(task.timer);
      if (!task.cancelled) task.reject(err);
    }
    this.removeWorker(worker);
    if (!this.destroyed) this.createWorker();
  }

  private handleExit(worker: Worker, _code: number) {
    // Reject any in-flight task
    const task = this.busyWorkers.get(worker);
    this.busyWorkers.delete(worker);
    if (task) {
      if (task.timer) clearTimeout(task.timer);
      if (!task.cancelled) {
        task.reject(new Error(`Worker exited unexpectedly (code=${_code})`));
      }
    }
    // Always clean up and rebuild (even for code=0, which is maxTasks exit)
    this.removeWorker(worker);
    if (!this.destroyed) {
      this.createWorker();
    }
  }

  private removeWorker(worker: Worker) {
    const idx = this.workers.indexOf(worker);
    if (idx !== -1) this.workers.splice(idx, 1);
    const idleIdx = this.idleWorkers.indexOf(worker);
    if (idleIdx !== -1) this.idleWorkers.splice(idleIdx, 1);
  }

  get size(): number {
    return this.workers.length;
  }

  get pending(): number {
    return this.taskQueue.length;
  }

  async destroy() {
    this.destroyed = true;
    for (const task of this.taskQueue) {
      if (task.timer) clearTimeout(task.timer);
      task.reject(new Error('WorkerPool destroyed'));
    }
    this.taskQueue = [];
    await Promise.all(this.workers.map((w) => w.terminate()));
    this.workers = [];
    this.idleWorkers = [];
    this.busyWorkers.clear();
  }
}
