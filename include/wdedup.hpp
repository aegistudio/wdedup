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
 * @file wdedup.hpp
 * @author Haoran Luo
 * @brief wdedup Stages Definition
 *
 * This file defines different stages inside word deduplication tool.
 * The main program orchestrate these stages, other libraries could
 * also pick up some of the stages as their own use.
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
 * @param[in] syncDistance the synchronization distance, set to
 * 0 means to disable such synchronization.
 * @return the maximum id of the output file. The file may be 
 * missing if later stage removes these file (via GC), and wprof 
 * will not verify that if ilog indicates this stage has been 
 * completed.
 * @throw wdedup::Error when specified original file is missing,
 * cannot create file under working directory, etc.
 */
size_t wprof(wdedup::Config& cfg, const std::string& path, 
	size_t syncDistance = 0) throw (wdedup::Error);

/**
 * @brief Executes the merge stage on the original file.
 *
 * Please notice when the underlying log indicates wmerge has been 
 * finished, the wmerge stage simply collects the final file name
 * from the log. No I/O will be performed in such situation.
 *
 * @param[inout] cfg the configuration of current task.
 * @param[in] leaves number of leaf profiles generated by wprof.
 * @param[in] disableGC disable garbage collection. Please notice
 * that files GC-ed in previous execution can not be recovered.
 * @return the id of the final merged log. The files that are
 * lower than this id might be missing due to GC. All GC operation
 * must be done after logging.  And wmerge will not verify if ilog
 * indicates this stage has been completed.
 * @throw wdedup::Error when some of the profile file is missing,
 * cannot create file under working directory, etc.
 */
size_t wmerge(wdedup::Config& cfg, size_t leaves, 
		bool disableGC) throw (wdedup::Error);

/**
 * @brief Executes the find-first stage on the original file.
 *
 * There's no logging generated in this stage. So scanning the final
 * merged log will be repeated if executed on a working directory
 * containing a finished task. (It also makes it possible to
 * execute different task based on the final merged result).
 * @throw wdedup::Error when the final profile is missing, etc.
 *
 * @return empty string if all words are duplicated, or the single
 * string that appers first.
 */
std::string wfindfirst(wdedup::Config& cfg, size_t root) 
		throw (wdedup::Error);
} // namespace wdedup
