# BSDSF24A014-OS-A02

Operating Systems Programming Assignment 02 — a C reimplementation of a subset of GNU/Linux `ls`.

**IMPORTANT:** This is an independent educational implementation because the official instructor starter source was not supplied in the attached PDF. **See [`STARTER_CODE_NOTICE.md`](STARTER_CODE_NOTICE.md)** before using this for assessment. The instructor requires incremental changes based on their `ls-v1.0.0` program.

## Build and test (Linux, Ubuntu, or WSL)

```bash
make
./bin/ls
./bin/ls -l
./bin/ls -x
./bin/ls -R .
./bin/ls -lR src
make test
make clean
```

Requires `gcc`, `make`, `python3` and `git` (for the history). Install on Ubuntu with `sudo apt install build-essential python3 git`.

## Versions and required features

| Version | Feature | Example |
|---|---|---|
| v1.0.0 | Initial build; simple listing | `./bin/ls` |
| v1.1.0 | Long listing with permission bits, owner, group, size, time | `./bin/ls -l` |
| v1.2.0 | Down-then-across columns | `./bin/ls` |
| v1.3.0 | Across-then-down columns | `./bin/ls -x` |
| v1.4.0 | Alphabetical sorting with `qsort()` | `./bin/ls -l` |
| v1.5.0 | ANSI colors by file type | `./bin/ls` |
| v1.6.0 | Recursive directory listing | `./bin/ls -R` |

## Structure

```text
BSDSF24A014-OS-A02/
├── src/ls-v1.0.0.c       # Name preserved through all versions, evolving source
├── obj/                  # Compiled objects
├── bin/                  # Executable output
├── man/                  # Reserved for optional man page
├── tests/test_ls.py      # Feature regression tests
├── Makefile
├── README.md
├── REPORT.md
├── GITHUB_PUBLISH.md
├── STARTER_CODE_NOTICE.md
└── publish_github.sh
```

## Design summary

1. `collect_directory()` uses `opendir()`, `readdir()`, `lstat()` and a growing array of `Entry` records.
2. `display_long()` renders metadata; `display_vertical()` and `display_horizontal()` format columns.
3. `qsort()` sorts filenames once before any display mode.
4. `print_name()` applies the appropriate color and resets it.
5. `do_ls()` traverses subdirectories after printing a directory, without following symlinks.

The long-listing, columns, sorting, colors, and recursion are intended to match the assignment's specified subset of GNU `ls`, not all of GNU coreutils.

## Git workflow

The downloadable repository's `.git` folder includes local stage branches and annotated tags. Run `git branch -a`, `git log --oneline --graph --all`, and `git tag -n` to inspect. The repository has **not** been published to your GitHub account. See `GITHUB_PUBLISH.md` for publishing instructions and individual release assets, which are supplied in the separate `release-assets/` folder of the ZIP package.

## Learning requirement

The instructor requires students to watch the making-of-`ls` lecture and explain each line of code during a viva; this cannot be completed or certified for you. Study the C implementation and report before presenting it.
