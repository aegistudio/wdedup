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
 * @brief Defines a profile segment.
 *
 * Statistical information are collected while executing wprof 
 * so that they can be used in merge planning.
 */
struct ProfileSegment {
	/// ID of the profile segment.
	size_t id;

	/// Start of current profile segment.
	size_t start;

	/// End of current profile segment.
	size_t end;

	/// (Physical) size of current profile segment. 
	size_t size;
};

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
 * @return the file generated while profiling. All file MUST be
 * ordered by their order corresponding to original file, and 
 * none of them should overlaps.
 * @throw wdedup::Error when specified original file is missing,
 * cannot create file under working directory, etc.
 */
std::vector<wdedup::ProfileSegment>
wprof(wdedup::Config& cfg, const std::string& path, 
	size_t syncDistance = 0) throw (wdedup::Error);

/// @brief Defines a merge plan.
struct MergePlan {
	/// ID of the merged segment. It MUST NOT conflicts with
	/// those ID appeared in profile segment.
	size_t id;

	/// Left segment ID of the merged segment.
	size_t left;

	/// Right segment ID of the merged segment.
	size_t right;
};

/**
 * @brief Defines a merged segment.
 *
 * Statistical information are collected while executing wmerge
 * and they can be used for dynamically planning with merge planner.
 */
struct MergeSegment {
	/// The plan that has been executed.
	wdedup::MergePlan plan;

	/// (Physical) size of the merged segment.
	size_t size;
};

/**
 * @brief Defines the merge planner.
 *
 * The merge planner determines next nodes to merge. Valid merge 
 * plan can be generated based on leaf segments information collected
 * in wprof stage result, and merged segments information collecting
 * as wmerge executes.
 *
 * The merge planner must ensure that it yields the identical result
 * when given same leaf segments information and merged segments
 * information.
 */
struct MergePlanner {
	/// Virtual destructor for pure virtual class.
	virtual ~MergePlanner() noexcept {}

	/**
	 * @brief Pop the execution plan for execution. 
	 * 
	 * @param[in] plan current output merge plan.
	 * @return if true is returned, it means there's merging to be 
	 * performed. If false is returned, all pages are merged
	 * and the plan.id must be set to the finally merged page.
	 */
	virtual bool pop(wdedup::MergePlan& plan) = 0;

	/// Push the lastly merged profile for re-planning.
	virtual void push(wdedup::MergeSegment) = 0;
};

/**
 * @brief Executes the merge stage on the original file.
 *
 * Please notice when the underlying log indicates wmerge has been 
 * finished, the wmerge stage simply collects the final file name
 * from the log. No I/O will be performed in such situation.
 *
 * @param[inout] cfg the configuration of current task.
 * @param[in] planner provides merge plan for wmerge.
 * @param[in] disableGC disable garbage collection. Please notice
 * that files GC-ed in previous execution can not be recovered.
 * @return the id of the final merged log. The files that are
 * lower than this id might be missing due to GC. All GC operation
 * must be done after logging.  And wmerge will not verify if ilog
 * indicates this stage has been completed.
 * @throw wdedup::Error when some of the profile file is missing,
 * cannot create file under working directory, etc.
 */
size_t wmerge(wdedup::Config& cfg, wdedup::MergePlanner& planner, 
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
