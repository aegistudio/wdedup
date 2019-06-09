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
 * @file main.cpp
 * @author Haoran Luo
 * @brief wdedup Main Program
 *
 * This file implements the wdedup::Config interface, parses console
 * arguments, and orchestrate different stage of wdedup.
 */
#include "wconfig.hpp"
#include "wdedup.hpp"
#include "impl/wpflsimple.hpp"
#include "impl/wpflfilter.hpp"
#include "impl/wcli.hpp"
#include "wtypes.hpp"
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cassert>
#include <cstring>

int main(int argc, char** argv) {
	// Parse arguments using the command line parser.
	wdedup::ProgramOptions options;
	int retcode = argparse(argc, argv, options);
	if(retcode != 0) return retcode;
	if(!options.run) return 0;

	// Forward some command line arguments.
	static const std::string& fileInput = options.origfile;
	static const std::string& workdir = options.workdir;
	static std::string logPath = workdir + "/log";

	// Arguments are parsed, now attempt to initialize and run stages.
	try {
		// Initialize the log mode, and get it shared.
		static wdedup::FileMode logMode;
		logMode.log = true;

		// Initialize the profile mode, and get it shared.
		static wdedup::FileMode profileMode;
		profileMode.log = false;

		// Configuration as stack object to be operated by main function.
		struct MainConfig : public wdedup::Config {
			// Unique pointer managing the log file input.
			std::unique_ptr<wdedup::SequentialFile> pilog;

			// Check pilog's nullity as recovery done.
			virtual bool hasRecoveryDone() throw (wdedup::Error) {
				return pilog == nullptr;
			}

			// Return the pilog as ilog result.
			virtual wdedup::SequentialFile& ilog() noexcept {
				assert(pilog != nullptr);
				return *pilog;
			}

			// Helper for opening the log input.
			void openLogInput() throw (wdedup::Error) {
				pilog = std::unique_ptr<wdedup::SequentialFile>(
					new wdedup::SequentialFile(logPath, "log", logMode));
			}

			// Unique pointer managing the log file output.
			std::unique_ptr<wdedup::AppendFile> polog;

			// Return the pilog as ilog result.
			virtual wdedup::AppendFile& olog() noexcept {
				assert(polog != nullptr);
				return *polog;
			}

			// Helper for opening the log output.
			void openLogOutput() throw (wdedup::Error) {
				polog = std::unique_ptr<wdedup::AppendFile>(
					new wdedup::AppendFile(logPath, "log", logMode));
			}

			/// Trigger log exception by throwing out.
			virtual void logCorrupt() throw (wdedup::Error) {
				throw wdedup::Error(EIO, logPath, "log");
			}

			// Mark the recovery as done and switch the pilog with polog.
			virtual void recoveryDone() throw (wdedup::Error) {
				if(!(pilog != nullptr && polog == nullptr)) return;
				pilog = std::unique_ptr<wdedup::SequentialFile>();
				openLogOutput();
			}

			// Profile output creation function.
			virtual std::unique_ptr<wdedup::ProfileOutput>
				openOutput(std::string path) throw (wdedup::Error) {
				return std::unique_ptr<wdedup::ProfileOutput>(
					new wdedup::ProfileOutputSimple(
						workdir + "/" + path, profileMode));
			}

			// Profile input creation function.
			virtual std::unique_ptr<wdedup::ProfileInput>
				openInput(std::string path) throw (wdedup::Error) {
				return std::unique_ptr<wdedup::ProfileInput>(
					new wdedup::ProfileInputSimple(
						workdir + "/" + path, profileMode));
			}

			// Profile singular input creation function.
			virtual std::unique_ptr<wdedup::ProfileInput>
				openSingularInput(std::string path) throw (wdedup::Error) {
				return std::unique_ptr<wdedup::ProfileInput>(
					new wdedup::ProfileInputFilter(openInput(path)));
			}

			// Remove existing file if it already exists.
			virtual void remove(std::string path) throw (wdedup::Error) {
				::remove((workdir + "/" + path).c_str());
			}
		} config;

		// Check whether the working directory exists.
		struct stat stwdir; if(stat(workdir.c_str(), &stwdir) < 0) {
			bool shouldThrow = true;
			// Attempt to create the directory if it does not exists.
			if(errno == ENOENT) 
				if(mkdir(workdir.c_str(), S_IRUSR | S_IWUSR) == 0) {
					shouldThrow = false;
					config.openLogOutput();
				}
			if(shouldThrow) throw wdedup::Error(errno, workdir, "workdir");
		} else {
			// Check whether the specified path is a directory.
			if(!S_ISDIR(stwdir.st_mode)) throw wdedup::Error(EIO, workdir, "workdir");

			// Check whether the log file exists. And perform reading/writing.
			struct stat stlog; if(stat(logPath.c_str(), &stlog) < 0) {
				if(errno == ENOENT) config.openLogOutput();
				else config.logCorrupt();
			} else {
				if(!S_ISREG(stlog.st_mode)) config.logCorrupt();
				config.openLogInput();
			}
		}

		// Checking or writing out metadata. Different version of wdedup cannot operate
		// on the same workding directory.
		static const std::string version = "20190609.0001";	// Version identifier.
		if(!config.hasRecoveryDone()) {
			// Verify metadata information, to avoid disconsistency.
			std::string expectedVersion; config.ilog() >> expectedVersion;
			if(expectedVersion != version) config.logCorrupt();
		} else {
			config.recoveryDone();

			// Write out metadata for ensuring consistent recovery operations.
			config.olog() << version << wdedup::sync;
		}

		// Commence the processing of wprof.
		size_t leaves = wprof(config, fileInput);

		// Merge the result generated by wprof.
		size_t root = wmerge(config, leaves);

		// Find the root entry and print it out.
		std::string result = wfindfirst(config, root);
		if(result != "") std::cout << result << std::endl;
	} catch(wdedup::Error err) {
		// Report the error to the users and exit with status code.
		std::cerr << "Error: " << err.path;
		if(err.role != "") std::cerr << " (" << err.role << "): ";
		std::cerr << strerror(err.eno) << std::endl;
		return -err.eno;
	}
	return 0;
}
