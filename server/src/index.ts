#!/usr/bin/env node

/**
 * ZQOffice REST API Server
 *
 * 通过 HTTP REST API 暴露 Office 文件处理能力。
 *
 * 启动：node dist/index.js
 * 环境变量：
 *   PORT=3000              监听端口（默认 3000）
 *   ZQOFFICE_WORKERS=4     Worker 线程数
 *   UPLOAD_DIR=./uploads   上传文件临时目录
 */

import Fastify from 'fastify';
import cors from '@fastify/cors';
import multipart from '@fastify/multipart';
import { ZQOffice } from '@zqoffice/bridge';
import { registerRoutes } from './routes';

const PORT = parseInt(process.env.PORT ?? '3000', 10);
const WORKERS = parseInt(process.env.ZQOFFICE_WORKERS ?? '4', 10);

async function main() {
  const app = Fastify({ logger: true });
  const zq = new ZQOffice({ workers: WORKERS });

  // 插件
  await app.register(cors, { origin: true });
  await app.register(multipart, { limits: { fileSize: 100 * 1024 * 1024 } }); // 100MB

  // 注入 zq 实例
  app.decorate('zqoffice', zq);

  // 注册路由
  await app.register(registerRoutes, { prefix: '/api' });

  // 健康检查
  app.get('/health', async () => ({
    status: 'ok',
    ...zq.status,
  }));

  // 启动
  await app.listen({ port: PORT, host: '0.0.0.0' });
  console.log(`[ZQOffice Server] http://localhost:${PORT}`);
  console.log(`[ZQOffice Server] Workers: ${WORKERS}`);

  // 优雅退出
  const shutdown = async () => {
    console.log('[ZQOffice Server] Shutting down...');
    await zq.destroy();
    await app.close();
    process.exit(0);
  };
  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);
}

main().catch((err) => {
  console.error('[ZQOffice Server] Fatal error:', err);
  process.exit(1);
});
