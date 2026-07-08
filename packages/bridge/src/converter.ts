/**
 * 转换器封装 — 封装 C++ 原生模块的调用
 *
 * 通过 Worker Pool 实现高并发转换，主线程不阻塞。
 */

import { writeFileSync } from 'fs';
import { OfficeWorkerPool } from './worker-pool';
import type {
  ZQSheetDocument,
  ZQSlideDocument,
  ZQDocDocument,
  ConvertTask,
  ConvertResult,
} from './types';

export class Converter {
  constructor(private pool: OfficeWorkerPool) {}

  // =================================================================
  // Excel
  // =================================================================

  /** xlsx 文件 → JSON 对象 */
  async xlsxToJson(filePath: string, options?: Record<string, any>): Promise<ZQSheetDocument> {
    const str = await this.pool.exec<string>(
      'xlsxToZQSheet',
      [filePath, null, null, JSON.stringify(options ?? {})]
    );
    return JSON.parse(str);
  }

  /** JSON 对象 → xlsx 文件 */
  async jsonToXlsx(data: ZQSheetDocument, outputPath: string, options?: Record<string, any>): Promise<void> {
    await this.pool.exec(
      'zqSheetToXlsx',
      [JSON.stringify(data), outputPath, null, null, JSON.stringify(options ?? {})]
    );
  }

  // =================================================================
  // PowerPoint
  // =================================================================

  /** pptx 文件 → JSON 对象 */
  async pptxToJson(filePath: string, options?: Record<string, any>): Promise<ZQSlideDocument> {
    const str = await this.pool.exec<string>(
      'pptxToZQSlide',
      [filePath, null, null, JSON.stringify(options ?? {})]
    );
    return JSON.parse(str);
  }

  /** JSON 对象 → pptx 文件 */
  async jsonToPptx(data: ZQSlideDocument, outputPath: string, options?: Record<string, any>): Promise<void> {
    await this.pool.exec(
      'zqSlideToPptx',
      [JSON.stringify(data), outputPath, null, null, JSON.stringify(options ?? {})]
    );
  }

  // =================================================================
  // Word
  // =================================================================

  /** docx 文件 → JSON 对象 */
  async docxToJson(filePath: string, options?: Record<string, any>): Promise<ZQDocDocument> {
    const str = await this.pool.exec<string>(
      'docxToZQDoc',
      [filePath, null, null, JSON.stringify(options ?? {})]
    );
    return JSON.parse(str);
  }

  /** JSON 对象 → docx 文件 */
  async jsonToDocx(data: ZQDocDocument, outputPath: string, options?: Record<string, any>): Promise<void> {
    await this.pool.exec(
      'zqDocToDocx',
      [JSON.stringify(data), outputPath, null, null, JSON.stringify(options ?? {})]
    );
  }

  // =================================================================
  // 通用
  // =================================================================

  /** 检测文件格式 */
  async detectFormat(filePath: string): Promise<number> {
    return this.pool.exec<number>('zqofficeDetectFileFormat', [filePath]);
  }

  /** 通用导出：Office 文件 → JSON 文件 */
  async exportToJson(sourcePath: string, outputPath: string, options?: Record<string, any>): Promise<string> {
    return this.pool.exec<string>(
      'zqofficeExport',
      [sourcePath, outputPath, JSON.stringify(options ?? {})]
    );
  }

  /** 通用导入：JSON → Office 文件 */
  async importFromJson(jsonStr: string, outputPath: string, options?: Record<string, any>): Promise<string> {
    return this.pool.exec<string>(
      'zqofficeImport',
      [jsonStr, outputPath, JSON.stringify(options ?? {})]
    );
  }

  /** 格式转换：Office → Office */
  async convertFile(sourcePath: string, outputPath: string, format: string, options?: Record<string, any>): Promise<string> {
    return this.pool.exec<string>(
      'zqExportFile',
      [sourcePath, outputPath, format, JSON.stringify(options ?? {})]
    );
  }

  // =================================================================
  // 批量操作
  // =================================================================

  /** 批量转换（自动并发） */
  async batchConvert(tasks: ConvertTask[], format: 'xlsx' | 'pptx' | 'docx'): Promise<ConvertResult> {
    const result: ConvertResult = { success: 0, failed: 0, errors: [] };

    const promises = tasks.map(async ({ input, output }) => {
      try {
        const methodName = `${format}ToJson` as 'xlsxToJson' | 'pptxToJson' | 'docxToJson';
        const json = await this[methodName](input);
        writeFileSync(output, JSON.stringify(json, null, 2));
        result.success++;
      } catch (err: any) {
        result.failed++;
        result.errors.push(`${input}: ${err.message}`);
      }
    });

    await Promise.all(promises);
    return result;
  }
}
