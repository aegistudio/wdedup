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
 * @file wmerge.cpp
 * @author Haoran Luo
 * @brief wdedup Merge Implementation
 *
 * This file implements the merging function, see the header file
 * for more definition details.
 */
#include "wdedup.hpp"
#include <queue>
#include <cassert>

namespace wdedup {

/**
 * @brief Indicates the type of current log item.
 *
 * Giving wmerge log items type makes it easier to determine the
 * boundary of stage from the perspective of logging.
 */
enum class WMergeLog : char {
	/**
	 * @brief Records a successfully merged profile.
	 *
	 * A payload of execution plan will be followed.
	 */
	merge = 'm',

	/// Indicates the ending of wmerge stage.
	end = 'x'
};

size_t wmerge(
	wdedup::Config& cfg, wdedup::MergePlanner& planner, bool disableGC
) throw (wdedup::Error) {
	wdedup::MergePlan plan;

	// Attempt to recover log and remove processed nodes.
	// Currently the removed nodes must match the order of
	// dequeued node.
	if(!cfg.hasRecoveryDone()) while(!(cfg.ilog().eof())) {
		// Parse the current line of log.
		char type; cfg.ilog() >> type;
		switch(type) {
		case (char)wdedup::WMergeLog::end:
			// Indicates this is the end of current log
			// and wprof stage has been completed.
			if(planner.pop(plan)) cfg.logCorrupt();
			return plan.id;
		case (char)wdedup::WMergeLog::merge:
			// Parse the segment parameters.
			size_t left, right, out, size;
			cfg.ilog() >> left >> right >> out >> size;

			// If the logged item does not match the 
			// working items, report errors.
			if(!planner.pop(plan)) cfg.logCorrupt();
			if(plan.left != left) cfg.logCorrupt();
			if(plan.right != right) cfg.logCorrupt();
			if(plan.id != out) cfg.logCorrupt();

			// Garbage collect merged nodes.
			if(!disableGC) {
				cfg.remove(std::to_string(left));
				cfg.remove(std::to_string(right));
			}

			// Place back the merged node.
			wdedup::MergeSegment merged;
			merged.plan = plan;
			merged.size = size;
			planner.push(merged);

			break; // switch.
		default:
			// Report corruption for unknown log item type.
			cfg.logCorrupt();
		}
	}

	// Recovery should be ended in current stage, we must work and
	// produces loggings from current point and in later stages.
	cfg.recoveryDone();

	// Perform iterative merging on the generated profiles.
	while(planner.pop(plan)) {
		std::unique_ptr<wdedup::ProfileInput> left =
			cfg.openInput(std::to_string(plan.left));
		std::unique_ptr<wdedup::ProfileInput> right =
			cfg.openInput(std::to_string(plan.right));
		std::unique_ptr<wdedup::ProfileOutput> out =
			cfg.openOutput(std::to_string(plan.id));

		// Remove the lesser one to the output node.
		while((!left->empty()) && (!right->empty())) {
			// Reserve the different log profile items.
			if(left->peek().word < right->peek().word)
				out->push(std::move(left->pop()));
			else if(left->peek().word > right->peek().word)
				out->push(std::move(right->pop()));

			// Merge the same profile items into repeated item.
			else {
				out->push(ProfileItem(left->peek().word));
				left->pop(); right->pop();
			}
		}

		// Finish up the left one by pushing.
		while(!left->empty()) out->push(std::move(left->pop()));
		while(!right->empty()) out->push(std::move(right->pop()));
		size_t size = out->close();

		// Write out the persistent finished log.
		cfg.olog() << wdedup::WMergeLog::merge 
			<< plan.left << plan.right << plan.id 
			<< size << wdedup::sync;
		
		// Perform garbage collection.
		if(!disableGC) {
			cfg.remove(std::to_string(plan.left));
			cfg.remove(std::to_string(plan.right));
		}

		// Place back the merged node.
		wdedup::MergeSegment merged;
		merged.plan = plan;
		merged.size = size;
		planner.push(merged);
	}

	// Return the single node of current layer.
	cfg.olog() << wdedup::WMergeLog::end << wdedup::sync;
	return plan.id;
}

} // namespace wdedup
