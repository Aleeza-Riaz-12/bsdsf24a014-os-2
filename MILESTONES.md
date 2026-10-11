# Commit and release milestone checklist

- `v1.0.0`: Project setup, working one-per-line listing.
- `v1.1.0`: `-l`, permissions, ownership, timestamps and symlink destinations.
- `v1.2.0`: Terminal-width-based default vertical column display.
- `v1.3.0`: `-x` row-major column display.
- `v1.4.0`: Sorting with `qsort()` before printing.
- `v1.5.0`: File-type-based ANSI color output.
- `v1.6.0`: Recursive directory listing with `-R`.

The local `.git` history contains a separate feature branch and annotated tag for each of six upgrade milestones. The prepared `release-assets/` contains Linux x86-64 executables for each release. Note: the assignment requires actual GitHub Releases with uploaded binary assets, which are **not created locally**; run `publish_github.sh` after authenticating to GitHub.
