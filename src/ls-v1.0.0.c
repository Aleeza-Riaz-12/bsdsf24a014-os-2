#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
/*
 * PA-02: progressively enhanced ls utility (C / Linux).
 * Independent educational implementation; see STARTER_CODE_NOTICE.md.
 * Current milestone: 1.4.0.
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
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <sys/ioctl.h>
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

typedef enum { DISPLAY_SIMPLE, DISPLAY_LONG, DISPLAY_HORIZONTAL } DisplayMode;
static DisplayMode mode = DISPLAY_SIMPLE;
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

/* qsort expects a comparator between two array elements (Entry objects). */
static int compare_entries(const void *left, const void *right)
{
    const Entry *a = left, *b = right;
    return strcmp(a->name, b->name);
}

static void permission_string(mode_t m, char out[11])
{
    out[0] = S_ISREG(m) ? '-' : S_ISDIR(m) ? 'd' : S_ISLNK(m) ? 'l' :
             S_ISCHR(m) ? 'c' : S_ISBLK(m) ? 'b' : S_ISFIFO(m) ? 'p' :
             S_ISSOCK(m) ? 's' : '?';
    out[1] = (m & S_IRUSR) ? 'r' : '-';
    out[2] = (m & S_IWUSR) ? 'w' : '-';
    out[3] = (m & S_IXUSR) ? 'x' : '-';
    out[4] = (m & S_IRGRP) ? 'r' : '-';
    out[5] = (m & S_IWGRP) ? 'w' : '-';
    out[6] = (m & S_IXGRP) ? 'x' : '-';
    out[7] = (m & S_IROTH) ? 'r' : '-';
    out[8] = (m & S_IWOTH) ? 'w' : '-';
    out[9] = (m & S_IXOTH) ? 'x' : '-';
    if (m & S_ISUID) out[3] = (m & S_IXUSR) ? 's' : 'S';
    if (m & S_ISGID) out[6] = (m & S_IXGRP) ? 's' : 'S';
    if (m & S_ISVTX) out[9] = (m & S_IXOTH) ? 't' : 'T';
    out[10] = '\0';
}

static void owner_name(uid_t uid, char *buffer, size_t size)
{
    struct passwd *pw = getpwuid(uid);
    if (pw) snprintf(buffer, size, "%s", pw->pw_name);
    else snprintf(buffer, size, "%ju", (uintmax_t)uid);
}

static void group_name(gid_t gid, char *buffer, size_t size)
{
    struct group *gr = getgrgid(gid);
    if (gr) snprintf(buffer, size, "%s", gr->gr_name);
    else snprintf(buffer, size, "%ju", (uintmax_t)gid);
}

static int decimal_width(uintmax_t number)
{
    int width = 1;
    while (number >= 10) { number /= 10; ++width; }
    return width;
}

static void print_name(const Entry *entry)
{
    fputs(entry->name, stdout);
}

static void display_long(const EntryList *entries)
{
    int links_width = 1, size_width = 1;
    size_t owner_width = 1, group_width = 1;
    uintmax_t block_total = 0;
    for (size_t i = 0; i < entries->length; ++i) {
        const Entry *e = &entries->items[i];
        if (!e->valid_info) continue;
        char owner[256], group[256];
        owner_name(e->info.st_uid, owner, sizeof(owner));
        group_name(e->info.st_gid, group, sizeof(group));
        size_t len = strlen(owner);
        if (len > owner_width) owner_width = len;
        len = strlen(group);
        if (len > group_width) group_width = len;
        int width = decimal_width((uintmax_t)e->info.st_nlink);
        if (width > links_width) links_width = width;
        width = decimal_width((uintmax_t)(e->info.st_size < 0 ? 0 : e->info.st_size));
        if (width > size_width) size_width = width;
        if (e->info.st_blocks > 0) block_total += (uintmax_t)e->info.st_blocks;
    }
    /* st_blocks counts 512-byte blocks; GNU ls normally reports 1KiB blocks. */
    printf("total %ju\n", (block_total + 1) / 2);
    for (size_t i = 0; i < entries->length; ++i) {
        const Entry *e = &entries->items[i];
        if (!e->valid_info) { print_name(e); putchar('\n'); continue; }
        char permissions[11], owner[256], group[256], when[32] = "??? ?? ??:??";
        permission_string(e->info.st_mode, permissions);
        owner_name(e->info.st_uid, owner, sizeof(owner));
        group_name(e->info.st_gid, group, sizeof(group));
        char timebuf[32];
        if (ctime_r(&e->info.st_mtime, timebuf)) {
            /* ctime format: Www Mmm dd hh:mm:ss yyyy; show Mmm dd hh:mm. */
            memcpy(when, timebuf + 4, 12);
            when[12] = '\0';
        }
        printf("%s %*ju %-*s %-*s %*jd %s ", permissions,
               links_width, (uintmax_t)e->info.st_nlink,
               (int)owner_width, owner, (int)group_width, group,
               size_width, (intmax_t)e->info.st_size, when);
        print_name(e);
        if (S_ISLNK(e->info.st_mode)) {
            /* Do not follow symbolic links; print the link target as GNU ls -l does. */
            char target[4096];
            ssize_t read_count = readlink(e->path, target, sizeof(target) - 1);
            if (read_count >= 0) {
                target[read_count] = '\0';
                printf(" -> %s", target);
            }
        }
        putchar('\n');
    }
}

static unsigned terminal_width(void)
{
    struct winsize window = {0};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) == 0 && window.ws_col > 0)
        return window.ws_col;
    /* For redirected output and unit tests, use COLUMNS if set. */
    const char *env = getenv("COLUMNS");
    if (env && *env) {
        char *end;
        unsigned long width = strtoul(env, &end, 10);
        if (*end == '\0' && width > 0 && width <= UINT_MAX) return (unsigned)width;
    }
    return 80;
}

static size_t columns_fit(const EntryList *entries, size_t *column_width)
{
    size_t longest = 0;
    for (size_t i = 0; i < entries->length; ++i) {
        size_t n = strlen(entries->items[i].name);
        if (n > longest) longest = n;
    }
    *column_width = longest + 2;
    size_t columns = terminal_width() / *column_width;
    if (columns < 1) columns = 1;
    if (columns > entries->length) columns = entries->length;
    return columns;
}

static void print_padded(const Entry *entry, size_t width, int last_column)
{
    print_name(entry);
    if (!last_column) {
        size_t used = strlen(entry->name);
        for (size_t i = used; i < width; ++i) putchar(' ');
    }
}

static void display_vertical(const EntryList *entries)
{
    if (entries->length == 0) return;
    size_t width;
    size_t cols = columns_fit(entries, &width);
    size_t rows = (entries->length + cols - 1) / cols;
    for (size_t row = 0; row < rows; ++row) {
        for (size_t col = 0; col < cols; ++col) {
            size_t index = col * rows + row;
            if (index >= entries->length) continue;
            int last = (col == cols - 1 || (col + 1) * rows + row >= entries->length);
            print_padded(&entries->items[index], width, last);
        }
        putchar('\n');
    }
}

static void display_horizontal(const EntryList *entries)
{
    if (entries->length == 0) return;
    size_t width;
    size_t cols = columns_fit(entries, &width);
    for (size_t i = 0; i < entries->length; ++i) {
        int last = (i % cols == cols - 1 || i + 1 == entries->length);
        print_padded(&entries->items[i], width, last);
        if (last) putchar('\n');
    }
}

static void display_entries(const EntryList *entries)
{
    if (mode == DISPLAY_LONG) { display_long(entries); return; }
    if (mode == DISPLAY_HORIZONTAL) { display_horizontal(entries); return; }
    display_vertical(entries);
}

static int do_ls(const char *directory)
{
    EntryList entries = {0};
    if (collect_directory(directory, &entries) != 0) {
        free_entries(&entries);
        return -1;
    }
    qsort(entries.items, entries.length, sizeof(Entry), compare_entries);
    display_entries(&entries);
    free_entries(&entries);
    return 0;
}

int main(int argc, char **argv)
{
    int opt;
    while ((opt = getopt(argc, argv, "lx")) != -1) {
        switch (opt) {
            case 'l': mode = DISPLAY_LONG; break;
            case 'x': mode = DISPLAY_HORIZONTAL; break;
            default:
                fprintf(stderr, "Usage: %s [-lx] [directory ...]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }
    int first = optind;
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