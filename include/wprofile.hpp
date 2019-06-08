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
 * @file wprofile.hpp
 * @author Haoran Luo
 * @brief wdedup Profile header.
 *
 * This file describes the profile structures. Profiles are stored in a
 * (logically) sorted-by-word, FIFO and immutable file.
 *
 * According to previous studying of sorted table implementation, multiple 
 * variance of physical implementation is observed, and which one excels 
 * has not been verified. So the profiler file is designed as a virtual 
 * interface, and provided as wdedup::Config item.
 */
#pragma once
#include "wtypes.hpp"
#include <string>

namespace wdedup {

/**
 * @brief Defines the profile item in profile input and output.
 *
 * Various implementation must customize their interfaces to return such
 * kind of items.
 */
struct ProfileItem {
 	/// Current recorded word.
	std::string word;

	/// Whether this word has been repeated.
	bool repeated;

	/// The first occurence of the word.
	/// If repeated is true, this field should be ignored by the
	/// scanning algorithms.
	fileoff_t occur;

	/// Construct a repeated item.
	ProfileItem(std::string word) noexcept: 
		word(std::move(word)), repeated(true), occur(0) {}

	/// Construct a single occurence item.
	ProfileItem(std::string word, fileoff_t occur) noexcept:
		word(std::move(word)), repeated(false), occur(occur) {}

	/// Move constructor of a profile item.
	ProfileItem(ProfileItem&& item) noexcept:
		word(std::move(item.word)), 
		repeated(item.repeated), occur(item.occur) {}
};

/// @brief Defines the virtual read interface of profile.
struct ProfileInput {
	/// Virtual destructor for pure virtual classes.
	virtual ~ProfileInput() noexcept {};

	/// Test whether there's content in the file.
	virtual bool empty() const noexcept = 0;

	/// Peeking the head profileItem from the input table.
	/// If there's no more content in the table, the content
	/// returned by the table will be undefined.
	virtual const ProfileItem& peek() const noexcept = 0;

	/// Pop the head profileItem from the input table.
	/// If there's no more content in the table, popping 
	/// from the table will cause exception to be thrown.
	virtual ProfileItem pop() throw (wdedup::Error) = 0;
};

/// @brief Defines the virtual write interface of profile.
struct ProfileOutput {
	/// Virtual destructor for pure virtual classes.
	virtual ~ProfileOutput() noexcept {};

	/// Push content to the profile output. 
	/// Exception will be thrown if there's I/O error on the 
	/// underlying (append-only) files.
	virtual void push(ProfileItem) throw (wdedup::Error) = 0;

	/// Indicates that this is the end of profile output.
	virtual void close() throw (wdedup::Error) = 0;
};

} // namespace wdedup
