/**
 * 示例：批量转换
 *
 * 批量将 xlsx 文件转换为 JSON
 */

const { ZQOffice } = require('@zqoffice/bridge');

async function main() {
  const zq = new ZQOffice({ workers: 8 });

  try {
    const result = await zq.batchConvert([
      { input: 'a.xlsx', output: 'a.json' },
      { input: 'b.xlsx', output: 'b.json' },
      { input: 'c.xlsx', output: 'c.json' },
    ], 'xlsx');

    console.log(`成功: ${result.success}, 失败: ${result.failed}`);
    if (result.errors.length) console.error('错误:', result.errors);
  } finally {
    await zq.destroy();
  }
}

main().catch(console.error);
