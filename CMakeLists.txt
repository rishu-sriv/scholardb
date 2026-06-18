cmake_minimum_required(VERSION 3.16)
project(StudentRecordSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(third_party)

# ---------------------------------------------------------------------------
# IXWebSocket — fetched at configure time, plain ws:// only (no TLS/deflate)
# ---------------------------------------------------------------------------
include(FetchContent)

set(USE_TLS          OFF CACHE BOOL "" FORCE)
set(USE_DEFLATE      OFF CACHE BOOL "" FORCE)
set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG        v11.4.5
)
FetchContent_MakeAvailable(ixwebsocket)

# ---------------------------------------------------------------------------
# Catch2 v3 — test framework, fetched at configure time
# ---------------------------------------------------------------------------
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)

# ---------------------------------------------------------------------------
# common_lib — shared by server, client, and test binaries
# ---------------------------------------------------------------------------
add_library(common_lib STATIC
    src/common/Student.cpp
    src/common/CSVHandler.cpp
    src/common/StudentValidator.cpp
    src/repository/StudentRepository.cpp
)

# ---------------------------------------------------------------------------
# phase1_loader — Phase 1/2 console menu
# ---------------------------------------------------------------------------
add_executable(phase1_loader
    src/common/main_phase1.cpp
)
target_link_libraries(phase1_loader PRIVATE common_lib)

# ---------------------------------------------------------------------------
# server
# ---------------------------------------------------------------------------
add_executable(server
    src/server/main_server.cpp
    src/server/WebSocketServer.cpp
    src/server/ConnectionManager.cpp
)
target_link_libraries(server PRIVATE common_lib ixwebsocket)

# ---------------------------------------------------------------------------
# client
# ---------------------------------------------------------------------------
add_executable(client
    src/client/main_client.cpp
    src/client/WebSocketClient.cpp
)
target_link_libraries(client PRIVATE common_lib ixwebsocket)

# ---------------------------------------------------------------------------
# run_tests — Catch2 test suite
# ---------------------------------------------------------------------------
add_executable(run_tests
    tests/test_csv_handler.cpp
    tests/test_validator.cpp
    tests/test_repository.cpp
)
target_link_libraries(run_tests PRIVATE common_lib Catch2::Catch2WithMain)
target_include_directories(run_tests PRIVATE ${CMAKE_SOURCE_DIR})
