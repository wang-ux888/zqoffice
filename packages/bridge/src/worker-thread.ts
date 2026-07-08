/**
 * Worker 线程实现
 *
 * 每个 Worker 独立加载 C++ 原生模块（独立内存空间），
 * 接收来自主线程的任务消息，执行原生调用后返回结果。
 *
 * 达到 maxTasks 上限后自动退出，由主进程重建（防内存泄漏）。
 */

import { parentPort, workerData } from 'worker_threads';

// 每个 Worker 独立加载 C++ 模块
let native: any;
try {
  native = require('zqoffice');
  native.zqofficeInitialize(null, 0);
} catch (err: any) {
  parentPort?.postMessage({
    id: '__init_error__',
    error: `Failed to load zqoffice native module: ${err.message}`,
  });
}

let taskCount = 0;
const maxTasks: number = workerData?.maxTasks ?? 1000;

parentPort?.on('message', (msg: { id: string; method: string; args: any[] }) => {
  const { id, method, args } = msg;

  try {
    if (!native) {
      throw new Error('Native module not loaded');
    }

    const fn = native[method];
    if (typeof fn !== 'function') {
      throw new Error(`Unknown native method: ${method}`);
    }

    const result = fn.apply(null, args);
    parentPort?.postMessage({ id, data: result });
  } catch (err: any) {
    parentPort?.postMessage({ id, error: err.message });
  }

  // 达到上限后自动退出
  if (++taskCount >= maxTasks) {
    try {
      native.zqofficeUninitialize();
    } catch {}
    process.exit(0);
  }
});

// 优雅退出
process.on('SIGTERM', () => {
  try {
    native?.zqofficeUninitialize();
  } catch {}
  process.exit(0);
});
