// Definitions common to CPU and GPU . Do NOT put structures here or 
// make any use of of long long without paying attention to alignment
// issues!

#ifndef COMMON_H
#define COMMON_H

#define TRUE   1
#define FALSE  0


// GGRIII: If you like having hair (on your head, not in clumps, in your
// hands), then the following parens are pretty important
#define KILOBYTE                  1024
#define MEGABYTE                  (1024 * KILOBYTE)
#define GIGABYTE                  (1024 * MEGABYTE)
#define TERABYTE                  (1024 * GIGABYTE)
#define PETABYTE                  (1024 * TERABYTE)
#define EXABYTE                   (1024 * PETABYTE)

// SIZE_OF_BUFFER indicates how much data to read from an image file
// at a time. This size should be a multiple of the maximum cluster size
// that Scalpel will encounter if "quick" mode is ever used.
#define SIZE_OF_BUFFER            (10 * MEGABYTE)

// The maximum number of patterns of the maximal length the GPU will currently
// support. Requires statically allocated structures.
#define MAX_PATTERNS 1024
#define MAX_PATTERN_LENGTH 20

#define PATTERN_CASESEN (MAX_PATTERN_LENGTH - 1)
#define PATTERN_WCSHIFT (MAX_PATTERN_LENGTH - 2)

// Size of the header / footer lookup tables, and the end marker for each row.
#define LOOKUP_ROWS	256
#define LOOKUP_COLUMNS	16
#define LOOKUP_ENDOFROW	127

// Number of threads per block to execute on the GPU.
// Do not change me unless you REALLY know what you're doing!
// Probably still don't.
#define THREADS_PER_BLOCK	256

// The maximum number if header + footer matches supported for finding on
// each block of data of size THREADS_PER_BLOCK.
#define MAX_RESULTS_PER_BLOCK	120 // 16 is definitely too small

// Results are encoded as header / footer index (byte) followed by the index
// where the header / footer was found in the block (byte).
#define RESULTS_SIZE_PER_BLOCK	(MAX_RESULTS_PER_BLOCK*2)

#endif // COMMON_H
