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
 * @file wiobase.hpp
 * @author Haoran Luo
 * @brief wdedup I/O base implementation.
 *
 * This file defines the I/O base implementation. Please notice that
 * the implementation should be orchestrated by wio.cpp.
 */
#pragma once
#include <cstddef>
#include <vector>
#include <functional>
#include "wio.hpp"

namespace wdedup {

/// Buffer size of the read buffer, usually equals to a page size or
/// a sector size so that I/O transmissions could be optimized.
/// TODO(haoran.luo): try to fetch the page size or sector size using
/// CMake's configure_file() headers.
static const size_t bufsiz = 4096;

/**
 * @brief Defines the sequential-scan file base.
 *
 * The implementation handles creation and basic operations for reading
 * such file. You can add more operations like compression, CRC checking 
 * through decorator patterns.
 */
struct SequentialFileBase : public SequentialFile::Impl {
	/// @brief Open a file under the given path.
	SequentialFileBase(const char*, std::function<void(int)>, 
		fileoff_t) throw (wdedup::Error);

	/// Close the file when the object get destructed.
	virtual ~SequentialFileBase() noexcept;

	// Override the pure virtual methods.
	virtual void read(char*, size_t) throw(wdedup::Error) override;
	virtual bool eof() const noexcept override;
	virtual fileoff_t tell() const noexcept override;

	/// Reference to the error report function.
	const std::function<void(int)> report;

	/// The file descriptor that is open for reading.
	const int fd;
private:
	/// The buffer storing the fetched content.
	mutable char readbuf[bufsiz];

	/// The offset of data in the read buffer.
	mutable size_t readoff;

	/// The available data length in the read buffer.
	mutable size_t readlen;

	/// The current position of the pointer in the file.
	mutable fileoff_t filetell;
};


/**
 * @brief Defines the append-only output file base.
 *
 * The implementation handles the creation and basic operations for writing
 * such file. Just like the SequentialFileBase counter part.
 */
class AppendFileBase : public AppendFile::Impl {
protected:
	/// The current file tell of the output file.
	fileoff_t filetell;
public:
	/// @brief Open or create a file under the given path.
	AppendFileBase(const char*, std::function<void(int)>) throw (wdedup::Error);

	/// Close the file when the object get destructed.
	virtual ~AppendFileBase() noexcept;

	// Override the pure virtual methods.
	virtual void write(const char*, size_t) throw(wdedup::Error) override;

	/// No synchronization for such file, and no delegating as it is the base.
	virtual void sync() throw(wdedup::Error) override {}

	/// The telling method is waiting for its children to implement.
	virtual fileoff_t tell() const noexcept { return filetell; }

	/// Reference to the error report function.
	const std::function<void(int)> report;

	/// The file descriptor that is open for writing.
	const int fd;
};

/**
 * @brief Defines the log output file.
 *
 * The implementation requires an extra buffer to guarantee log feature that
 * contents between wdedup::sync must be written as a integrity part.
 */
struct AppendFileLog : public AppendFileBase {
	/// @brief Open or create a log file under the given path.
	AppendFileLog(const char*, std::function<void(int)>) throw (wdedup::Error);

	/// Close the file when the object get destructed.
	virtual ~AppendFileLog() noexcept {}

	// Override the pure virtual methods.
	virtual void write(const char*, size_t) throw(wdedup::Error) override;

	/// No synchronization for such file, and no delegating as it is the base.
	virtual void sync() throw(wdedup::Error) override;
private:
	/// The synchronization buffer that holds the next content to synchronize.
	std::vector<char> writebuf;
};

/**
 * @brief Defines the buffer output file.
 *
 * The implementation is used to reduce the overhead of writing files, in the unit
 * of reduced syscall.
 */
struct AppendFileBuffer : public AppendFileBase {
	/// @brief Open or create a log file under the given path.
	AppendFileBuffer(const char*, std::function<void(int)>) throw (wdedup::Error);

	/// Close the file when the object get destructed.
	virtual ~AppendFileBuffer() noexcept {}

	// Override the pure virtual methods.
	virtual void write(const char*, size_t) throw(wdedup::Error) override;

	/// No synchronization for such file, and no delegating as it is the base.
	virtual void sync() throw(wdedup::Error) override;
private:
	/// The buffer buffering the content to write.
	char writebuf[bufsiz];

	/// The length of data in the write buffer.
	size_t writelen;
};

} // namespace wdedup
