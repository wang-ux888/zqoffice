# ZQOffice

C++ Office 文档转换引擎，支持 Office 文件与 JSON 格式的双向转换。

## 支持的格式

- **Excel**: xlsx, xls, csv
- **PowerPoint**: pptx, ppt
- **Word**: docx

## 功能特性

- 高性能 C++ 原生模块
- 支持大文件处理（百万级单元格）
- 完整的 OOXML 规范支持
- 跨平台支持（Windows/macOS/Linux）
- Node-API 绑定，易于集成

## 安装

```bash
npm install zqoffice
```

## 使用

### 基本用法

```javascript
const zqoffice = require('zqoffice');

// 初始化引擎
zqoffice.zqofficeInitialize(null, 0);

// Excel 转 JSON
const json = zqoffice.xlsxToZQSheet('input.xlsx', null, null, '{}');
console.log(JSON.parse(json));

// JSON 转 Excel
const xlsxJson = JSON.stringify({
  sheets: [{
    name: 'Sheet1',
    cells: [
      { row: 0, column: 0, type: 'string', value: 'Hello' },
      { row: 0, column: 1, type: 'number', value: 42 }
    ],
    mergedCells: [],
    rowCount: 10,
    columnCount: 5
  }],
  coreProperties: { title: 'Test', creator: 'ZQOffice', created: '', modified: '' }
});

zqoffice.zqSheetToXlsx(xlsxJson, 'output.xlsx', null, null, '{}');

// 反初始化引擎
zqoffice.zqofficeUninitialize();
```

### 支持的 API

#### 引擎生命周期
- `zqofficeInitialize(workingDirectory, mode)` - 初始化引擎
- `zqofficeUninitialize()` - 反初始化引擎
- `zqofficeDetectFileFormat(filePath)` - 检测文件格式

#### Excel 转换
- `xlsxToZQSheet(filePath, dataDirectory, reserved, optionsJson)` - xlsx → JSON
- `zqSheetToXlsx(clientVarsJson, outputPath, dataDirectory, reserved, optionsJson)` - JSON → xlsx

#### PowerPoint 转换
- `pptxToZQSlide(filePath, dataDirectory, reserved, optionsJson)` - pptx → JSON
- `zqSlideToPptx(clientVarsJson, outputPath, dataDirectory, reserved, optionsJson)` - JSON → pptx

#### Word 转换
- `docxToZQDoc(filePath, dataDirectory, reserved, optionsJson)` - docx → JSON
- `zqDocToDocx(clientVarsJson, outputPath, dataDirectory, reserved, optionsJson)` - JSON → docx

#### 通用导出
- `zqofficeExport(sourcePath, outputPath, optionsJson)` - Office → JSON 文件
- `zqofficeImport(clientVarsJson, outputPath, optionsJson)` - JSON → Office 文件
- `zqExportFile(sourcePath, outputPath, formatName, optionsJson)` - Office → Office

#### PowerPoint 流式读取
- `createPowerPointReader(filePath)` - 创建流式读取器
- `destroyPowerPointReader(reader)` - 销毁流式读取器

## JSON 数据结构

### Excel (ZQ Sheet JSON)
```json
{
  "sheets": [{
    "name": "Sheet1",
    "cells": [
      { "row": 0, "column": 0, "type": "string", "value": "Hello" },
      { "row": 0, "column": 1, "type": "number", "value": 42 }
    ],
    "mergedCells": [],
    "rowCount": 10,
    "columnCount": 5
  }],
  "coreProperties": {
    "title": "Document Title",
    "creator": "Author",
    "created": "2026-01-01T00:00:00.000Z",
    "modified": "2026-01-01T00:00:00.000Z"
  }
}
```

### PowerPoint (ZQ Slide JSON)
```json
{
  "slides": [{
    "index": 0,
    "layout": "blank",
    "shapes": [{
      "id": "shape_0",
      "type": "text",
      "name": "Title 1",
      "geometry": { "x": 100, "y": 50, "width": 600, "height": 80 },
      "text": {
        "paragraphs": [{
          "alignment": "center",
          "runs": [{ "text": "Hello", "font": { "family": "Calibri", "size": 32 } }]
        }]
      }
    }],
    "notes": "Speaker notes"
  }],
  "slideSize": { "width": 9144000, "height": 6858000 },
  "coreProperties": { "title": "", "creator": "", "created": "", "modified": "" }
}
```

### Word (ZQ Doc JSON)
```json
{
  "blocks": [{
    "id": "block_0",
    "type": "paragraph",
    "paragraph": {
      "style": "Normal",
      "alignment": "left",
      "runs": [{ "text": "Hello World", "font": { "family": "Calibri", "size": 11 } }]
    }
  }],
  "coreProperties": { "title": "", "creator": "", "created": "", "modified": "" }
}
```

## 构建

### 依赖
- CMake 3.18+
- C++17 编译器（MSVC 2019+ / clang 10+ / gcc 9+）
- Node.js 18+

### 编译
```bash
npm run build
```

### 运行测试
```bash
npm test
```

## 许可证

Apache License 2.0
