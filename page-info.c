/*
 * smaps.c
 *
 *  Created on: Jan 31, 2017
 *      Author: tdowns
 */

#include "page-info.h"

#include <stdio.h>
#include <sys/types.h>
#include <linux/kernel-page-flags.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <err.h>
#include <assert.h>
#include <limits.h>


#define PM_PFRAME_MASK         ((1ULL << 55) - 1)
#define PM_SOFT_DIRTY           (1ULL << 55)
#define PM_MMAP_EXCLUSIVE       (1ULL << 56)
#define PM_FILE                 (1ULL << 61)
#define PM_SWAP                 (1ULL << 62)
#define PM_PRESENT              (1ULL << 63)


/** bundles a flag with its description */
typedef struct {
    int flag_num;
    char const *name;
    bool show_default;
} flag;

#define FLAG_SHOW(name) { KPF_ ## name, # name, true },
#define FLAG_HIDE(name) { KPF_ ## name, # name, false },

const flag kpageflag_defs[] = {
        FLAG_SHOW(LOCKED       )
        FLAG_HIDE(ERROR        )
        FLAG_HIDE(REFERENCED   )
        FLAG_HIDE(UPTODATE     )
        FLAG_HIDE(DIRTY        )
        FLAG_HIDE(LRU          )
        FLAG_SHOW(ACTIVE       )
        FLAG_SHOW(SLAB         )
        FLAG_HIDE(WRITEBACK    )
        FLAG_HIDE(RECLAIM      )
        FLAG_SHOW(BUDDY        )
        FLAG_SHOW(MMAP         )
        FLAG_SHOW(ANON         )
        FLAG_SHOW(SWAPCACHE    )
        FLAG_SHOW(SWAPBACKED   )
        FLAG_SHOW(COMPOUND_HEAD)
        FLAG_SHOW(COMPOUND_TAIL)
        FLAG_SHOW(HUGE         )
        FLAG_SHOW(UNEVICTABLE  )
        FLAG_SHOW(HWPOISON     )
        FLAG_SHOW(NOPAGE       )
        FLAG_SHOW(KSM          )
        FLAG_SHOW(THP          )
        /* older kernels won't have these new flags, so conditionally compile in support for them */
#ifdef KPF_BALLOON
        FLAG_SHOW(BALLOON      )
#endif
#ifdef KPF_ZERO_PAGE
        FLAG_SHOW(ZERO_PAGE    )
#endif
#ifdef KPF_IDLE
        FLAG_SHOW(IDLE         )
#endif

        { -1, 0, false }  // sentinel
};

#define kpageflag_count (sizeof(kpageflag_defs)/sizeof(kpageflag_defs[0]) - 1)

#define ITERATE_FLAGS for (flag const *f = kpageflag_defs; f->flag_num != -1; f++)


// x-macro for doing some operation on all the pagemap flags
#define PAGEMAP_X(fn) \
    fn(softdirty ) \
    fn(exclusive ) \
    fn(file      ) \
    fn(swapped   ) \
    fn(present   )

static unsigned get_page_size() {
    long psize = sysconf(_SC_PAGESIZE);
    assert(psize >= 1 && psize <= UINT_MAX);
    return (unsigned)psize;
}

/* round the given pointer down to the page boundary (i.e,. return a pointer to the page it lives in) */
static inline void *pagedown(void *p, unsigned psize) {
    return (void *)(((uintptr_t)p) & -(uintptr_t)psize);
}

/**
 * Extract the interesting info from a 64-bit pagemap value, and return it as a page_info.
 */
page_info extract_info(uint64_t bits) {
    page_info ret = {};
    ret.pfn         = bits & PM_PFRAME_MASK;
    ret.softdirty   = bits & PM_SOFT_DIRTY;
    ret.exclusive   = bits & PM_MMAP_EXCLUSIVE;
    ret.file        = bits & PM_FILE;
    ret.swapped     = bits & PM_SWAP;
    ret.present     = bits & PM_PRESENT;
    return ret;
}

/* print page_info to the given file */
void fprint_info(FILE* f, page_info info) {
    fprintf(f,
            "PFN: %p\n"
            "softdirty = %d\n"
            "exclusive = %d\n"
            "file      = %d\n"
            "swapped   = %d\n"
            "present   = %d\n",
            (void*)info.pfn,
            info.softdirty,
            info.exclusive,
            info.file,
            info.swapped,
            info.present);
}

void print_info(page_info info) {
    fprint_info(stdout, info);
}

flag_count get_flag_count(page_info_array infos, int flag_num) {
    flag_count ret = {};

    if (flag_num < 0 || flag_num > 63) {
        return ret;
    }

    uint64_t flag = (1ULL << flag_num);

    ret.flag = flag_num;
    ret.pages_total = infos.num_pages;

    for (size_t i = 0; i < infos.num_pages; i++) {
        page_info info = infos.info[i];
        if (info.kpageflags_ok) {
            ret.pages_set += (info.kpageflags & flag) == flag;
            ret.pages_available++;
        }
    }
    return ret;
}

/**
 * Print the table header that lines up with the tabluar format used by the "table" printing
 * functions. Called by fprint_ratios, or you can call it yourself if you want to prefix the
 * output with your own columns.
 */
void fprint_info_header(FILE *file) {
    fprintf(file,  "         PFN  sdirty   excl   file swappd presnt ");
    ITERATE_FLAGS { if (f->show_default) fprintf(file, "%4.4s ", f->name); }
    fprintf(file, "\n");
}

/* print one info in a tabular format (as a single row) */
void fprint_info_row(FILE *file, page_info info) {
    fprintf(file, "%12p %7d%7d%7d%7d%7d ",
            (void*)info.pfn,
            info.softdirty,
            info.exclusive,
            info.file,
            info.swapped,
            info.present);

    if (info.kpageflags_ok) {
        ITERATE_FLAGS { if (f->show_default) fprintf(file, "%4d ", !!(info.kpageflags & (1ULL << f->flag_num))); }
    }
    fprintf(file, "\n");
}

#define DECLARE_ACCUM(name) size_t name ## _accum = 0;
#define INCR_ACCUM(name)    name ## _accum += info->name;
#define PRINT_ACCUM(name)   fprintf(file, "%7.4f", (double)name ## _accum / infos.num_pages);


void fprint_ratios_noheader(FILE *file, page_info_array infos) {
    PAGEMAP_X(DECLARE_ACCUM);
    size_t total_kpage_ok = 0;
    size_t flag_totals[kpageflag_count] = {};
    for (size_t p = 0; p < infos.num_pages; p++) {
        page_info *info = &infos.info[p];
        PAGEMAP_X(INCR_ACCUM);
        if (info->kpageflags_ok) {
            total_kpage_ok++;
            int i = 0;
            ITERATE_FLAGS {
                flag_totals[i++] += !!(info->kpageflags & (1ULL << f->flag_num));
            }
        }
    }

    printf("%12s ", "----------");
    PAGEMAP_X(PRINT_ACCUM)

    int i = 0;
    if (total_kpage_ok > 0) {
        ITERATE_FLAGS {
            if (f->show_default) fprintf(file, " %4.2f", (double)flag_totals[i] / total_kpage_ok);
            i++;
        }
    }
    fprintf(file, "\n");
}

/*
 * Print a table with one row per page from the given infos.
 */
void fprint_ratios(FILE *file, page_info_array infos) {
    fprint_info_header(file);
    fprint_ratios_noheader(file, infos);
}

/*
 * Prints a summary of all the pages in the given array as ratios: the fraction of the time the given
 * flag was set.
 */
void fprint_table(FILE *f, page_info_array infos) {
    fprintf(f, "%zu total pages\n", infos.num_pages);
    fprint_info_header(f);
    for (size_t p = 0; p < infos.num_pages; p++) {
        fprint_info_row(f, infos.info[p]);
    }
}



/**
 * Get info for a single page indicated by the given pointer (which may point anywhere in the page)
 */
page_info get_page_info(void *p) {
    // just get the info array for a single page
    page_info_array onepage = get_info_for_range(p, (char *)p + 1);
    assert(onepage.num_pages == 1);
    page_info ret = onepage.info[0];
    free_info_array(onepage);
    return ret;
}

/**
 * Get information for each page in the range from start (inclusive) to end (exclusive).
 */
page_info_array get_info_for_range(void *start, void *end) {
    unsigned psize = get_page_size();
    void *start_page = pagedown(start, psize);
    void *end_page   = pagedown(end - 1, psize) + psize;
    size_t page_count = start < end ? (end_page - start_page) / psize : 0;
    assert(page_count == 0 || start_page < end_page);

    if (page_count == 0) {
        return (page_info_array){ 0, NULL };
    }

    page_info *infos = malloc((page_count + 1) * sizeof(page_info));

    // open the pagemap file
    FILE *pagemap_file = fopen("/proc/self/pagemap", "rb");
    if (!pagemap_file) err(EXIT_FAILURE, "failed to open pagemap");

    // seek to the first page
    if (fseek(pagemap_file, (uintptr_t)start_page / psize * sizeof(uint64_t), SEEK_SET)) err(EXIT_FAILURE, "pagemap seek failed");

    size_t bitmap_bytes = page_count * sizeof(uint64_t);
    uint64_t* bitmap = malloc(bitmap_bytes);
    assert(bitmap);
    size_t readc = fread(bitmap, bitmap_bytes, 1, pagemap_file);
    if (readc != 1) err(EXIT_FAILURE, "unexpected fread(pagemap) return: %zu", readc);

    fclose(pagemap_file);

    FILE *kpageflags_file = NULL;
    enum { INIT, OPEN, FAILED  } file_state = INIT;

    for (size_t page_idx = 0; page_idx < page_count; page_idx++) {
        page_info info = extract_info(bitmap[page_idx]);

        if (info.pfn) {
            // we got a pfn, try to read /proc/kpageflags

            // open file if not open
            if (file_state == INIT) {
                kpageflags_file = fopen("/proc/kpageflags", "rb");
                if (!kpageflags_file) {
                    warn("failed to open kpageflags");
                    file_state = FAILED;
                } else {
                    file_state = OPEN;
                }
            }

            if (file_state == OPEN) {
                uint64_t bits;
                if (fseek(kpageflags_file, info.pfn * sizeof(bits), SEEK_SET)) err(EXIT_FAILURE, "kpageflags seek failed");
                if ((readc = fread(&bits, sizeof(bits), 1, kpageflags_file)) != 1) err(EXIT_FAILURE, "unexpected fread(kpageflags) return: %zu", readc);
                info.kpageflags_ok = true;
                info.kpageflags = bits;
            }
        }

        infos[page_idx] = info;
    }

    if (kpageflags_file)
        fclose(kpageflags_file);

    free(bitmap);

    return (page_info_array){ page_count, infos };
}

void free_info_array(page_info_array infos) {
    free(infos.info);
}

int flag_from_name(char const *name) {
    ITERATE_FLAGS {
        if (strcasecmp(f->name, name) == 0) {
            return f->flag_num;
        }
    }
    return -1;
}


