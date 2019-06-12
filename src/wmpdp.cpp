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
 * @brief wdedup Dynamic Programming Merge Planner Implementation
 *
 * This file implements the wdedup::MergePlannerDP, see the header 
 * file for more definition details.
 */
#include "wdedup.hpp"
#include "impl/wmpdp.hpp"
#include <queue>
#include <cassert>

namespace wdedup {

MergePlannerDP::MergePlannerDP(wdedup::Config& config,
	std::vector<ProfileSegment> segment): plans(), cursor(0) {
	
	// Eliminate root cases.
	if(segment.size() == 0) config.logCorrupt();
	if(segment.size() == 1) {
		root = segment[0].id;
		return;
	}

	// Perform dynamic programming.
	struct dpItem {
		// The size of current item.
		size_t length;

		// The cost to merge current item.
		size_t cost;

		// The separation point c, where [a, c] and
		// [c+1, b] forms the item.
		size_t separation;

		// The generated id, used for assigning.
		size_t id;
	};
	size_t n = segment.size();
	std::vector<std::unique_ptr<dpItem[]>> dp(n);
	for(size_t i = 0; i < n; ++ i) 
		dp[i] = std::unique_ptr<dpItem[]>(new dpItem[n]);

	// Initialize the diagonal line.
	size_t put = 0;
	for(size_t i = 0; i < n; ++ i) {
		dp[i][i].length = segment[i].size;
		dp[i][i].cost = 0;
		dp[i][i].separation = 0;
		dp[i][i].id = segment[i].id + 1;
		if(segment[i].id >= put) put = segment[i].id + 1;
	}

	// The cost calculation function.
	auto costFunc = [](const dpItem& l, const dpItem& r) -> size_t {
		return l.cost + r.cost + (l.length + r.length) * 2;
	};

	// Calculate the cost and the separation.
	for(size_t l = 1; l < n; ++ l) { // l: the stride.
		for(size_t j = 0; j + l < n; ++ j) { // [j, j+l]
			size_t m = j + l;
			dp[j][m].cost = costFunc(dp[j][j], dp[j + 1][m]);
			dp[j][m].length = dp[j][j].length + dp[j + 1][m].length;
			dp[j][m].separation = j;
			dp[j][m].id = 0;
			for(size_t k = j + 1; k + 1 <= m; ++ k) {
				// [j,k] and [k+1, j+l].
				size_t newcost = costFunc(dp[j][k], dp[k + 1][m]);
				if(newcost < dp[j][m].cost) {
					dp[j][m].cost = newcost;
					dp[j][m].separation = k;
				}
			}
		}
	}

	// Retrace the nodes required to assemble the root node.
	struct queueItem {
		size_t l, r;
		queueItem(size_t l, size_t r) noexcept: l(l), r(r) {}
	};
	std::vector<queueItem> queue;
	queue.push_back(queueItem(0, n - 1));
	for(size_t k = 0; k < queue.size(); ++ k) {
		// Fetch the start and end point.
		size_t l = queue[k].l;
		size_t r = queue[k].r;
		assert(l < r);

		// Fetch the center and push new items.
		size_t s = dp[l][r].separation;
		if(s > l) queue.push_back(queueItem(l, s));
		if(s + 1 < r) queue.push_back(queueItem(s + 1, r));
	}

	// Retrace and generate the merge plan.
	for(size_t i = queue.size(); i > 0; -- i) {
		// Update the DP item for those on the path.
		size_t j = i - 1;
		dpItem& item = dp[queue[j].l][queue[j].r];
		dpItem& left = dp[queue[j].l][item.separation];
		assert(left.id != 0);
		dpItem& right = dp[item.separation + 1][queue[j].r];
		assert(right.id != 0);
		item.id = put + 1; ++ put;

		// Update the plans.
		wdedup::MergePlan plan;
		plan.left = left.id - 1;
		plan.right = right.id - 1;
		plan.id = item.id - 1;
		plans.push_back(plan);
	}

	// Deduce the generated root node.
	root = plans[plans.size() - 1].id;
}

bool MergePlannerDP::pop(wdedup::MergePlan& plan) {
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
