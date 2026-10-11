# PA-02 — Technical Report

**Roll number:** BSDSF24A014  
**Project:** Implementation of a simplified GNU/Linux `ls` utility in C  
**Versions covered:** v1.0.0 through v1.6.0

These answers describe the provided independent reference implementation. Review the code and express the ideas in your own words before your viva.

## Feature 2 — Complete Long Listing Format (`v1.1.0`)

**Q1. What is the crucial difference between `stat()` and `lstat()`? When is `lstat()` more appropriate in `ls`?**

`stat(path, &buf)` follows a symbolic link and returns metadata about its *target*. `lstat(path, &buf)` does not follow the link; it returns metadata about the *link itself*. For `ls -l`, `lstat()` is generally more appropriate because a symlink should be displayed as a symlink (`l`) and its target can be printed as `name -> target` using `readlink()`. Our code uses `lstat()` in `push_entry()`; this also prevents recursive traversal from following directory symlinks accidentally.

**Q2. How are the file type and permission bits extracted from `st_mode`?**

`st_mode` is a bitmask that stores the type and permission bits. A bitwise AND (`&`) isolates a particular flag. For example, `mode & S_IRUSR` is nonzero if the owner has read permission, and `mode & S_IXGRP` checks whether the group can execute. The `S_ISDIR(mode)`, `S_ISREG(mode)`, and `S_ISLNK(mode)` macros detect file types by checking the masked type field (related to `S_IFMT`, `S_IFDIR`, etc.). `permission_string()` forms a 10-character string: one file-type character, then owner/group/others `rwx` groups. Set-user-ID, set-group-ID, and sticky bits are represented with `s/S` or `t/T` in the execute positions.

## Feature 3 — Vertical Column Display (`v1.2.0`)

**Q1. Explain down-then-across printing. Why is a simple linear print loop insufficient?**

Read all filenames into a dynamic array and find the longest name. Determine how many columns fit and compute `rows = (count + cols - 1) / cols`. Print by *screen row*, but read entries from the array at `index = column * rows + row`. For six filenames and three columns (two rows), the first row prints array indices 0, 2, 4; the second prints 1, 3, 5. Simply printing indices 0, 1, 2, ... would create an *across-then-down* layout, not the requested vertical layout. Each displayed name is padded according to the maximum filename width.

**Q2. What does `ioctl()` do here? What are the limitations of a fixed 80-character width?**

`ioctl(STDOUT_FILENO, TIOCGWINSZ, &window)` asks the terminal driver for the current terminal window size. The program uses `window.ws_col` to compute how many columns fit. When stdout is not a terminal, our program uses a valid `COLUMNS` environment variable or 80 as a fallback. A fixed width of 80 wastes space on large terminals and can cause premature wrapping on narrow ones; a real terminal-width query adapts naturally when the window is resized.

## Feature 4 — Horizontal Column Display (`v1.3.0`)

**Q1. Compare the complexities of vertical and horizontal printing. Which needs more pre-calculation?**

Both need the number of entries, the longest filename and the available terminal width. Vertical printing needs one additional conceptual step: the number of rows and the column-major array mapping (`col * rows + row`). Horizontal printing is simpler: print entries in their original order, advance from left to right, then wrap after `cols` entries. Therefore vertical printing requires more layout pre-calculation and index arithmetic.

**Q2. How are the `-l`, `-x` and default display modes managed?**

The program uses a `DisplayMode` enum (`DISPLAY_SIMPLE`, `DISPLAY_LONG`, `DISPLAY_HORIZONTAL`). `getopt()` processes the CLI options and updates the enum when it reads `-l` or `-x`. `display_entries()` selects exactly one display function. With no option, it uses vertical printing from v1.2.0. For combinations such as `-lx`, the last processed display option wins. The recursion flag added later is independent of display mode.

## Feature 5 — Alphabetical Sorting (`v1.4.0`)

**Q1. Why do all entries need to be read before sorting? What happens with millions of files?**

`readdir()` returns directory entries one at a time in filesystem order, which is not guaranteed to be alphabetical. Sorting requires the collection, so we first store all entries in a dynamically allocated array; we then call `qsort()` before formatting the display. This uses O(n) extra memory for entries, names and paths and typically O(n log n) comparison time. With millions of files, allocation may be costly, sorting may be slow, and the program may run out of memory. A scalable alternative would involve external sorting or streaming, but that is outside this assignment.

**Q2. Why does `qsort()` require a comparator with `const void *` arguments?**

The signature is `int compare_entries(const void *left, const void *right)`. `qsort()` is generic and receives a pointer to each array element without knowing its type. Our comparator casts those pointers to `const Entry *` and compares their `name` fields with `strcmp()`. A negative result means the first name sorts before the second; zero means equal; a positive result means after. `const` means the comparator should not modify the items being compared.

## Feature 6 — Colorized Output (`v1.5.0`)

**Q1. How do ANSI escape codes produce color? Show printing a name in green.**

ANSI escape codes are control sequences interpreted by terminals. `\033[` starts an escape sequence; `32m` switches to green and `0m` resets formatting. For example:

```c
printf("\033[1;32m%s\033[0m\n", "run.sh");
```

The code's `print_name()` uses `entry_color()` to select the color and resets it immediately after the name. This preserves normal formatting for padding and subsequent text.

**Q2. Which permission bits indicate that a file is executable?**

The execute permission bits are `S_IXUSR` (owner), `S_IXGRP` (group) and `S_IXOTH` (others). The expression `mode & (S_IXUSR | S_IXGRP | S_IXOTH)` is nonzero when any of them is set. Our implementation applies green coloring only if the file is a regular file and at least one execute bit is present. It gives symlinks priority over executable colors, directories blue, archive extensions red, and special files reverse video.

## Feature 7 — Recursive Listing (`v1.6.0`)

**Q1. What is a base case, and what is the base case for recursive `ls`?**

A base case is a condition where a recursive function stops making further recursive calls. `do_ls()` prints the current directory and then calls itself only for entries whose `lstat()` mode is a directory. If a directory has no visible subdirectories, the loop makes no recursive calls and returns. The names `.` and `..` are excluded because hidden names are skipped; following them would otherwise create cycles. Symbolic links to directories are not followed either.

**Q2. Why construct a full path before calling the recursive function?**

A directory entry contains a name such as `subdir`, but that name is relative to the *process's current working directory*, not automatically relative to the directory currently being listed. If the program runs `do_ls("parent_dir")` and then calls `do_ls("subdir")`, it will incorrectly try to open `./subdir`. `join_path("parent_dir", "subdir")` creates `parent_dir/subdir`, ensuring `opendir()` and `lstat()` access the correct child. This also avoids changing the process's working directory during recursion.

## Testing and validation

Run `make clean && make && make test`. The automated tests verify the default vertical columns, `-l` metadata, `-x` horizontal columns, case-sensitive alphabetical ordering, ANSI color codes, recursion into nested directories, no traversal of directory symlinks, hiding dotfiles by default, and error handling. Also try `./bin/ls`, `./bin/ls -l`, `./bin/ls -x`, `./bin/ls -R tests`, and `./bin/ls -lR tests` in a Linux terminal. Testing in a real terminal is important for `ioctl(TIOCGWINSZ)`.

## Viva preparation — a 30-second description

"I developed a directory listing program using `opendir` and `readdir`. I saved file entries in a dynamic array and used `lstat` to fetch metadata. I added long listing, two column layouts based on terminal width, alphabetical sorting with `qsort`, ANSI colors based on file type, and recursive directory traversal. The Makefile builds it, and Git branches and tags preserve each milestone. I tested it with different directories and file types."

## Important implementation notes

- The current package is an **independent implementation**, not a confirmed modification of the instructor's exact starter file. Consult `STARTER_CODE_NOTICE.md` and integrate the instructor-provided starting code before claiming full compliance with the handout.
- Long listing uses the standard `ctime_r()` timestamp format; older files still display `hh:mm` rather than GNU `ls`'s conditional year format. Column widths use byte lengths rather than Unicode display-cell widths. These are known differences from full GNU `ls` behavior.
- The `-a` option is not implemented because it is explicitly optional in the handout.
- This report explains the concepts; the instructor requires you to be able to demonstrate them in your viva.
