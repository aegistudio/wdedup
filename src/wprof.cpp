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
 * @file wprof.cpp
 * @author Haoran Luo
 * @brief wdedup Profiler Implementation
 *
 * This file implements the profiling function, see the header file
 * for more definition details.
 */
#include "wdedup.hpp"
//#include "impl/wsortdedup.hpp"
#include "impl/wtreededup.hpp"
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

namespace wdedup {

// Current implementation that is used as deduplicator.
using Dedup = wdedup::TreeDedup;

// Helper for judging whether a character is whitespace.
inline bool isWhitespace(char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/// Performs operations related to the original file.
struct OriginalFileReader {
	/// Caching previously read data, if the data is really
	/// too long. This helps reducing number of alloc calls.
	std::vector<char> cache;
	
	/// Length of previous string to skip.
	size_t prevskip;

	/// Constructor for the file reader.
	OriginalFileReader(): cache(), prevskip(0) {}

	/// Read a string from the reader. The returned pointer is
	/// available until next invocation to readString.
	///
	/// The caller should ensure that the file is exclusive to
	/// the reader.
	const char* readString(wdedup::SequentialFile& f,
		fileoff_t& woffset) throw (wdedup::Error);
};

static inline void eliminateWhitespace(
	wdedup::SequentialFile& f) throw (wdedup::Error) {

	// White space elimination.
	char* bufptr = nullptr; size_t bufsize = 0;
	while(true) {
		if(f.eof()) return;
		f.bufferptr(bufptr, bufsize);
		for(size_t i = 0; i < bufsize; ++ i) {
			if(!isWhitespace(bufptr[i])) {
				if(i > 0) f.bufferskip(i);
				return;
			}
		}
		f.bufferskip(bufsize);
	}
}

const char* OriginalFileReader::readString(
	wdedup::SequentialFile& f, fileoff_t& woffset) throw (wdedup::Error) {
	// Discard the previous content.
	{ std::vector<char> empty; std::swap(cache, empty); }
	if(prevskip > 0) f.bufferskip(prevskip);
	eliminateWhitespace(f);

	// Commonly used buffer variable.
	char* bufptr = nullptr; size_t bufsize = 0;
	if(f.eof()) return nullptr;
	woffset = f.tell();

	// Perform in-place replacing when the word is short enough.
	f.bufferptr(bufptr, bufsize);
	for(size_t i = 0; i < bufsize; ++ i) {
		if(isWhitespace(bufptr[i])) {
			bufptr[i] = '\0';
			prevskip = i + 1;
			return bufptr;
		}
	}

	// The word seems to be too long, so perform caching.
	prevskip = 0;
	while(true) {
		// Place previous content into the cache.
		{
			size_t cachesize = cache.size();
			cache.resize(cachesize + bufsize);
			memcpy(&cache[cachesize], bufptr, bufsize);
			f.bufferskip(bufsize);
		}

		// Perform next step of reading.
		if(f.eof()) break;
		f.bufferptr(bufptr, bufsize);

		// Find whitespace inside the string.
		for(size_t i = 0; i < bufsize; ++ i) {
			if(isWhitespace(bufptr[i])) {
				bufptr[i] = '\0';
				size_t cachesize = cache.size();
				cache.resize(cachesize + i + 1);
				memcpy(&cache[cachesize], bufptr, i + 1);
				f.bufferskip(i + 1);
				return cache.data();
			}
		}
	}
	
	// Place the string to the s.
	cache.push_back('\0');
	return cache.data();
}

/**
 * @brief Indicates the type of current log item.
 *
 * Giving wprof log items type makes it easier to determine the
 * boundary of stage from the perspective of logging.
 */
enum class WProfLog : char {
	/**
	 * @brief Records a successfully persisted profile.
	 *
	 * The log should be of format 
	 * ```c++
	 * struct {
	 *     offset_type start, end;
	 * };
	 * ```
	 */
	segment = 's',

	/// Indicates the ending of wprof stage.
	end = 'e'
};

size_t wprof(
	wdedup::Config& cfg, const std::string& path
) throw (wdedup::Error) {
	// The control counters for wprof routine.
	size_t segments = 0;
	fileoff_t offset = 0;

	// Recover previous execution states.
	if(!cfg.hasRecoveryDone()) while(!(cfg.ilog().eof())) {
		// Parse the current line of log.
		char type; cfg.ilog() >> type;
		switch(type) {
		case (char)wdedup::WProfLog::end:
			// Indicates this is the end of current log
			// and wprof stage has been completed.
			return segments;
		case (char)wdedup::WProfLog::segment:
			// Parse the segment parameters.
			fileoff_t start, end;
			cfg.ilog() >> start >> end;

			// If the segment start does not matches the 
			// end of previous segment, report corruption.
			if(start != offset) cfg.logCorrupt();
			offset = end + 1;
			++ segments;
			break;
		default:
			// Report corruption for unknown log item type.
			cfg.logCorrupt();
		}
	}

	// Recovery should be ended in current stage, we must work and
	// produces loggings from current point and in later stages.
	cfg.recoveryDone();

	// Stat the file to ensure our operations to the file is valid.
	static const char* role = "original-file";
	struct stat st; if(stat(path.c_str(), &st) < 0)
		throw wdedup::Error(errno, path, role);
	if(st.st_size < offset)	// The file is too small for reading.
		throw wdedup::Error(EIO, path, role);
	if(S_ISDIR(st.st_mode))	// Directory must not be used as a file.
		throw wdedup::Error(EISDIR, path, role);
	if(!S_ISREG(st.st_mode)) // Only regular file can be used now.
		throw wdedup::Error(EIO, path, role);

	// Open file and reposition the file read pointer to the offset.
	// XXX(haoran.luo): We CANNOT use std::fstream here. Because when the file
	// reaches EOF, the std::fstream::tellg will always return pos_type(-1),
	// making us writting out wrong value about the file to be operated.
	wdedup::FileMode originalMode;
	originalMode.seekset = offset;
	wdedup::SequentialFile originalFile(path, role, originalMode);
	wdedup::OriginalFileReader reader;

	// Loop reading the files. And writing out the content.
	bool iseof = false;  
	const char* inputEntry = nullptr; size_t inputLength = 0;
	fileoff_t woffset;
	while(!iseof || inputEntry != nullptr) {
		auto wm = cfg.workmem();
		wdedup::Dedup dedup(std::get<0>(wm), std::get<1>(wm));

		// Place the remaining entry first.
		if(inputEntry != nullptr) if(!dedup.insert(inputEntry, inputLength, woffset)) 
			throw std::logic_error("Insufficient working memory.");
		inputEntry = nullptr;

		// Recorded in order to mark milestone when dedup.insert failed.
		fileoff_t prevoff;

		// Read an item from the original file first.
		while(!iseof) {
			prevoff = originalFile.tell();
			inputEntry = reader.readString(originalFile, woffset);
			if(inputEntry != nullptr) {
				// TODO(haoran.luo): get length while readString.
				inputLength = strlen(inputEntry); 

				// Place the newly read entry.
				iseof = false;
				if(dedup.insert(inputEntry, inputLength, woffset)) 
					inputEntry = nullptr;
				else break;
			}
			else {
				prevoff = originalFile.tell();
				iseof = true;
			}
		}

		// Write the current entries to the underlying file.
		std::string segmentName = std::to_string(segments);
		cfg.remove(segmentName);
		wdedup::Dedup::pour(std::move(dedup), 
			cfg.openOutput(segmentName));
		cfg.olog() << wdedup::WProfLog::segment << 
			offset << (prevoff - 1) << wdedup::sync;
		offset = prevoff;
		++ segments;
	}

	// Write out to the log that the wprof stage has finished.
	cfg.olog() << wdedup::WProfLog::end << wdedup::sync;
	return segments;
}

} // namespace wdedup
