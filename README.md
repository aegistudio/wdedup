# wdedup - Word Deduplication Tool for Large File

This tool finds the first non-repeating word in a large file
(est. up to 100GB). It is decomposed into multiple stages, you 
can alter some of the stages for customizing it into your
functions, like finding the first N-words, etc.

## Basic Approaches

Word deduplication for massive file problem can be solved via
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