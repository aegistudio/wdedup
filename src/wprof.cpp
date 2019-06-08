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
#include "wprof.hpp"
#include "impl/wsortdedup.hpp"
#include <fstream>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

namespace wdedup {

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
	 * Validation will be performed during recovery, and
	 * failing to 
	 */
	segment = 's',
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
	size_t userpageSize = 16l * 1024l * 1024l * 1024l; // 16GB.
	std::shared_ptr<void> userpage([=]() -> void* {
		void* userpage = mmap(NULL, userpageSize, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		if(userpage == MAP_FAILED) 
			throw std::runtime_error("Cannot allocate enough memory.");
	}(), [=](void* p) { munmap(p, userpageSize); });

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
	std::fstream originalFile(path, std::ios::in);
	if(offset > 0) originalFile.seekg(offset, std::ios::beg);
	if(originalFile.bad() || originalFile.fail())
		throw wdedup::Error(errno, path, role);

	// Loop reading the files. And writing out the content.
	bool iseof = false;  
	std::string inputEntry;
	while(!iseof || inputEntry != "") {
		wdedup::SortDedup dedup(userpage.get(), userpageSize);

		// Place the remaining entry first.
		if(inputEntry != "" && !dedup.insert(inputEntry, offset)) 
			throw std::logic_error("Insufficient working memory.");
		inputEntry = "";

		// Read an item from the original file first.
		while(!iseof) {
			if(originalFile >> inputEntry) {
				// Place the newly read entry.
				iseof = false;
				fileoff_t woffset = originalFile.tellg();
				woffset = woffset - inputEntry.size();
				if(dedup.insert(inputEntry, woffset)) 
					inputEntry = "";
				else break;
			}
			else {
				inputEntry = "";
				iseof = true;
			}
		}

		// Write the current entries to the underlying file.
		wdedup::SortDedup::pour(std::move(dedup), 
			cfg.openOutput(std::to_string(segments)));
		fileoff_t curoff = originalFile.tellg();
		std::cerr << offset << " " << (curoff - 1) << std::endl;
		cfg.olog() << wdedup::WProfLog::segment << 
			offset << (curoff - 1) << wdedup::sync;
		offset = curoff;
		++ segments;
	}

	// Write out to the log that the wprof stage has finished.
	cfg.olog() << wdedup::WProfLog::end << wdedup::sync;
	return segments;
}

} // namespace wdedup
