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
 * @file wio.cpp
 * @author Haoran Luo
 * @brief wdedup I/O orchestrations.
 *
 * See corresponding header for interface definitions.
 */
#include "impl/wiobase.hpp"

namespace wdedup {

// Returning a std::function for "::Impl"s to return error.
static inline std::function<void(int)> getReportFunction(
	std::string path, std::string role) {
	return [=](int eno) { 
		throw wdedup::Error(eno, path, role);
	};
}

SequentialFile::SequentialFile(std::string path, std::string role,
	FileMode mode) throw (wdedup::Error) : pimpl(nullptr) {

	// Initialize the basic append file.
	pimpl = std::unique_ptr<SequentialFile::Impl>(
		new SequentialFileBase(path.c_str(), 
		getReportFunction(path, role)));
}

AppendFile::AppendFile(std::string path, std::string role, 
	FileMode mode) throw (wdedup::Error) : pimpl(nullptr) {

	if(mode.log) {
		// Initialize the log append file.
		pimpl = std::unique_ptr<AppendFile::Impl>(
			new AppendFileLog(path.c_str(), 
			getReportFunction(path, role)));
	} else {
		// Initialize the buffer append file.
		pimpl = std::unique_ptr<AppendFile::Impl>(
			new AppendFileBuffer(path.c_str(), 
			getReportFunction(path, role)));
	}
}

} // namespace wdedup
