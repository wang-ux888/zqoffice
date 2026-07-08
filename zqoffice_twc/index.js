// =============================================================================
// zqoffice/index.js
//
// ZQOffice Node.js 包入口
//
// 加载原生模块 zqoffice.node，提供 15 个 C API 的 JavaScript 绑定
// 支持开发模式（从 build 目录加载）和生产模式（从 prebuilds 加载）
// =============================================================================

'use strict';

const path = require('path');
const fs = require('fs');

/**
 * 根据平台和架构推断原生模块文件名
 */
function getNativeModuleName() {
    const platform = process.platform;
    const arch = process.arch;
    return 'zqoffice.node';
}

/**
 * 查找原生模块路径
 * 搜索顺序：
 *   1. prebuilds/<platform>-<arch>/zqoffice.node（prebuildify 产物）
 *   2. build/out/bin/Release/zqoffice.node（CMake 构建产物，开发模式）
 *   3. build/Release/zqoffice.node（node-gyp 构建产物）
 */
function findNativeModule() {
    const moduleName = getNativeModuleName();
    const packageRoot = __dirname;

    // 候选路径列表
    const candidates = [
        // prebuildify 产物
        path.join(packageRoot, 'prebuilds', `${process.platform}-${process.arch}`, moduleName),
        // CMake 构建产物（开发模式）
        path.join(packageRoot, 'build', 'out', 'bin', 'Release', moduleName),
        path.join(packageRoot, 'build', 'out', 'bin', 'Debug', moduleName),
        // node-gyp 构建产物
        path.join(packageRoot, 'build', 'Release', moduleName),
        path.join(packageRoot, 'build', 'Debug', moduleName),
    ];

    for (const candidate of candidates) {
        if (fs.existsSync(candidate)) {
            return candidate;
        }
    }

    return null;
}

// 查找并加载原生模块
const modulePath = findNativeModule();
if (!modulePath) {
    throw new Error(
        `ZQOffice: native module not found.\n` +
        `Please build it first:\n` +
        `  npm run build\n` +
        `Or install prebuild: npm run prebuild:install`
    );
}

let zqoffice;
try {
    zqoffice = require(modulePath);
} catch (e) {
    throw new Error(`ZQOffice: failed to load native module at ${modulePath}: ${e.message}`);
}

// 重新导出所有 API
module.exports = zqoffice;

// 导出模块路径供调试
module.exports.__modulePath = modulePath;
module.exports.__version = '1.0.0';
