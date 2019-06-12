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
 * @file wsortdedup.hpp
 * @author Haoran Luo
 * @brief wdedup Sort Deduplication Algorithm
 *
 * This file defines the sort deduplication algorithm interface. It
 * can be used in the profiling stage to generate a deduplication
 * profile from the original input file.
 */
#pragma once
#include "wprofile.hpp"
#include "impl/wwmman.hpp"
#include "wbloom.hpp"
#include <memory>

namespace wdedup {

/// SortDedupItem used for storing information about words.
struct SortDedupItem final {
	/// The Bloom-ed string key.
	wdedup::Bloom bloom;

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


/**
 * @brief This file defines the sort analogous deduplication
 * algorithm that is done on the specified working memory.
 *
 * It is guaranteed that no additional malloc is called when using
 * the working memory, and no memory will be leaked when the
 * wdedup::SortDedup object get destructed.
 */
struct SortDedup final {
	/// Build the SortDedup upon preallocated working memory.
	SortDedup(void*, size_t) noexcept;

	/// Copy constructor is deleted for SortDedup.
	SortDedup(const SortDedup&) = delete;

	/// Move constructor is required for pour operation.
	SortDedup(SortDedup&&) noexcept;

	/// Deconstruct the working memory.
	~SortDedup() noexcept {}

	/**
	 * Insert a word into the dedup pool.
	 *
	 * @param[in] word the word to be appended.
	 * @param[in] len the length of the word.
	 * @param[in] offset the offset of the word in document.
	 * @return true if the dedup has appended the word, false
	 *         if it cannot be appended, false will be returned,
	 *         and the object remains unchanged.
	 */
	bool insert(const char* word, size_t len, fileoff_t offset) noexcept;

	/// Pour the content of SortDedup into an open file.
	/// The pool will be then inaccessible, no matter success
	/// or fail while pouring.
	/// Both pool and profile output will be automatically destroyed 
	/// once after the operation is done.
	static size_t pour(SortDedup, std::unique_ptr<wdedup::ProfileOutput>) 
			throw (wdedup::Error);
private:
	/// The working memory manager used to allocate objects.
	wdedup::MemoryManager<wdedup::SortDedupItem> wmman;
};

} // namespace wdedup
