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
 * @file wcli.hpp
 * @author Haoran Luo
 * @brief wdedup Command Line Interface Parser
 *
 * This file defines the command line parser, which parses arguments
 * for the main function.
 */
#pragma once
#include "wprofile.hpp"
#include <memory>

namespace wdedup {

/// @brief Parsed program options.
struct ProgramOptions {
	/// The original file name, should never be empty.
	std::string origfile;

	/// The working directory, should never be empty.
	std::string workdir;

	/// Would the program needs to execute on.
	bool run;

	/// The size of working memory, in unit of bytes.
	size_t workmem;

	/// Whether the working memory will be page pinned.
	bool pagePinned;

	/// Whether the profile only flag is specified.
	bool profileOnly;

	/// Whether the merge only flag is specified.
	bool mergeOnly;

	/// Whether garbage collection is disabled.
	bool disableGC;
};

/**
 * @brief Parses the command line argument of wdedup.
 *
 * When the returned value is not zero, an error occurs, and the 
 * result is the error code. When the returned value is zero, depends 
 * on whether options.run is set to true to either execute on or exit 
 * with code 0.
 */
int argparse(int argc, char** argv, wdedup::ProgramOptions& options);

/// The minimum working memory required to run the program.
static constexpr size_t minWorkmem = 4096;

} // namespace wdedup
