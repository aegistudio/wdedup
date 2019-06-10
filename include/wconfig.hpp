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
 * @file wconfig.hpp
 * @author Haoran Luo
 * @brief wdedup Task Configuration Interface
 *
 * This file describes the task configuration interface, where
 * command line arguments will be parsed and surrounding resources
 * will be initialized.
 *
 * Each stage of wdedup will continue on their last persisted 
 * recovery point, owing to the persisted log file. If wdedup
 * fails with OOM, I/O error (like no free space on disk), etc.,
 * assuming there's no corruption on these file (the wdedup 
 * itself is guaranteed not to cause corruption), migrating
 * (to disk of sufficient space) and rerun wdedup will continue
 * on the lastly available progress.
 */
#pragma once
#include <memory>
#include <tuple>
#include <string>
#include "wio.hpp"
#include "wprofile.hpp"

namespace wdedup {

/**
 * @brief Task Configuration Interface
 *
 * The task configuration struct aggregating accessible interfaces
 * (arg-parsed and initialized by the main program) for each stage
 * of wdedup. Please notice that config must remains as brief
 * as possible so not all files can be placed into the config.
 */
struct Config {
	/// Virtual destructor for pure virtual interface.
	virtual ~Config() noexcept {}

	/// Check whether log recovery has done.
	virtual bool hasRecoveryDone() throw (wdedup::Error) = 0;

	/// Retrieve the input log file. Assertion failure will be triggered 
	/// if invoked with hasRecoveryDone() returning true.
	virtual wdedup::SequentialFile& ilog() noexcept = 0;

	/// Retrieve the output log file. Assertion failure will be triggered
	/// if invoked with hasRecoveryDone() returning false.
	virtual wdedup::AppendFile& olog() noexcept = 0;

	/// Trigger recovery done. Will close the log file input and reopen
	/// the log file in append-only mode.
	virtual void recoveryDone() throw (wdedup::Error) = 0;

	/// Trigger log exception by throwing out.
	virtual void logCorrupt() throw (wdedup::Error) = 0;

	/// Create a profile output under workdir for writing profile table.
	virtual std::unique_ptr<wdedup::ProfileOutput> 
			openOutput(std::string path) throw (wdedup::Error) = 0;

	/// Open a profile input under workdir for reading profile table.
	virtual std::unique_ptr<wdedup::ProfileInput>
			openInput(std::string path) throw (wdedup::Error) = 0;

	/// Open a profile input that filter out repeated profile item.
	virtual std::unique_ptr<wdedup::ProfileInput>
			openSingularInput(std::string path) throw (wdedup::Error) = 0;

	/// Remove specified log file if it already exists.
	virtual void remove(std::string path) throw (wdedup::Error) = 0;

	/// Retrieve the working memory of the program.
	virtual std::tuple<void*, size_t> workmem() const noexcept = 0;
};

} // namespace wdedup
