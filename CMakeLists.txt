cmake_minimum_required(VERSION 3.25)
project(ultrakill_scoring_system_type_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(FetchContent)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.2.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

add_executable(ultrakill_scoring_system_type_test src/main.cpp)
target_link_libraries(ultrakill_scoring_system_type_test PRIVATE SDL3::SDL3)

if(WIN32)
  add_custom_command(TARGET ultrakill_scoring_system_type_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:SDL3::SDL3-shared>
      $<TARGET_FILE_DIR:ultrakill_scoring_system_type_test>)
endif()
