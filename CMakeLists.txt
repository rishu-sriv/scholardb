cmake_minimum_required(VERSION 3.16)
project(StudentRecordSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(third_party)

# ---------------------------------------------------------------------------
# IXWebSocket — fetched at configure time, no manual vendoring needed.
# TLS and zlib compression are off; we only need plain ws:// for this project.
# ---------------------------------------------------------------------------
include(FetchContent)

set(USE_TLS     OFF CACHE BOOL "" FORCE)
set(USE_DEFLATE OFF CACHE BOOL "" FORCE)
set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG        v11.4.5
)
FetchContent_MakeAvailable(ixwebsocket)

# ---------------------------------------------------------------------------
# common_lib — shared by server and client binaries.
# Phase 8: StudentValidator added.
# ---------------------------------------------------------------------------
add_library(common_lib STATIC
    src/common/Student.cpp
    src/common/CSVHandler.cpp
    src/common/StudentValidator.cpp
    src/repository/StudentRepository.cpp
)

# ---------------------------------------------------------------------------
# phase1_loader — Phase 1/2 console menu (CSV load + repository CRUD test)
# ---------------------------------------------------------------------------
add_executable(phase1_loader
    src/common/main_phase1.cpp
)
target_link_libraries(phase1_loader PRIVATE common_lib)

# ---------------------------------------------------------------------------
# server — Phase 3: bare echo server.
# ConnectionManager and full routing wired in later phases.
# ---------------------------------------------------------------------------
add_executable(server
    src/server/main_server.cpp
    src/server/WebSocketServer.cpp
    src/server/ConnectionManager.cpp
)
target_link_libraries(server PRIVATE common_lib ixwebsocket)

# ---------------------------------------------------------------------------
# client — Phase 5: connects, sends one hardcoded message, prints the echo.
# Full CLI menu wired in once Protocol.h exists (Phase 6).
# ---------------------------------------------------------------------------
add_executable(client
    src/client/main_client.cpp
    src/client/WebSocketClient.cpp
)
target_link_libraries(client PRIVATE common_lib ixwebsocket)
