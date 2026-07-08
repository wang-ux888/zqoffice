/**
 * 打印管理器层 — 打印区域/标题/页面设置/预览/导出PDF
 * 全部原创实现
 *
 * 职责:
 *   1. 打印区域管理: 设置/获取打印区域
 *   2. 打印标题: 设置重复出现的标题行/列 (每页顶部/左侧)
 *   3. 页面设置: 纸张大小/方向/边距/缩放/居中/适应一页
 *   4. 分页计算: 根据打印区域与页面大小计算分页
 *   5. 打印预览: 在模态框中显示分页预览
 *   6. 导出 PDF: 通过 Electron IPC 调用主进程打印, 或降级为浏览器打印
 *
 * 单位约定:
 *   - 纸张/边距: 毫米 (mm)
 *   - 行高/列宽: 像素 (px, 与 SheetModel 一致)
 *   - 转换: 1mm ≈ 3.7795px (96 DPI)
 */

import { SheetModel, CellStyle } from './data-model';

// ============================================================
// 类型定义
// ============================================================

/** 纸张大小 */
export type PaperSize = 'A4' | 'A3' | 'Letter' | 'Legal';

/** 页面方向 */
export type PageOrientation = 'portrait' | 'landscape';

/** 页面边距 (mm) */
export interface PageMargins {
  top: number;
  bottom: number;
  left: number;
  right: number;
}

/** 页面设置 */
export interface PrintPageSetup {
  /** 纸张大小 */
  paperSize: PaperSize;
  /** 页面方向 */
  orientation: PageOrientation;
  /** 页面边距 (mm) */
  margins: PageMargins;
  /** 缩放比例 1-100 */
  scale: number;
  /** 是否缩放到一页 (为 true 时忽略 scale, 自动计算适应比例) */
  fitToPage: boolean;
  /** 水平居中 */
  horizontalCentered: boolean;
  /** 垂直居中 */
  verticalCentered: boolean;
}

/** 区域范围 (0-based, 含两端) */
export interface PrintAreaRange {
  startRow: number;
  startCol: number;
  endRow: number;
  endCol: number;
}

/** 标题行/列范围 (0-based, 含两端) */
export interface PrintTitleRange {
  start: number;
  end: number;
}

/** 单页范围信息 */
export interface PrintPage {
  /** 页码 (从 1 开始) */
  index: number;
  /** 该页起始行 (含标题行) */
  startRow: number;
  /** 该页结束行 */
  endRow: number;
  /** 该页起始列 (含标题列) */
  startCol: number;
  /** 该页结束列 */
  endCol: number;
  /** 该页包含的标题行范围 */
  titleRowRange: PrintTitleRange | null;
  /** 该页包含的标题列范围 */
  titleColRange: PrintTitleRange | null;
}

/** 分页计算结果 */
export interface PaginationResult {
  /** 所有页 */
  pages: PrintPage[];
  /** 总页数 */
  pageCount: number;
  /** 实际使用的缩放比例 (1-100, fitToPage 时为计算值) */
  effectiveScale: number;
  /** 每页可用内容宽度 (px) */
  contentWidth: number;
  /** 每页可用内容高度 (px) */
  contentHeight: number;
}

// ============================================================
// 常量
// ============================================================

/** 毫米转像素 (96 DPI) */
const MM_TO_PX = 96 / 25.4;

/** 纸张尺寸表 (mm, 纵向时的宽×高) */
const PAPER_SIZE_MM: Record<PaperSize, { width: number; height: number }> = {
  A4: { width: 210, height: 297 },
  A3: { width: 297, height: 420 },
  Letter: { width: 216, height: 279 },
  Legal: { width: 216, height: 356 },
};

/** 默认页面设置 */
const DEFAULT_PAGE_SETUP: PrintPageSetup = {
  paperSize: 'A4',
  orientation: 'portrait',
  margins: { top: 19.05, bottom: 19.05, left: 19.05, right: 19.05 }, // 0.75 inch
  scale: 100,
  fitToPage: false,
  horizontalCentered: false,
  verticalCentered: false,
};

// ============================================================
// 打印管理器
// ============================================================

export class PrintManager {
  private sheet: SheetModel;
  private printArea: PrintAreaRange | null = null;
  private titleRows: PrintTitleRange | null = null;
  private titleCols: PrintTitleRange | null = null;
  private pageSetup: PrintPageSetup = { ...DEFAULT_PAGE_SETUP, margins: { ...DEFAULT_PAGE_SETUP.margins } };
  /** 预览模态框根元素 (用于销毁) */
  private previewOverlay: HTMLDivElement | null = null;

  constructor(sheet: SheetModel) {
    this.sheet = sheet;
  }

  // ============================================================
  // 打印区域管理
  // ============================================================

  /** 设置打印区域 */
  setPrintArea(range: PrintAreaRange): void {
    // 规范化: 确保 start <= end
    this.printArea = {
      startRow: Math.min(range.startRow, range.endRow),
      endRow: Math.max(range.startRow, range.endRow),
      startCol: Math.min(range.startCol, range.endCol),
      endCol: Math.max(range.startCol, range.endCol),
    };
  }

  /** 获取打印区域 (未设置时返回 null) */
  getPrintArea(): PrintAreaRange | null {
    return this.printArea ? { ...this.printArea } : null;
  }

  /** 清除打印区域 */
  clearPrintArea(): void {
    this.printArea = null;
  }

  /** 获取实际打印区域 (未设置时使用已用区域) */
  getEffectivePrintArea(): PrintAreaRange {
    if (this.printArea) return { ...this.printArea };
    return this.computeUsedRange();
  }

  // ============================================================
  // 打印标题行/列
  // ============================================================

  /** 设置打印标题行 (每页顶部重复, rows 为行号范围) */
  setPrintTitleRows(rows: PrintTitleRange | null): void {
    if (rows === null) {
      this.titleRows = null;
      return;
    }
    this.titleRows = {
      start: Math.min(rows.start, rows.end),
      end: Math.max(rows.start, rows.end),
    };
  }

  /** 设置打印标题列 (每页左侧重复, cols 为列号范围) */
  setPrintTitleCols(cols: PrintTitleRange | null): void {
    if (cols === null) {
      this.titleCols = null;
      return;
    }
    this.titleCols = {
      start: Math.min(cols.start, cols.end),
      end: Math.max(cols.start, cols.end),
    };
  }

  /** 获取打印标题行 */
  getPrintTitleRows(): PrintTitleRange | null {
    return this.titleRows ? { ...this.titleRows } : null;
  }

  /** 获取打印标题列 */
  getPrintTitleCols(): PrintTitleRange | null {
    return this.titleCols ? { ...this.titleCols } : null;
  }

  // ============================================================
  // 页面设置
  // ============================================================

  /** 设置页面参数 (合并更新) */
  setPageSetup(config: Partial<PrintPageSetup>): void {
    if (config.margins) {
      this.pageSetup.margins = { ...this.pageSetup.margins, ...config.margins };
    }
    if (config.scale !== undefined) {
      this.pageSetup.scale = Math.max(10, Math.min(100, config.scale));
    }
    if (config.paperSize) this.pageSetup.paperSize = config.paperSize;
    if (config.orientation) this.pageSetup.orientation = config.orientation;
    if (config.fitToPage !== undefined) this.pageSetup.fitToPage = config.fitToPage;
    if (config.horizontalCentered !== undefined) this.pageSetup.horizontalCentered = config.horizontalCentered;
    if (config.verticalCentered !== undefined) this.pageSetup.verticalCentered = config.verticalCentered;
  }

  /** 获取页面设置 (拷贝) */
  getPageSetup(): PrintPageSetup {
    return { ...this.pageSetup, margins: { ...this.pageSetup.margins } };
  }

  /** 重置为默认页面设置 */
  resetPageSetup(): void {
    this.pageSetup = { ...DEFAULT_PAGE_SETUP, margins: { ...DEFAULT_PAGE_SETUP.margins } };
  }

  // ============================================================
  // 分页计算
  // ============================================================

  /**
   * 计算分页
   * 算法:
   *   1. 根据纸张大小/方向/边距计算每页可用内容尺寸 (px)
   *   2. 若 fitToPage, 计算适应比例使整个打印区域放进一页
   *   3. 减去标题行/列占用空间, 得到非标题区域的可用尺寸
   *   4. 沿列方向累计列宽分页, 沿行方向累计行高分页
   *   5. 行页 × 列页 笛卡尔积得到所有页
   */
  calculatePages(): PaginationResult {
    const area = this.getEffectivePrintArea();
    const setup = this.pageSetup;

    // 1. 纸张尺寸 (mm) → 内容尺寸 (px)
    const paper = PAPER_SIZE_MM[setup.paperSize];
    let paperW = paper.width;
    let paperH = paper.height;
    if (setup.orientation === 'landscape') {
      [paperW, paperH] = [paperH, paperW];
    }
    const contentW = (paperW - setup.margins.left - setup.margins.right) * MM_TO_PX;
    const contentH = (paperH - setup.margins.top - setup.margins.bottom) * MM_TO_PX;

    // 2. 标题行/列尺寸
    const titleRowsH = this.titleRows ? this.sumRowHeights(this.titleRows.start, this.titleRows.end) : 0;
    const titleColsW = this.titleCols ? this.sumColWidths(this.titleCols.start, this.titleCols.end) : 0;

    // 3. 计算有效缩放比例
    let effectiveScale: number;
    if (setup.fitToPage) {
      effectiveScale = this.computeFitScale(area, contentW, contentH, titleRowsH, titleColsW);
    } else {
      effectiveScale = setup.scale;
    }
    const scale = effectiveScale / 100;

    // 4. 每页非标题区域可用尺寸 (内容尺寸随缩放变化: 缩放越小容纳越多内容像素)
    //    内容物理尺寸固定, 缩放比例 s 下可容纳的内容像素 = contentPx / s
    //    标题也随缩放渲染, 占用 titlePx 内容像素 (物理 titlePx * s)
    //    所以非标题可用内容像素 = contentPx / s - titlePx
    const availW = contentW / scale - titleColsW;
    const availH = contentH / scale - titleRowsH;

    // 5. 非标题行/列范围
    const nonTitleRowStart = this.titleRows ? this.titleRows.end + 1 : area.startRow;
    const nonTitleRowEnd = area.endRow;
    const nonTitleColStart = this.titleCols ? this.titleCols.end + 1 : area.startCol;
    const nonTitleColEnd = area.endCol;

    // 6. 沿行方向分页
    const rowPages = this.splitByDimension(
      nonTitleRowStart,
      nonTitleRowEnd,
      availH,
      (idx: number) => this.sheet.isRowHidden(idx) ? 0 : this.sheet.getRowHeight(idx)
    );
    // 7. 沿列方向分页
    const colPages = this.splitByDimension(
      nonTitleColStart,
      nonTitleColEnd,
      availW,
      (idx: number) => this.sheet.isColumnHidden(idx) ? 0 : this.sheet.getColumnWidth(idx)
    );

    // 8. 笛卡尔积生成页
    const pages: PrintPage[] = [];
    let pageIndex = 1;
    for (const rp of rowPages) {
      for (const cp of colPages) {
        // 每页范围含标题行/列; clamp 到打印区域边界
        const startRow = this.titleRows ? this.titleRows.start : Math.max(rp.start, area.startRow);
        const endRow = Math.min(rp.end, area.endRow);
        const startCol = this.titleCols ? this.titleCols.start : Math.max(cp.start, area.startCol);
        const endCol = Math.min(cp.end, area.endCol);
        pages.push({
          index: pageIndex++,
          startRow,
          endRow,
          startCol,
          endCol,
          titleRowRange: this.titleRows ? { ...this.titleRows } : null,
          titleColRange: this.titleCols ? { ...this.titleCols } : null,
        });
      }
    }

    // 无数据时至少返回一页
    if (pages.length === 0) {
      pages.push({
        index: 1,
        startRow: area.startRow,
        endRow: area.startRow,
        startCol: area.startCol,
        endCol: area.startCol,
        titleRowRange: this.titleRows ? { ...this.titleRows } : null,
        titleColRange: this.titleCols ? { ...this.titleCols } : null,
      });
    }

    return {
      pages,
      pageCount: pages.length,
      effectiveScale,
      contentWidth: contentW,
      contentHeight: contentH,
    };
  }

  /**
   * 计算适应一页的缩放比例 (1-100)
   * 要求: (标题尺寸 + 非标题总尺寸) * s ≤ 内容尺寸
   */
  private computeFitScale(
    area: PrintAreaRange,
    contentW: number,
    contentH: number,
    titleRowsH: number,
    titleColsW: number
  ): number {
    const nonTitleRowStart = this.titleRows ? this.titleRows.end + 1 : area.startRow;
    const nonTitleColStart = this.titleCols ? this.titleCols.end + 1 : area.startCol;
    const nonTitleH = this.sumRowHeights(nonTitleRowStart, area.endRow);
    const nonTitleW = this.sumColWidths(nonTitleColStart, area.endCol);

    // s ≤ contentW / (titleColsW + nonTitleW)
    // s ≤ contentH / (titleRowsH + nonTitleH)
    const totalW = titleColsW + nonTitleW;
    const totalH = titleRowsH + nonTitleH;
    let scale = 100;
    if (totalW > 0) {
      scale = Math.min(scale, (contentW / totalW) * 100);
    }
    if (totalH > 0) {
      scale = Math.min(scale, (contentH / totalH) * 100);
    }
    // 不放大, 最小 10%
    scale = Math.max(10, Math.min(100, scale));
    return Math.floor(scale);
  }

  /**
   * 沿单一维度 (行或列) 分页
   * @param start 起始索引
   * @param end 结束索引
   * @param avail 可用尺寸 (px)
   * @param getSize 索引→尺寸 的函数
   * @returns 分页区间数组 (每个区间为 [start, end] 闭区间)
   */
  private splitByDimension(
    start: number,
    end: number,
    avail: number,
    getSize: (idx: number) => number
  ): { start: number; end: number }[] {
    // 起始超过结束 (无非标题内容) → 返回单个退化区间 (页内仅显示标题)
    if (start > end) {
      return [{ start, end }];
    }
    // 可用尺寸非正 → 全部内容放一页 (避免无限分页)
    if (avail <= 0) {
      return [{ start, end }];
    }
    const pages: { start: number; end: number }[] = [];
    let pageStart = start;
    let used = 0;
    for (let i = start; i <= end; i++) {
      const size = getSize(i);
      // 超出可用尺寸且当前页已有内容 → 断页
      if (used + size > avail && used > 0) {
        pages.push({ start: pageStart, end: i - 1 });
        pageStart = i;
        used = 0;
      }
      used += size;
    }
    // 推入最后一页
    pages.push({ start: pageStart, end });
    return pages;
  }

  /** 累计 [start, end] 范围行高 (跳过隐藏行) */
  private sumRowHeights(start: number, end: number): number {
    let total = 0;
    for (let r = start; r <= end; r++) {
      if (!this.sheet.isRowHidden(r)) {
        total += this.sheet.getRowHeight(r);
      }
    }
    return total;
  }

  /** 累计 [start, end] 范围列宽 (跳过隐藏列) */
  private sumColWidths(start: number, end: number): number {
    let total = 0;
    for (let c = start; c <= end; c++) {
      if (!this.sheet.isColumnHidden(c)) {
        total += this.sheet.getColumnWidth(c);
      }
    }
    return total;
  }

  /** 计算已用区域 (有数据的单元格的范围) */
  private computeUsedRange(): PrintAreaRange {
    const cells = this.sheet.getAllCells();
    if (cells.length === 0) {
      return { startRow: 0, startCol: 0, endRow: 0, endCol: 0 };
    }
    let minR = Infinity, maxR = -1, minC = Infinity, maxC = -1;
    for (const c of cells) {
      if (c.row < minR) minR = c.row;
      if (c.row > maxR) maxR = c.row;
      if (c.column < minC) minC = c.column;
      if (c.column > maxC) maxC = c.column;
    }
    return { startRow: minR, startCol: minC, endRow: maxR, endCol: maxC };
  }

  // ============================================================
  // 打印预览 (模态框)
  // ============================================================

  /**
   * 显示打印预览 (模态框)
   * @returns 是否成功打开
   */
  showPrintPreview(): boolean {
    if (this.previewOverlay) {
      // 已打开, 关闭旧的
      this.closePrintPreview();
    }
    const pagination = this.calculatePages();
    if (pagination.pages.length === 0) return false;

    const overlay = this.createPreviewModal(pagination);
    this.previewOverlay = overlay;
    document.body.appendChild(overlay);
    return true;
  }

  /** 关闭打印预览 */
  closePrintPreview(): void {
    if (this.previewOverlay) {
      this.previewOverlay.remove();
      this.previewOverlay = null;
    }
  }

  /** 创建预览模态框 */
  private createPreviewModal(pagination: PaginationResult): HTMLDivElement {
    const overlay = document.createElement('div');
    overlay.className = 'zq-print-preview-overlay';

    const dialog = document.createElement('div');
    dialog.className = 'zq-print-preview-dialog';

    // 标题栏
    const header = document.createElement('div');
    header.className = 'zq-print-preview-header';
    const title = document.createElement('span');
    title.textContent = `打印预览 (共 ${pagination.pageCount} 页, 缩放 ${pagination.effectiveScale}%)`;
    header.appendChild(title);

    const closeBtn = document.createElement('button');
    closeBtn.textContent = '关闭';
    closeBtn.className = 'zq-print-preview-btn';
    closeBtn.addEventListener('click', () => this.closePrintPreview());
    header.appendChild(closeBtn);

    // 工具栏
    const toolbar = document.createElement('div');
    toolbar.className = 'zq-print-preview-toolbar';

    const prevBtn = document.createElement('button');
    prevBtn.textContent = '上一页';
    prevBtn.className = 'zq-print-preview-btn';
    const nextBtn = document.createElement('button');
    nextBtn.textContent = '下一页';
    nextBtn.className = 'zq-print-preview-btn';
    const pageLabel = document.createElement('span');
    pageLabel.className = 'zq-print-preview-pagelabel';
    const exportBtn = document.createElement('button');
    exportBtn.textContent = '导出 PDF';
    exportBtn.className = 'zq-print-preview-btn zq-print-preview-primary';

    toolbar.appendChild(prevBtn);
    toolbar.appendChild(pageLabel);
    toolbar.appendChild(nextBtn);
    toolbar.appendChild(exportBtn);

    // 页面容器
    const pageContainer = document.createElement('div');
    pageContainer.className = 'zq-print-preview-pages';

    // 当前页索引 (从 0 开始)
    let currentPage = 0;
    const renderPageAt = (idx: number): void => {
      pageContainer.innerHTML = '';
      if (idx < 0 || idx >= pagination.pages.length) return;
      const page = pagination.pages[idx];
      const pageEl = this.renderPreviewPage(page, pagination);
      pageContainer.appendChild(pageEl);
      pageLabel.textContent = `第 ${idx + 1} / ${pagination.pageCount} 页`;
      prevBtn.disabled = idx === 0;
      nextBtn.disabled = idx === pagination.pages.length - 1;
    };

    prevBtn.addEventListener('click', () => {
      if (currentPage > 0) {
        currentPage--;
        renderPageAt(currentPage);
      }
    });
    nextBtn.addEventListener('click', () => {
      if (currentPage < pagination.pages.length - 1) {
        currentPage++;
        renderPageAt(currentPage);
      }
    });
    exportBtn.addEventListener('click', () => {
      void this.exportPDF();
    });

    dialog.appendChild(header);
    dialog.appendChild(toolbar);
    dialog.appendChild(pageContainer);
    overlay.appendChild(dialog);

    // 点击遮罩不关闭 (避免误操作), 按 Esc 关闭
    overlay.addEventListener('click', (e) => {
      if (e.target === overlay) {
        // 不关闭, 仅提示
      }
    });
    const escHandler = (e: KeyboardEvent): void => {
      if (e.key === 'Escape') {
        this.closePrintPreview();
        document.removeEventListener('keydown', escHandler);
      }
    };
    document.addEventListener('keydown', escHandler);

    // 渲染第一页
    renderPageAt(0);

    // 注入样式 (仅一次)
    this.injectPreviewStyles();

    return overlay;
  }

  /** 渲染单个预览页 (DOM) */
  private renderPreviewPage(page: PrintPage, pagination: PaginationResult): HTMLDivElement {
    const setup = this.pageSetup;
    const paper = PAPER_SIZE_MM[setup.paperSize];
    let paperW = paper.width;
    let paperH = paper.height;
    if (setup.orientation === 'landscape') {
      [paperW, paperH] = [paperH, paperW];
    }
    const paperWpx = paperW * MM_TO_PX;
    const paperHpx = paperH * MM_TO_PX;

    // 缩略图缩放 (适应预览容器)
    const maxPreviewW = 760;
    const thumbScale = Math.min(1, maxPreviewW / paperWpx);

    const pageEl = document.createElement('div');
    pageEl.className = 'zq-print-preview-page';
    pageEl.style.width = `${paperWpx * thumbScale}px`;
    pageEl.style.height = `${paperHpx * thumbScale}px`;
    pageEl.style.padding = `${setup.margins.top * MM_TO_PX * thumbScale}px ${setup.margins.right * MM_TO_PX * thumbScale}px ${setup.margins.bottom * MM_TO_PX * thumbScale}px ${setup.margins.left * MM_TO_PX * thumbScale}px`;

    // 内容容器 (使用自然像素尺寸, 由 transform 缩放到预览大小)
    const content = document.createElement('div');
    content.className = 'zq-print-preview-content';
    content.style.width = `${pagination.contentWidth}px`;
    content.style.height = `${pagination.contentHeight}px`;
    content.style.transform = `scale(${thumbScale * (pagination.effectiveScale / 100)})`;
    content.style.transformOrigin = 'top left';
    // 居中
    if (setup.horizontalCentered) {
      content.style.margin = '0 auto';
    }
    if (setup.verticalCentered) {
      content.style.position = 'relative';
      content.style.top = '50%';
      content.style.transform = `scale(${thumbScale * (pagination.effectiveScale / 100)}) translateY(-50%)`;
      content.style.transformOrigin = 'top center';
    }

    // 渲染单元格 (绝对定位, 缩放由外层 content 的 transform 处理)
    let y = 0;
    for (let r = page.startRow; r <= page.endRow; r++) {
      if (this.sheet.isRowHidden(r)) continue;
      const rh = this.sheet.getRowHeight(r);
      let x = 0;
      for (let c = page.startCol; c <= page.endCol; c++) {
        if (this.sheet.isColumnHidden(c)) continue;
        const cw = this.sheet.getColumnWidth(c);
        const val = this.sheet.getCellValue(r, c);
        const text = val === null ? '' : String(val);
        const cellEl = document.createElement('div');
        cellEl.className = 'zq-print-preview-cell';
        cellEl.textContent = text;
        cellEl.style.left = `${x}px`;
        cellEl.style.top = `${y}px`;
        cellEl.style.width = `${cw}px`;
        cellEl.style.height = `${rh}px`;

        // 应用样式 (字体/对齐/颜色)
        const style = this.sheet.getCellStyle(r, c);
        if (style.bold) cellEl.style.fontWeight = 'bold';
        if (style.italic) cellEl.style.fontStyle = 'italic';
        if (style.color) cellEl.style.color = style.color;
        if (style.backgroundColor) cellEl.style.background = style.backgroundColor;
        if (style.horizontalAlignment) {
          cellEl.style.textAlign = style.horizontalAlignment === 'general' ? 'left' : style.horizontalAlignment;
        }
        // 标题行/列加粗高亮
        const isTitleRow = page.titleRowRange && r >= page.titleRowRange.start && r <= page.titleRowRange.end;
        const isTitleCol = page.titleColRange && c >= page.titleColRange.start && c <= page.titleColRange.end;
        if (isTitleRow || isTitleCol) {
          cellEl.style.fontWeight = 'bold';
          cellEl.style.background = cellEl.style.background || '#f5f5f5';
        }
        content.appendChild(cellEl);
        x += cw;
      }
      y += rh;
    }

    // 由于 transform 缩放, content 的逻辑尺寸为 contentWidth/effectiveScale, 这里直接用绝对定位的像素 (已是逻辑像素)
    // 重置 transform 以使用 thumbScale * effectiveScale
    pageEl.appendChild(content);
    return pageEl;
  }

  /** 注入预览样式 (仅一次) */
  private injectPreviewStyles(): void {
    if (document.getElementById('zq-print-preview-styles')) return;
    const style = document.createElement('style');
    style.id = 'zq-print-preview-styles';
    style.textContent = `
      .zq-print-preview-overlay {
        position: fixed;
        top: 0; left: 0; right: 0; bottom: 0;
        background: rgba(0,0,0,0.5);
        z-index: 10000;
        display: flex;
        align-items: center;
        justify-content: center;
        font-family: -apple-system, "Microsoft YaHei", sans-serif;
      }
      .zq-print-preview-dialog {
        background: #fff;
        border-radius: 8px;
        width: 900px;
        max-width: 95vw;
        height: 90vh;
        max-height: 95vh;
        display: flex;
        flex-direction: column;
        overflow: hidden;
        box-shadow: 0 8px 32px rgba(0,0,0,0.3);
      }
      .zq-print-preview-header {
        display: flex;
        align-items: center;
        justify-content: space-between;
        padding: 12px 16px;
        border-bottom: 1px solid #e0e0e0;
        font-size: 14px;
        font-weight: bold;
      }
      .zq-print-preview-toolbar {
        display: flex;
        align-items: center;
        gap: 8px;
        padding: 8px 16px;
        border-bottom: 1px solid #e0e0e0;
      }
      .zq-print-preview-btn {
        padding: 4px 12px;
        border: 1px solid #d0d0d0;
        background: #fff;
        border-radius: 4px;
        cursor: pointer;
        font-size: 13px;
      }
      .zq-print-preview-btn:hover:not(:disabled) {
        background: #f0f0f0;
      }
      .zq-print-preview-btn:disabled {
        opacity: 0.5;
        cursor: not-allowed;
      }
      .zq-print-preview-primary {
        background: #4787f5;
        color: #fff;
        border-color: #4787f5;
        margin-left: auto;
      }
      .zq-print-preview-primary:hover {
        background: #3a78e0;
      }
      .zq-print-preview-pagelabel {
        font-size: 13px;
        color: #666;
        min-width: 100px;
        text-align: center;
      }
      .zq-print-preview-pages {
        flex: 1;
        overflow: auto;
        padding: 24px;
        background: #f5f5f5;
        display: flex;
        justify-content: center;
        align-items: flex-start;
      }
      .zq-print-preview-page {
        background: #fff;
        box-shadow: 0 2px 12px rgba(0,0,0,0.15);
        position: relative;
        box-sizing: content-box;
      }
      .zq-print-preview-content {
        position: relative;
        overflow: hidden;
      }
      .zq-print-preview-cell {
        position: absolute;
        box-sizing: border-box;
        border-right: 1px solid #e0e0e0;
        border-bottom: 1px solid #e0e0e0;
        padding: 1px 3px;
        overflow: hidden;
        white-space: nowrap;
        text-overflow: ellipsis;
        font-size: 12px;
        color: #333;
      }
    `;
    document.head.appendChild(style);
  }

  // ============================================================
  // 导出 PDF
  // ============================================================

  /**
   * 导出 PDF
   * 优先通过 Electron IPC 调用主进程打印 (契约: window.electronAPI.printToPDF(html) => Promise<boolean>)
   * 降级: 打开新窗口并调用浏览器打印 (可另存为 PDF)
   * @returns 是否成功
   */
  async exportPDF(): Promise<boolean> {
    const html = this.buildPrintHTML();

    // 1. 优先 Electron IPC
    const electronAPI = (window as unknown as {
      electronAPI?: {
        printToPDF?: (html: string) => Promise<boolean>;
      };
    }).electronAPI;
    if (electronAPI && typeof electronAPI.printToPDF === 'function') {
      try {
        return await electronAPI.printToPDF(html);
      } catch {
        return false;
      }
    }

    // 2. 降级: 新窗口浏览器打印
    const win = window.open('', '_blank');
    if (!win) return false;
    win.document.open();
    win.document.write(html);
    win.document.close();
    win.focus();
    // 等待渲染后触发打印
    setTimeout(() => {
      try {
        win.print();
      } catch {
        // 忽略打印异常
      }
    }, 400);
    return true;
  }

  /**
   * 构建打印 HTML (含所有页, 使用 @page 规则设置纸张/边距)
   */
  private buildPrintHTML(): string {
    const pagination = this.calculatePages();
    const setup = this.pageSetup;
    const paper = PAPER_SIZE_MM[setup.paperSize];
    const orientation = setup.orientation;
    const marginStr = `${setup.margins.top}mm ${setup.margins.right}mm ${setup.margins.bottom}mm ${setup.margins.left}mm`;
    const scale = setup.fitToPage ? pagination.effectiveScale : setup.scale;

    const pagesHTML = pagination.pages
      .map((p) => this.renderPageHTML(p))
      .join('\n');

    return `<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<title>打印 - ${this.escapeHTML(this.sheet.name)}</title>
<style>
  @page {
    size: ${paper.width}mm ${paper.height}mm ${orientation === 'landscape' ? 'landscape' : 'portrait'};
    margin: ${marginStr};
  }
  * { box-sizing: border-box; }
  html, body {
    margin: 0;
    padding: 0;
    font-family: -apple-system, "Microsoft YaHei", "Segoe UI", sans-serif;
    font-size: 12px;
    color: #333;
  }
  .zq-print-page {
    position: relative;
    width: ${pagination.contentWidth}px;
    height: ${pagination.contentHeight}px;
    page-break-after: always;
    overflow: hidden;
    transform: scale(${scale / 100});
    transform-origin: top left;
    ${setup.horizontalCentered ? 'margin: 0 auto;' : ''}
    ${setup.verticalCentered ? 'display: flex; align-items: center;' : ''}
  }
  .zq-print-page:last-child {
    page-break-after: auto;
  }
  .zq-print-cell {
    position: absolute;
    border-right: 1px solid #d0d0d0;
    border-bottom: 1px solid #d0d0d0;
    padding: 1px 3px;
    overflow: hidden;
    white-space: nowrap;
    text-overflow: ellipsis;
  }
</style>
</head>
<body>
${pagesHTML}
</body>
</html>`;
  }

  /** 渲染单页为 HTML 片段 */
  private renderPageHTML(page: PrintPage): string {
    const parts: string[] = [];
    parts.push(`<div class="zq-print-page">`);

    let y = 0;
    for (let r = page.startRow; r <= page.endRow; r++) {
      if (this.sheet.isRowHidden(r)) continue;
      const rh = this.sheet.getRowHeight(r);
      let x = 0;
      for (let c = page.startCol; c <= page.endCol; c++) {
        if (this.sheet.isColumnHidden(c)) continue;
        const cw = this.sheet.getColumnWidth(c);
        const val = this.sheet.getCellValue(r, c);
        const text = val === null ? '' : String(val);
        const style = this.sheet.getCellStyle(r, c);
        const cssText = this.buildCellCSS(style, r, c, page);
        parts.push(
          `<div class="zq-print-cell" style="left:${x}px;top:${y}px;width:${cw}px;height:${rh}px;${cssText}">${this.escapeHTML(
            text
          )}</div>`
        );
        x += cw;
      }
      y += rh;
    }

    parts.push('</div>');
    return parts.join('');
  }

  /** 构建单元格内联 CSS (字体/对齐/颜色) */
  private buildCellCSS(
    style: CellStyle,
    row: number,
    col: number,
    page: PrintPage
  ): string {
    const decls: string[] = [];
    if (style.bold) decls.push('font-weight:bold');
    if (style.italic) decls.push('font-style:italic');
    if (style.underline) decls.push('text-decoration:underline');
    if (style.color) decls.push(`color:${style.color}`);
    if (style.backgroundColor) decls.push(`background:${style.backgroundColor}`);
    if (style.fontSize) decls.push(`font-size:${style.fontSize}px`);
    if (style.fontFamily) decls.push(`font-family:${style.fontFamily}`);
    if (style.horizontalAlignment) {
      decls.push(`text-align:${style.horizontalAlignment === 'general' ? 'left' : style.horizontalAlignment}`);
    }
    if (style.verticalAlignment) {
      decls.push(`vertical-align:${style.verticalAlignment}`);
    }
    // 标题行/列加粗
    const isTitleRow = page.titleRowRange && row >= page.titleRowRange.start && row <= page.titleRowRange.end;
    const isTitleCol = page.titleColRange && col >= page.titleColRange.start && col <= page.titleColRange.end;
    if ((isTitleRow || isTitleCol) && !style.bold) {
      decls.push('font-weight:bold');
    }
    return decls.join(';');
  }

  /** HTML 转义 */
  private escapeHTML(s: string): string {
    return s
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#39;');
  }

  // ============================================================
  // 序列化
  // ============================================================

  /** 序列化打印配置 (用于持久化) */
  toJSON(): {
    printArea: PrintAreaRange | null;
    titleRows: PrintTitleRange | null;
    titleCols: PrintTitleRange | null;
    pageSetup: PrintPageSetup;
  } {
    return {
      printArea: this.printArea ? { ...this.printArea } : null,
      titleRows: this.titleRows ? { ...this.titleRows } : null,
      titleCols: this.titleCols ? { ...this.titleCols } : null,
      pageSetup: { ...this.pageSetup, margins: { ...this.pageSetup.margins } },
    };
  }

  /** 从 JSON 恢复打印配置 */
  fromJSON(data: {
    printArea?: PrintAreaRange | null;
    titleRows?: PrintTitleRange | null;
    titleCols?: PrintTitleRange | null;
    pageSetup?: Partial<PrintPageSetup>;
  }): void {
    this.printArea = data.printArea ? { ...data.printArea } : null;
    this.titleRows = data.titleRows ? { ...data.titleRows } : null;
    this.titleCols = data.titleCols ? { ...data.titleCols } : null;
    if (data.pageSetup) {
      this.pageSetup = {
        ...DEFAULT_PAGE_SETUP,
        ...data.pageSetup,
        margins: { ...DEFAULT_PAGE_SETUP.margins, ...(data.pageSetup.margins || {}) },
      };
    }
  }
}
