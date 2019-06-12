# Word Deduplication Tool (for massive file).
# Copyright © 2019 Haoran Luo
#
# Permission is hereby granted, free of charge, to any person 
# obtaining a copy of this software and associated documentation 
# files (the “Software”), to deal in the Software without 
# restriction, including without limitation the rights to use, 
# copy, modify, merge, publish, distribute, sublicense, and/or 
# sell copies of the Software, and to permit persons to whom the 
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be 
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, 
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS 
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
# IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
# THE SOFTWARE.
#
# This tool is designed for deduplicating words in massive file,
# and find the first or list of non-duplicating ones. The output
# words will be ordered by their first occurence in the file.
cmake_minimum_required(VERSION 3.5)
project(wdedup)

# Configurable options for building wdedup.
option(WDEDUP_RUNTESTS "Build and run unit tests (GoogleTest required)." ON)

# Make sure that at least C++11 is used to avoid problems.
set(CMAKE_CXX_STANDARD 11)

# Makes it impossible to be built on platform other than Linux
# currently, as we relies on features like mmap(), posix_fadvise().
if(NOT "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
  message(SEND_ERROR "wdedup can only be built on Linux currently.")
endif()

# Include directory for wdedup.
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

# Convenient path for specifying some source file as part of the building.
set(WDEDUP_SRCPATH   "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(WDEDUP_TESTSPATH "${CMAKE_CURRENT_SOURCE_DIR}/tests")

# Boost::ProgramOptions required for building the command line parsing.
find_package(Boost 1.58.0 REQUIRED COMPONENTS program_options)
if(NOT ${Boost_FOUND})
  message(SEND_ERROR "Boost::ProgramOptions required for command line parsing.")
else()
  include_directories(${Boost_INCLUDE_DIR})
endif()

# PkgConfig and LibBsd required for embedded rbtree data structure.
find_package(PkgConfig REQUIRED)
if(NOT ${PKGCONFIG_FOUND})
  message(SEND_ERROR "PkgConfig required for retrieving package config.")
endif()
pkg_check_modules(LIBBSD REQUIRED libbsd)
if(NOT ${LIBBSD_FOUND})
  message(SEND_ERROR "LibBSD required for defining embedded rbtree structures.")
endif()

# Configure and use GoogleTest as unit test driver.
if(WDEDUP_RUNTESTS)
  # Enable CTest provided by CMake.
  include(CTest)
  enable_testing()

  # Find and import GoogleTest (GTest::Main, GTest::GTest).
  find_package(GTest REQUIRED)
  if(NOT ${GTEST_FOUND})
    message(SEND_ERROR "WDEDUP_RUNTEST enabled with GoogleTest not found.")
  endif()

  # Configure macros for conveniently defining some test cases.
  macro(wdedup_testcase WDEDUP_TESTNAME)
    add_executable("${WDEDUP_TESTNAME}.test"
        "${WDEDUP_TESTSPATH}/${WDEDUP_TESTNAME}.cpp" ${ARGN})
    target_link_libraries("${WDEDUP_TESTNAME}.test" GTest::GTest GTest::Main)
    add_test("wdedup.${WDEDUP_TESTNAME}" "${WDEDUP_TESTNAME}.test")
  endmacro(wdedup_testcase)

  # Define test cases just inside the tests directory.
  add_subdirectory(${WDEDUP_TESTSPATH})
endif() # WDEDUP_RUNTEST

# Add our main target (wdedup) here.
add_executable(wdedup "${WDEDUP_SRCPATH}/main.cpp"
                      "${WDEDUP_SRCPATH}/wio.cpp"
                      "${WDEDUP_SRCPATH}/wiobase.cpp"
                      "${WDEDUP_SRCPATH}/wpflsimple.cpp"
                      "${WDEDUP_SRCPATH}/wpflfilter.cpp"
                      "${WDEDUP_SRCPATH}/wprof.cpp"
                      "${WDEDUP_SRCPATH}/wmerge.cpp"
                      "${WDEDUP_SRCPATH}/wmpsimple.cpp"
                      "${WDEDUP_SRCPATH}/wfindfirst.cpp"
# (Deprecated) #      "${WDEDUP_SRCPATH}/wsortdedup.cpp"
                      "${WDEDUP_SRCPATH}/wtreededup.cpp"
                      "${WDEDUP_SRCPATH}/wcli.cpp")
target_link_libraries(wdedup Boost::program_options)
