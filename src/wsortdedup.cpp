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
#include "wbloom.hpp"
#include <cassert>
#include <cstring>
#include <sstream>
#include <algorithm>

namespace wdedup {

/// SortDedupItem used for storing information about words.
struct SortDedupItem final {
	/// The Bloom-ed string key.
	Bloom bloom;

	/// The first occurence of this item.
	fileoff_t occur;

	/// Indicate this item equals the next one.
	bool operator==(const SortDedupItem& that) const noexcept {
		return bloom == that.bloom;
	}
	/// Indicate this item is less than the next one.
	bool operator<(const SortDedupItem& that) const noexcept {
		return bloom < that.bloom;
	}
};

SortDedup::SortDedup(void* vmaddr, size_t vmsize) noexcept:
	vmaddr(vmaddr), vmsize(vmsize), poolsize(0), arraysize(0) {}

SortDedup::SortDedup(SortDedup&& rhs) noexcept:
	vmaddr(rhs.vmaddr), vmsize(rhs.vmsize), 
	poolsize(rhs.poolsize), arraysize(rhs.arraysize) {

	// Make rhs an invalid wdedup::SortDedup object.
	rhs.vmaddr = (void*)0;
	rhs.vmsize = 0;
	rhs.poolsize = 0;
	rhs.arraysize = 0;
}

bool SortDedup::insert(const std::string& word, fileoff_t offset) noexcept {
	if(word.size() == 0) return false; // Invalid word specified.
	assert(vmsize > 0);	// If vmsize <= 0, it must be a bug in caller.

	// Profile the word.
	Bloom bloomed;
	size_t allocpool = bloomed.decompose(word);

	// Test whether allocation will be done.
	size_t newvmsize = (allocpool + poolsize) 
		+ (arraysize + 1) * sizeof(SortDedupItem);
	if(newvmsize > vmsize) return false;

	// Push the new item into the deduplication sorter.
	size_t curarraysize = arraysize; arraysize ++;
	SortDedupItem& item = ((SortDedupItem*)vmaddr)[curarraysize];
	item.bloom = bloomed;	item.occur = offset;
	if(allocpool > 0) {
		size_t newpoolsize = poolsize + allocpool;
		poolsize = newpoolsize;
		char* pool = &((char*)vmaddr)[vmsize - newpoolsize];
		memcpy(pool, bloomed.pool, allocpool - 1);
		pool[allocpool - 1] = '\0';
		item.bloom.pool = pool;
	}
	return true;
}

void SortDedup::pour(
	SortDedup dedup, std::unique_ptr<wdedup::ProfileOutput> output
) throw (wdedup::Error) {
	assert(output != nullptr);

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
	SortDedupItem* items = (SortDedupItem*)dedup.vmaddr;
	std::sort(&items[0], &items[dedup.arraysize]);

	// Scan and sequentially output the content.
	size_t j = 0;
	for(size_t i = 1; i < dedup.arraysize; ++ i) {
		// Output the marked content if it is not a duplication.
		if(!(items[i] == items[j])) {
			writeitem(items[j], j, i - 1);
			j = i;
		}
	}
	writeitem(items[j], j, dedup.arraysize - 1);
	output->close();
}

} // namespace wdedup
