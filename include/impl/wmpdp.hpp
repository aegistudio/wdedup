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
 * @file wmpsimple.hpp
 * @author Haoran Luo
 * @brief wdedup Dynamic Programming Merge Planner
 *
 * This file defines the merge planner that perform dynamic 
 * programming to find a merge plan with minimal I/O.
 */
#pragma once
#include <string>
#include <vector>
#include "wtypes.hpp"
#include "wdedup.hpp"

namespace wdedup {

/// Defines the DP-based merge planner.
struct MergePlannerDP : public MergePlanner {
	/// Constructor of the merge planner.
	MergePlannerDP(wdedup::Config& config,
			std::vector<wdedup::ProfileSegment>);

	/// Virtual destructor for pure virtual class.
	virtual ~MergePlannerDP() noexcept {}

	/// Implements the pop function.
	virtual bool pop(wdedup::MergePlan& plan) override;

	/// Implements the push function.
	virtual void push(wdedup::MergeSegment) override {}
private:
	/// The generated plan.
	std::vector<wdedup::MergePlan> plans;

	/// The current cursor on the plans.
	size_t cursor;

	/// The root node of the merged plans.
	size_t root;
};

} // namespace wdedup
