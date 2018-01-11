# Hacking on GNU Datamash

This file contains random collection of hacking tid bits.

## Building from git

Use the following commands to build the latest version of datamash
(including development/unreleased features):

    git clone git://git.savannnah.gnu.org/datamash.git
    cd datamash
    ./bootstrap
    ./configure
    make
    make check

The following programs and versions are required when building from
git sources:

    autoconf (>=2.62)
    automake (>=1.11.1)
    autopoint and gettext (>=0.19.4)
    gperf
    pkg-config
    makeinfo (part of GNU texinfo)
    perl

These programs are for development from git but are not needed when
building from the released tar archives found on
https://ftp.gnu.org/gnu/datamash/ .

## Compilation Options

To use a specific compiler (e.g. `clang`), run:

    ./configure CC=clang

To enable warnings-as-errors (`-Werror`), run:

    ./configure --enable-werror

Warnings-as-errors is enabled automatically when building from git repository.  
To disable, run:

    ./configure --disable-werror

To disable code optimization (e.g. for easier debugging), run:

    # during configuration
    ./configure CFLAGS="-g -O0"
    # or during make
    make CFLAGS="-g -O0"

To enable LINT mode:

    ./configure --enable-lint

To test Debian-hardening configuration, run:

    make deb-hard

## Testing

### Common tests

To run the most common tests:

    make check

To run a specific test:

    make check TESTS=./tests/datamash-tests.pl

### Expensive tests

Expensive tests use valgrind to find memory usage errors, and use specially
mounted file-system to emulate errors.  
To create the special file systems, run:

    ./build-aux/create_corrupted_file_system.sh
    mkdir /tmp/badfs/
    sudo mount -o sync bad_disk.img /tmp/badfs/
    sudo chown $USER /tmp/fullfs/

    ./build-aux/create_small_file_system.sh
    mkdir /tmp/fullfs/
    sudo mount -o sync tiny_disk.img /tmp/fullfs/
    sudo chown $USER /tmp/fullfs/

Then run the expensive tests:

    make check-expensive

### Debug information in unit tests

To enable debug information when testing (to debug the testing framework):

    make check VERBOSE=yes DEBUG=1

The `DEBUG` environment variable is checked in `./tests/Coreutils.pm`.

### Test tarball distribution

To create a tarball (for a new release) and test it run:

    make distcheck

The filename will be based on the current git status
(e.g. `datamash-1.0.6.40-8207-dirty.tar.gz`).

### Test on remote systems

To test a tarball (or a git repository) on a remote host, run:

    ./build-aux/check-remote-make.sh FILE.tar.gz HOST

The `FILE.tar.gz` will be copied (scp) to host `HOST`, then it will be compiled
using `./configure && make && make check`. `HOST` must be configured for
password-less login.

See `./build-aux/check-remote-make.sh -h` for many more options.


## Coverage

To generate coverage information using `gcov` and `lcov`, run:

    make coverage

Coverage information will be stored in:

    ./doc/coverage/index.html

By default, the coverage rule runs the non-expensive tests.
To check coverage with expensive tests, run:

    make coverage-expensive

It seems recent versions of `lcov` disabled branch coverage by default
(checking only line and function coverage).  
To re-enable branch coverage, either:

* edit your `.lcovrc` file (e.g `/etc/lcovrc`) and change
    `lcov_branch_coverage` setting to 1
* add `--rc lcov_branch_coverage=1` to your lcov command line

## Static Code Analysis

Static Analysis target uses clang's `scan-build` tool. Run:

    make static-analysis

## Coding Style

To check the source code files against the most common syntax rules, run:

    make syntax-check

Most syntax-check rules are defined in `maint.mk` (auto-generated from gnulib's
`./gnulib/top/maint.mk`).  
Few additional rules are defined in `cfg.mk` (copied
from GNU Coreutils).

Syntax-check rules begin with `sc_` (e.g. `sc_long_lines`). The rule name will
appear when syntax-check fails:

    $ make syntax-check
    ...
    ...
    maint.mk: found trailing blank(s)
    make: *** [sc_trailing_blank] Error 1

After fixing the offending file, to check this rule alone (instead of
re-checking all syntax-check rules), use:

    make sc_traling_blank


## Make tips

### Silent build rules

To show detailed build commands, run:

    # During configure
    ./configure --disable-silent-rules
    # or during make
    make V=1

To show hide detailed build commands (showing only the file being built), run:

    # During configure
    ./configure --enable-silent-rules
    # or during make
    make V=0

### Non-Recursive Make

The build system uses **non recursive makefile** structure.  
Search for the 1997 paper "Recursive Make Considered Harmful" by Peter Miller,  
and see examples of switching to non-recursive make in
[GNU Coreutils](https://git.savannah.gnu.org/cgit/coreutils.git/log/?qt=grep&q=recursive+make),
[GNU Hello](https://git.savannah.gnu.org/cgit/hello.git/log/?qt=grep&q=recursive),
[GNU Datamash](https://git.savannah.gnu.org/cgit/datamash.git/commit/?id=9f4afd218aadc306f888d57295df3f77ae37be1f)

### Makefile structure

`Makefile.am` is the main makefile, which generates `Makefile.in` and finally
`Makefile` during `./bootstrap` or `autoreconf -i`.

`Makefile.am` includes the following items:

* lists source files in `datamash_SOURCES`
* lists tests in `TESTS`.
* `./lib/local.mk` - gnulib files
* `./doc/local.mk` - manual generation

The generated `Makefile` makes use of the following files:

* `maint.mk` - maintenance targets, from gnulib.
* `cfg.mk` - custom targets and configurations for datamash.

## Portability

### Windows using mingw

Compilation for windows using MinGW compilers requires the following CFLAGS:

    CFLAGS="-D__USE_MINGW_ANSI_STDIO=1"

`./configure` supports an option `--enable-mingw` which sets up the correct flags.
This option should be automatically detected correctly when using cross-compilation,
such as:

    ./configure --host=x86_64-w64-mingw32

Compilation under Cygwin does not require this flag.  
See related discussion: <https://lists.gnu.org/archive/html/bug-gnulib/2014-09/msg00052.html>
