#!/bin/sh

## Copyright (C) 2014 Assaf Gordon <assafgordon@gmail.com>
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
## This scripts lists the existing Git Tags, and asks to set a new git tag.
## Git tags are used to automatically set the program version.
##

die()
{
  BASE=$(basename "$0")
  echo "$BASE error: $@" >&2
  exit 1
}

cd $(dirname "$0")/.. || die "failed to set directory"


##
## Show existing tags, revisions and dates
##
echo "Existing Git Tags:"
echo
for TAG in $(git tag) ;
do
	echo "  Tag:	$TAG"
	git log --format="format:  Rev:	%H%n  Subj:	%s%n  Date:	%ad (%ar)" -n 1 "$TAG"
	echo
done

##
## Show the current git tag, which could be something like "2.0.3-112-abcd-dirty"
##
echo "Current Git ID:"
GITID=$(./build-aux/git-version-gen .tarball-version) || die "Failed to get git id"
echo "  $GITID"
echo

##
## On auto-tag the 'master' branch.
## For more complicated releases, do them manually
BRANCH=$(git branch | awk '/^\*/{print $2}')
[ "$BRANCH" = "master" ] || die "Seems like this is not the 'master' branch. Aborting"

##
## Ensure no un-commited changes
##
UNCOMMITED=$(git status -uno -s) || die "failed to get git status"
[ -z "$UNCOMMITED" ] || die "There are uncommited changes. Aborting"


##
## Ask if the user wants to create a new git tag
##
echo "Do you want to create a new git tag (i.e. mark a new version)?"
printf "Type 'yes' + enter to continue, or CTRL-C to stop: "
read NEWTAG
[ "$NEWTAG" = "yes" ] || { echo "Aborting." >&2 ; exit 1 ; }
echo
echo


##
## Ask for the git tag
##
printf "Enter new git tag, in the form of 'vX.Y.Z' (e.g. 'v2.1.13'): "
read NEWVERSION
echo "$NEWVERSION" | grep -E -q '^v[0-9]+\.[0-9]+\.[0-9]+$' || { echo "invalid version format ($NEWVERSION), expecting 'vX.Y.Z'" >&2 ; exit 1 ; }

##
## Add this tag to the git repository
##
git tag -a "$NEWVERSION" -m "New Version $NEWVERSION"
echo

## Verify updated tag
NEWGITID=$(./build-aux/git-version-gen .tarball-version) || die "Failed to get new git id"
[ "v$NEWGITID" = "$NEWVERSION" ] || die "Failed to set new git ID (./build-aux/git-version-gen did not return '$NEWVERSION')"

echo
## Re-generate 'configure' with the new tagged version
echo "Rebuilding package..."
./bootstrap || die "bootstrap failed"
./configure || die "configure failed"
make clean || die "make clean failed"
make -j || die "make failed"
make dist || die "make dist failed"
make distcheck || die "make distcheck failed"


##
## Suggest further action
##
TARBALL=compute-$NEWGITID.tar.gz
SRCURL=https://s3.amazonaws.com/agordon/compute/src/$TARBALL
SHA1=$(sha1sum "$TARBALL" | cut -f1 -d" ")
echo "

Further actions:
 1. Upload source tarball to S3:
       ./build-aux/aws-upload.sh "$TARBALL" src

 2. Update DEB package
       ssh deb7 ./make_deb.sh $SRCURL

 3. Update RPM package
       ssh centos ./make_rpm.sh $SRCURL

 4. Update FreeBSD binary
       ssh fbsd ./make_bsd.sh $SRCURL

 5. Update Homebrew Formula
       URL:   $SRCURL
       SHA1:  $SHA1

 6. Update Travis-CI MacOSX
       git checkout -b macosx1 master
       cp .travis.yml.MAC .travis.yml
       git add .travis.yml
       git commit -m "Travis Mac OS X Update"
       git push --set-upstream origin macosx1
       git checkout master

 7. Create static binary:
       ./build-aux/make_bin_dist.sh

 8. Update GitHub-pages (and website) to version $NEWGITID
       git co gh-pages
       vi _config.yml

 9. Push new tag to GitHub:     | To remove local git tag:
       git push --tags          |      git tag -d $NEWVERSION
                                | To remove Remote GitHub tag:
                                |      git tag -d $NEWVERSION
                                |      git push origin :refs/tags/$NEWVERSION

 10. Upload source tarball to GitHub Releases:
       https://github.com/agordon/compute/releases

"
