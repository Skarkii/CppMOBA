# CMakeLists.txt : CMake project for MOBA, include source and define
# project specific logic here.
#

cmake_minimum_required(VERSION 3.25)

project(MOBA LANGUAGES CXX)

option(MOBA_BUILD_SERVER "Build the MOBA server" ON)
option(MOBA_BUILD_CLIENT "Build the MOBA client" ON)

if(NOT MOBA_BUILD_SERVER AND NOT MOBA_BUILD_CLIENT)
    message(FATAL_ERROR "Nothing to build: enable MOBA_BUILD_SERVER and/or MOBA_BUILD_CLIENT")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Hot Reload (Edit and Continue) for MSVC in Debug/RelWithDebInfo.
# CMP0141 is already NEW because of cmake_minimum_required(3.25).
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
    "$<IF:$<CXX_COMPILER_ID:MSVC>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
include(FetchContent)

# ENet: reliable + unreliable channels over one UDP connection.
FetchContent_Declare(enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG        v1.3.18
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(enet)
# ENet's own CMake doesn't export its include folder, so add it here.
# SYSTEM keeps our warning flags from firing on ENet's headers.
target_include_directories(enet SYSTEM INTERFACE ${enet_SOURCE_DIR}/include)
# ENet 1.3.18 only links its Windows libraries for MinGW, so add them for MSVC.
if(WIN32 AND NOT MINGW)
    target_link_libraries(enet ws2_32 winmm)
endif()

# raylib: window, input and rendering for the client.
if(MOBA_BUILD_CLIENT)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        6.0
        GIT_SHALLOW    TRUE
        SYSTEM         # keeps our warning flags from firing on raylib.h
    )
    FetchContent_MakeAvailable(raylib)
endif()

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(SHARED_SOURCES
    src/shared/net.cpp
    src/shared/net.hpp
    src/shared/serialize.hpp
    src/shared/protocol.hpp
)

set(SERVER_SOURCES
    src/server/main.cpp
)

set(CLIENT_SOURCES
    src/client/main.cpp
    src/client/game.cpp
    src/client/player.cpp
)

function(moba_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wshadow -Wconversion -Wsign-conversion
            -Wnon-virtual-dtor -Wold-style-cast
        )
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Shared -> MOBA_shared (static library used by server and client)
# ---------------------------------------------------------------------------
add_library(MOBA_shared STATIC ${SHARED_SOURCES})
add_library(MOBA::shared ALIAS MOBA_shared)
target_include_directories(MOBA_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/shared)
target_link_libraries(MOBA_shared PRIVATE enet)
moba_set_warnings(MOBA_shared)

# ---------------------------------------------------------------------------
# Server -> MOBA_server
# ---------------------------------------------------------------------------
if(MOBA_BUILD_SERVER)
    add_executable(MOBA_server ${SERVER_SOURCES})
    target_include_directories(MOBA_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/server)
    target_link_libraries(MOBA_server PRIVATE MOBA::shared)
    target_compile_definitions(MOBA_server PRIVATE MOBA_SERVER=1)
    moba_set_warnings(MOBA_server)
endif()

# ---------------------------------------------------------------------------
# Client -> MOBA_client
# ---------------------------------------------------------------------------
if(MOBA_BUILD_CLIENT)
    add_executable(MOBA_client ${CLIENT_SOURCES})
    target_include_directories(MOBA_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/client)
    target_link_libraries(MOBA_client PRIVATE MOBA::shared raylib)
    target_compile_definitions(MOBA_client PRIVATE MOBA_CLIENT=1)
    moba_set_warnings(MOBA_client)
endif()