# Publish the prepared repository to GitHub

GitHub publication is **not yet done**. A GitHub login/permission is needed. The local repo includes milestone branches and annotated tags; GitHub Releases and binaries must be uploaded once authenticated. Before publishing for grading, see `STARTER_CODE_NOTICE.md` and rebase this work onto the instructor's actual code where necessary.

## One-time GitHub setup

1. Open https://github.com and sign in to the account you want to submit from.
2. Install the GitHub CLI (`gh`) in Ubuntu/WSL. For example, `sudo apt install gh` where your distro offers it, or follow https://cli.github.com/manual/installation.
3. Run `gh auth login` and follow the browser verification prompt. Never paste a personal access token into chat.
4. Inspect the staged commits: `git log --oneline --graph --all`, `git branch -a`, `git tag -n`.
5. If the repository is not already created, the script can create the public repository. The local README exists in the first commit.

## Publish command

From the **repository root**:

```bash
bash publish_github.sh
```

This will (a) create or use `BSDSF24A014-OS-A02` under the authenticated GitHub account, (b) push `main`, all feature branches, and annotated tags, and (c) create the six GitHub Releases, each with the compiled Linux x86-64 asset prepared for that version. The binary files must be available under `../release-assets` (as they are in the ZIP) or you can build the binaries from their Git tags yourself. The script refuses to overwrite an existing GitHub Release. Review all content and confirm the required starter code before publishing.

The latest source is in `src/ls-v1.0.0.c` and builds on Linux using `make`. Attached release binaries are built for Linux x86-64 and may need rebuilding on another architecture; commands for rebuilding are:

```bash
git switch --detach v1.1.0
make clean && make
# attach bin/ls to that version's release
# repeat for remaining v1.2.0 ... v1.6.0
# then restore with: git switch main
```

Git commands for manual publication (after creating your own empty remote repo):

```bash
git remote add origin https://github.com/YOUR_USERNAME/BSDSF24A014-OS-A02.git
git push -u origin main
git push origin --all
git push origin --tags
```

Publish the six releases manually on the GitHub repository's Releases page if you do not use the `gh` script.
