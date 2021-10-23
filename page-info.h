/*
 * page-info.h
 */

#ifndef PAGE_INFO_H_
#define PAGE_INFO_H_

#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* page frame number: if present, the physical frame for the page */
    uint64_t pfn;
    /* soft-dirty set */
    bool softdirty;
    /* exclusively mapped, see e.g., https://patchwork.kernel.org/patch/6787921/ */
    bool exclusive;
    /* is a file mapping */
    bool file;
    /* page is swapped out */
    bool swapped;
    /* page is present, i.e, a physical page is allocated */
    bool present;
    /* if true, the kpageflags were successfully loaded, if false they were not (and are all zero) */
    bool kpageflags_ok;
    /* the 64-bit flag value extracted from /proc/kpageflags only if pfn is non-null */
    uint64_t kpageflags;

} page_info;
/*
 * Information for a number of virtually consecutive pages.
 */
typedef struct {
    /* how many page_info structures are in the array pointed to by info */
    size_t num_pages;

    /* pointer to the array of page_info structures */
    page_info *info;
} page_info_array;


typedef struct {
    /* the number of pages on which this flag was set, always <= pages_available */
    size_t pages_set;

    /* the number of pages on which information could be obtained */
    size_t pages_available;

    /* the total number of pages examined, which may be greater than pages_available if
     * the flag value could not be obtained for some pages (usually because the pfn is not available
     * since the page is not yet present or because running as non-root.
     */
    size_t pages_total;

    /* the flag the values were queried for */
    int flag;

} flag_count;

/**
 * Examine the page info in infos to count the number of times a specified /proc/kpageflags flag was set,
 * effectively giving you a ratio, so you can say "80% of the pages for this allocation are backed by
 * huge pages" or whatever.
 *
 * The flags *must* come from kpageflags (these are not the same as those in /proc/pid/pagemap) and
 * are declared in linux/kernel-page-flags.h.
 *
 * Ideally, the flag information is available for all the pages in the range, so you can
 * say something about the entire range, but this is often not the case because (a) flags
 * are not available for pages that aren't present and (b) flags are generally never available
 * for non-root users. So the ratio structure indicates both the total number of pages as
 * well as the number of pages for which the flag information was available.
 */
flag_count get_flag_count(page_info_array infos, int flag);

/**
 * Given the case-insensitive name of a flag, return the flag number (the index of the bit
 * representing this flag), or -1 if the flag is not found. The "names" of the flags are
 * the same as the macro names in <linux/kernel-page-flags.h> without the KPF_ prefix.
 *
 * For example, the name of the transparent hugepages flag is "THP" and the corresponding
 * macro is KPF_THP, and the value of this macro and returned by this method is 22.
 *
 * You can generate the corresponding mask value to check the flag using (1ULL << value).
 */
int flag_from_name(char const *name);

/**
 * Print the info in the page_info structure to stdout.
 */
void print_info(page_info info);

/**
 * Print the info in the page_info structure to the give file.
 */
void fprint_info(FILE* file, page_info info);


/**
 * Print the table header that lines up with the tabluar format used by the "table" printing
 * functions. Called by fprint_ratios, or you can call it yourself if you want to prefix the
 * output with your own columns.
 */
void fprint_info_header(FILE *file);

/* print one info in a tabular format (as a single row) */
void fprint_info_row(FILE *file, page_info info);


/**
 * Print the ratio for each flag in infos. The ratio is the number of times the flag was set over
 * the total number of pages (or the total number of pages for which the information could be obtained).
 */
void fprint_ratios_noheader(FILE *file, page_info_array infos);
/*
 * Print a table with one row per page from the given infos.
 */
void fprint_ratios(FILE *file, page_info_array infos);

/*
 * Prints a summary of all the pages in the given array as ratios: the fraction of the time the given
 * flag was set.
 */
void fprint_table(FILE *f, page_info_array infos);


/**
 * Get info for a single page indicated by the given pointer (which may point anywhere in the page).
 */
page_info get_page_info(void *p);

/**
 * Get information for each page in the range from start (inclusive) to end (exclusive).
 */
page_info_array get_info_for_range(void *start, void *end);

/**
 * Free the memory associated with the given page_info_array. You shouldn't use it after this call.
 */
void free_info_array(page_info_array infos);

#ifdef __cplusplus
}
#endif

#endif /* PAGE_INFO_H_ */
