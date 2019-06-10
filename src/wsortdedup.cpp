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
 * @file wsortdedup.cpp
 * @author Haoran Luo
 * @brief wdedup Sort Deduplication Algorithm Implementation.
 *
 * This file implements the wheapdedup.hpp. See corresponding header
 * file for interface details.
 */
#include "impl/wsortdedup.hpp"
#include "impl/wwmman.hpp"
#include "wbloom.hpp"
#include <cassert>
#include <cstring>
#include <sstream>
#include <algorithm>

namespace wdedup {

SortDedup::SortDedup(void* vmaddr, size_t vmsize) noexcept: 
	wmman(vmaddr, vmsize) {}

SortDedup::SortDedup(SortDedup&& rhs) noexcept:
	wmman(std::move(rhs.wmman)) {}

bool SortDedup::insert(const std::string& word, fileoff_t offset) noexcept {
	if(word.size() == 0) return false; // Invalid word specified.

	// Profile the word.
	Bloom bloomed;
	size_t allocpool = bloomed.decompose(word);

	// Allocate new portion of memory.
	SortDedupItem* newitem = nullptr;
	char* newpool = nullptr;
	if(!wmman.alloc(allocpool, newitem, newpool)) return false;

	// Push the new item into the deduplication sorter.
	newitem->bloom = bloomed;	newitem->occur = offset;
	if(allocpool > 0) {
		memcpy(newpool, bloomed.pool, allocpool - 1);
		newpool[allocpool - 1] = '\0';
		newitem->bloom.pool = newpool;
	}
	return true;
}

void SortDedup::pour(
	SortDedup dedup, std::unique_ptr<wdedup::ProfileOutput> output
) throw (wdedup::Error) {
	assert(output != nullptr);
	if(dedup.wmman.size() == 0) return;

	// Public code for writing out item based on duplication.
	auto writeitem = [&] (const SortDedupItem& item, size_t first, size_t last) {
		// Construct the string content.
		std::string word = item.bloom.reconstruct();

		// Construct other content.
		if(first == last) 
			output->push(ProfileItem(word, item.occur));
		else 	output->push(ProfileItem(word));
	};

	// Perform sorting on the array items.
	SortDedupItem* items = (SortDedupItem*)dedup.wmman.begin();
	std::sort(dedup.wmman.begin(), dedup.wmman.end());

	// Scan and sequentially output the content.
	size_t j = 0;
	for(size_t i = 1; i < dedup.wmman.size(); ++ i) {
		// Output the marked content if it is not a duplication.
		if(!(items[i] == items[j])) {
			writeitem(items[j], j, i - 1);
			j = i;
		}
	}
	writeitem(items[j], j, dedup.wmman.size() - 1);
	output->close();
}

} // namespace wdedup
