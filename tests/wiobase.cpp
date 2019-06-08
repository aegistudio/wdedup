/* Copyright © 2019 Haoran Luo
 *
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the “Software”), to deal in the Software without 
 * restriction, including without limitation the rights to use, 
 * copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS 
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE. */
/**
 * @file tests/wiobase.cpp
 * @author Haoran Luo
 * @brief wdedup I/O base tests.
 *
 * This file is unit test for wiobase.cpp. See corresponding header 
 * and source file for details.
 */
#include "gtest/gtest.h"
#include "impl/wiobase.hpp"
#include "wtypes.hpp"

// We must mockup wdedup::SequentialFile and wdedup::AppendFile to
// ensure the testing unit will be the specified impl themselves.
namespace wdedup {

// Mocked up sequential-scan file driver for testing.
SequentialFile::SequentialFile(std::string path, std::string role) 
	throw (wdedup::Error) : pimpl(nullptr) {

	std::function<void(int)> report = [=](int eno) {
		throw wdedup::Error(eno, path, role);
	};

	pimpl = std::unique_ptr<SequentialFile::Impl>(
		new SequentialFileBase(path.c_str(), report));
}

// Mocked up append-only file driver for testing.
AppendFile::AppendFile(std::string path, std::string role, bool log)
	throw (wdedup::Error) : pimpl(nullptr) {

	std::function<void(int)> report = [=](int eno) {
		throw wdedup::Error(eno, path, role);
	};

	pimpl = std::unique_ptr<AppendFile::Impl>(
		new AppendFileBuffer(path.c_str(), report));
}
}

/**
 * wio.readwrite: this file tests the basic read/write of 
 * wdedup::SequentialFileBase and wdedup::AppendFileBuffer,
 * which are base wrapper for linux file descriptor.
 */
TEST(wio, readwrite) {
	// Repetition time for writing file.
	static const size_t times = 1000000;
	static const char* filename = "wio.readwrite.temp";
	remove(filename);	// Make sure absence of file.

	// Write phase of the append file.
	{
		wdedup::AppendFile wb(filename, "test");
		for(int i = 0; i < times; i ++) wb << i;
		wb << std::string("Haha");
		for(float i = 0.0; i < times; i += 1.0) wb << i;
		wb << wdedup::sync; // Flushing is required at EOF.
	}

	// Read phase of the append file.
	{
		wdedup::SequentialFile sb(filename, "test");
		{ int number; 
		for(int i = 0; i < times; i ++) {
			sb >> number; EXPECT_EQ(number, i); } }
		{ std::string text;
		sb >> text; EXPECT_EQ(text, "Haha"); }
		{ float number; 
		for(float i = 0.0; i < times; i += 1.0) {
			sb >> number; EXPECT_EQ(number, i); } }
		EXPECT_TRUE(sb.eof()); // Make sure no more content is written.
	}
}
