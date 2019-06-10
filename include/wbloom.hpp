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
 * @file wbloom.hpp
 * @author Haoran Luo
 * @brief wdedup Bloom-ed string
 *
 * This file defines the Bloom-ed string struct: a string is decomposed 
 * into a bloom part and a pool part. The bloom struct is usually 
 * embedded into larger data structure nodes, so that fixed size operation 
 * can be performed, and cache coherency will be increased.
 * 
 * Please notice that the bloom does not manage the pool passed in, so 
 * the pooled string needs to be manually managed by their caller.
 */
#pragma once
#include <string>
#include <sstream>
#include <cstring>

namespace wdedup {

/**
 * @brief The Bloom struct.
 *
 * The struct can be embedded into data nodes to represent a 
 * std::string key. The class also provides decompose, comparison and 
 * reconstruct operations.
 */
struct Bloom final {
	/// The type of the bloom part.
	typedef unsigned long bloom_t;

	/// The Bloom part of the original string.
	bloom_t bloom;

	/// The pool part of the original string.
	/// It is NOT managed by this struct.
	const char* pool;

	/// Construct a empty bloom string.
	Bloom() noexcept: bloom(0), pool(nullptr) {}

	/// Destructor of the Bloom class.
	~Bloom() noexcept {}

	/// Copy constructor of the bloomed string.
	Bloom(const Bloom& b) noexcept: bloom(b.bloom), pool(b.pool) {}

	/**
	 * @brief Decompose a string into Bloom-ed string.
	 *
	 * The pool part will based on word.c_str(). And the length of 
	 * the pool part will be returned.
	 *
	 * @param[in] word the string to be decomposed.
	 * @return the length of the bloomed part.
	 */
	inline size_t decompose(const std::string& word) noexcept {
		// Profile the word.
		bloom = (bloom_t)0; pool = nullptr;
		size_t n = 0; for(; n < sizeof(bloom_t); ++ n) {
			char w = n < word.size()? word[n] : 0;
			bloom = (bloom_t)((bloom << 8) | (w & 0x0ffl));
		}
		size_t allocpool = 0; if(n < word.size()) {
			// If allocation is inevitable, the allocated 
			// string must be null terminated, so the size 
			/// must plus one.
			allocpool = word.size() - n + 1;
			pool = &word[n];
		}
		return allocpool;
	}

	/**
	 * @brief Difference operator of the Bloom-ed string.
	 *
	 * The bloomed part will be compared first. And if the 
	 * prefix represented by the bloom part are identical, the
	 * pool part will be compared then.
	 */
	inline int operator-(const Bloom& that) const noexcept {
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

	/// Delegated less-than operator of the bloom.
	inline bool operator<(const Bloom& that) const noexcept {
		return (*this - that) < 0;
	}

	/// Delegated equal operator of the bloom.
	inline bool operator==(const Bloom& that) const noexcept {
		return (*this - that) == 0;
	}

	/// Reconstruct the original string and return.
	inline std::string reconstruct() const {
		std::stringstream sbuild;

		// Reconstruct the string from the bloom.
		char bloomrev[sizeof(bloom_t) + 1];
		char* pointer = &bloomrev[sizeof(bloom_t)];
		*pointer = '\0';	// Make it a C-string.
		for(int i = 0; i < sizeof(bloom_t); i ++) {
			pointer --;
			*pointer = (char)((bloom >> (8 * i)) & 0x0ff);
		}
		sbuild << bloomrev;

		// Reconstruct the string from the pool.
		if(pool != nullptr) sbuild << pool;
		return std::move(sbuild.str());
	}
};

} // namespace wdedup
