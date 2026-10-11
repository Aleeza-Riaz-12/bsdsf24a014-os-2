#!/usr/bin/env bash
# Only run after reviewing the code and the starter-code notice.
set -euo pipefail
REPO_NAME='BSDSF24A014-OS-A02'
command -v gh >/dev/null || { echo 'Install the GitHub CLI first: https://cli.github.com/' >&2; exit 1; }
git rev-parse --is-inside-work-tree >/dev/null
if ! gh auth status >/dev/null 2>&1; then
  echo 'Authenticate with: gh auth login' >&2
  exit 1
fi
USERNAME="$(gh api user --jq .login)"
FULL_NAME="${USERNAME}/${REPO_NAME}"
if ! gh repo view "$FULL_NAME" >/dev/null 2>&1; then
  gh repo create "$FULL_NAME" --public --description 'Operating Systems Programming Assignment 02'
fi
URL="https://github.com/${FULL_NAME}.git"
if git remote get-url origin >/dev/null 2>&1; then
  echo "The existing origin is $(git remote get-url origin)"
  if [ "$(git remote get-url origin)" != "$URL" ]; then
    echo "Set your origin manually to $URL before continuing." >&2
    exit 1
  fi
else
  git remote add origin "$URL"
fi
git push -u origin main
git push origin --all
git push origin --tags
for v in 1.1.0 1.2.0 1.3.0 1.4.0 1.5.0 1.6.0; do
  ASSET="../release-assets/ls-v${v}"
  if [[ ! -f "$ASSET" ]]; then
    echo "Missing $ASSET; rebuild from tag and upload separately." >&2
    continue
  fi
  if gh release view "v${v}" --repo "$FULL_NAME" >/dev/null 2>&1; then
    echo "Existing release v${v}; not changing it."
  else
    gh release create "v${v}" "$ASSET" --repo "$FULL_NAME" \
      --title "Version ${v}" --notes "Operating Systems PA-02 milestone v${v}. Linux x86-64 binary asset."
  fi
done
echo "Published to https://github.com/$FULL_NAME"
