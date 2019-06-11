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
#include <cstring>
#include <vector>
#include "wtypes.hpp"

namespace wdedup {
/**
 * @brief The flags for reading and writing files.
 *
 * In principle, to avoid problem, the flags for creating a
 * wdedup::AppendFile must matches the wdedup::SequentialFile.
 */
struct FileMode {
	/// Indicates whether the file will be opened as a log file.
	/// Log file will ensure that data between wdedup::sync will
	/// either be missing or written out as a integrity part.
	/// (wdedup::AppendFile will use this flag, however the
	///  wdedup::SequentialFile will ignore because there's no
	///  difference between a non-log and a log file).
	bool log;

	/// Perform seekset before reading the specified file. Setting 
	/// this variable to 0 will force to read from the start.
	/// (wdedup::SequentialFile will use this flag, however the
	///  wdedup::AppendFile will ignore because they always 
	///  write to the end of the file).
	fileoff_t seekset;

	/// Default constructor of the file mode.
	FileMode() noexcept: log(false), seekset(0) {}

	/// Copy constructor of the file mode.
	FileMode(const FileMode& c) noexcept: log(c.log), 
		seekset(c.seekset) {}
};

/**
 * @brief Defines the sequential-scan input file.
 *
 * The file can only be read sequentially, no random seeking
 * is permitted, making it impossible to read randomly. Some
 * special flags are configured for such files, so that OS
 * can load pages into file buffers in a desired way.
 */
struct SequentialFile final {
	/// Interface aids hiding implementaiton specific details
	/// of reading from the sequential file. However, this class 
	/// must provides a libray/application level interface for 
	/// orchestrating "Impl"s.
	struct Impl {
		/// Virtual deconstructor for pure virtual classes.
		virtual ~Impl() noexcept {}

		/// Read chunk from the file.
		/// @throw wdedup::Error when premature EOF is 
		/// reached, or file is corrupted (when there's 
		/// compression).
		virtual void read(char*, size_t) throw (wdedup::Error) = 0;

		/// Fetch the underlying buffer pointer from the file.
		///
		/// Exception will be thrown if it is currently EOF and
		/// bufferptr is still called.
		///
		/// The call will invalidates previous returned bufferptr. 
		virtual void bufferptr(char*&, size_t&) throw (wdedup::Error) = 0;

		/// Skip up to size bytes in the file.
		///
		/// This call should be used associated with bufferptr.
		/// The skipping should be less than or equal to the size
		/// returned by bufferptr.
		virtual void bufferskip(size_t) throw (wdedup::Error) = 0;
	protected:
		/// Whether it is EOF currently. Can only be modified
		/// when Impl::read is invoked.
		bool eof;

		/// Telling current position of the sequential file. Can
		/// only be modified when Impl::read is invoked.
		fileoff_t tell;

		/// Initialize the fields for children.
		Impl() noexcept: eof(false), tell(0) {}

		/// Should befriend the wdedup::SequentialFile.
		friend class wdedup::SequentialFile;
	};

	/**
	 * @brief Open a file under the given path.
	 *
	 * The constructor orchestrates SequentialFile::Impl provided by
	 * the library, so that they can provide desired features. 
	 *
	 * @param[in] path the full path to the file.
	 * @param[in] role the role of the file.
	 * @param[in] mode the mode for opening the sequential file.
	 * @throw wdedup::Error If the specified file is not found, an 
	 * error will be thrown and the caller needs to handle such cases.
	 */
	SequentialFile(std::string path, std::string role, 
			FileMode mode) throw (wdedup::Error);

	/// Close the file when the object get destructed.
	~SequentialFile() noexcept {};

	/// Delegates the read interface.
	inline void read(char* buf, size_t siz) throw(wdedup::Error) {
		pimpl->read(buf, siz);
	}

	/// Delegates the read interface.
	inline void bufferptr(char*& buf, size_t& siz) throw (wdedup::Error) {
		pimpl->bufferptr(buf, siz);
	}

	/// Delegates the skip interface.
	inline void bufferskip(size_t siz) throw(wdedup::Error) {
		pimpl->bufferskip(siz);
	}

	/// Delegates the eof interface.
	inline bool eof() const noexcept { return pimpl->eof; }

	/// Delegates the tell interface.
	inline fileoff_t tell() const noexcept { return pimpl->tell; }
private:
	/// Pointer to specific implementation of sequential file.
	std::unique_ptr<SequentialFile::Impl> pimpl;
};

/// Read data of specific type from the sequential file.
template<typename dataType> inline SequentialFile& operator>>(
	SequentialFile& seq, dataType& d
) throw (wdedup::Error) {
	seq.read((char*)&d, sizeof(d));
	return seq;
}

/// Read string from the sequential file, terminated by '\0'.
inline SequentialFile& operator>>(
	SequentialFile& seq, std::string& str
) throw (wdedup::Error) {
	char* bufptr = nullptr; size_t bufsiz = 0;

	// Retrieve the starting of bufferptr. This is the
	// initial buffer pointer chunk, in-place string
	// creation will be performed when '\0' is found.
	seq.bufferptr(bufptr, bufsiz);
	for(size_t i = 0; i < bufsiz; ++ i)
		if(bufptr[i] == '\0') {
			// Null found in string, skip and return.
			std::string result(bufptr);
			seq.bufferskip(i + 1);
			str = std::move(result);
			return seq;
		}
	
	// Null terminator not found in bufferptr, so we
	// will have to load more buffers.
	std::vector<char> strbld;

	// Read until '\0' is expected.
	while(true) {
		size_t bldsize = strbld.size();
		memcpy(&strbld[bldsize], bufptr, bufsiz);
		seq.bufferskip(bufsiz);
		seq.bufferptr(bufptr, bufsiz);
		for(size_t i = 0; i < bufsiz; ++ i)
			if(bufptr[i] == '\0') {
				// Null found, return now.
				size_t size = strbld.size();
				strbld.resize(strbld.size() + i + 1);
				memcpy(&strbld[size], bufptr, i + 1);
				std::string result(strbld.data());
				seq.bufferskip(i + 1);
				str = std::move(result);
				return seq;
			}
	}
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
struct AppendFile final {
	/// Interface aids hiding implementation specific details
	/// of writing to the sequential file.
	struct Impl {
		/// Virtual deconstructor for pure virtual classes.
		virtual ~Impl() noexcept {}

		/// Writing chunk from the file.
		/// @throw wdedup::Error when I/O error occurs.
		virtual void write(const char*, size_t) throw (wdedup::Error) = 0;

		/// Flush current cached chunk of file.
		/// @throw wdedup::Error when I/O error occurs.
		virtual void sync() throw (wdedup::Error) = 0;
	protected:
		/// Telling current position of the append file. Can only be 
		/// modified when Impl::write and Impl::sync is invoked.
		wdedup::fileoff_t tell;

		/// Initialize the fields for children.
		Impl() noexcept: tell(0) {}

		/// Should befriend the wdedup::AppendFile.
		friend class wdedup::AppendFile;
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
	 * @param[in] mode the mode for opening the sequential file.
	 * @throw wdedup::Error If I/O error occurs, like no permission,
	 * no more free space, no more
	 */
	AppendFile(std::string path, std::string role, 
			FileMode mode) throw (wdedup::Error);

	/// Close the file when the object get destructed.
	~AppendFile() noexcept {}

	/// Delegates the write interface.
	inline void write(const char* buf, size_t siz) throw(wdedup::Error) {
		pimpl->write(buf, siz);
	}

	/// Delegates the size interface.
	inline fileoff_t tell() const noexcept { return pimpl->tell; }

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
	app.write((char*)&d, sizeof(d));
	return app;
}

/// Write string to the append file, terminated by '\0'.
inline AppendFile& operator<<(
	AppendFile& app, const std::string& str
) throw (wdedup::Error) {
	app.write((char*)str.c_str(), str.size());
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
