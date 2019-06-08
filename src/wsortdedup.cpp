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
#include <cassert>
#include <cstring>
#include <sstream>
#include <algorithm>

namespace wdedup {

/// SortDedupItem used for storing information about words.
struct SortDedupItem final {
	/// Pointer to string pool, or 0 if there's no more content 
	/// to be pooled. The pointee string must be null terminated
	/// so it is qualified C-string.
	char* pool;

	/// The first characters in the string pool. You can perform
	/// bloom filtering instead of string comparisons.
	unsigned long bloom;

	/// The first occurence of this item.
	fileoff_t occur;

	/// Calculate difference between this item and that item.
	int operator-(const SortDedupItem& that) const noexcept {
		// Compare the bloom.
		if(bloom != that.bloom) 
			return bloom > that.bloom? 1 : -1;

		// Compare the nullity of the heap.
		if(pool == nullptr) {
			return that.pool == nullptr? 0 : -1;
		} else {
			if(that.pool == nullptr) return 1;
			return strcmp(pool, that.pool);
		}
	}

	/// Indicate this item is greater than the next one.
	bool operator<(const SortDedupItem& that) const noexcept {
		return (*this - that) < 0;
	}

	/// Indicates this item equals the next one.
	bool operator==(const SortDedupItem& that) const noexcept {
		return (*this - that) == 0;
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
	unsigned long bloom = 0;
	size_t n = 0; for(; n < sizeof(unsigned long); ++ n) {
		char w = n < word.size()? word[n] : 0;
		bloom = (bloom << 8) | (w & 0x0ffl);
	}
	size_t allocpool = 0; if(n < word.size()) 
		// If allocation is inevitable, the allocated string must be null 
		// terminated, so the size must plus one.
		allocpool = word.size() - n + 1;

	// Test whether allocation will be done.
	size_t newvmsize = (allocpool + poolsize) 
		+ (arraysize + 1) * sizeof(SortDedupItem);
	if(newvmsize > vmsize) return false;

	// Push the new item into the deduplication sorter.
	size_t curarraysize = arraysize; arraysize ++;
	SortDedupItem& item = ((SortDedupItem*)vmaddr)[curarraysize];
	item.bloom = bloom;	item.occur = offset;
	if(allocpool > 0) {
		size_t newpoolsize = poolsize + allocpool;
		poolsize = newpoolsize;
		char* pool = &((char*)vmaddr)[vmsize - newpoolsize];
		memcpy(pool, &word[n], allocpool - 1);
		pool[allocpool] = '\0';
		item.pool = pool;
	} else item.pool = nullptr;
	return true;
}

void SortDedup::pour(
	SortDedup dedup, std::unique_ptr<wdedup::ProfileOutput> output
) throw (wdedup::Error) {
	assert(output != nullptr);

	// Public code for writing out item based on duplication.
	auto writeitem = [&] (const SortDedupItem& item, size_t first, size_t last) {
		std::string word;

		// Construct the string content.
		{
			std::stringstream sbuild;

			// Reconstruct the string from the bloom.
			char bloomrev[sizeof(unsigned long) + 1];
			char* pointer = &bloomrev[sizeof(unsigned long)];
			*pointer = '\0';	// Make it a C-string.
			for(int i = 0; i < sizeof(unsigned long); i ++) {
				pointer --;
				*pointer = (char)((item.bloom >> (8 * i)) & 0x0ff);
			}
			sbuild << bloomrev;

			// Reconstruct the string from the pool.
			if(item.pool != nullptr) sbuild << item.pool;
			word = std::move(sbuild.str());
		}

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
