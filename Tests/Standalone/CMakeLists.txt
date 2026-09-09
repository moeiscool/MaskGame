# Copyright (c) 2026 Moe. MIT licensed. See LICENSE.
#
# Builds the engine-free rules layer on its own and runs its tests. The same
# sources are compiled into the Unreal module by Source/MaskGame/MaskGame.Build.cs,
# so a green run here means the game's rules hold before the editor is ever opened.

cmake_minimum_required(VERSION 3.16)
project(MaskGameRulesTests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

set(RULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/MaskGame/Rules)

add_library(MaskGameRules STATIC
  ${RULES_DIR}/CycleClock.cpp
  ${RULES_DIR}/SongMatcher.cpp
  ${RULES_DIR}/ProgressionState.cpp
  ${RULES_DIR}/MaskRules.cpp
  ${RULES_DIR}/ScheduleTable.cpp
)
target_include_directories(MaskGameRules PUBLIC ${RULES_DIR})

enable_testing()

foreach(SUITE CycleClock SongMatcher Progression MaskRules ScheduleTable)
  add_executable(Test${SUITE} Test${SUITE}.cpp)
  target_link_libraries(Test${SUITE} PRIVATE MaskGameRules)
  target_include_directories(Test${SUITE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  add_test(NAME ${SUITE} COMMAND Test${SUITE})
endforeach()

# The content tables are validated by a script rather than by C++, because what
# they need checking against is the enums in MaskGameTypes.h and each other, both
# of which are far easier to parse than to link.
find_package(Python3 COMPONENTS Interpreter)
if (Python3_Interpreter_FOUND)
  add_test(NAME DataTables
           COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../validate_data.py)
else()
  message(WARNING "Python 3 not found; the DataTables suite will be skipped.")
endif()
