cmake_minimum_required(VERSION 3.16)
project(s21_key_value_store_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(MODEL_COMMON_SRC
    ${SRC_DIR}/model/record.cpp
    ${SRC_DIR}/model/parser.cpp
    ${SRC_DIR}/model/key_value_store.cpp
    ${SRC_DIR}/model/command_processor.cpp
)

# ───────── Статическая библиотека: hash_table ─────────
add_library(hash_table STATIC
    ${MODEL_COMMON_SRC}
    ${SRC_DIR}/model/hash_table.cpp
)
target_include_directories(hash_table PUBLIC ${SRC_DIR})

# ───────── Статическая библиотека: self_balancing_binary_search_tree ───────
add_library(self_balancing_binary_search_tree STATIC
    ${MODEL_COMMON_SRC}
    ${SRC_DIR}/model/self_balancing_binary_search_tree.cpp
)
target_include_directories(self_balancing_binary_search_tree PUBLIC ${SRC_DIR})

# ───────── Статическая библиотека: controller ─────────
add_library(controller STATIC ${SRC_DIR}/controller/controller.cpp)
target_include_directories(controller PUBLIC ${SRC_DIR})
target_link_libraries(controller PRIVATE hash_table
    self_balancing_binary_search_tree)

# ───────── Статическая библиотека: view ─────────
add_library(view STATIC ${SRC_DIR}/view/view.cpp)
target_include_directories(view PUBLIC ${SRC_DIR})
target_link_libraries(view PRIVATE controller)

# ───────── Консольное приложение ─────────
add_executable(key_value_store ${SRC_DIR}/main.cpp)
target_link_libraries(key_value_store PRIVATE hash_table
    self_balancing_binary_search_tree controller view)

# ───────── Google Test ──
find_package(Threads REQUIRED)
find_package(GTest QUIET)

# ───────── Юнит-тесты ─────────
add_executable(test_model
    ${SRC_DIR}/model/tests/key_value_store_test.cpp
    ${SRC_DIR}/model/tests/hash_table_test.cpp
    ${SRC_DIR}/model/tests/self_balancing_binary_search_tree_test.cpp
    ${SRC_DIR}/model/tests/parser_test.cpp
    ${SRC_DIR}/model/tests/command_test.cpp
    ${SRC_DIR}/model/tests/response_test.cpp
)
target_include_directories(test_model PUBLIC ${SRC_DIR})
target_link_libraries(test_model PRIVATE hash_table self_balancing_binary_search_tree controller GTest::gtest GTest::gtest_main)