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
 * @file wcli.cpp
 * @author Haoran Luo
 * @brief wdedup Command Line Parser implementation.
 *
 * This file implements the header file impl/wcli.hpp. See 
 * corresponding header for implementation details.
 */
#include "impl/wcli.hpp"
#include <boost/program_options.hpp>
#include <vector>
#include <regex>
#include <string>
#include <iostream>
#include <sstream>

// Helper for fully replacing components inside string.
static inline void strsub(std::string& str,
		std::string l, std::string r) {
	std::string::size_type pos = std::string::npos;
	while((pos = str.find(l)) != std::string::npos) {
		str.replace(pos, l.size(), r);
		pos += r.size();
	}
}

// Helper for converting string into memory size.
static inline size_t strmemsize(std::string str) {
	std::regex rememsize("(\\d+)([kKmMgGtT]?)[bB]?");
	std::smatch result; if(std::regex_match(str, result, rememsize)) {
		std::string num, unit(" ");
		num = result[1].str();
		if(result.size() > 2) unit = result[2].str();

		size_t v = std::stoul(num);
		if(unit.size() > 0) switch(unit[0]) {
			case 'k': case 'K': return v << 10;
			case 'm': case 'M': return v << 20;
			case 'g': case 'G': return v << 30;
			case 't': case 'T': return v << 40;
			default: return v;
		}
	} else throw std::logic_error("Malformed memory size: \"" + str + "\".");
}

namespace wdedup {

/**
 * @brief Parses the command line argument of wdedup.
 *
 * When the returned value is not zero, an error occurs, 
 * and the result is the error code. When the returned
 * value is zero, depends on whether options.run is set
 * to true to either execute on or exit with code 0.
 */
int argparse(int argc, char** argv, wdedup::ProgramOptions& options) {
	namespace po = boost::program_options;
	options.run = true;

	// Configurable arguments for this application.
	bool help = false;
	typedef std::vector<std::string> positionalHolder;
	positionalHolder origfile, workdir;

	// Initialize positional arguments.
	po::options_description positionals("Positional Arguments");
	positionals.add_options()
		("origfile", po::value<positionalHolder>(&origfile),
			"The original file taken to perform word deduplication. "
			"Currently only regular files are accepted.")
		("workdir", po::value<positionalHolder>(&workdir),
			"Specifies the working directory for memorizing "
			"intermediate data and progression log. Previously "
			"interrupted progression will be resumed if the "
			"provided progression log is valid.");

	// Initialize optional flags (environment configurations).
	po::options_description optionals("Options");
	optionals.add_options()
		("help,h", po::bool_switch(&help), "Print this help message.")
		("memory-size,m", po::value<std::string>()->default_value("1g"),
			"Configure the size of working memory. The wdedup "
			"will attempt to allocate such size of memory when it "
			"starts and record its working data into the memory.")
		("page-pinned,p", po::bool_switch(&options.pagePinned),
			"Configure whether the working memory should be page "
			"pinned (not swapped out and resides in RAM).");

	// Initialize debug flags (used for debugging purpose).
	po::options_description debugs("Debug Flags");
	debugs.add_options()
		("wprof-only", po::bool_switch(&options.profileOnly),
			"Cause wdedup to perform profiling (wprof) and exits "
			"soon after wprof is completed.")
		("wmerge-only", po::bool_switch(&options.mergeOnly),
			"Cause wdedup to perform merging (wmerge) and exits "
			"soon after wmerge is completed.")
		("disable-gc", po::bool_switch(&options.disableGC),
			"Prevent wmerge from garbage collecting intermediate "
			"pages, so that these pages can be analyzed.");

	// Aggregate as argument parser.
	po::options_description usage;
	usage.add(positionals).add(optionals).add(debugs);

	// Retrieve a formatted help message.
	auto getHelpMessage = [&]() -> std::string {
		std::stringstream fmt;
		fmt << "Usage: " << argv[0] << " [--flags] FILE WORKDIR" 
			<< std::endl
			<< "Performs word deduplication for large files in "
			<< "a I/O-based and recoverable way."
			<< std::endl << usage << std::endl;

		std::string result = fmt.str();
		strsub(result,	"--origfile arg", 
				"FILE          ");
		strsub(result,	"--workdir arg",
				"WORKDIR      ");
		return result;
	};

	// Attempt to perform parsing.
	try {
		// Initialize and parse arguments from command line.
		po::variables_map vm;
		po::positional_options_description pod;
		pod.add("origfile", 1); pod.add("workdir", 1);
		po::store(po::command_line_parser(argc, argv).
			options(usage).positional(pod).run(), vm);
		po::notify(vm);

		// Print out help message if required.
		if(help) {
			std::cerr << getHelpMessage();
			options.run = false;
			return 0;
		}

		// Take the positional arguments.
		if(origfile.size() >= 1) options.origfile = origfile[0];
		else throw std::logic_error("FILE must be specified");

		if(workdir.size() >= 1) options.workdir = workdir[0];
		else throw std::logic_error("WORKDIR must be specified");

		// Parse the memory size, and make sure it is no less than minWorkmem.
		options.workmem = strmemsize(vm["memory-size"].as<std::string>());
		if(options.workmem < wdedup::minWorkmem) {
			std::stringstream wmerr;
			wmerr << "At least " << wdedup::minWorkmem 
				<< " bytes workmem is required.";
			throw std::logic_error(wmerr.str());
		}
	} catch(std::logic_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << getHelpMessage();
		return -1;
	}

	return 0;
}

} // namespace wdedup
