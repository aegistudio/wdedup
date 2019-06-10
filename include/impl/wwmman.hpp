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
 * @file wwmman.hpp
 * @author Haoran Luo
 * @brief wdedup Working Memory Manager.
 *
 * This file defines the working memory manager. The working memory
 * is allocated in a double-ended flavour:
 * - Array End allocates fixed size value objects. Please notice that
 *   the object will not get destructed so that the objects needs to
 *   be trivially destructible and default constructible.
 * - Pool End allocates variant sized string. The variant sized end
 *   will also not run destructor as well as any constructor.
 */
#pragma once
#include <cstddef>
#include <type_traits>

namespace wdedup {

/// @brief Working Memory Manager
template<typename itemType> struct MemoryManager {
	/// Make sure valid item type is used before creating.
	static_assert(	std::is_trivially_destructible<itemType>::value &&
			std::is_nothrow_default_constructible<itemType>::value,
			"The specified class needs to be trivially destructible "
			"and nothrow default constructible.");

	/// Build the SortDedup upon preallocated working memory.
	MemoryManager(void* vmaddr, size_t vmsize) noexcept:
		vmaddr(vmaddr), vmsize(vmsize),
		poolsize(0), arraysize(0) {}

	/// Copy constructor is deleted for SortDedup.
	MemoryManager(const MemoryManager&) = delete;

	/// Move constructor is required for pour operation.
	MemoryManager(MemoryManager&& rhs) noexcept:
		vmaddr(rhs.vmaddr), vmsize(rhs.vmsize),
		poolsize(rhs.poolsize), arraysize(rhs.arraysize) {
		rhs.vmaddr = nullptr;
		rhs.vmsize = 0;
		rhs.poolsize = 0;
		rhs.arraysize = 0;
	}

	/// Deconstruct the working memory.
	~MemoryManager() noexcept {}

	/**
	 * @brief Allocates new portion of working memory.
	 *
	 * If memory can be allocated, it is also initialized and returned 
	 * to the user, true will be returned. Otherwise false will be 
	 * returned and the manager remains unchanged.
	 *
	 * Under no circumstance should alloc throws exception.
	 * @param[in] allocpool size of object allocated in pool end.
	 * @param[out] item allocated item, unchanged if allocation fails.
	 * @param[out] pool allocated pool, unchanged if allocpool == 0 or 
	 * allocation fails.
	 * @return true if allocation complete, false otherwise.
	 */
	bool alloc(size_t allocpool, itemType*& item, char*& pool) noexcept {
		// Test whether allocation can be completed.
		size_t newvmsize = (allocpool + poolsize) 
			+ (arraysize + 1) * sizeof(itemType);
		if(newvmsize > vmsize) return false;

		// Allocate new item on the array side.
		size_t curarraysize = arraysize; arraysize ++;
		item = &(begin()[curarraysize]);
		new ((void*)item) itemType(); // Placement new on item.

		// Allocate new pool item on the pool side.
		if(allocpool > 0) {
			size_t newpoolsize = poolsize + allocpool;
			poolsize = newpoolsize;
			pool = &((char*)vmaddr)[vmsize - newpoolsize];
		}

		return true;
	}

	// Return the start of the array, performing type casting.
	itemType* begin() const noexcept { 
		return reinterpret_cast<itemType*>(vmaddr);
	}

	// Return the end of the array, performing type casting.
	itemType* end() const noexcept { return &(begin())[arraysize]; }

	// Return number of item allocated by manager.
	size_t size() const noexcept { return arraysize; }
private:
	/// The virtual memory used as working memory.
	void* vmaddr;

	/// The available size of memory that can be used for 
	/// deduplication. In unit of bytes.
	size_t vmsize;

	/// The size occupied by the string pool. Variadic string
	/// are stored in the pool. In unit of bytes.
	size_t poolsize;

	/// The size occupied by the array items. In unit of items.
	size_t arraysize;
};

} // namespace wdedup
