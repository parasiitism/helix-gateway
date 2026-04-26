# ═══════════════════════════════════════════════════════════════
#  Helix Data Plane — CMake build configuration
# ═══════════════════════════════════════════════════════════════

cmake_minimum_required(VERSION 3.25)

project(helix
    VERSION 0.1.0
    DESCRIPTION "Helix LLM Gateway — C++20 data plane"
    LANGUAGES CXX
)

# ── Modern C++ defaults ─────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # for clangd / VS Code IntelliSense

# Default to Release if user didn't specify
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ── Compiler warnings ───────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Wshadow -Wnon-virtual-dtor
    )
endif()

# ── Sanitizers in Debug builds ──────────────────────────────────
option(HELIX_ENABLE_ASAN "Enable AddressSanitizer"   OFF)
option(HELIX_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(HELIX_ENABLE_TSAN "Enable ThreadSanitizer"    OFF)

if(HELIX_ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()
if(HELIX_ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif()
if(HELIX_ENABLE_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

# ── Dependencies ───────────────────────────────────────────────
find_package(Drogon CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(simdjson CONFIG REQUIRED)

# ── Main executable ─────────────────────────────────────────────
add_executable(helix-gateway
    src/main.cpp
)

target_include_directories(helix-gateway
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(helix-gateway
    PRIVATE
        Drogon::Drogon
        spdlog::spdlog
        simdjson::simdjson
)

# ── Versioning macro ────────────────────────────────────────────
target_compile_definitions(helix-gateway PRIVATE
    HELIX_VERSION="${PROJECT_VERSION}"
)

# ── Testing & benchmarks (gated) ────────────────────────────────
option(HELIX_BUILD_TESTS      "Build unit tests"          ON)
option(HELIX_BUILD_BENCHMARKS "Build microbenchmarks"     OFF)

if(HELIX_BUILD_TESTS)
    enable_testing()
    # add_subdirectory(tests)   # uncomment in Week 2+
endif()

if(HELIX_BUILD_BENCHMARKS)
    # add_subdirectory(benchmarks)  # uncomment in Week 11
endif()

# ── Install rules ───────────────────────────────────────────────
install(TARGETS helix-gateway RUNTIME DESTINATION bin)

# ── Status summary ──────────────────────────────────────────────
message(STATUS "═══════════════════════════════════════════════")
message(STATUS "  Helix Gateway v${PROJECT_VERSION}")
message(STATUS "  Build type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler:     ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  ASan:         ${HELIX_ENABLE_ASAN}")
message(STATUS "  UBSan:        ${HELIX_ENABLE_UBSAN}")
message(STATUS "═══════════════════════════════════════════════")