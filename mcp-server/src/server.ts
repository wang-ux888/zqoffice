/**
 * ZQOffice MCP Server — Tool 定义
 *
 * 暴露 10 个 MCP Tool 给 AI Agent
 */

import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { ZQOffice } from '@zqoffice/bridge';
import { z } from 'zod';

export function createServer(zq: ZQOffice): McpServer {
  const server = new McpServer({
    name: 'zqoffice',
    version: '1.0.0',
  });

  // =================================================================
  // Tool 1: 读取 Excel
  // =================================================================
  server.tool(
    'zq_read_excel',
    '读取 Excel 文件，返回 JSON 数据。支持 xlsx/xls/csv 格式。',
    {
      filePath: z.string().describe('xlsx 文件路径'),
      sheetIndex: z.number().optional().default(0).describe('工作表索引（默认 0）'),
    },
    async ({ filePath, sheetIndex }) => {
      try {
        const data = await zq.xlsxToJson(filePath);
        const sheet = data.sheets[sheetIndex ?? 0];
        if (!sheet) {
          return { content: [{ type: 'text', text: `错误：工作表索引 ${sheetIndex} 不存在` }] };
        }
        return {
          content: [{
            type: 'text',
            text: JSON.stringify({
              sheetName: sheet.name,
              rowCount: sheet.rowCount,
              columnCount: sheet.columnCount,
              cellCount: sheet.cells.length,
              cells: sheet.cells.slice(0, 500),
              mergedCells: sheet.mergedCells,
              coreProperties: data.coreProperties,
            }, null, 2),
          }],
        };
      } catch (err: any) {
        return { content: [{ type: 'text', text: `错误：${err.message}` }], isError: true };
      }
    },
  );

  // =================================================================
  // Tool 2: 写入 Excel
  // =================================================================
  server.tool(
    'zq_write_excel',
    '将 JSON 数据写入 Excel 文件。数据格式为 ZQ Sheet JSON。',
    {
      data: z.object({
        sheets: z.array(z.any()),
        coreProperties: z.any().optional(),
      }).describe('ZQ Sheet JSON 数据'),
      outputPath: z.string().describe('输出 xlsx 文件路径'),
    },
    async ({ data, outputPath }) => {
      try {
        await zq.jsonToXlsx(data as any, outputPath);
        return { content: [{ type: 'text', text: `已保存到 ${outputPath}` }] };
      } catch (err: any) {
        return { content: [{ type: 'text', text: `错误：${err.message}` }], isError: true };
      }
    },
  );

  // =================================================================
  // Tool 3: 编辑 Excel（最常用）
  // =================================================================
  server.tool(
    'zq_edit_excel',
    '读取 Excel → 执行编辑操作序列 → 保存。支持：setCell, setFormula, insertRow, insertColumn, deleteRow, deleteColumn, setCellStyle, renameSheet, addSheet, setCoreProperties',
    {
      inputPath: z.string().describe('输入 xlsx 文件路径'),
      outputPath: z.string().describe('输出 xlsx 文件路径'),
      operations: z.array(z.object({
        action: z.string().describe('操作类型'),
        params: z.record(z.any()).describe('操作参数'),
      })).describe('操作序列'),
    },
    async ({ inputPath, outputPath, operations }) => {
      try {
        await zq.applyOperations(inputPath, outputPath, operations);
        return {
          content: [{
            type: 'text',
            text: `已执行 ${operations.length} 个操作，保存到 ${outputPath}`,
          }],
        };
      } catch (err: any) {
        return { content: [{ type: 'text', text: `错误：${err.message}` }], isError: true };
      }
    },
  );

  // =================================================================
  // Tool 4: 读取 PowerPoint
  // =================================================================
  server.tool(
    'zq_read_pptx',
    '读取 PowerPoint 文件，返回 JSON 数据。',
    {
      filePath: z.string().describe('pptx 文件路径'),
    },
    async ({ filePath }) => {
      try {
        const data = await zq.pptxToJson(filePath);
        return {
          content: [{
            type: 'text',
            text: JSON.stringify({
              slideCount: data.slides.length,
              slideSize: data.slideSize,
              slides: data.slides,
              coreProperties: data.coreProperties,
            }, null, 2),
          }],
        };
      } catch (err: any) {
        return { content: [{ type: 'text', text: `错误：${err.message}` }], isError: true };
      }
    },
  );

  // =================================================================
  // Tool 5: 写入 PowerPoint
  // =================================================================
  server.tool(
    'zq_write_pptx',
    '将 JSON 数据写入 PowerPoint 文件。',
    {
      data: z.object({
        slides: z.array(z.any()),
        slideSize: z.any().optional(),
        coreProperties: z.any().optional(),
      }).describe('ZQ Slide JSON 数据'),
      outputPath: z.string().describe('输出 pptx 文件路径'),
    },
    async ({ data, outputPath }) => {
      try {
        await zq.jsonToPptx(data as any, outputPath);
        return { content: [{ type: 'text', text: `已保存到 ${outputPath}` }] };
      } catch (err: any) {
        return { content: [{ type: 'text', text: `错误：${err.message}` }], isError: true };
      }
    },
  );

  // =================================================================
  // Tool 6: 读取 Word
  // =================================================================
  server.tool(
    'zq_read_docx',
    '读取 Word 文件，返回 JSON 数据。',
    {
      filePath: z.string().describe('docx 文件路径'),
    },
    async ({ filePath }) => {
      try {
        const data = await zq.docxToJson(filePath);
        return {
          content: [{
            type: 'text',
            text: JSON.stringify({
              blockCount: data.blocks.length,
              blocks: data.blocks,
              coreProperties: data.coreProperties,
            }, null, 2),
          }],
        };
      } catch (err: any) {
        return { content: [{ type: 'text', text: `错误：${err.message}` }], isError: true };
      }
    },
  );

  // =================================================================
  // Tool 7: 写入 Word
  // =================================================================
  server.tool(
    'zq_write_docx',
    '将 JSON 数据写入 Word 文件。',
    {
      data: z.object({
        blocks: z.array(z.any()),
        coreProperties: z.any().optional(),
      }).describe('ZQ Doc JSON 数据'),
      outputPath: z.string().describe('输出 docx 文件路径'),
    },
    async ({ data, outputPath }) => {
      try {
        await zq.jsonToDocx(data as any, outputPath);
        return { content: [{ type: 'text', text: `已保存到 ${outputPath}` }] };
      } catch (err: any) {
        return { content: [{ type: 'text', text: `错误：${err.message}` }], isError: true };
      }
    },
  );

  // =================================================================
  // Tool 8: 检测文件格式
  // =================================================================
  server.tool(
    'zq_detect_format',
    '检测 Office 文件格式。返回格式枚举值。',
    {
      filePath: z.string().describe('文件路径'),
    },
    async ({ filePath }) => {
      try {
        const format = await zq.detectFormat(filePath);
        const formatNames: Record<number, string> = {
          0: 'Unknown', 1: 'XLSX', 2: 'XLS', 3: 'CSV',
          4: 'PPTX', 5: 'PPT', 6: 'DOCX', 7: 'DOC',
        };
        return {
          content: [{
            type: 'text',
            text: JSON.stringify({ format, formatName: formatNames[format] ?? 'Unknown' }),
          }],
        };
      } catch (err: any) {
        return { content: [{ type: 'text', text: `错误：${err.message}` }], isError: true };
      }
    },
  );

  // =================================================================
  // Tool 9: 批量转换
  // =================================================================
  server.tool(
    'zq_batch_convert',
    '批量转换 Office 文件（高并发）。支持 xlsx/pptx/docx 互转。',
    {
      files: z.array(z.object({
        input: z.string().describe('输入文件路径'),
        output: z.string().describe('输出文件路径'),
      })).describe('转换任务列表'),
      format: z.enum(['xlsx', 'pptx', 'docx']).describe('目标格式'),
    },
    async ({ files, format }) => {
      try {
        const result = await zq.batchConvert(files, format);
        return { content: [{ type: 'text', text: JSON.stringify(result) }] };
      } catch (err: any) {
        return { content: [{ type: 'text', text: `错误：${err.message}` }], isError: true };
      }
    },
  );

  // =================================================================
  // Tool 10: 获取 Worker 状态
  // =================================================================
  server.tool(
    'zq_status',
    '获取 ZQOffice 引擎状态（Worker 数量、等待任务数）。',
    {},
    async () => {
      const status = zq.status;
      return {
        content: [{
          type: 'text',
          text: JSON.stringify(status),
        }],
      };
    },
  );

  return server;
}
