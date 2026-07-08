/**
 * 图表引擎层 — SVG 图表绘制与管理
 * 全部原创实现, 使用纯 SVG 绘制 (不依赖第三方库)
 */

import { SheetModel } from './data-model';

// ============================================================
// 类型定义
// ============================================================

/** 图表类型 */
export type ChartType = 'bar' | 'line' | 'pie';

/** 图表数据范围 */
export interface ChartDataRange {
  startRow: number;
  startCol: number;
  endRow: number;
  endCol: number;
}

/** 图表配置 */
export interface ChartConfig {
  id: string;
  type: ChartType;
  dataRange: ChartDataRange;
  title: string;
  x: number;              // 图表左上角 x 坐标 (像素, 相对于 canvas 内容)
  y: number;              // 图表左上角 y 坐标
  width: number;          // 图表区域宽度
  height: number;         // 图表区域高度
  colors?: string[];      // 系列颜色
  showLegend?: boolean;   // 是否显示图例
  showLabels?: boolean;   // 是否显示数据标签
}

/** 图表引擎事件回调 */
export interface ChartEngineCallbacks {
  /** 选中图表变化时触发 (null 表示取消选中) */
  onChartSelect?: (id: string | null) => void;
  /** 图表数据变化 (增删改) 时触发 */
  onChartChange?: () => void;
}

/** 调整手柄方向 */
type HandleDirection = 'nw' | 'n' | 'ne' | 'w' | 'e' | 'sw' | 's' | 'se';

/** 拖拽状态 */
interface ChartDragState {
  type: 'move' | 'resize';
  chartId: string;
  handle: HandleDirection;
  startX: number;
  startY: number;
  origX: number;
  origY: number;
  origWidth: number;
  origHeight: number;
}

/** 提取的图表数据 (柱状图/折线图) */
interface SeriesData {
  categories: string[];
  series: { name: string; values: number[] }[];
}

/** 提取的饼图数据 */
interface PieData {
  labels: string[];
  values: number[];
}

// ============================================================
// 图表引擎
// ============================================================

export class ChartEngine {
  private sheet: SheetModel;
  private container: HTMLElement;
  private callbacks: ChartEngineCallbacks;

  /** 图表渲染层 (覆盖在单元格上方, 随内容滚动) */
  private layer: HTMLDivElement;
  /** 选中边框和手柄容器 */
  private selectionOverlay: HTMLDivElement;
  /** 8 个调整手柄 */
  private handles = new Map<HandleDirection, HTMLDivElement>();

  /** 图表配置数据 */
  private charts = new Map<string, ChartConfig>();
  /** 图表 DOM 元素映射 */
  private chartEls = new Map<string, HTMLDivElement>();

  /** 当前选中的图表 ID */
  private selectedId: string | null = null;

  /** 拖拽/调整状态 */
  private dragState: ChartDragState | null = null;

  /** 全局事件监听是否已安装 */
  private globalListenersInstalled = false;

  /** 绑定的全局事件处理函数引用 */
  private boundMouseMove: (e: MouseEvent) => void;
  private boundMouseUp: () => void;

  /** 默认颜色系列 */
  private readonly DEFAULT_COLORS = [
    '#4787f5', '#52c41a', '#faad14', '#f5222d',
    '#722ed1', '#13c2c2', '#eb2f96', '#fa8c16',
    '#a0d911', '#36cfc9', '#4091f7', '#ffd666',
  ];

  /** SVG 命名空间 */
  private readonly SVG_NS = 'http://www.w3.org/2000/svg';

  constructor(
    container: HTMLElement,
    sheet: SheetModel,
    callbacks: ChartEngineCallbacks = {}
  ) {
    this.container = container;
    this.sheet = sheet;
    this.callbacks = callbacks;

    // 预绑定全局事件处理函数
    this.boundMouseMove = (e) => this.onMouseMove(e);
    this.boundMouseUp = () => this.onMouseUp();

    // 创建图表渲染层
    this.layer = document.createElement('div');
    this.layer.className = 'zq-sheet-chart-layer';
    this.container.appendChild(this.layer);

    // 创建选中覆盖层
    this.selectionOverlay = document.createElement('div');
    this.selectionOverlay.className = 'zq-sheet-chart-selection';
    this.selectionOverlay.style.display = 'none';
    this.layer.appendChild(this.selectionOverlay);

    this.setupStyles();
    this.setupGlobalListeners();
  }

  // ============================================================
  // 样式设置
  // ============================================================

  private setupStyles(): void {
    if (document.getElementById('zq-sheet-chart-styles')) return;
    const style = document.createElement('style');
    style.id = 'zq-sheet-chart-styles';
    style.textContent = `
      .zq-sheet-chart-layer {
        position: absolute;
        top: 0;
        left: 0;
        pointer-events: none;
        z-index: 30;
      }
      .zq-sheet-chart-container {
        position: absolute;
        pointer-events: auto;
        cursor: move;
        background: #fff;
        box-shadow: 0 2px 8px rgba(0, 0, 0, 0.12);
        border-radius: 4px;
        overflow: hidden;
      }
      .zq-sheet-chart-container svg {
        display: block;
        width: 100%;
        height: 100%;
        pointer-events: none;
      }
      .zq-sheet-chart-container.selected {
        outline: 2px solid #4787f5;
        outline-offset: 0;
      }
      .zq-sheet-chart-selection {
        position: absolute;
        pointer-events: none;
        z-index: 31;
        box-sizing: border-box;
      }
      .zq-sheet-chart-handle {
        position: absolute;
        width: 8px;
        height: 8px;
        background: #fff;
        border: 1px solid #4787f5;
        border-radius: 1px;
        pointer-events: auto;
        z-index: 32;
        box-sizing: border-box;
      }
      .zq-sheet-chart-handle.nw { cursor: nw-resize; }
      .zq-sheet-chart-handle.n  { cursor: n-resize; }
      .zq-sheet-chart-handle.ne { cursor: ne-resize; }
      .zq-sheet-chart-handle.w  { cursor: w-resize; }
      .zq-sheet-chart-handle.e  { cursor: e-resize; }
      .zq-sheet-chart-handle.sw { cursor: sw-resize; }
      .zq-sheet-chart-handle.s  { cursor: s-resize; }
      .zq-sheet-chart-handle.se { cursor: se-resize; }
    `;
    document.head.appendChild(style);
  }

  // ============================================================
  // 全局事件监听
  // ============================================================

  private setupGlobalListeners(): void {
    if (this.globalListenersInstalled) return;
    this.globalListenersInstalled = true;
    document.addEventListener('mousemove', this.boundMouseMove);
    document.addEventListener('mouseup', this.boundMouseUp);
  }

  /** 鼠标移动处理 (拖拽移动 / 调整大小) */
  private onMouseMove(e: MouseEvent): void {
    if (!this.dragState) return;

    const chart = this.charts.get(this.dragState.chartId);
    if (!chart) return;

    const dx = e.clientX - this.dragState.startX;
    const dy = e.clientY - this.dragState.startY;

    if (this.dragState.type === 'move') {
      // 移动图表: 直接更新 x, y
      chart.x = this.dragState.origX + dx;
      chart.y = this.dragState.origY + dy;
    } else if (this.dragState.type === 'resize') {
      // 调整大小: 根据手柄方向调整宽高和位置
      const handle = this.dragState.handle;
      let newWidth = this.dragState.origWidth;
      let newHeight = this.dragState.origHeight;
      let newX = this.dragState.origX;
      let newY = this.dragState.origY;

      // 水平方向
      if (handle.includes('e')) {
        newWidth = this.dragState.origWidth + dx;
      } else if (handle.includes('w')) {
        newWidth = this.dragState.origWidth - dx;
        newX = this.dragState.origX + dx;
      }

      // 垂直方向
      if (handle.includes('s')) {
        newHeight = this.dragState.origHeight + dy;
      } else if (handle.includes('n')) {
        newHeight = this.dragState.origHeight - dy;
        newY = this.dragState.origY + dy;
      }

      // 限制最小尺寸
      const minW = 100;
      const minH = 80;
      if (newWidth < minW) {
        if (handle.includes('w')) {
          newX = this.dragState.origX + (this.dragState.origWidth - minW);
        }
        newWidth = minW;
      }
      if (newHeight < minH) {
        if (handle.includes('n')) {
          newY = this.dragState.origY + (this.dragState.origHeight - minH);
        }
        newHeight = minH;
      }

      chart.x = newX;
      chart.y = newY;
      chart.width = newWidth;
      chart.height = newHeight;
    }

    this.renderChart(chart);
    this.updateSelectionOverlay();
    this.callbacks.onChartChange?.();
  }

  /** 鼠标松开: 结束拖拽 */
  private onMouseUp(): void {
    this.dragState = null;
  }

  // ============================================================
  // 图表 CRUD
  // ============================================================

  /** 生成唯一 ID */
  private generateId(): string {
    return `chart_${Date.now()}_${Math.random().toString(36).slice(2, 8)}`;
  }

  /** 获取颜色 (使用用户配置或默认颜色) */
  private getColor(config: ChartConfig, index: number): string {
    const colors = config.colors ?? this.DEFAULT_COLORS;
    return colors[index % colors.length];
  }

  /**
   * 添加图表
   * @param config 图表配置 (不含 id, id 将自动生成)
   * @returns 图表 ID
   */
  addChart(config: Omit<ChartConfig, 'id'>): string {
    const id = this.generateId();
    const chart: ChartConfig = { ...config, id };
    this.charts.set(id, chart);
    this.renderChart(chart);
    this.callbacks.onChartChange?.();
    return id;
  }

  /**
   * 删除图表
   * @param id 图表 ID
   */
  removeChart(id: string): void {
    if (!this.charts.has(id)) return;
    this.charts.delete(id);
    const el = this.chartEls.get(id);
    if (el) {
      el.remove();
      this.chartEls.delete(id);
    }
    if (this.selectedId === id) {
      this.deselect();
    }
    this.callbacks.onChartChange?.();
  }

  /**
   * 更新图表配置
   * @param id 图表 ID
   * @param config 要更新的配置字段
   */
  updateChart(id: string, config: Partial<Omit<ChartConfig, 'id'>>): void {
    const chart = this.charts.get(id);
    if (!chart) return;
    Object.assign(chart, config);
    this.renderChart(chart);
    if (this.selectedId === id) {
      this.updateSelectionOverlay();
    }
    this.callbacks.onChartChange?.();
  }

  /**
   * 获取图表配置
   * @param id 图表 ID
   */
  getChart(id: string): ChartConfig | null {
    return this.charts.get(id) ?? null;
  }

  /** 获取所有图表 */
  getAllCharts(): ChartConfig[] {
    return Array.from(this.charts.values());
  }

  // ============================================================
  // 选中管理
  // ============================================================

  /** 选中图表 (显示边框和 8 个调整手柄) */
  selectChart(id: string): void {
    this.selectedId = id;
    this.chartEls.forEach((el, chartId) => {
      el.classList.toggle('selected', chartId === id);
    });
    this.updateSelectionOverlay();
    this.callbacks.onChartSelect?.(id);
  }

  /** 取消选中 */
  deselect(): void {
    this.selectedId = null;
    this.chartEls.forEach(el => el.classList.remove('selected'));
    this.selectionOverlay.style.display = 'none';
    this.handles.forEach(h => h.style.display = 'none');
    this.callbacks.onChartSelect?.(null);
  }

  /** 获取当前选中的图表 ID */
  getSelectedId(): string | null {
    return this.selectedId;
  }

  // ============================================================
  // 数据提取 — 从 SheetModel 提取图表数据
  // ============================================================

  /** 从单元格值提取数字 */
  private toNumber(value: string | number | boolean | null): number {
    if (value === null) return 0;
    if (typeof value === 'number') return value;
    if (typeof value === 'boolean') return value ? 1 : 0;
    const num = parseFloat(value);
    return isNaN(num) ? 0 : num;
  }

  /**
   * 提取柱状图/折线图数据
   * 数据格式:
   *   - 第一行 (排除第一列) = 分类标签 (x 轴)
   *   - 第一列 (排除第一行) = 系列名称
   *   - 其余 = 数值数据
   */
  private extractSeriesData(range: ChartDataRange): SeriesData {
    const categories: string[] = [];
    const series: { name: string; values: number[] }[] = [];

    // 提取分类标签 (第一行, 跳过第一列)
    for (let c = range.startCol + 1; c <= range.endCol; c++) {
      const cell = this.sheet.getCell(range.startRow, c);
      categories.push(cell ? String(cell.value) : '');
    }

    // 提取数据系列 (后续每行一个系列)
    for (let r = range.startRow + 1; r <= range.endRow; r++) {
      const nameCell = this.sheet.getCell(r, range.startCol);
      const name = nameCell ? String(nameCell.value) : `系列${r - range.startRow}`;
      const values: number[] = [];
      for (let c = range.startCol + 1; c <= range.endCol; c++) {
        const cell = this.sheet.getCell(r, c);
        values.push(this.toNumber(cell ? cell.value : null));
      }
      series.push({ name, values });
    }

    return { categories, series };
  }

  /**
   * 提取饼图数据
   * 数据格式:
   *   - 第一列 = 标签
   *   - 第二列 = 数值
   *   - 每行一个数据点
   */
  private extractPieData(range: ChartDataRange): PieData {
    const labels: string[] = [];
    const values: number[] = [];

    for (let r = range.startRow; r <= range.endRow; r++) {
      const labelCell = this.sheet.getCell(r, range.startCol);
      const valueCell = this.sheet.getCell(r, range.startCol + 1);
      labels.push(labelCell ? String(labelCell.value) : '');
      values.push(this.toNumber(valueCell ? valueCell.value : null));
    }

    return { labels, values };
  }

  // ============================================================
  // SVG 辅助方法
  // ============================================================

  /** XML 文本转义 */
  private escapeXml(text: string): string {
    return text
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&apos;');
  }

  /** 极坐标转笛卡尔坐标 (角度以度为单位, 0 度 = 3 点钟方向, 顺时针) */
  private polarToCartesian(cx: number, cy: number, radius: number, angle: number): { x: number; y: number } {
    const rad = angle * Math.PI / 180;
    return {
      x: cx + radius * Math.cos(rad),
      y: cy + radius * Math.sin(rad),
    };
  }

  /** 生成饼图扇形路径 */
  private describeSlice(cx: number, cy: number, radius: number, startAngle: number, endAngle: number): string {
    const start = this.polarToCartesian(cx, cy, radius, startAngle);
    const end = this.polarToCartesian(cx, cy, radius, endAngle);
    const largeArcFlag = endAngle - startAngle > 180 ? '1' : '0';
    return `M ${cx} ${cy} L ${start.x.toFixed(2)} ${start.y.toFixed(2)} A ${radius} ${radius} 0 ${largeArcFlag} 1 ${end.x.toFixed(2)} ${end.y.toFixed(2)} Z`;
  }

  /** 格式化数字标签 (去除多余小数) */
  private formatNumber(n: number): string {
    if (Math.abs(n) >= 1000) {
      return Math.round(n).toString();
    }
    return n.toFixed(Math.abs(n) < 10 ? 1 : 0).replace(/\.0$/, '');
  }

  // ============================================================
  // SVG 图表绘制
  // ============================================================

  /**
   * 绘制柱状图
   * 布局: 标题(顶部) → 图表区(含 Y 轴和柱子) → X 轴标签 → 图例(底部)
   */
  private buildBarChartSVG(config: ChartConfig, data: SeriesData): string {
    const w = config.width;
    const h = config.height;
    const showLegend = config.showLegend ?? true;
    const showLabels = config.showLabels ?? false;

    // 布局尺寸
    const titleH = config.title ? 28 : 0;
    const legendH = showLegend && data.series.length > 0 ? 24 : 0;
    const yAxisW = 50;
    const xAxisH = 30;
    const padding = 8;

    // 图表绘制区域
    const chartX = yAxisW + padding;
    const chartY = titleH + padding;
    const chartW = w - chartX - padding;
    const chartH = h - chartY - xAxisH - legendH - padding;

    if (chartW <= 0 || chartH <= 0) {
      return `<rect width="${w}" height="${h}" fill="#fff"/><text x="${w / 2}" y="${h / 2}" text-anchor="middle" fill="#999" font-size="13">区域太小</text>`;
    }

    // 计算最大值
    let maxVal = 0;
    for (const s of data.series) {
      for (const v of s.values) {
        if (v > maxVal) maxVal = v;
      }
    }
    if (maxVal === 0) maxVal = 1;

    // Y 轴刻度 (5 等分)
    const yTicks = 5;
    const yTickStep = maxVal / yTicks;
    const yTickValues: number[] = [];
    for (let i = 0; i <= yTicks; i++) {
      yTickValues.push(yTickStep * i);
    }

    let svg = '';

    // 标题
    if (config.title) {
      svg += `<text x="${w / 2}" y="${titleH - 6}" text-anchor="middle" font-size="14" font-weight="bold" fill="#333">${this.escapeXml(config.title)}</text>`;
    }

    // Y 轴网格线和刻度
    for (let i = 0; i <= yTicks; i++) {
      const y = chartY + chartH - (chartH * i / yTicks);
      const val = yTickValues[i];
      // 网格线
      svg += `<line x1="${chartX}" y1="${y.toFixed(1)}" x2="${chartX + chartW}" y2="${y.toFixed(1)}" stroke="#e8e8e8" stroke-width="1"/>`;
      // 刻度文字
      svg += `<text x="${chartX - 4}" y="${(y + 4).toFixed(1)}" text-anchor="end" font-size="11" fill="#999">${this.formatNumber(val)}</text>`;
    }

    // X 轴和 Y 轴线
    svg += `<line x1="${chartX}" y1="${chartY}" x2="${chartX}" y2="${chartY + chartH}" stroke="#d0d0d0" stroke-width="1"/>`;
    svg += `<line x1="${chartX}" y1="${(chartY + chartH).toFixed(1)}" x2="${chartX + chartW}" y2="${(chartY + chartH).toFixed(1)}" stroke="#d0d0d0" stroke-width="1"/>`;

    // 柱子
    const catCount = data.categories.length;
    if (catCount > 0 && data.series.length > 0) {
      const groupW = chartW / catCount;
      const seriesCount = data.series.length;
      const barGap = 2;
      const barW = Math.max(2, (groupW - barGap * (seriesCount + 1)) / seriesCount);

      for (let ci = 0; ci < catCount; ci++) {
        // X 轴分类标签
        const labelX = chartX + groupW * ci + groupW / 2;
        const labelY = chartY + chartH + 14;
        svg += `<text x="${labelX.toFixed(1)}" y="${labelY}" text-anchor="middle" font-size="11" fill="#666">${this.escapeXml(data.categories[ci])}</text>`;

        // 每个系列的柱子
        for (let si = 0; si < seriesCount; si++) {
          const val = data.series[si].values[ci] ?? 0;
          const barH = (val / maxVal) * chartH;
          const barX = chartX + groupW * ci + barGap + si * (barW + barGap);
          const barY = chartY + chartH - barH;
          const color = this.getColor(config, si);

          svg += `<rect x="${barX.toFixed(1)}" y="${barY.toFixed(1)}" width="${barW.toFixed(1)}" height="${barH.toFixed(1)}" fill="${color}" rx="1"/>`;

          // 数据标签
          if (showLabels && val !== 0) {
            svg += `<text x="${(barX + barW / 2).toFixed(1)}" y="${(barY - 3).toFixed(1)}" text-anchor="middle" font-size="10" fill="#666">${this.formatNumber(val)}</text>`;
          }
        }
      }
    }

    // 图例
    if (showLegend && data.series.length > 0) {
      const legendY = h - legendH + 12;
      let legendX = padding;
      for (let si = 0; si < data.series.length; si++) {
        const color = this.getColor(config, si);
        const name = data.series[si].name;
        const textW = name.length * 12 + 20;
        svg += `<rect x="${legendX}" y="${legendY - 8}" width="12" height="12" fill="${color}" rx="1"/>`;
        svg += `<text x="${legendX + 16}" y="${legendY}" font-size="11" fill="#666">${this.escapeXml(name)}</text>`;
        legendX += textW;
        if (legendX > w - 60) break; // 超出宽度则截断
      }
    }

    return svg;
  }

  /**
   * 绘制折线图
   * 布局: 标题(顶部) → 图表区(含 Y 轴和折线) → X 轴标签 → 图例(底部)
   */
  private buildLineChartSVG(config: ChartConfig, data: SeriesData): string {
    const w = config.width;
    const h = config.height;
    const showLegend = config.showLegend ?? true;
    const showLabels = config.showLabels ?? false;

    // 布局尺寸
    const titleH = config.title ? 28 : 0;
    const legendH = showLegend && data.series.length > 0 ? 24 : 0;
    const yAxisW = 50;
    const xAxisH = 30;
    const padding = 8;

    // 图表绘制区域
    const chartX = yAxisW + padding;
    const chartY = titleH + padding;
    const chartW = w - chartX - padding;
    const chartH = h - chartY - xAxisH - legendH - padding;

    if (chartW <= 0 || chartH <= 0) {
      return `<rect width="${w}" height="${h}" fill="#fff"/><text x="${w / 2}" y="${h / 2}" text-anchor="middle" fill="#999" font-size="13">区域太小</text>`;
    }

    // 计算最大值
    let maxVal = 0;
    for (const s of data.series) {
      for (const v of s.values) {
        if (v > maxVal) maxVal = v;
      }
    }
    if (maxVal === 0) maxVal = 1;

    // Y 轴刻度
    const yTicks = 5;
    const yTickStep = maxVal / yTicks;

    let svg = '';

    // 标题
    if (config.title) {
      svg += `<text x="${w / 2}" y="${titleH - 6}" text-anchor="middle" font-size="14" font-weight="bold" fill="#333">${this.escapeXml(config.title)}</text>`;
    }

    // Y 轴网格线和刻度
    for (let i = 0; i <= yTicks; i++) {
      const y = chartY + chartH - (chartH * i / yTicks);
      const val = yTickStep * i;
      svg += `<line x1="${chartX}" y1="${y.toFixed(1)}" x2="${chartX + chartW}" y2="${y.toFixed(1)}" stroke="#e8e8e8" stroke-width="1"/>`;
      svg += `<text x="${chartX - 4}" y="${(y + 4).toFixed(1)}" text-anchor="end" font-size="11" fill="#999">${this.formatNumber(val)}</text>`;
    }

    // X 轴和 Y 轴线
    svg += `<line x1="${chartX}" y1="${chartY}" x2="${chartX}" y2="${chartY + chartH}" stroke="#d0d0d0" stroke-width="1"/>`;
    svg += `<line x1="${chartX}" y1="${(chartY + chartH).toFixed(1)}" x2="${chartX + chartW}" y2="${(chartY + chartH).toFixed(1)}" stroke="#d0d0d0" stroke-width="1"/>`;

    // 折线
    const catCount = data.categories.length;
    if (catCount > 0 && data.series.length > 0) {
      const stepX = chartW / catCount;

      for (let si = 0; si < data.series.length; si++) {
        const series = data.series[si];
        const color = this.getColor(config, si);

        // 构建折线路径
        let pathD = '';
        const points: { x: number; y: number; val: number }[] = [];

        for (let ci = 0; ci < catCount; ci++) {
          const val = series.values[ci] ?? 0;
          const px = chartX + stepX * ci + stepX / 2;
          const py = chartY + chartH - (val / maxVal) * chartH;
          points.push({ x: px, y: py, val });
          if (ci === 0) {
            pathD += `M ${px.toFixed(1)} ${py.toFixed(1)}`;
          } else {
            pathD += ` L ${px.toFixed(1)} ${py.toFixed(1)}`;
          }
        }

        // 绘制折线
        svg += `<path d="${pathD}" fill="none" stroke="${color}" stroke-width="2" stroke-linejoin="round" stroke-linecap="round"/>`;

        // 绘制数据点
        for (const pt of points) {
          svg += `<circle cx="${pt.x.toFixed(1)}" cy="${pt.y.toFixed(1)}" r="3" fill="#fff" stroke="${color}" stroke-width="1.5"/>`;
          if (showLabels && pt.val !== 0) {
            svg += `<text x="${pt.x.toFixed(1)}" y="${(pt.y - 8).toFixed(1)}" text-anchor="middle" font-size="10" fill="#666">${this.formatNumber(pt.val)}</text>`;
          }
        }
      }

      // X 轴分类标签
      for (let ci = 0; ci < catCount; ci++) {
        const labelX = chartX + stepX * ci + stepX / 2;
        const labelY = chartY + chartH + 14;
        svg += `<text x="${labelX.toFixed(1)}" y="${labelY}" text-anchor="middle" font-size="11" fill="#666">${this.escapeXml(data.categories[ci])}</text>`;
      }
    }

    // 图例
    if (showLegend && data.series.length > 0) {
      const legendY = h - legendH + 12;
      let legendX = padding;
      for (let si = 0; si < data.series.length; si++) {
        const color = this.getColor(config, si);
        const name = data.series[si].name;
        const textW = name.length * 12 + 24;
        svg += `<line x1="${legendX}" y1="${legendY - 2}" x2="${legendX + 16}" y2="${legendY - 2}" stroke="${color}" stroke-width="2"/>`;
        svg += `<circle cx="${legendX + 8}" cy="${legendY - 2}" r="3" fill="#fff" stroke="${color}" stroke-width="1.5"/>`;
        svg += `<text x="${legendX + 20}" y="${legendY}" font-size="11" fill="#666">${this.escapeXml(name)}</text>`;
        legendX += textW;
        if (legendX > w - 60) break;
      }
    }

    return svg;
  }

  /**
   * 绘制饼图
   * 布局: 标题(顶部) → 饼图区域 → 图例(底部)
   */
  private buildPieChartSVG(config: ChartConfig, data: PieData): string {
    const w = config.width;
    const h = config.height;
    const showLegend = config.showLegend ?? true;
    const showLabels = config.showLabels ?? true;

    // 布局尺寸
    const titleH = config.title ? 28 : 0;
    const legendH = showLegend && data.labels.length > 0 ? 24 : 0;
    const padding = 8;

    // 饼图区域
    const pieAreaY = titleH + padding;
    const pieAreaH = h - pieAreaY - legendH - padding;
    const pieAreaW = w - padding * 2;
    const pieCx = w / 2;
    const pieCy = pieAreaY + pieAreaH / 2;
    const pieRadius = Math.max(10, Math.min(pieAreaW, pieAreaH) / 2 - padding);

    if (pieRadius <= 10) {
      return `<rect width="${w}" height="${h}" fill="#fff"/><text x="${w / 2}" y="${h / 2}" text-anchor="middle" fill="#999" font-size="13">区域太小</text>`;
    }

    // 计算总和
    const total = data.values.reduce((sum, v) => sum + v, 0);

    let svg = '';

    // 标题
    if (config.title) {
      svg += `<text x="${w / 2}" y="${titleH - 6}" text-anchor="middle" font-size="14" font-weight="bold" fill="#333">${this.escapeXml(config.title)}</text>`;
    }

    if (total <= 0) {
      svg += `<text x="${pieCx}" y="${pieCy}" text-anchor="middle" fill="#999" font-size="13">无数据</text>`;
      return svg;
    }

    // 绘制扇形
    let currentAngle = -90; // 从 12 点钟方向开始
    const labelData: { angle: number; label: string; value: number; percent: number }[] = [];

    for (let i = 0; i < data.values.length; i++) {
      const value = data.values[i];
      if (value <= 0) continue;
      const sliceAngle = (value / total) * 360;
      const endAngle = currentAngle + sliceAngle;
      const color = this.getColor(config, i);

      // 扇形路径
      svg += `<path d="${this.describeSlice(pieCx, pieCy, pieRadius, currentAngle, endAngle)}" fill="${color}" stroke="#fff" stroke-width="1"/>`;

      // 记录标签数据
      const midAngle = currentAngle + sliceAngle / 2;
      labelData.push({
        angle: midAngle,
        label: data.labels[i] ?? `项${i + 1}`,
        value: value,
        percent: (value / total) * 100,
      });

      currentAngle = endAngle;
    }

    // 数据标签 (在扇形外侧)
    if (showLabels) {
      for (const ld of labelData) {
        const labelPos = this.polarToCartesian(pieCx, pieCy, pieRadius + 14, ld.angle);
        const anchor = labelPos.x > pieCx ? 'start' : 'end';
        svg += `<text x="${labelPos.x.toFixed(1)}" y="${labelPos.y.toFixed(1)}" text-anchor="${anchor}" font-size="10" fill="#666">${this.escapeXml(ld.label)} ${ld.percent.toFixed(1)}%</text>`;
        // 引导线
        const lineStart = this.polarToCartesian(pieCx, pieCy, pieRadius, ld.angle);
        const lineEnd = this.polarToCartesian(pieCx, pieCy, pieRadius + 10, ld.angle);
        svg += `<line x1="${lineStart.x.toFixed(1)}" y1="${lineStart.y.toFixed(1)}" x2="${lineEnd.x.toFixed(1)}" y2="${lineEnd.y.toFixed(1)}" stroke="#ccc" stroke-width="0.5"/>`;
      }
    }

    // 图例
    if (showLegend && data.labels.length > 0) {
      const legendY = h - legendH + 12;
      let legendX = padding;
      for (let i = 0; i < data.labels.length; i++) {
        if (data.values[i] <= 0) continue;
        const color = this.getColor(config, i);
        const name = data.labels[i] ?? `项${i + 1}`;
        const textW = name.length * 12 + 20;
        svg += `<rect x="${legendX}" y="${legendY - 8}" width="12" height="12" fill="${color}" rx="1"/>`;
        svg += `<text x="${legendX + 16}" y="${legendY}" font-size="11" fill="#666">${this.escapeXml(name)}</text>`;
        legendX += textW;
        if (legendX > w - 60) break;
      }
    }

    return svg;
  }

  /** 根据图表类型生成 SVG 内容 */
  private buildChartSVG(config: ChartConfig): string {
    const w = config.width;
    const h = config.height;

    let content = '';
    if (config.type === 'bar') {
      const data = this.extractSeriesData(config.dataRange);
      content = this.buildBarChartSVG(config, data);
    } else if (config.type === 'line') {
      const data = this.extractSeriesData(config.dataRange);
      content = this.buildLineChartSVG(config, data);
    } else if (config.type === 'pie') {
      const data = this.extractPieData(config.dataRange);
      content = this.buildPieChartSVG(config, data);
    }

    return `<svg xmlns="http://www.w3.org/2000/svg" width="${w}" height="${h}" viewBox="0 0 ${w} ${h}"><rect width="${w}" height="${h}" fill="#fff"/>${content}</svg>`;
  }

  // ============================================================
  // DOM 渲染
  // ============================================================

  /** 渲染图表 (创建或更新 DOM) */
  private renderChart(config: ChartConfig): void {
    let el = this.chartEls.get(config.id);
    if (!el) {
      el = document.createElement('div');
      el.className = 'zq-sheet-chart-container';
      el.dataset.chartId = config.id;

      // 鼠标按下: 选中并启动拖拽
      el.addEventListener('mousedown', (e) => {
        e.stopPropagation();
        this.selectChart(config.id);
        this.startDrag(config.id, 'move', e);
      });

      this.layer.appendChild(el);
      this.chartEls.set(config.id, el);
    }

    // 更新位置和尺寸
    el.style.left = `${config.x}px`;
    el.style.top = `${config.y}px`;
    el.style.width = `${config.width}px`;
    el.style.height = `${config.height}px`;

    // 生成 SVG 并渲染
    el.innerHTML = this.buildChartSVG(config);

    if (this.selectedId === config.id) {
      el.classList.add('selected');
      this.updateSelectionOverlay();
    }
  }

  /** 更新选中覆盖层 (边框 + 8 个调整手柄) */
  private updateSelectionOverlay(): void {
    if (!this.selectedId) {
      this.selectionOverlay.style.display = 'none';
      this.handles.forEach(h => h.style.display = 'none');
      return;
    }

    const chart = this.charts.get(this.selectedId);
    if (!chart) {
      this.selectionOverlay.style.display = 'none';
      this.handles.forEach(h => h.style.display = 'none');
      return;
    }

    // 更新边框位置
    this.selectionOverlay.style.display = 'block';
    this.selectionOverlay.style.left = `${chart.x}px`;
    this.selectionOverlay.style.top = `${chart.y}px`;
    this.selectionOverlay.style.width = `${chart.width}px`;
    this.selectionOverlay.style.height = `${chart.height}px`;
    this.selectionOverlay.style.border = '2px solid #4787f5';

    // 创建/更新 8 个调整手柄
    const directions: HandleDirection[] = ['nw', 'n', 'ne', 'w', 'e', 'sw', 's', 'se'];
    const handlePositions: Record<HandleDirection, { left: string; top: string }> = {
      nw: { left: '-4px', top: '-4px' },
      n:  { left: 'calc(50% - 4px)', top: '-4px' },
      ne: { left: 'calc(100% - 4px)', top: '-4px' },
      w:  { left: '-4px', top: 'calc(50% - 4px)' },
      e:  { left: 'calc(100% - 4px)', top: 'calc(50% - 4px)' },
      sw: { left: '-4px', top: 'calc(100% - 4px)' },
      s:  { left: 'calc(50% - 4px)', top: 'calc(100% - 4px)' },
      se: { left: 'calc(100% - 4px)', top: 'calc(100% - 4px)' },
    };

    for (const dir of directions) {
      let handle = this.handles.get(dir);
      if (!handle) {
        handle = document.createElement('div');
        handle.className = `zq-sheet-chart-handle ${dir}`;
        handle.dataset.handle = dir;
        handle.addEventListener('mousedown', (e) => {
          e.stopPropagation();
          e.preventDefault();
          this.selectChart(chart.id);
          this.startDrag(chart.id, 'resize', e, dir);
        });
        this.selectionOverlay.appendChild(handle);
        this.handles.set(dir, handle);
      }
      handle.style.display = 'block';
      handle.style.left = handlePositions[dir].left;
      handle.style.top = handlePositions[dir].top;
    }
  }

  // ============================================================
  // 拖拽启动
  // ============================================================

  /** 启动拖拽 (移动或调整大小) */
  private startDrag(chartId: string, type: 'move' | 'resize', e: MouseEvent, handle?: HandleDirection): void {
    const chart = this.charts.get(chartId);
    if (!chart) return;

    this.dragState = {
      type,
      chartId,
      handle: handle ?? 'se',
      startX: e.clientX,
      startY: e.clientY,
      origX: chart.x,
      origY: chart.y,
      origWidth: chart.width,
      origHeight: chart.height,
    };
  }

  // ============================================================
  // 序列化
  // ============================================================

  /** 从 JSON 加载图表数据 (覆盖现有数据) */
  loadFromJSON(charts: unknown[]): void {
    this.clear();
    for (const item of charts) {
      const obj = item as Record<string, unknown>;
      this.charts.set(obj.id as string, {
        id: obj.id as string,
        type: obj.type as ChartType,
        dataRange: obj.dataRange as ChartDataRange,
        title: (obj.title as string) ?? '',
        x: obj.x as number,
        y: obj.y as number,
        width: obj.width as number,
        height: obj.height as number,
        colors: obj.colors as string[] | undefined,
        showLegend: obj.showLegend as boolean | undefined,
        showLabels: obj.showLabels as boolean | undefined,
      });
    }
    this.renderAll();
  }

  /** 序列化为 JSON 数组 (存入 Sheet.charts 字段) */
  toJSON(): unknown[] {
    return Array.from(this.charts.values());
  }

  // ============================================================
  // 刷新与销毁
  // ============================================================

  /** 清除所有图表 */
  clear(): void {
    this.charts.clear();
    this.chartEls.forEach(el => el.remove());
    this.chartEls.clear();
    this.deselect();
  }

  /** 重新渲染所有图表 */
  renderAll(): void {
    // 清除旧 DOM
    this.chartEls.forEach(el => el.remove());
    this.chartEls.clear();
    this.deselect();

    for (const chart of this.charts.values()) {
      this.renderChart(chart);
    }
  }

  /** 刷新所有图表 SVG (单元格数据变化时调用) */
  refresh(): void {
    for (const chart of this.charts.values()) {
      this.renderChart(chart);
    }
  }

  /** 刷新指定图表 (单元格数据变化时调用) */
  refreshChart(id: string): void {
    const chart = this.charts.get(id);
    if (chart) {
      this.renderChart(chart);
    }
  }

  /** 更新工作表引用 (切换工作表时调用) */
  setSheet(sheet: SheetModel): void {
    this.sheet = sheet;
    this.renderAll();
  }

  /** 销毁, 移除所有 DOM 和事件监听 */
  destroy(): void {
    this.clear();
    this.selectionOverlay.remove();
    this.layer.remove();
    if (this.globalListenersInstalled) {
      document.removeEventListener('mousemove', this.boundMouseMove);
      document.removeEventListener('mouseup', this.boundMouseUp);
      this.globalListenersInstalled = false;
    }
  }
}
