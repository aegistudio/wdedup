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
 * @file wtypes.hpp
 * @author Haoran Luo
 * @brief wdedup Basic types header.
 *
 * This file describes the basic types that are shared between 
 * different header/source files in wdedup.
 */
#pragma once
#include <string>

namespace wdedup {

/**
 * @brief thrown information about unrecoverable errors.
 *
 * The error assumes no defect in wdedup itself and the error is 
 * caused by file given. The main program catches and reports them.
 *
 * Other errors like std::out_of_memory will just terminate the 
 * program with uncaught exception reported. There's no need to
 * attempt recovering the result.
 */
struct Error {
	/// @brief The errno for this error (if any).
	int eno;

	/// @brief The file causing such I/O error. Usually full
	/// path of the file is required.
	std::string path;

	/// @brief The role of the file while processing.
	std::string role;

	/// Constructor for the error struct.
	Error(int eno, std::string path, std::string role) noexcept: 
		eno(eno), path(std::move(path)), role(std::move(role))  {}
};

} // namespace wdedup
