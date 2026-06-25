cmake_minimum_required(VERSION 3.15)
project(uopsim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

# --- コア（ヘッダオンリー） ---
add_library(uopsim_core INTERFACE)
target_include_directories(uopsim_core INTERFACE include) 

# --- 実行ファイル ---
add_executable(polyval_bench apps/polyval_bench.cpp)
target_link_libraries(polyval_bench PRIVATE uopsim_core)

add_executable(xctr_bench apps/xctr_bench.cpp)
target_link_libraries(xctr_bench PRIVATE uopsim_core)

add_executable(compare_cpus apps/compare_cpus.cpp)
target_link_libraries(compare_cpus PRIVATE uopsim_core)

add_executable(xctr_polyval_bench apps/xctr_polyval_bench.cpp)
target_link_libraries(xctr_polyval_bench PRIVATE uopsim_core)
