/**
 * ZQOffice REST API 路由定义
 *
 * API 端点：
 *   POST /api/xlsx/read        读取 Excel
 *   POST /api/xlsx/write       写入 Excel
 *   POST /api/xlsx/edit        编辑 Excel（读→操作→存）
 *   POST /api/pptx/read        读取 PowerPoint
 *   POST /api/pptx/write       写入 PowerPoint
 *   POST /api/docx/read        读取 Word
 *   POST /api/docx/write       写入 Word
 *   POST /api/detect           检测文件格式
 *   POST /api/batch            批量转换
 *   GET  /api/status            引擎状态
 */

import { FastifyInstance, FastifyRequest, FastifyReply } from 'fastify';
import { ZQOffice, EditOperation, ConvertTask } from '@zqoffice/bridge';

declare module 'fastify' {
  interface FastifyInstance {
    zqoffice: ZQOffice;
  }
}

// =================================================================
// 输入校验辅助函数
// =================================================================

function isNonEmptyString(v: unknown): v is string {
  return typeof v === 'string' && v.length > 0;
}

function isNonNegativeInteger(v: unknown): boolean {
  return typeof v === 'number' && Number.isInteger(v) && v >= 0;
}

function isPlainObject(v: unknown): v is Record<string, unknown> {
  return typeof v === 'object' && v !== null && !Array.isArray(v);
}

export async function registerRoutes(app: FastifyInstance) {
  const zq = app.zqoffice;

  // =================================================================
  // Excel
  // =================================================================

  /** POST /api/xlsx/read — 读取 Excel */
  app.post('/api/xlsx/read', async (req: FastifyRequest, reply: FastifyReply) => {
    const { filePath, sheetIndex, options } = req.body as any;
    if (!isNonEmptyString(filePath)) {
      return reply.status(400).send({ error: 'filePath must be a non-empty string' });
    }
    if (sheetIndex !== undefined && !isNonNegativeInteger(sheetIndex)) {
      return reply.status(400).send({ error: 'sheetIndex must be a non-negative integer' });
    }

    try {
      const data = await zq.xlsxToJson(filePath, options);
      if (sheetIndex !== undefined) {
        return { sheet: data.sheets[sheetIndex], coreProperties: data.coreProperties };
      }
      return data;
    } catch (err: any) {
      return reply.status(500).send({ error: err.message });
    }
  });

  /** POST /api/xlsx/write — 写入 Excel */
  app.post('/api/xlsx/write', async (req: FastifyRequest, reply: FastifyReply) => {
    const { data, outputPath, options } = req.body as any;
    if (!isPlainObject(data) || !Array.isArray(data.sheets)) {
      return reply.status(400).send({ error: 'data must be an object with a sheets array' });
    }
    if (!isNonEmptyString(outputPath)) {
      return reply.status(400).send({ error: 'outputPath must be a non-empty string' });
    }

    try {
      await zq.jsonToXlsx(data, outputPath, options);
      return { success: true, outputPath };
    } catch (err: any) {
      return reply.status(500).send({ error: err.message });
    }
  });

  /** POST /api/xlsx/edit — 编辑 Excel（读→操作→存） */
  app.post('/api/xlsx/edit', async (req: FastifyRequest, reply: FastifyReply) => {
    const { inputPath, outputPath, operations, options } = req.body as any;
    if (!isNonEmptyString(inputPath)) {
      return reply.status(400).send({ error: 'inputPath must be a non-empty string' });
    }
    if (!isNonEmptyString(outputPath)) {
      return reply.status(400).send({ error: 'outputPath must be a non-empty string' });
    }
    if (!Array.isArray(operations)) {
      return reply.status(400).send({ error: 'operations must be an array' });
    }

    try {
      await zq.applyOperations(inputPath, outputPath, operations);
      return { success: true, outputPath, operationCount: operations.length };
    } catch (err: any) {
      return reply.status(500).send({ error: err.message });
    }
  });

  // =================================================================
  // PowerPoint
  // =================================================================

  /** POST /api/pptx/read — 读取 PowerPoint */
  app.post('/api/pptx/read', async (req: FastifyRequest, reply: FastifyReply) => {
    const { filePath, options } = req.body as any;
    if (!isNonEmptyString(filePath)) {
      return reply.status(400).send({ error: 'filePath must be a non-empty string' });
    }

    try {
      return await zq.pptxToJson(filePath, options);
    } catch (err: any) {
      return reply.status(500).send({ error: err.message });
    }
  });

  /** POST /api/pptx/write — 写入 PowerPoint */
  app.post('/api/pptx/write', async (req: FastifyRequest, reply: FastifyReply) => {
    const { data, outputPath, options } = req.body as any;
    if (!isPlainObject(data)) {
      return reply.status(400).send({ error: 'data must be an object' });
    }
    if (!isNonEmptyString(outputPath)) {
      return reply.status(400).send({ error: 'outputPath must be a non-empty string' });
    }

    try {
      await zq.jsonToPptx(data, outputPath, options);
      return { success: true, outputPath };
    } catch (err: any) {
      return reply.status(500).send({ error: err.message });
    }
  });

  // =================================================================
  // Word
  // =================================================================

  /** POST /api/docx/read — 读取 Word */
  app.post('/api/docx/read', async (req: FastifyRequest, reply: FastifyReply) => {
    const { filePath, options } = req.body as any;
    if (!isNonEmptyString(filePath)) {
      return reply.status(400).send({ error: 'filePath must be a non-empty string' });
    }

    try {
      return await zq.docxToJson(filePath, options);
    } catch (err: any) {
      return reply.status(500).send({ error: err.message });
    }
  });

  /** POST /api/docx/write — 写入 Word */
  app.post('/api/docx/write', async (req: FastifyRequest, reply: FastifyReply) => {
    const { data, outputPath, options } = req.body as any;
    if (!isPlainObject(data)) {
      return reply.status(400).send({ error: 'data must be an object' });
    }
    if (!isNonEmptyString(outputPath)) {
      return reply.status(400).send({ error: 'outputPath must be a non-empty string' });
    }

    try {
      await zq.jsonToDocx(data, outputPath, options);
      return { success: true, outputPath };
    } catch (err: any) {
      return reply.status(500).send({ error: err.message });
    }
  });

  // =================================================================
  // 通用
  // =================================================================

  /** POST /api/detect — 检测文件格式 */
  app.post('/api/detect', async (req: FastifyRequest, reply: FastifyReply) => {
    const { filePath } = req.body as any;
    if (!isNonEmptyString(filePath)) {
      return reply.status(400).send({ error: 'filePath must be a non-empty string' });
    }

    try {
      const format = await zq.detectFormat(filePath);
      const formatNames: Record<number, string> = {
        0: 'Unknown', 1: 'XLSX', 2: 'XLS', 3: 'CSV',
        4: 'PPTX', 5: 'PPT', 6: 'DOCX', 7: 'DOC',
      };
      return { format, formatName: formatNames[format] ?? 'Unknown' };
    } catch (err: any) {
      return reply.status(500).send({ error: err.message });
    }
  });

  /** POST /api/batch — 批量转换 */
  app.post('/api/batch', async (req: FastifyRequest, reply: FastifyReply) => {
    const { files, format } = req.body as any;
    if (!Array.isArray(files)) {
      return reply.status(400).send({ error: 'files must be an array' });
    }
    if (format !== 'xlsx' && format !== 'pptx' && format !== 'docx') {
      return reply.status(400).send({ error: "format must be one of 'xlsx', 'pptx', 'docx'" });
    }

    try {
      return await zq.batchConvert(files, format);
    } catch (err: any) {
      return reply.status(500).send({ error: err.message });
    }
  });

  /** GET /api/status — 引擎状态 */
  app.get('/api/status', async () => zq.status);
}
