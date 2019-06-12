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
 * @file wpflsimple.cpp
 * @author Haoran Luo
 * @brief wdedup Simple Profile Implementation
 *
 * See corresponding header for interface definitions.
 */
#include "impl/wpflsimple.hpp"
#include <stdexcept>

namespace wdedup {

ProfileInputSimple::ProfileInputSimple(std::string path, FileMode mode) 
	throw (wdedup::Error) : input(std::move(path), "profile-simple", mode), 
	head(""), isempty(true) {

	popFill();
}

void ProfileInputSimple::popFill() throw (wdedup::Error) {
	if(input.eof()) isempty = true;
	else {
		char repeated;
		isempty = false;
		input >> head.word >> repeated;

		// When the item is repeated, the value will be any non zero
		// value, otherwise it will be zero followed by an occurance.
		if(repeated != 0) head.repeated = true;
		else {
			head.repeated = false;
			input >> head.occur;
		}
	}
}

bool ProfileInputSimple::empty() const noexcept { return isempty; }

const ProfileItem& ProfileInputSimple::peek() const noexcept { return head; }

wdedup::ProfileItem ProfileInputSimple::pop() throw (wdedup::Error) {
	wdedup::ProfileItem result(std::move(head));
	popFill();
	return result;
}

ProfileOutputSimple::ProfileOutputSimple(std::string path, FileMode mode) 
	throw (wdedup::Error) : output(path, "profile-simple", mode) {}

void ProfileOutputSimple::push(ProfileItem pi) throw (wdedup::Error) {
	output << pi.word;
	if(pi.repeated) output << (char)1;
	else output << (char)0 << pi.occur;
}

size_t ProfileOutputSimple::close() throw (wdedup::Error) {
	output << wdedup::sync;
	return output.tell();
}

} // namespace wdedup
