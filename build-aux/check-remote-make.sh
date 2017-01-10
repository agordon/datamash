#!/bin/sh

## Copyright (C) 2014-2017 Assaf Gordon <assafgordon@gmail.com>
##
## This file is part of GNU Datamash.
##
## GNU Datamash is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## GNU Datamash is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with GNU Datamash.  If not, see <http://www.gnu.org/licenses/>.


##
## A small helper script to easily copy, build and check
## a tarball (after 'make dist') on a remote host.
##
## The remote host should be properly configured for password-less login,
## with all the necessary programs already installed.
##

die()
{
BASE=$(basename "$0")
echo "$BASE: error: $@" >&2
exit 1
}

show_help_and_exit()
{
BASE=$(basename "$0")
echo "
$BASE - build & test automation

Usage:  $BASE [OPTIONS] SOURCE HOST

SOURCE - A source for the package, either:
  1. a local tarball filename
  2. a remote tarball filename (http/ftp)
  3. a remote git repository

HOST - Name of remote host to build and test on.
   HOST must be pre-configured for password-less login,
   and all required programs (e.g. C compiler) must be pre-installed.

OPTIONS:
   -h          - This help screen.
   -b BRANCH   - If SOURCE is GIT, check-out BRANCH
                 (instead of the default 'master' branch)
   -c PARAM    - Add PARAM as parameter to './configure'
   -m PARAM    - Add PARAM as parameter to 'make'
   -e NAME=VAL - Add NAME=VAL as environment variable to all commands.

NOTE:
  1. When building from tarball, will use './configure && make && make check'
  2. When building from git, will add './bootstrap' as well.
  3. Quoting is tricky, best to avoid double/single qutoes with -c/-m .
  4. Do not use spaces or quotes with '-e' - this will likely fail.

Examples:

  # Download the tarball, copy to host 'pinky', and run
  #   ./configure && make && make check
  # on the remote host
  $BASE http://www.gnu.org/gnu/datamash/datamash-1.0.5.tar.gz pinky

  # Copy the local tarball to host 'brain', and run
  #   ./configure CC=clang && make -j && make check
  $BASE -c CC=lang -m -j datamash-1.0.5.tar.gz brain

  # Clone the git repository to host 'tom', with branch 'test', then run:
  #   git clone -b test git://git.savannah.gnu.org/datamash.git &&
  #    cd datamash &&
  #      ./bootstrap &&
  #        ./configure &&
  #          make && make check
  $BASE -b test git://git.savannah.gnu.org/datamash.git tom

  # Clone the git repository to host 'jerry', and run:
  #  git clone git://git.savannah.gnu.org/datamash.git &&
  #   cd datamash &&
  #     PATH=/tmp/custom/bin:\$PATH ./bootstrap &&
  #       PATH=/tmp/custom/bin:\$PATH ./configure &&
  #         PATH=/tmp/custom/bin:\$PATH make &&
  #           PATH=/tmp/custom/bin:\$PATH make check
  $BASE -e 'PATH=/tmp/custom/bin:\$PATH' \
               git://git.savannah.gnu.org/datamash.git jerry
"
exit 0
}


##
## Script starts here
##

## parse parameterse
show_help=
configure_params=
make_params=
env_params=
git_branch=
while getopts b:c:m:e:h name
do
        case $name in
        b)      git_branch="-b '$OPTARG'"
                ;;
        c)      configure_params="$configure_params '$OPTARG'"
                ;;
        m)      make_params="$make_params '$OPTARG'"
                ;;
        e)      echo "$OPT_ARG" | grep -E -q '^[A-Za-z0-9]=' ||
                   die "error: '-e $OPTARG' doesn't look like a " \
                           "valid environment variable"
                env_params="$env_PARAMS $OPTARG"
                ;;
        h)      show_help=y
                ;;
        ?)      die "Try -h for help."
        esac
done
[ ! -z "$show_help" ] && show_help_and_exit;

shift $((OPTIND-1))

SOURCE=$1
TARGET_HOST=$2
[ -z "$SOURCE" ] && die "missing SOURCE file name or URL " \
                            "(e.g. datamash-1.0.1.tar.gz). Try -h for help."
[ -z "$TARGET_HOST" ] &&
  die "missing target HOST name (e.g. fbsd). Try -h for help."
shift 2


GIT_REPO=
TARBALL=

##
## Validate source (git, remote file, local file)
##

## Is the source a git repository or a local TARBALL file?
if echo "$SOURCE" | grep -E -q '^git://|\.git$' ; then
  ## a Git repository source
  git ls-remote "$SOURCE" >/dev/null ||
    die "source ($SOURCE) is not a valid remote git repository"
  GIT_REPO=$SOURCE
  BASENAME=$(basename $GIT_REPO)
elif echo "$SOURCE" |
	grep -E -q '^(ht|f)tp://[A-Za-z0-9\_\.\/-]*\.tar\.(gz|bz2|xz)' ; then
  ## a remote tarball source
  TMP1=$(basename "$SOURCE") || die "failed to get basename of '$SOURCE'"
  if [ -e "$TMP1" ] ; then
    echo "$TMP1 already exists locally, skipping download." >&2
  else
    wget -O "$TMP1" "$SOURCE" || die "failed to download '$SOURCE'"
  fi
  TARBALL="$TMP1"
else
  ## assume a local tarball source
  [ -e "$SOURCE" ] || die "source file $SOURCE not found"
  TARBALL="$SOURCE"
fi


##
## If it's a local file, determine compression type
##
if test -n "$TARBALL" ; then
  FILENAME=$(basename "$TARBALL" ) || exit 1
  EXT=${FILENAME##*.tar.}
  if test "$EXT" = "gz" ; then
    COMPPROG=gzip
  elif test "$EXT" = "bz2" ; then
    COMPPROG=bzip2
  elif test "$EXT" = "xz" ; then
    COMPPROG=xz
  else
    die "unknown compression extension ($EXT) in filename ($FILENAME)"
  fi
  BASENAME=$(basename "$TARBALL" .tar.$EXT) || exit 1
fi

## Test connectivity
## If configured properly, this should work without requiring a password.
UNAME=$(ssh -n "$TARGET_HOST" "uname -a") ||
    die "failed to run 'uname' on target host '$TARGET_HOST'"

##
## Create a (remote) temporary directory
##
DIR=$(ssh -q "$TARGET_HOST" mktemp -d /tmp/$BASENAME.XXXXXX) ||
    die "failed to create temporary directory on remote host $TARGET_HOST"

##
## Copy the tarball to the remote machine
##
if test -n "$TARBALL" ; then
  scp -q "$TARBALL" "$TARGET_HOST:$DIR/" ||
      die "Failed to scp '$TARBALL' to '$TARGET_HOST:$DIR/'"
fi

##
## The script to build the package
##
SCRIPT="
echo '--SYSTEM-INFO-START--' ;
echo 'UNAME_KERNEL='\$(uname -s) ;
echo 'UNAME_KERNEL_RELEASE='\$(uname -r) ;
echo 'UNAME_KERNEL_VERSION='\$(uname -v) ;
echo 'UNAME_MACHINE='\$(uname -m 2>/dev/null||echo unknown) ;
echo 'UNAME_HARDWARE='\$(uname -i 2>/dev/null||echo unknown) ;
echo 'UNAME_PROCESSOR='\$(uname -p 2>/dev/null||echo unknown) ;
echo 'UNAME_OS='\$(uname -o 2>/dev/null||echo unknown) ;
echo 'DISTRIBUTION='\$(
           lsb_release -si 2>/dev/null||uname -o 2>/dev/null||echo unknown) ;
echo 'DISTRIBUTION_VERSION='\$(
           lsb_release -sr 2>/dev/null||uname -r 2>/dev/null||echo unknown) ;
echo 'PACKAGE_BASENAME=$BASENAME' ;
echo 'PACKAGE_SOURCE=$SOURCE' ;
echo 'START_DATE_UTC='\$(date -u +%Y-%m-%d_%H%M%S) ;
echo 'START_DATE_LOCAL='\$(date) ;
echo 'HOSTNAME='\$(hostname -f) ;
echo 'BUILD_DIR=$DIR' ;
echo 'BUILD_CONFIGURE_PARAMS=$configure_params' ;
echo 'BUILD_MAKE_PARAMS=$make_params' ;
echo 'BUILD_ENV_PARAMS=$env_params' ;
echo '--SYSTEM-INFO-END--' ;
"

##
## Extract and run on the remote machine
##
## NOTE: about the convoluted 'cd' command:
##   Most release tarballs contain a sub-directory with the same name as
##   the tarball itself (e.g. 'grep-2.9.1-abcd.tar.gz' will contain
##   './grep-2.9.1-abcd/').
##   But few tarballs (especially alpha-stage and temporary ones send to
##   GNU platform-testers can might contain other sub-directory names
##   (e.g. 'grep-2.9.1.tar.gz' might have './grep-ss' sub directory).
##   So use 'find' to find the first sub directory
##   (assuming there's only one).
if test -n "$TARBALL" ; then
SCRIPT="$SCRIPT
cd $DIR ;
$COMPPROG -dc $FILENAME | tar -xf - &&
  cd \$(find . -maxdepth 1 -type d | tail -n 1) &&
"
elif test -n "$GIT_REPO" ; then
SCRIPT="$SCRIPT
cd $DIR ;
git clone $git_branch $GIT_REPO $BASENAME &&
  cd $BASENAME &&
  $env_params ./bootstrap &&
"
else
  die "internal error: not a TARBALL or a GIT_REPO"
fi

##
## Add common building steps
##
SCRIPT="$SCRIPT
  $env_params ./configure $configure_params &&
  $env_params make $make_params &&
  $env_params make check &&
  cd $(dirname $DIR) &&
  rm -r $(basename $DIR)
"

ssh -q "$TARGET_HOST" "$SCRIPT" 2>&1 ||
	die "Failed to build '$TARBALL' on '$TARGET_HOST:$DIR/'"
