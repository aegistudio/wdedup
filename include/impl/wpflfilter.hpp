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
 * @file wpflfilter.hpp
 * @author Haoran Luo
 * @brief wdedup Profile Filter Implementation
 *
 * This file defines the profile filter implementation. That only
 * produces non-repeating element input.
 */
#pragma once
#include "wprofile.hpp"
#include <memory>

namespace wdedup {

/// @brief The simple format of ProfileInput.
class ProfileInputFilter final : public wdedup::ProfileInput {
	/// The delegated profile input.
	std::unique_ptr<wdedup::ProfileInput> delegated;
public:
	/**
	 * Construct a profile input reading specified path. The file
	 * is assumed to be simple formatted.
	 *
	 * Besides normal opening and configuring operations in input,
	 * a prefetching will be performed, and error will be thrown if
	 * I/O error occurs while prefetching.
	 */
	ProfileInputFilter(std::unique_ptr<wdedup::ProfileInput>) 
			throw (wdedup::Error);

	/// Profile input destructor.
	virtual ~ProfileInputFilter() noexcept {}

	/// Attempt to peek whether it is end of file.
	virtual bool empty() const noexcept override;

	/// Attempt to peek the head item from the file.
	virtual const wdedup::ProfileItem& peek() const noexcept override;

	/// Attempt to pop the head item from the file.
	virtual wdedup::ProfileItem pop() throw (wdedup::Error) override;
};

} // namespace wdedup
