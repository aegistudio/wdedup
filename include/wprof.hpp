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
 * @file wprof.hpp
 * @author Haoran Luo
 * @brief wdedup Profiler Definition
 *
 * This file defines the interface for wprof, which profiles the
 * original input file and creates SSTable files covering every
 * single segments of input file.
 */
#pragma once
#include <string>
#include "wtypes.hpp"
#include "wconfig.hpp"

namespace wdedup {

/**
 * @brief Executes the profiler on the original file.
 *
 * Please notice when the underlying log indicates wprof has been 
 * finished, the wprof stage simply collects those file names from 
 * the log. No I/O will be performed in such situation.
 *
 * @param[inout] cfg the configuration of current task.
 * @param[in] path the original file path.
 * @return the maximum id of the output file. The file may be 
 * missing if later stage removes these file (via GC), and wprof 
 * will not verify that if cfg.log indicates this stage has been 
 * completed.
 * @throw wdedup::error when specified original file is missing,
 * cannot create file under working directory, etc.
 */
size_t wprof(wdedup::Config& cfg, const std::string& path) 
		throw (wdedup::Error);
	
} // namespace wdedup
