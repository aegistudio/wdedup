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

# Make sure that at least C++11 is used to avoid problems.
set(CMAKE_CXX_STANDARD 11)

# Makes it impossible to be built on platform other than Linux
# currently, as we relies on features like mmap(), posix_fadvise().
if(NOT "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
message(SEND_ERROR "wdedup can only be built on Linux currently.")
endif()

# Include directory for wdedup.
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

# XXX(haoran.luo): this is a build stub for testing wio sources.
# Will be removed when the wdedup main program has been done.
add_library(wdedup_io "${CMAKE_CURRENT_SOURCE_DIR}/src/wiobase.cpp")
