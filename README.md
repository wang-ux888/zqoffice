# ZQOffice

Office 文档转换与编辑引擎，支持 xlsx / pptx / docx 双向转换和结构化编辑。

## 项目结构

```
zqoffice/
├── zqoffice_twc/       C++ 转换引擎（Office ↔ JSON）
├── zqoffice_edit/      TypeScript 编辑引擎（Sheet/Doc/Slide）
├── packages/bridge/    桥接层（Worker Pool + 统一 API）
├── mcp-server/         MCP Tool 服务（AI Agent 集成）
├── server/             REST API 服务（HTTP 接口）
└── examples/           使用示例
```

## 快速开始

### 安装

```bash
pnpm install
pnpm build
```

### MCP Server（AI Agent 集成）

```bash
pnpm start:mcp
```

### REST API Server

```bash
pnpm start:server
```

### 编程调用

```typescript
import { ZQOffice } from '@zqoffice/bridge';

const zq = new ZQOffice({ workers: 4 });

// 读取 → 编辑 → 保存
await zq.editXlsx('input.xlsx', (data) => {
  data.sheets[0].cells.push({ row: 0, column: 0, type: 'string', value: 'Hello' });
  return data;
}, 'output.xlsx');

await zq.destroy();
```

## API

| 方法 | 说明 |
|------|------|
| `xlsxToJson(path)` | xlsx → JSON |
| `jsonToXlsx(data, path)` | JSON → xlsx |
| `pptxToJson(path)` | pptx → JSON |
| `jsonToPptx(data, path)` | JSON → pptx |
| `docxToJson(path)` | docx → JSON |
| `jsonToDocx(data, path)` | JSON → docx |
| `editXlsx(in, fn, out)` | 读取→编辑→保存 |
| `applyOperations(in, out, ops)` | 操作序列执行 |
| `batchConvert(tasks, fmt)` | 批量并发转换 |

## License

MIT
