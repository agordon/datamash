#!/bin/sh

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
## Ask if the user wants to push (=publish) this version to GitHub.
##
echo ""
echo "Do you want to publish this version (push it to github and make it public)?"
echo "NOTE: Modifying published tags is highly discouraged, so there's no going back."
echo "      If later on you want to change this tag (or re-tag another revision),"
echo "      The recommended way is to simply bump the verison and publish a new tag."
echo ""
echo "If you abort now, you can later delete the tag with 'git tag -d $NEWVERSION',"
echo "or publish the tag with 'git push --tags'"
echo ""
printf "Type 'yes' + enter to continue and publish the new tag, or CTRL-C to stop: "
read PUSHTAG
if [ "$PUSHTAG" = "yes" ] ; then
	echo "Pushing tags"
	git push --tags || die "Pushing new tags failed"
fi
echo
echo
echo

##
## Suggest further action
##
echo "

Further actions:
 1. Upload source tarball to S3:
       ./build-aux/aws-upload.sh compute-$NEWGITID.tar.gz src

 2. Upload source tarball to GitHub Releases:
       https://github.com/agordon/compute/releases

 3. Update Homebrew Formula

 4. Update Travis-CI MacOSX
       git co TravisMacOSX4
       git merge master
       git push

 5. Create static binary:
       ./build-aux/make_bin_dist.sh

 6. Update RPM package

 7. Update DEB package

 8. Update GitHub-pages (and website) to version $NEWGITID
       git co gh-pages
       vi _config.yml

"
