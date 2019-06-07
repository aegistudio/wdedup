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
 * @file wio.hpp
 * @author Haoran Luo
 * @brief wdedup I/O abstractions.
 *
 * This file defines specific interface for serializing and 
 * deserializing sequential-scan and append-only files within
 * wdedup.
 *
 * Profile files and recovery log are such kind of files, and 
 * abstracting such interface will decouple the data serialized
 * from how will the data be serialized, making it easy for adding
 * features such as compression.
 *
 * Basic data types, magic numbers, and "std::string"s can be 
 * serialized/deserialized from such files. And files are closed
 * once the file object get destructed.
 */
#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <sstream>
#include "wtypes.hpp"

namespace wdedup {

/**
 * @brief Defines the sequential-scan input file.
 *
 * The file can only be read sequentially, no random seeking
 * is permitted, making it impossible to read randomly. Some
 * special flags are configured for such files, so that OS
 * can load pages into file buffers in a desired way.
 */
struct SequentialFile {
	/// Interface aids hiding implementaiton specific details
	/// of reading from the sequential file. However, this class 
	/// must provides a libray/application level interface for 
	/// orchestrating "Impl"s.
	struct Impl {
		/// Virtual deconstructor for pure virtual classes.
		virtual ~Impl() noexcept = 0;

		/// Read chunk from the file.
		/// @throw wdedup::Error when premature EOF is 
		/// reached, or file is corrupted (when there's 
		/// compression).
		virtual void read(char*, size_t) throw (wdedup::Error) = 0;

		/// Judege whether it is EOF currently.
		virtual bool eof() const noexcept = 0;
	};

	/**
	 * @brief Open a file under the given path.
	 *
	 * The constructor orchestrates SequentialFile::Impl provided by
	 * the library, so that they can provide desired features. 
	 *
	 * @param [in] path the full path to the file.
	 * @param [in] role the role of the file.
	 * @throw wdedup::Error If the specified file is not found, an 
	 * error will be thrown and the caller needs to handle such cases.
	 */
	SequentialFile(std::string path, std::string role) throw (wdedup::Error);

	/// Close the file when the object get destructed.
	~SequentialFile() noexcept {};

	/// Delegates the read interface.
	inline void read(char* buf, size_t siz) throw(wdedup::Error) {
		pimpl->read(buf, siz);
	}

	/// Judge whether the sequential file is EOF.
	inline bool eof() const noexcept { return pimpl->eof(); }
private:
	/// Pointer to specific implementation of sequential file.
	std::unique_ptr<SequentialFile::Impl> pimpl;
};

/// Read data of specific type from the sequential file.
template<typename dataType> inline SequentialFile& operator>>(
	SequentialFile& seq, dataType& d
) throw (wdedup::Error) {
	seq.read(&d, sizeof(d));
	return seq;
}

/// Read string from the sequential file, terminated by '\0'.
inline SequentialFile& operator>>(
	SequentialFile& seq, std::string& str
) throw (wdedup::Error) {
	std::stringstream strbld; 

	// Read until '\0' is expected.
	while(true) {
		char c;
		seq.read(&c, 1);
		if(c) strbld << c;
		else break;
	}

	/// Return the string and break.
	str = std::move(strbld.str());
	return seq;
}

/**
 * @brief Defines the append-only output file.
 *
 * The file can only be written to via appending, no random
 * seeking is allowed. 
 *
 * Sometimes log-style synchronizing is required. In this 
 * case, unsynchronized content will never be written out
 * to the file. This prevents confusion about distinguishing
 * corruption from semi-persisted log.
 */
struct AppendFile {
	/// Interface aids hiding implementation specific details
	/// of writing to the sequential file.
	struct Impl {
		/// Virtual deconstructor for pure virtual classes.
		virtual ~Impl() noexcept = 0;

		/// Writing chunk from the file.
		/// @throw wdedup::Error when I/O error occurs.
		virtual void write(const char*, size_t) throw (wdedup::Error) = 0;

		/// Flush current cached chunk of file.
		/// @throw wdedup::Error when I/O error occurs.
		virtual void sync() throw (wdedup::Error) = 0;
	};

	/**
	 * @brief Open or create a sequential file.
	 *
	 * The constructor orchestrates SequentialFile::Impl provided by
	 * the library, so that they can provide desired features. 
	 *
	 * If the file is opened as a log, the original file will not be
	 * overwritten but the 
	 *
	 * @param[in] path the full path to the file.
	 * @param[in] role the role of the file.
	 * @param[in] log whether this file is a log file.
	 * @throw wdedup::Error If I/O error occurs, like no permission,
	 * no more free space, no more
	 */
	AppendFile(std::string path, std::string role, bool log = false) throw (wdedup::Error);

	/// Close the file when the object get destructed.
	~AppendFile() noexcept {}

	/// Delegates the write interface.
	inline void write(const char* buf, size_t siz) throw(wdedup::Error) {
		pimpl->write(buf, siz);
	}

	/// Delegates the sync interface.
	inline void sync() throw(wdedup::Error) { pimpl->sync(); }
private:
	/// Pointer to specific implementation of sequential file.
	std::unique_ptr<AppendFile::Impl> pimpl;
};

/// Write data of specific type to the append file.
template<typename dataType> inline AppendFile& operator<<(
	AppendFile& app, const dataType& d
) throw (wdedup::Error) {
	app.write(&d, sizeof(d));
	return app;
}

/// Write string to the append file, terminated by '\0'.
inline AppendFile& operator<<(
	AppendFile& app, const std::string& str
) throw (wdedup::Error) {
	app.write(str.c_str(), str.size());
	app.write("\0", 1);
	return app;
}

/// Specific object that flushes the append file, just like std::endl.
struct AppendSynchronizer {};
static constexpr AppendSynchronizer sync;
inline AppendFile& operator<<(
	AppendFile& app, const AppendSynchronizer&
) throw (wdedup::Error) {
	app.sync();
	return app;
}

} // namespace wdedup
