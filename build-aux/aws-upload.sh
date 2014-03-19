#!/bin/sh

##
## A small helper script to upload files to the AWS storage.
##
## NOTES:
## 1.  'aws' must be installed, from the Python package "awscli"
##     (use 'sudo pip install awscli')
## 2. The profile (see 'AWS_PROFILE' below) must be defined in
##    "~/.aws/config" file, such as:
##        [profile gordon]
##        output = table
##        region = us-east-1
##        aws_access_key_id = xxxxxx
##        aws_secret_access_key = xxxxxxxxxxxxxx

AWS_PROFILE=gordon
AWS_BUCKET=agordon
AWS_DIRECTORY=compute

die()
{
  BASE=$(basename "$0")
  echo "$BASE error: $@" >&2
  exit 1
}

usage()
{
  BASE=$(basename "$0")
  echo "Upload Files to AWS's storage

Usage: $BASE FILE [src|bin]

Will upload FILE to aws's s3://$AWS_BUCKET/$AWS_DIRECTORY/[src|bin]/FILE .

Example:
  \$ $BASE compute-1.0.3.tar.gz src
  upload: compute-1.0.3.tar.gz to s3://$AWS_BUCKET/$AWS_DIRECTORY/src/compute-1.0.3.tar.gz
  URL: https://s3.amazonaws.com/$AWS_BUCKET/$AWS_DIRECTORY/src/compute-1.0.3.tar.gz

"
  exit 1
}

FILE="$1"
DEST="$2"
[ -z "$FILE" ] && usage
[ -z "$DEST" ] && usage
[ "$DEST" = "src" -o "$DEST" = "bin" ] || die "destination (2nd param) must be 'src' or 'bin'"

[ -e "$FILE" ] || die "File '$FILE' not found"

which aws>/dev/null || die "The 'aws' program was not found. Install it with 'sudo pip install awscli'"

aws --profile "$AWS_PROFILE" s3 cp \
	--acl public-read \
	--storage-class REDUCED_REDUNDANCY \
	"$FILE" \
	s3://$AWS_BUCKET/$AWS_DIRECTORY/$DEST/ || die "failed to upload file to AWS"

URL=https://s3.amazonaws.com/$AWS_BUCKET/$AWS_DIRECTORY/$DEST/$(basename "$FILE")

echo "URL: $URL"

