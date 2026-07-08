/**
 * 样式引擎 — 字体/对齐/边框/背景/数字格式
 * 全部原创实现
 */

import { CellStyle, BorderStyle } from './data-model';

export class StyleEngine {
  /** 默认字体 */
  readonly DEFAULT_FONT_FAMILY = '-apple-system, "Microsoft YaHei", "Segoe UI", sans-serif';
  readonly DEFAULT_FONT_SIZE = 13;
  readonly DEFAULT_COLOR = '#333333';
  readonly DEFAULT_BG_COLOR = '#ffffff';

  /**
   * 将 CellStyle 应用到 DOM 元素
   */
  applyStyle(el: HTMLElement, style: CellStyle): void {
    // 重置
    el.style.fontFamily = this.DEFAULT_FONT_FAMILY;
    el.style.fontSize = `${this.DEFAULT_FONT_SIZE}px`;
    el.style.color = this.DEFAULT_COLOR;
    el.style.background = this.DEFAULT_BG_COLOR;
    el.style.fontWeight = 'normal';
    el.style.fontStyle = 'normal';
    el.style.textDecoration = 'none';
    el.style.textAlign = 'left';
    el.style.verticalAlign = 'middle';
    el.style.borderTop = '';
    el.style.borderRight = '';
    el.style.borderBottom = '';
    el.style.borderLeft = '';
    el.style.padding = '2px 4px';
    el.style.overflow = 'hidden';
    el.style.whiteSpace = 'nowrap';
    el.style.textOverflow = 'ellipsis';

    if (!style) return;

    // 字体
    if (style.fontFamily) el.style.fontFamily = style.fontFamily;
    if (style.fontSize) el.style.fontSize = `${style.fontSize}px`;
    if (style.bold) el.style.fontWeight = 'bold';
    if (style.italic) el.style.fontStyle = 'italic';
    if (style.underline) el.style.textDecoration = 'underline';
    if (style.strikeThrough) {
      el.style.textDecoration = el.style.textDecoration === 'underline'
        ? 'underline line-through' : 'line-through';
    }

    // 颜色
    if (style.color) el.style.color = style.color;
    if (style.backgroundColor) el.style.background = style.backgroundColor;

    // 对齐
    if (style.horizontalAlignment) {
      el.style.textAlign = style.horizontalAlignment === 'general' ? 'left' : style.horizontalAlignment;
    }
    if (style.verticalAlignment) {
      el.style.verticalAlign = style.verticalAlignment === 'middle' ? 'middle' :
        style.verticalAlignment === 'top' ? 'top' : 'bottom';
    }

    // 换行
    if (style.wrapText) {
      el.style.whiteSpace = 'pre-wrap';
      el.style.wordBreak = 'break-all';
      el.style.overflow = 'hidden';
    }

    // 文字旋转
    if (style.textRotation && style.textRotation > 0) {
      el.style.transform = `rotate(${-style.textRotation}deg)`;
      el.style.transformOrigin = 'center center';
    }

    // 缩进
    if (style.indent && style.indent > 0) {
      el.style.paddingLeft = `${4 + style.indent * 8}px`;
    }

    // 边框
    if (style.borderTop) el.style.borderTop = this.borderToCss(style.borderTop);
    if (style.borderRight) el.style.borderRight = this.borderToCss(style.borderRight);
    if (style.borderBottom) el.style.borderBottom = this.borderToCss(style.borderBottom);
    if (style.borderLeft) el.style.borderLeft = this.borderToCss(style.borderLeft);
  }

  /**
   * 边框样式转 CSS
   */
  private borderToCss(border: BorderStyle): string {
    const width = border.style === 'thin' ? '1px' :
      border.style === 'medium' ? '2px' :
      border.style === 'thick' ? '3px' : '1px';
    const type = border.style === 'dashed' ? 'dashed' :
      border.style === 'dotted' ? 'dotted' :
      border.style === 'double' ? 'double' : 'solid';
    return `${width} ${type} ${border.color}`;
  }

  /**
   * 数字格式化
   */
  formatNumber(value: number, format: string): string {
    if (format === 'General' || !format) {
      // 通用格式: 整数直接显示, 小数最多 11 位
      if (Number.isInteger(value)) return String(value);
      return String(Math.round(value * 1e11) / 1e11);
    }

    // 数字格式: 0 / 0.00 / #,##0 / #,##0.00
    if (format === '0') {
      return String(Math.round(value));
    }
    if (format === '0.00') {
      return value.toFixed(2);
    }
    if (format === '#,##0') {
      return Math.round(value).toLocaleString('en-US');
    }
    if (format === '#,##0.00') {
      return value.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 });
    }

    // 百分比: 0% / 0.00%
    if (format === '0%') {
      return `${Math.round(value * 100)}%`;
    }
    if (format === '0.00%') {
      return `${(value * 100).toFixed(2)}%`;
    }

    // 科学计数法: 0.00E+00
    if (format === '0.00E+00') {
      return value.toExponential(2);
    }

    // 日期格式: yyyy-mm-dd / yyyy/mm/dd / yyyy-mm-dd hh:mm:ss
    if (format.includes('y') || format.includes('d') || format.includes('h') || format.includes('m')) {
      return this.formatDate(value, format);
    }

    // 货币格式: ¥#,##0.00 / $#,##0.00
    if (format.includes('¥') || format.includes('$')) {
      const symbol = format.includes('¥') ? '¥' : '$';
      const negative = value < 0;
      const abs = Math.abs(value);
      const formatted = abs.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 });
      return negative ? `${symbol}(${formatted})` : `${symbol}${formatted}`;
    }

    // 默认
    return String(value);
  }

  /**
   * 日期格式化
   * value 是从 1900-01-01 开始的天数 (Excel 日期序列号)
   */
  private formatDate(serial: number, format: string): string {
    // Excel 日期序列号: 1 = 1900-01-01
    // 但 Excel 有 1900 年闰年 bug (2月29日), 所以序列号 >= 60 需要 -1
    const adjusted = serial >= 60 ? serial - 1 : serial;
    const epoch = new Date(Date.UTC(1899, 11, 31)); // 1899-12-31
    const date = new Date(epoch.getTime() + adjusted * 86400000);

    const yyyy = date.getUTCFullYear();
    const mm = String(date.getUTCMonth() + 1).padStart(2, '0');
    const dd = String(date.getUTCDate()).padStart(2, '0');
    const hh = String(date.getUTCHours()).padStart(2, '0');
    const min = String(date.getUTCMinutes()).padStart(2, '0');
    const ss = String(date.getUTCSeconds()).padStart(2, '0');

    let result = format;
    result = result.replace(/yyyy/g, String(yyyy));
    result = result.replace(/yy/g, String(yyyy).slice(-2));
    result = result.replace(/mm/g, mm);
    result = result.replace(/dd/g, dd);
    result = result.replace(/hh/g, hh);
    result = result.replace(/ss/g, ss);
    // 单独的 m 表示月份 (不带前导零)
    result = result.replace(/(?<![m])m(?!m)/g, String(date.getUTCMonth() + 1));
    // 单独的 d 表示日期 (不带前导零)
    result = result.replace(/(?<![d])d(?!d)/g, String(date.getUTCDate()));

    return result;
  }

  /**
   * 合并样式 (base + override)
   */
  mergeStyles(base: CellStyle, override: Partial<CellStyle>): CellStyle {
    return { ...base, ...override };
  }

  /**
   * 创建默认样式
   */
  createDefaultStyle(): CellStyle {
    return {
      fontFamily: this.DEFAULT_FONT_FAMILY,
      fontSize: this.DEFAULT_FONT_SIZE,
      bold: false,
      italic: false,
      underline: false,
      strikeThrough: false,
      color: this.DEFAULT_COLOR,
      backgroundColor: this.DEFAULT_BG_COLOR,
      horizontalAlignment: 'general',
      verticalAlignment: 'middle',
      wrapText: false,
      numberFormat: 'General',
    };
  }

  /**
   * 判断两个样式是否相同
   */
  isSameStyle(a: CellStyle, b: CellStyle): boolean {
    const keys: (keyof CellStyle)[] = [
      'fontFamily', 'fontSize', 'bold', 'italic', 'underline', 'strikeThrough',
      'color', 'backgroundColor', 'horizontalAlignment', 'verticalAlignment',
      'wrapText', 'textRotation', 'numberFormat', 'indent',
    ];
    for (const key of keys) {
      if (a[key] !== b[key]) return false;
    }
    // 检查边框
    const borderKeys: (keyof CellStyle)[] = ['borderTop', 'borderRight', 'borderBottom', 'borderLeft'];
    for (const key of borderKeys) {
      const va = a[key] as BorderStyle | undefined;
      const vb = b[key] as BorderStyle | undefined;
      if (!va && !vb) continue;
      if (!va || !vb) return false;
      if (va.style !== vb.style || va.color !== vb.color) return false;
    }
    return true;
  }
}
