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
 * @file wpflsimple.hpp
 * @author Haoran Luo
 * @brief wdedup Simple Profile Implementation
 *
 * This file defines the I/O simple implementation. This implementation
 * requires a single file, and "ProfileItem"s are stored as sorted 
 * K-V pairs in the profile.
 */
#pragma once
#include "wprofile.hpp"
#include "wio.hpp"

namespace wdedup {

/// @brief The simple format of ProfileInput.
class ProfileInputSimple final : public wdedup::ProfileInput {
	/// The simple profile input.
	wdedup::SequentialFile input;

	/// The currently fetched profile entry.
	wdedup::ProfileItem head;

	/// Whether it is currently empty.
	bool isempty;

	/// Pop and fill the next item in the file.
	void popFill() throw (wdedup::Error);
public:
	/**
	 * Construct a profile input reading specified path. The file
	 * is assumed to be simple formatted.
	 *
	 * Besides normal opening and configuring operations in input,
	 * a prefetching will be performed, and error will be thrown if
	 * I/O error occurs while prefetching.
	 */
	ProfileInputSimple(std::string path, 
		wdedup::FileMode mode) throw (wdedup::Error);

	/// Profile input destructor.
	virtual ~ProfileInputSimple() noexcept {}

	/// Attempt to peek whether it is end of file.
	virtual bool empty() const noexcept override;

	/// Attempt to peek the head item from the file.
	virtual const wdedup::ProfileItem& peek() const noexcept override;

	/// Attempt to pop the head item from the file.
	virtual wdedup::ProfileItem pop() throw (wdedup::Error) override;
};

/// @brief The simple format of ProfileOutput.
class ProfileOutputSimple final : public wdedup::ProfileOutput {
	/// The simple profile output.
	wdedup::AppendFile output;
public:
	/**
	 * Construct a profile output writing specified path. The file
	 * will be written in simple format.
	 */
	ProfileOutputSimple(std::string path,
		wdedup::FileMode mode) throw (wdedup::Error);

	/// Profile output destructor.
	virtual ~ProfileOutputSimple() noexcept {}

	/// Push content to the profile output.
	virtual void push(ProfileItem) throw (wdedup::Error) override;

	/// Indicates that this is the end of profile output.
	virtual size_t close() throw (wdedup::Error) override;
};

} // namespace wdedup
