#!/bin/sh

# Find the path to datamash/.git
gitdir=$(git rev-parse --absolute-git-dir)
[ $? -ne 0 ] && exit 1

# Find the path to datamash/hooks
hookdir="$(dirname "$gitdir")/hooks"

# Install hooks
ln -fs "$hookdir/pre-commit.sh" "$gitdir/hooks/pre-commit" || \
    { echo "Unable to install pre-commit hook" >&2; exit 1; }
