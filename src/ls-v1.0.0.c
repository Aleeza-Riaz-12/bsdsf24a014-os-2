#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
/*
 * PA-02: progressively enhanced ls utility (C / Linux).
 * Independent educational implementation; see STARTER_CODE_NOTICE.md.
 * Current milestone: 1.0.0.
 */
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
typedef struct {
    char *name;
    char *path;
    struct stat info;
    int valid_info;
} Entry;

typedef struct {
    Entry *items;
    size_t length;
    size_t capacity;
} EntryList;

static int had_error = 0;

/* Allocate a joined path; this avoids chdir() and keeps recursion safe. */
static char *join_path(const char *parent, const char *child)
{
    size_t a = strlen(parent), b = strlen(child);
    int slash = (a > 0 && parent[a - 1] != '/');
    if (a > SIZE_MAX - b - (size_t)slash - 1) return NULL;
    char *result = malloc(a + b + (size_t)slash + 1);
    if (!result) return NULL;
    memcpy(result, parent, a);
    if (slash) result[a++] = '/';
    memcpy(result + a, child, b + 1);
    return result;
}

static void free_entries(EntryList *entries)
{
    for (size_t i = 0; i < entries->length; ++i) {
        free(entries->items[i].name);
        free(entries->items[i].path);
    }
    free(entries->items);
    entries->items = NULL;
    entries->length = entries->capacity = 0;
}

static int push_entry(EntryList *entries, const char *path, const char *name)
{
    if (entries->length == entries->capacity) {
        size_t capacity = entries->capacity ? entries->capacity * 2 : 16;
        if (capacity < entries->capacity || capacity > SIZE_MAX / sizeof(Entry)) return -1;
        Entry *resized = realloc(entries->items, capacity * sizeof(Entry));
        if (!resized) return -1;
        entries->items = resized;
        entries->capacity = capacity;
    }
    Entry next = {0};
    next.name = strdup(name);
    next.path = join_path(path, name);
    if (!next.name || !next.path) {
        free(next.name);
        free(next.path);
        return -1;
    }
    if (lstat(next.path, &next.info) == 0) next.valid_info = 1;
    else {
        fprintf(stderr, "ls: cannot stat '%s': %s\n", next.path, strerror(errno));
        had_error = 1;
    }
    entries->items[entries->length++] = next;
    return 0;
}

static int collect_directory(const char *directory, EntryList *entries)
{
    DIR *stream = opendir(directory);
    if (!stream) {
        fprintf(stderr, "ls: cannot open directory '%s': %s\n", directory, strerror(errno));
        had_error = 1;
        return -1;
    }
    int failed = 0;
    errno = 0;
    struct dirent *item;
    while ((item = readdir(stream)) != NULL) {
        if (item->d_name[0] == '.') continue; /* Like default ls, skip hidden files. */
        if (push_entry(entries, directory, item->d_name) != 0) {
            fprintf(stderr, "ls: out of memory while reading '%s'\n", directory);
            had_error = failed = 1;
            break;
        }
        errno = 0;
    }
    if (!failed && errno != 0) {
        fprintf(stderr, "ls: error reading '%s': %s\n", directory, strerror(errno));
        had_error = failed = 1;
    }
    if (closedir(stream) != 0) {
        fprintf(stderr, "ls: error closing '%s': %s\n", directory, strerror(errno));
        had_error = failed = 1;
    }
    return failed ? -1 : 0;
}

static void print_name(const Entry *entry)
{
    fputs(entry->name, stdout);
}

static void display_entries(const EntryList *entries)
{
    for (size_t i = 0; i < entries->length; ++i) {
        print_name(&entries->items[i]);
        putchar('\n');
    }
}

static int do_ls(const char *directory)
{
    EntryList entries = {0};
    if (collect_directory(directory, &entries) != 0) {
        free_entries(&entries);
        return -1;
    }
    display_entries(&entries);
    free_entries(&entries);
    return 0;
}

int main(int argc, char **argv)
{
    int first = 1;
    if (first == argc) {
        do_ls(".");
    } else {
        for (int i = first; i < argc; ++i) {
            if (argc - first > 1) {
                if (i > first) putchar('\n');
                printf("%s:\n", argv[i]);
            }
            do_ls(argv[i]);
        }
    }
    return had_error ? EXIT_FAILURE : EXIT_SUCCESS;
}