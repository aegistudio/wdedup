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
#include "impl/wsortdedup.hpp"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

namespace wdedup {

// Helper for judging whether a character is whitespace.
inline bool isWhitespace(char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/**
 * Simulation for std::fstream >> std::string. While returned, the
 * pointer must sit on EOF or an empty character.
 */
static bool readString(wdedup::SequentialFile& f, 
	std::string& s, fileoff_t& woffset) throw (wdedup::Error) {
	char c;

	// White space elimination.
	while(true) {
		if(f.eof()) return false;
		f >> c; if(!isWhitespace(c)) break;
	}
	woffset = f.tell() - 1;

	// Word generation.
	std::stringstream sbuild;
	do {
		sbuild << c;
		if(f.eof()) break;
		f >> c;
		if(isWhitespace(c)) break;
	} while(true);

	// Place the string to the s.
	s = sbuild.str();
	return true;
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

	// Allocate the memory space for executing wprof.
	// TODO(haoran.luo): consider it to be done by the caller?
	size_t userpageSize = 1l * 1024l * 1024l * 1024l; // 1GB.
	std::shared_ptr<void> userpage([=]() -> void* {
		void* userpage = mmap(NULL, userpageSize, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		if(userpage == MAP_FAILED) 
			throw std::runtime_error("Cannot allocate enough memory.");
		return userpage;
	}(), [=](void* p) { munmap(p, userpageSize); });
	std::shared_ptr<void> lockedpage([=]() -> void* {
		if(mlock(userpage.get(), userpageSize) != 0)
			throw std::runtime_error("Cannot lock memory page.");
		return userpage.get();
	}(), [=](void* p) { munlock(p, userpageSize); });

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

	// Loop reading the files. And writing out the content.
	bool iseof = false;  
	std::string inputEntry;
	fileoff_t woffset;
	while(!iseof || inputEntry != "") {
		wdedup::SortDedup dedup(userpage.get(), userpageSize);

		// Place the remaining entry first.
		if(inputEntry != "" && !dedup.insert(inputEntry, woffset)) 
			throw std::logic_error("Insufficient working memory.");
		inputEntry = "";

		// Recorded in order to mark milestone when dedup.insert failed.
		fileoff_t prevoff;

		// Read an item from the original file first.
		while(!iseof) {
			prevoff = originalFile.tell();
			if(readString(originalFile, inputEntry, woffset)) {
				// Place the newly read entry.
				iseof = false;
				if(dedup.insert(inputEntry, woffset)) 
					inputEntry = "";
				else break;
			}
			else {
				prevoff = originalFile.tell();
				inputEntry = "";
				iseof = true;
			}
		}

		// Write the current entries to the underlying file.
		std::string segmentName = std::to_string(segments);
		cfg.remove(segmentName);
		wdedup::SortDedup::pour(std::move(dedup), 
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
