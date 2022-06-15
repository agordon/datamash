Git Hooks
=========

git client-side hooks are not considered part of the repository and
aren't included in a `git clone`. Running `hooks/setup-hooks.sh` will
install some useful ones for you.

This is only needed if you're working on the datamash source; if
you're just compiling it, there's no reason to do this.

Installed Hooks
===============

pre-commit
----------

Makes sure the code tree passes `make syntax-check` before allowing a
commit.
