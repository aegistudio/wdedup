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
 * @file wpflfilter.cpp
 * @author Haoran Luo
 * @brief wdedup Profile Filter Implementation
 *
 * See corresponding header for interface definitions.
 */
#include "impl/wpflfilter.hpp"

namespace wdedup {

ProfileInputFilter::ProfileInputFilter(
	std::unique_ptr<wdedup::ProfileInput> mdelegated) 
	throw (wdedup::Error) : delegated(std::move(mdelegated)) {

	while((!delegated->empty()) && delegated->peek().repeated)
			delegated->pop();
}

bool ProfileInputFilter::empty() const noexcept { return delegated->empty(); }

const ProfileItem& ProfileInputFilter::peek() const noexcept { return delegated->peek(); }

wdedup::ProfileItem ProfileInputFilter::pop() throw (wdedup::Error) {
	wdedup::ProfileItem result(std::move(delegated->pop()));
	while((!delegated->empty()) && delegated->peek().repeated)
			delegated->pop();
	return result;
}

} // namespace wdedup
