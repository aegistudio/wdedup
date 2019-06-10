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
 * @file wtreededup.cpp
 * @author Haoran Luo
 * @brief wdedup Tree Deduplication Algorithm Implementation.
 *
 * This file implements the wtreededup.hpp. See corresponding header
 * file for interface details.
 */
#include "impl/wtreededup.hpp"
#include "impl/wwmman.hpp"
#include "wbloom.hpp"
#include <cassert>
#include <cstring>

// Comparison interface for judging existence of object.
static int TreeDedupItemCmp(const TreeDedupItem* l, const TreeDedupItem* r) noexcept {
	return l->bloom - r->bloom;
}

// Generate interfaces for TreeDedupRbtree.
RB_GENERATE(TreeDedupRbtree, TreeDedupItem, rbnode, TreeDedupItemCmp);

namespace wdedup {

TreeDedup::TreeDedup(void* vmaddr, size_t vmsize) noexcept: 
	wmman(vmaddr, vmsize), root() {
	RB_INIT(&root);
}

TreeDedup::TreeDedup(TreeDedup&& rhs) noexcept:
	wmman(std::move(rhs.wmman)), root() {

	RB_INIT(&root);
	root.rbh_root = rhs.root.rbh_root;
	rhs.root.rbh_root = nullptr;
}

bool TreeDedup::insert(const std::string& word, fileoff_t offset) noexcept {
	if(word.size() == 0) return false; // Invalid word specified.

	// Profile the word.
	Bloom bloomed;
	size_t allocpool = bloomed.decompose(word);

	// Test whether the item already exists in the tree.
	{
		TreeDedupItem search;
		search.bloom = bloomed;

		// If item is found, mark item as repeated and return directly.
		TreeDedupItem* find = TreeDedupRbtree_RB_FIND(&root, &search);
		if(find != NULL) {
			find->occur = 0;
			return true;
		}
	}

	// Allocate new portion of memory.
	TreeDedupItem* newitem = nullptr;
	char* newpool = nullptr;
	if(!wmman.alloc(allocpool, newitem, newpool)) return false;

	// Initialize the tree node details.
	newitem->bloom = bloomed;	newitem->occur = offset + 1;
	if(allocpool > 0) {
		memcpy(newpool, bloomed.pool, allocpool - 1);
		newpool[allocpool - 1] = '\0';
		newitem->bloom.pool = newpool;
	}

	// Insert the tree node into the rbtree.
	TreeDedupRbtree_RB_INSERT(&root, newitem);

	return true;
}

void TreeDedup::pour(
	TreeDedup dedup, std::unique_ptr<wdedup::ProfileOutput> output
) throw (wdedup::Error) {
	assert(output != nullptr);

	// Iterate over every node inside the tree.
	TreeDedupItem* it = RB_MIN(TreeDedupRbtree, &dedup.root);
	for(; it != nullptr; it = TreeDedupRbtree_RB_NEXT(it)) {
		std::string word = it->bloom.reconstruct();
		if(it->occur == 0) output->push(ProfileItem(word));
		else output->push(ProfileItem(word, it->occur - 1));
	}
	output->close();
}

} // namespace wdedup
