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
 * @file wiobase.cpp
 * @author Haoran Luo
 * @brief wdedup I/O base implementation.
 *
 * See corresponding header for interface definitions.
 */
#include "impl/wiobase.hpp"
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

namespace wdedup {

SequentialFileBase::SequentialFileBase(
	const char* path, std::function<void(int)> report
) throw (wdedup::Error): report(report), fd(-1), readoff(0), readlen(0) {
	
	// Attempt to open the sequential file first. Exception will
	// be thrown if an invalid file descriptor is expected.
	fd = open(path, O_RDONLY);
	if(fd == -1) report(errno);

	// Use posix_fadvise() to make it more friendly for sequential read.
	if(posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL) < 0) report(errno);
}

SequentialFileBase::~SequentialFileBase() noexcept {
	if(fd != -1) close(fd);
}

void SequentialFileBase::read(char* buf, size_t size) throw (wdedup::Error) {
	while(size > 0) {
		// Read more data into the buffer if empty buffer.
		if(readoff == readlen) {
			ssize_t nextreadlen = ::read(fd, readbuf, bufsiz);
			if(nextreadlen == -1) report(errno);
			else if(nextreadlen == 0) report(EIO); // premature EOF.
			readoff = 0; readlen = (size_t)nextreadlen;
		}

		// Fill the file with remainder content of buffer.
		size_t currentRead = std::min(readlen - readoff, size);
		if(currentRead > 0) memcpy(buf, readbuf, currentRead);
		readoff += currentRead; size -= currentRead;
	}
}

bool SequentialFileBase::eof() const noexcept {
	if(readoff != readlen) return false;
	ssize_t nextreadlen = ::read(fd, readbuf, bufsiz);
	// As error will be detected in proceeding read, ignore.
	if(nextreadlen == -1) return false;
	else if(nextreadlen == 0) return true;
	readoff = 0; readlen = (size_t)nextreadlen;
}

AppendFileBase::AppendFileBase(
	const char* path, std::function<void(int)> report
) throw (wdedup::Error): report(report), fd(-1) {

	// Attempt to open the append-only file first. Exception will
	// be thrown if an invalid file descriptor is expected.
	fd = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if(fd == -1) report(errno);
}

AppendFileBase::~AppendFileBase() noexcept {
	if(fd != -1) close(fd);
}

void AppendFileBase::write(const char* buf, size_t size) throw (wdedup::Error) {
	if(::write(fd, buf, size) == -1) report(errno);
}

AppendFileLog::AppendFileLog(
	const char* path, std::function<void(int)> report
) throw (wdedup::Error): AppendFileBase(path, report), writebuf(0) {
}

void AppendFileLog::write(const char* buf, size_t size) throw (wdedup::Error) {
	size_t prevbufsiz = writebuf.size();
	writebuf.resize(prevbufsiz + size);
	memcpy(&writebuf[prevbufsiz], buf, size);
}

void AppendFileLog::sync() throw (wdedup::Error) {
	AppendFileBase::write(writebuf.data(), writebuf.size());
	{ std::vector<char> empty; writebuf.swap(empty); }
	if(fsync(fd) == -1) report(errno);
}

} // namespace wdedup
