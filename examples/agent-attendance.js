/**
 * 示例：Agent 考勤自动化
 *
 * 读取考勤表 → 标记迟到 → 生成汇总
 */

const { ZQOffice } = require('@zqoffice/bridge');

async function main() {
  const zq = new ZQOffice({ workers: 4 });

  try {
    // 1. 读取 Excel
    const data = await zq.xlsxToJson('attendance.xlsx');
    console.log(`读取成功: ${data.sheets[0].cells.length} 个单元格`);

    // 2. 执行操作序列
    await zq.applyOperations('attendance.xlsx', 'output.xlsx', [
      { action: 'setCell', params: { sheet: 0, row: 0, col: 5, value: '状态' } },
      { action: 'setFormula', params: { sheet: 0, row: 1, col: 5, formula: '=IF(D2>3,"异常","正常")' } },
      { action: 'setCellStyle', params: { sheet: 0, row: 0, col: 5, style: { bold: true, backgroundColor: '4472C4', color: 'FFFFFF' } } },
    ]);
    console.log('操作完成，已保存到 output.xlsx');
  } finally {
    await zq.destroy();
  }
}

main().catch(console.error);
