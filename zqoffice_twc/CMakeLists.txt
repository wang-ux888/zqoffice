# =============================================================================
# ZQOffice: C++ Office 文档转换引擎
# 完整实现 OOXML ↔ JSON 的双向转换（xlsx/xls/csv/pptx/ppt/docx）
# =============================================================================
cmake_minimum_required(VERSION 3.18)

project(zqoffice
    VERSION 1.0.0
    DESCRIPTION "ZQOffice: C++ Office document conversion engine"
    LANGUAGES CXX C
)

# ---------------------------------------------------------------------------
# 全局选项
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 模块化编译（避免宏污染）
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# 默认 Build Type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# 输出目录统一
set(ZQ_OFFICE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/out)
set(ZQ_OFFICE_LIB_DIR ${ZQ_OFFICE_OUTPUT_DIR}/lib)
set(ZQ_OFFICE_BIN_DIR ${ZQ_OFFICE_OUTPUT_DIR}/bin)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ZQ_OFFICE_LIB_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ZQ_OFFICE_LIB_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ZQ_OFFICE_BIN_DIR})

# 位置无关代码（用于 Node-API 动态库）
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# 选项开关
# ---------------------------------------------------------------------------
option(ZQ_OFFICE_BUILD_TOOLS "Build auxiliary tools (dump_symbols, cli)" OFF)
option(ZQ_OFFICE_BUILD_NODE_API "Build Node-API binding (zqoffice.node)" ON)
option(ZQ_OFFICE_USE_SYSTEM_ZLIB "Use system zlib instead of vendored" OFF)
option(ZQ_OFFICE_USE_SYSTEM_LIBXML2 "Use system libxml2 instead of vendored" OFF)
option(ZQ_OFFICE_USE_SYSTEM_ICU "Use system ICU instead of vendored" OFF)
option(ZQ_OFFICE_USE_SYSTEM_OPENSSL "Use system openssl instead of vendored" OFF)

# ---------------------------------------------------------------------------
# 平台检测
# ---------------------------------------------------------------------------
if(WIN32)
    add_compile_definitions(ZQ_OFFICE_PLATFORM_WINDOWS=1)
    # Windows MSVC 需要定义 WIN32_LEAN_AND_MEAN 避免 Windows.h 拖入 Winsock
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS _WINSOCKAPI_)
    # MSVC UTF-8
    if(MSVC)
        add_compile_options(/utf-8 /Zc:__cplusplus /permissive- /W4 /WX-)
        # 静态链接 CRT（避免目标机器缺 vcruntime）
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
elseif(APPLE)
    add_compile_definitions(ZQ_OFFICE_PLATFORM_MACOS=1)
else()
    add_compile_definitions(ZQ_OFFICE_PLATFORM_LINUX=1)
endif()

# ---------------------------------------------------------------------------
# 根目录
# ---------------------------------------------------------------------------
set(ZQ_OFFICE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ZQ_OFFICE_INCLUDE_DIR ${ZQ_OFFICE_ROOT_DIR}/include)
set(ZQ_OFFICE_SRC_DIR ${ZQ_OFFICE_ROOT_DIR}/src)
set(ZQ_OFFICE_THIRD_PARTY_DIR ${ZQ_OFFICE_ROOT_DIR}/third_party)
set(ZQ_OFFICE_TESTS_DIR ${ZQ_OFFICE_ROOT_DIR}/tests) # legacy, kept for reference

# ---------------------------------------------------------------------------
# 第三方库
# ---------------------------------------------------------------------------
# miniz：ZIP 读写 + deflate/inflate（OOXML 容器解析）
# Phase C 引入，替代规划的 zlib+minizip 组合（miniz 自带 deflate/inflate，零外部依赖）
include(FetchContent)
# miniz 3.0.2 的 CMakeLists.txt 用了老版本 cmake_minimum_required，CMake 4.x 需要策略兼容
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
FetchContent_Declare(
    miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG 3.0.2
    GIT_PROGRESS TRUE
)
# 静默 miniz 的安装目标
set(miniz_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(miniz_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(miniz_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(miniz)
unset(CMAKE_POLICY_VERSION_MINIMUM CACHE)

# pugixml：轻量级 C++ XML 解析库（DOM + SAX），MIT 协议
# Phase C 引入，替代规划的 libxml2（pugixml 更现代 C++，单文件，性能更高）
FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG v1.14
    GIT_PROGRESS TRUE
)
set(PUGIXML_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(pugixml)

# ---------------------------------------------------------------------------
# 引擎核心静态库：zqoffice_core
# ---------------------------------------------------------------------------
add_library(zqoffice_core STATIC)

target_include_directories(zqoffice_core
    PUBLIC
        ${ZQ_OFFICE_INCLUDE_DIR}
    PRIVATE
        ${ZQ_OFFICE_SRC_DIR}
)

target_compile_features(zqoffice_core PUBLIC cxx_std_17)

# Phase A 源文件（后续 Phase 通过 target_sources(APPEND) 追加）
file(GLOB ZQ_OFFICE_UTILS_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/utils/*.cpp
)
file(GLOB ZQ_OFFICE_API_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/api/*.cpp
)
file(GLOB ZQ_OFFICE_OLE2_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/ole2/*.cpp
)
file(GLOB ZQ_OFFICE_BIFF_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/biff/*.cpp
)
file(GLOB ZQ_OFFICE_ESCHER_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/escher/*.cpp
)
file(GLOB ZQ_OFFICE_ICU_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/icu/*.cpp
)
file(GLOB ZQ_OFFICE_OOXML_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/ooxml/*.cpp
    ${ZQ_OFFICE_SRC_DIR}/ooxml/dml/*.cpp
    ${ZQ_OFFICE_SRC_DIR}/ooxml/pml/*.cpp
    ${ZQ_OFFICE_SRC_DIR}/ooxml/sml/*.cpp
    ${ZQ_OFFICE_SRC_DIR}/ooxml/wml/*.cpp
    ${ZQ_OFFICE_SRC_DIR}/ooxml/opc/*.cpp
)
# Phase D 引入：drawing 模块（Drawing 转换：枚举 + Theme + TTShape + ChartConvert 等）
file(GLOB ZQ_OFFICE_DRAWING_ENUMS_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/drawing/enums/*.cpp
)
file(GLOB ZQ_OFFICE_DRAWING_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/drawing/*.cpp
)
# Phase D13-D14 引入：excel / powerpoint 客户端模块（OfficeArtClientTextbox / OfficeArtClientAnchor）
file(GLOB ZQ_OFFICE_EXCEL_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/excel/*.cpp
)
file(GLOB ZQ_OFFICE_POWERPOINT_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/powerpoint/*.cpp
)
# Phase E 引入：textlayout 模块（文本布局引擎：BaseRun/TextRun/ObjectRun/Paragraph/TextLine/LayoutPage 等）
file(GLOB ZQ_OFFICE_TEXTLAYOUT_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/textlayout/*.cpp
)
# Phase F 引入：crypto 模块（加密解密：IEncryption/Cipher/MessageDigest/XORObfuscation/RC4Encryption/AgileEncryption 等）
# 含 internal 子目录（AES/SHA/RC4/Base64 等内部算法实现）
file(GLOB ZQ_OFFICE_CRYPTO_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/crypto/*.cpp
    ${ZQ_OFFICE_SRC_DIR}/crypto/internal/*.cpp
)
# Phase G 引入：numfmt 模块（数字格式化：DateUtil + DateTime）
file(GLOB ZQ_OFFICE_NUMFMT_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/numfmt/*.cpp
)
# Phase G 引入：painter 模块（渲染适配层：Painter/LynxPoint/LynxRRect/LynxFontMetrics/NativeEvent）
# 含 lynx 子目录（Lynx 渲染框架适配类）
file(GLOB ZQ_OFFICE_PAINTER_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/painter/*.cpp
    ${ZQ_OFFICE_SRC_DIR}/painter/lynx/*.cpp
)
# Phase H 引入：api/json 模块（内部 JSON 库：JsonValue + Serializer + Parser）
file(GLOB ZQ_OFFICE_API_JSON_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/api/json/*.cpp
)
# Phase H 引入：api/converters 模块（转换器：XlsxConverter/PptxConverter/DocxConverter）
file(GLOB ZQ_OFFICE_API_CONVERTERS_SOURCES
    ${ZQ_OFFICE_SRC_DIR}/api/converters/*.cpp
)
target_sources(zqoffice_core PRIVATE
    ${ZQ_OFFICE_UTILS_SOURCES}
    ${ZQ_OFFICE_API_SOURCES}
    ${ZQ_OFFICE_API_JSON_SOURCES}
    ${ZQ_OFFICE_API_CONVERTERS_SOURCES}
    ${ZQ_OFFICE_OLE2_SOURCES}
    ${ZQ_OFFICE_BIFF_SOURCES}
    ${ZQ_OFFICE_ESCHER_SOURCES}
    ${ZQ_OFFICE_ICU_SOURCES}
    ${ZQ_OFFICE_OOXML_SOURCES}
    ${ZQ_OFFICE_DRAWING_ENUMS_SOURCES}
    ${ZQ_OFFICE_DRAWING_SOURCES}
    ${ZQ_OFFICE_EXCEL_SOURCES}
    ${ZQ_OFFICE_POWERPOINT_SOURCES}
    ${ZQ_OFFICE_TEXTLAYOUT_SOURCES}
    ${ZQ_OFFICE_CRYPTO_SOURCES}
    ${ZQ_OFFICE_NUMFMT_SOURCES}
    ${ZQ_OFFICE_PAINTER_SOURCES}
    ${ZQ_OFFICE_SRC_DIR}/version.cpp
)

# Windows 下需要链接 windows.h 相关库（OLE/Shell/Uuid）
# 注意：仅链接 Phase A 实际需要的最小集合，避免引入不必要依赖
if(WIN32)
    target_link_libraries(zqoffice_core PUBLIC
        ole32
        advapi32
    )
endif()

# 链接 miniz（ZIP 解压，Phase C 引入）
target_link_libraries(zqoffice_core PRIVATE miniz)

# 链接 pugixml（XML 解析，Phase C 引入）
target_link_libraries(zqoffice_core PRIVATE pugixml)

# ---------------------------------------------------------------------------
# Node-API 动态库：zqoffice.node
# ---------------------------------------------------------------------------
if(ZQ_OFFICE_BUILD_NODE_API)
    # 查找 Node-API 头文件（先尝试 node-addon-api，再尝试 nodejs）
    # 支持的搜索路径：
    #   1. 环境变量 NODE_INCLUDE_DIR / NODEJS_INCLUDE_DIR（用户手动指定）
    #   2. node-gyp 缓存路径（Windows: %LOCALAPPDATA%\node-gyp\Cache\<ver>\include\node）
    #   3. 系统安装路径（/usr/include/nodejs 等）
    find_path(NODE_API_INCLUDE_DIR
        NAMES node_api.h
        PATHS
            $ENV{NODE_INCLUDE_DIR}
            $ENV{NODEJS_INCLUDE_DIR}
            "$ENV{LOCALAPPDATA}/node-gyp/Cache"
            "$ENV{HOME}/.node-gyp"
            /usr/include/nodejs
            /usr/local/include/nodejs
            /opt/homebrew/include/nodejs
        PATH_SUFFIXES
            include/node          # node-gyp 缓存结构：<cache>/<ver>/include/node
            include               # 直接安装结构：<dir>/include
        DOC "Path to Node.js headers (node_api.h)"
    )

    # 如果上面的 find_path 没找到，尝试在 node-gyp 缓存中按版本号查找
    if(NOT NODE_API_INCLUDE_DIR)
        file(GLOB _node_gyp_versions
            "$ENV{LOCALAPPDATA}/node-gyp/Cache/[0-9]*"
            "$ENV{HOME}/.node-gyp/[0-9]*"
        )
        foreach(_ver_dir ${_node_gyp_versions})
            if(EXISTS "${_ver_dir}/include/node/node_api.h")
                set(NODE_API_INCLUDE_DIR "${_ver_dir}/include/node")
                message(STATUS "ZQOffice: Node-API headers found in node-gyp cache: ${NODE_API_INCLUDE_DIR}")
                break()
            endif()
        endforeach()
    endif()

    if(NOT NODE_API_INCLUDE_DIR)
        message(STATUS "ZQOffice: Node-API headers not found; Node-API binding will be skipped.")
        message(STATUS "  Set NODE_INCLUDE_DIR env var to enable (e.g. C:/Program Files/nodejs/include)")
        message(STATUS "  Or install Node.js headers via: npm install -g node-gyp && node-gyp install")
    else()
        message(STATUS "ZQOffice: Node-API headers found at ${NODE_API_INCLUDE_DIR}")

        add_library(zqoffice_node SHARED
            ${ZQ_OFFICE_SRC_DIR}/zqoffice_node.cpp
        )
        set_target_properties(zqoffice_node PROPERTIES
            OUTPUT_NAME zqoffice
            PREFIX ""
            SUFFIX ".node"
        )
        target_include_directories(zqoffice_node PRIVATE ${NODE_API_INCLUDE_DIR})
        target_link_libraries(zqoffice_node PRIVATE zqoffice_core)

        if(WIN32)
            target_compile_definitions(zqoffice_node PRIVATE ZQ_OFFICE_BUILDING_DLL=1)
        endif()

        # Windows 下需要链接 node.lib（Node-API 导入库）
        # node-gyp 缓存路径：<cache>/<ver>/<arch>/node.lib
        if(WIN32)
            # 从 NODE_API_INCLUDE_DIR 推导 node-gyp 缓存根目录
            # NODE_API_INCLUDE_DIR = <cache>/<ver>/include/node
            # node.lib 在 <cache>/<ver>/<arch>/node.lib
            get_filename_component(_node_include_parent "${NODE_API_INCLUDE_DIR}" DIRECTORY)  # <cache>/<ver>/include
            get_filename_component(_node_version_dir "${_node_include_parent}" DIRECTORY)      # <cache>/<ver>

            # 查找 node.lib（x64 或 x86）
            find_library(NODE_API_LIB
                NAMES node
                PATHS
                    "${_node_version_dir}/x64"
                    "${_node_version_dir}/win-x64"
                    "${_node_version_dir}/ia32"
                    "$ENV{NODE_LIB_DIR}"
                DOC "Path to node.lib (Node-API import library)"
            )

            if(NODE_API_LIB)
                message(STATUS "ZQOffice: Node-API import library found at ${NODE_API_LIB}")
                target_link_libraries(zqoffice_node PRIVATE ${NODE_API_LIB})
            else()
                message(WARNING "ZQOffice: node.lib not found; link may fail.")
                message(STATUS "  Set NODE_LIB_DIR env var to node.lib directory to enable")
            endif()

            # winmm（某些 Node-API 版本依赖）
            target_link_libraries(zqoffice_node PRIVATE winmm)
        endif()
    endif()
endif()

# ---------------------------------------------------------------------------
# 工具
# ---------------------------------------------------------------------------
if(ZQ_OFFICE_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

# ---------------------------------------------------------------------------
# 信息打印
# ---------------------------------------------------------------------------
message(STATUS "")
message(STATUS "========== ZQOffice ==========")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "  Platform:          ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})")
message(STATUS "  Compiler:          ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build Node-API:    ${ZQ_OFFICE_BUILD_NODE_API}")
message(STATUS "  Include dir:       ${ZQ_OFFICE_INCLUDE_DIR}")
message(STATUS "  Output dir:        ${ZQ_OFFICE_OUTPUT_DIR}")
message(STATUS "================================")
message(STATUS "")
