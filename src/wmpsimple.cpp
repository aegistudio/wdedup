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
 * @file wmpsimple.cpp
 * @author Haoran Luo
 * @brief wdedup Simple Merge Planner Implementation
 *
 * This file implements the wdedup::MergePlannerSimple, see the header 
 * file for more definition details.
 */
#include "wdedup.hpp"
#include "impl/wmpsimple.hpp"
#include <queue>
#include <cassert>

namespace wdedup {

MergePlannerSimple::MergePlannerSimple(wdedup::Config& config,
	std::vector<ProfileSegment> segment): plans(), cursor(0) {
	
	// Eliminate root cases.
	if(segment.size() == 0) config.logCorrupt();
	if(segment.size() == 1) {
		root = segment[0].id;
		return;
	}

	// The queues recording the remaining nodes in the queue.
	// The value of nodes will be index+1 so that 0 can be used
	// as layer separator.
	std::queue<size_t> nodes;

	// The next node to be generated.
	size_t put = 0;

	// Push initial nodes (leaf nodes).
	for(size_t i = 0; i < segment.size(); i ++) {
		nodes.push(segment[i].id + 1);
		if(segment[i].id >= put) put = segment[i].id + 1;
	}
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
		wdedup::MergePlan plan;
		plan.left = left - 1;
		plan.right = right - 1;
		plan.id = put;
		plans.push_back(plan);
		nodes.push(put + 1); ++ put;
	}

	// Deduce the generated root node.
	root = plans[plans.size() - 1].id;
}

bool MergePlannerSimple::pop(wdedup::MergePlan& plan) {
	if(cursor < plans.size()) {
		plan = plans[cursor];
		++ cursor;
		return true;
	} else {
		plan.id = root;
		return false;
	}
}

} // namespace wdedup
