# CMakeList.txt : CMake project for MGram, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.25)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("MGram" VERSION 0.1)

file(GLOB MGRAM_SOURCES "src/*.cpp")

include(user/APIInfo.cmake)

add_executable(${PROJECT_NAME} ${MGRAM_SOURCES})


if(WIN32)
  set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
  wxMSVC_VERSION_ABI_COMPAT # Important for wxWidgets v3.3.0+
TDAPI_ID=${TELEGRAM_API_ID}
TDAPI_HASH="${TELEGRAM_API_HASH}"
)

target_include_directories(${PROJECT_NAME} PRIVATE dep)


# --- Dependencies with FetchContent ---
include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/_dependencies)


# wxWidgets
FetchContent_Declare(wxWidgets
  GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
  GIT_TAG v3.3.0
  GIT_SHALLOW ON
  SYSTEM
)
set(wxBUILD_SHARED OFF CACHE BOOL "" FORCE)
set(wxBUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(wxBUILD_TESTS OFF CACHE BOOL "" FORCE)
set(wxBUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(wxBUILD_INSTALL OFF CACHE BOOL "" FORCE)


set(wxWidgets_USE_STATIC ON CACHE BOOL "" FORCE)
set(wxUSE_EXTENDED_RTTI OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(wxWidgets)


# TDLib
find_package(Td REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
  wx::core
  wx::base
  Td::TdStatic
)

