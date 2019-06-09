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
 * @file wfindfirst.cpp
 * @author Haoran Luo
 * @brief wdedup Find-First Implementation
 *
 * This file implements the finding function, see the header file
 * for more definition details.
 */
#include "wdedup.hpp"
#include <string>

namespace wdedup {

std::string wfindfirst(
	wdedup::Config& cfg, size_t root
) throw (wdedup::Error) {
	// Open the final merged result and find the first element.
	std::unique_ptr<wdedup::ProfileInput> singular =
		cfg.openSingularInput(std::to_string(root));

	// Perform sequential scanning and fetch the entry with lowest
	// occurence offset.
	std::string result; fileoff_t off = 0;
	while(!singular->empty()) {
		wdedup::ProfileItem item = singular->pop();

		// Judge whether current item should replace the current.
		bool replace = false;
		if(result == "") replace = true;
		else if(off > item.occur) replace = true;

		// Replace the current item.
		if(replace) {
			result = item.word;
			off = item.occur;
		}
	}
	return result;
}

} // namespace wdedup
