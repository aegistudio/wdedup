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
 * @brief Indicates the execution plan of merging stage.
 */
struct WMergePlan {
	/// Left node to be merged.
	size_t left;

	/// Right node to be merged.
	size_t right;

	/// Output node of this merging stage.
	size_t out;

	/// Default constructor for the merge plan.
	WMergePlan(size_t left, size_t right, size_t out) noexcept:
		left(left), right(right), out(out) {}
};

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
	wdedup::Config& cfg, size_t leaves
) throw (wdedup::Error) {
	// Eliminate corner conditions.
	if(leaves == 0) cfg.logCorrupt();
	if(leaves == 1) return 0;

	// Generate merging plans, the last item will be the final result.
	std::vector<wdedup::WMergePlan> plans;
	{
		// The queues recording the remaining nodes in the queue.
		// The value of nodes will be index+1 so that 0 can be used
		// as layer separator.
		std::queue<size_t> nodes;

		// The newly generated merged profile index.
		size_t put = leaves;

		// Push initial nodes (leaf nodes).
		for(size_t i = 0; i < leaves; i ++)
			nodes.push(i + 1);
		nodes.push(0);

		// Deduce nodes of every layers.
		size_t layerNodes = 0;
		while(true) {
			bool endLayer = false;
			size_t left, right;

			// Fetch the left node to be merged.
			if(!endLayer) {
				left = nodes.front(); nodes.pop();
				if(left == 0) endLayer = true;
				else ++ layerNodes;
			}

			// Fetch the right node to be merged.
			if(!endLayer) {
				right = nodes.front(); nodes.pop();
				if(right == 0) endLayer = true;
				else ++ layerNodes;
			}

			// Test whether it is the end of current layer.
			if(endLayer) {
				assert(layerNodes > 0);
				if(layerNodes == 1) break; // Single node.

				// Push back and continue.
				if(left != 0) nodes.push(left);
				layerNodes = 0; nodes.push(0); 
				continue; 
			}

			// Write out current node.
			plans.push_back(WMergePlan(left - 1, right - 1, put));
			nodes.push(put + 1); ++ put;
		}
	}

	// Attempt to recover log and remove processed nodes.
	// Currently the removed nodes must match the order of
	// dequeued node.
	size_t cursor = 0;
	if(!cfg.hasRecoveryDone()) while(!(cfg.ilog().eof())) {
		// Parse the current line of log.
		char type; cfg.ilog() >> type;
		switch(type) {
		case (char)wdedup::WMergeLog::end:
			// Indicates this is the end of current log
			// and wprof stage has been completed.
			if(cursor != plans.size()) cfg.logCorrupt();
			return plans[plans.size() - 1].out;
		case (char)wdedup::WMergeLog::merge:
			// Parse the segment parameters.
			size_t left, right, out;
			cfg.ilog() >> left >> right >> out;

			// If the logged item does not match the 
			// working items, report errors.
			if(plans[cursor].left != left) cfg.logCorrupt();
			if(plans[cursor].right != right) cfg.logCorrupt();
			if(plans[cursor].out != out) cfg.logCorrupt();
			++ cursor;

			// Garbage collect merged nodes.
			cfg.remove(std::to_string(left));
			cfg.remove(std::to_string(right));
			break;
		default:
			// Report corruption for unknown log item type.
			cfg.logCorrupt();
		}
	}

	// Recovery should be ended in current stage, we must work and
	// produces loggings from current point and in later stages.
	cfg.recoveryDone();

	// Perform iterative merging on the generated profiles.
	for(; cursor < plans.size(); ++ cursor) {
		// Open the profiles for writing.
		const wdedup::WMergePlan& current = plans[cursor];
		std::unique_ptr<wdedup::ProfileInput> left =
			cfg.openInput(std::to_string(current.left));
		std::unique_ptr<wdedup::ProfileInput> right =
			cfg.openInput(std::to_string(current.right));
		std::unique_ptr<wdedup::ProfileOutput> out =
			cfg.openOutput(std::to_string(current.out));

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
		out->close();

		// Write out the persistent finished log.
		cfg.olog() << wdedup::WMergeLog::merge << 
			current.left << current.right << current.out << wdedup::sync;
		
		// Perform garbage collection.
		cfg.remove(std::to_string(current.left));
		cfg.remove(std::to_string(current.right));
	}

	// Return the single node of current layer.
	cfg.olog() << wdedup::WMergeLog::end << wdedup::sync;
	return plans[plans.size() - 1].out;
}

} // namespace wdedup
