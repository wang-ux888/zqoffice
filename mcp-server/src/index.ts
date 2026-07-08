#!/usr/bin/env node

/**
 * ZQOffice MCP Server
 *
 * 通过 Model Context Protocol (MCP) 暴露 Office 文件处理能力给 AI Agent。
 *
 * 启动方式：
 *   node dist/index.js              # stdio 模式（Agent 直接调用）
 *   ZQOFFICE_PORT=3001 node dist/index.js  # HTTP 模式
 */

import { ZQOffice } from '@zqoffice/bridge';
import { createServer } from './server';

const WORKERS = parseInt(process.env.ZQOFFICE_WORKERS ?? '4', 10);
const PORT = parseInt(process.env.ZQOFFICE_PORT ?? '3001', 10);

async function main() {
  const zq = new ZQOffice({ workers: WORKERS });
  const server = createServer(zq);

  // stdio 模式（MCP 标准）
  if (!process.env.ZQOFFICE_PORT) {
    const { StdioServerTransport } = await import('@modelcontextprotocol/sdk/server/stdio.js');
    const transport = new StdioServerTransport();
    await server.connect(transport);
    console.error('[ZQOffice MCP] Running on stdio');
  } else {
    // HTTP 模式
    const { StreamableHTTPServerTransport } = await import('@modelcontextprotocol/sdk/server/streamableHttp.js');
    const http = await import('http');
    const httpServer = http.createServer(async (req, res) => {
      if (req.method === 'POST' && req.url === '/mcp') {
        const transport = new StreamableHTTPServerTransport({ sessionIdGenerator: undefined });
        await server.connect(transport);
        await transport.handleRequest(req, res);
      } else {
        res.writeHead(404);
        res.end('Not Found');
      }
    });
    httpServer.listen(PORT, () => {
      console.log(`[ZQOffice MCP] HTTP server on http://localhost:${PORT}/mcp`);
    });
  }

  // 优雅退出
  process.on('SIGINT', async () => {
    await zq.destroy();
    process.exit(0);
  });
  process.on('SIGTERM', async () => {
    await zq.destroy();
    process.exit(0);
  });
}

main().catch((err) => {
  console.error('[ZQOffice MCP] Fatal error:', err);
  process.exit(1);
});
