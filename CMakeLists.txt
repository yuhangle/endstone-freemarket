cmake_minimum_required(VERSION 3.15)

project(freemarket CXX)


set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BASE_VERSION "0.2.5")
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /utf-8")
endif()

include(FetchContent)
FetchContent_Declare(
        endstone
        GIT_REPOSITORY https://github.com/EndstoneMC/endstone.git
        GIT_TAG v0.11.7
)
FetchContent_MakeAvailable(endstone)

# fmt (header-only or static)
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 11.2.0
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

# endstone-inventoryui-api (embedded static library)
set(INVENTORYUI_BUILD_AS_STATIC ON)
FetchContent_Declare(
        endstone_inventoryui
        GIT_REPOSITORY https://github.com/yuhangle/endstone-inventoryui-api.git
        GIT_TAG main
        #SOURCE_DIR /home/yuhang/Documents/item/endstone-c/endstone-inventoryui
)
FetchContent_MakeAvailable(endstone_inventoryui)

# endstone-money-connect (header-only economy service interface)
FetchContent_Declare(
        endstone_money_connect
        GIT_REPOSITORY https://github.com/yuhangle/endstone-money-connect.git
        GIT_TAG main
)
FetchContent_MakeAvailable(endstone_money_connect)

file(GLOB PLUGIN_SOURCES
        CONFIGURE_DEPENDS
        "src/*.cpp"
)
endstone_add_plugin(${PROJECT_NAME} ${PLUGIN_SOURCES})

# SQLite3
find_package(SQLite3 REQUIRED)

find_package(Git QUIET)
if(GIT_FOUND)
    # 1. 查找最近的【纯净】正式版本 Tag (排除带 - 的 Tag)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0 --match "v[0-9]*" --exclude "*-*" HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE LAST_STABLE_TAG
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(LAST_STABLE_TAG)
        # 有稳定版基准：计算距离该 Tag 的提交数
        execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-list --count "${LAST_STABLE_TAG}..HEAD"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                OUTPUT_VARIABLE GIT_COMMIT_COUNT
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    else()
        # 无稳定版基准：计算总提交数
        execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                OUTPUT_VARIABLE GIT_COMMIT_COUNT
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()

    # 获取短哈希
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE GIT_SHORT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(GIT_COMMIT_COUNT "0")
    set(GIT_SHORT_HASH "unknown")
endif()

# 决定最终版本号
if(RELEASE_VERSION)
    set(PROJECT_VERSION "${BASE_VERSION}")
else()
    set(PROJECT_VERSION "${BASE_VERSION}-alpha.${GIT_COMMIT_COUNT}+${GIT_SHORT_HASH}")
endif()

# 生成 version.h
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/version.h"
)
# 将生成目录加入包含路径
target_include_directories(${PROJECT_NAME} PRIVATE include "${CMAKE_CURRENT_BINARY_DIR}/generated")


# 链接库
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        fmt::fmt-header-only
        nlohmann_json::nlohmann_json
        SQLite::SQLite3
        endstone_inventoryui
        endstone_money_connect
)

# Release 优化
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        # MSVC: 允许重复符号
        target_link_options(${PROJECT_NAME} PRIVATE /FORCE:MULTIPLE)
    else()
        # GCC/Clang: 函数分段 + GC 死代码消除
        target_compile_options(${PROJECT_NAME} PRIVATE -ffunction-sections -fdata-sections)
        target_link_options(${PROJECT_NAME} PRIVATE -Wl,--gc-sections -Wl,--allow-multiple-definition)
    endif()
endif()

# Linux Release 构建后自动 strip
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT MSVC)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND strip $<TARGET_FILE:${PROJECT_NAME}>
            COMMENT "Stripping debug symbols from ${PROJECT_NAME}")
endif()