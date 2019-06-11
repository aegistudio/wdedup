# wdedup - Word Deduplication Tool for Large File

This tool finds the first non-repeating word in a large file
(est. up to 100GB). It is decomposed into multiple stages, you 
can alter some of the stages for customizing it into your
functions, like finding the first N-words, etc.

## Project Building

To build the project, you needs to make sure these prerequisites
(tools or libraries) are installed:

```
GCC (G++)                     (C++ build tools)
CMake >= 3.5                  (C++ build framework)
GNU Make                      (Project build tool)
PkgConfig                     (Library configuration tool)
GoogleTest (optional)         (C++ test framework)
Boost.ProgramOptions          (CLI parser library)
LibBSD                        (Provider of embedded Rbtree)
```

Under ubuntu, you can install the prerequisites (except for
GoogleTest) by executing bash command:

```bash
sudo apt install build-essential cmake pkg-config \
                 libboost-program-options-dev libbsd-dev
```

GoogleTest can be build and installed manually by executing (you
can skip this step if you don't want to run test cases):

```bash
git clone https://github.com/google/googletest.git
cd googletest
mkdir bin
cd bin
cmake ..
make install # WARNING: 'sudo' might be required
```

After fulfilling prerequisites, make this project root directory
as your current working directory, and executes bash commands:

```bash
mkdir bin
cd bin
cmake .. # Or "cmake -DWDEDUP_RUNTESTS=OFF .." if not running tests.
make
```

And if the project is success to build, an executable named 
`wdedup` will be placed under the `bin` directory. By running 
`wdedup -h` will you see its command line usage.

To run test cases, having GoogleTest installed and 
`WDEDUP_RUNTESTS` set to `ON`, run `make test` or `ctest`.

## Basic Approaches

Word deduplication for large file problem can be solved via
divide-and-conquer approaches: the massive file is divided into
multiple disjoint segments, which forms the original file after
concatenated. A enumeration profile is generated for each 
segment. In turns they consolidate into larger profile (e.g.
profiles for segment `[a..b]` and `[b+1..c]` can be consolidated
into `[a..c]`), and finally forms the profile for the whole file.

For each segments, enumerate each words, recording whether they 
are repeated (or the amount of them if you like) and where is 
first occurence of the word in the file. The output word will
be ordered in lexical order, with repetition/amount and first
occurence offset after them.

For each enumeration profile, merge them to form larger profile:
the head words of two profiles will be compared, if not equal,
the less one will be written out, otherwise their repetition/
amount will be altered and merge, with the lesser occurence
be picked up.

All enumeration profile will form a whole profile, and a full
scan with Find-First or Find-TopN will be performed on such 
profile. A final result will be generated thereafter.

## Complexity Analysis

Denote original file size as `N`, memory size as `M`, ommiting
other parameters while analyzing.

While profiling, heap-sorting or rbtree will be performed, 
contributing up to `O(MlgM)` time complexity for each segment,
there're totally `N/M` segments so it contributes up to O(NlgM) 
time complexity in this stage. `O(N)` (sequential) I/O complexity 
will also be contributed in this stage.

While merging, each level of merging contributes up to `O(N)` 
time complexity, and there're `lg(N/M)` stages, so the merging
stage contribute up to `O(NlgN-NlgM)` time complexity. 
`O(NlgN-NlgM)` (sequential) I/O complexity will also be 
contributed in this stage.

While scanning, It contributes up to `O(N)` time complexity (if
we don't take other parameters into consideration). And `O(N)`
(sequential) I/O complexity will also be contributed in this stage.

Summing up the time complexity of each stages, the whole 
algorithm takes up to `O(NlgN)` time complexity, with `O(NlgN
-NlgM+2N)` I/O complexity. Increasing the memory size `M` will 
not affects time complexity, but reduces I/O complexity, 
which therefore reduces the running time.
