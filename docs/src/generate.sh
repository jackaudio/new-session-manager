#!/bin/sh


#The documentation is built statically and does not belong to the normal build process.
#Updating is part of the development process, not compiling or packaging.
#Run this before a release, or any time you want to update the docs or the README.

#This script takes common snippets of information and updates or generates source info files from
#them.
# parse src/nsmd.cpp for API version, insert into /docs/src/api/index.adoc
# parse /CHANGELOG first line for package version, insert into /meson.build and /docs/src/index.adoc
# generate /README.md  (shares text with manual index)
# generate /docs/src/index.adoc (shares text with readme)
# generate manpages (need all kind of information)
# convert all .adoc files to html in /docs/  (This enables github to directly present this dir as website)
#
#We do _not_ change the copyright date in files license-headers.
#They only exist to mark to year of the fork. In the future dates might be removed completely.

set -e  #Stop script on errors
set -u  #Trace unset variables as an error.

#Change pwd to root dir
parent_path=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "$parent_path"/../..
[ -f "CHANGELOG" ] || exit 1 #assert correct dir

#Gather data
ROOT=$(pwd) #save for later
VERSION=$(head -n 1 "CHANGELOG")

_MAJORAPI=$(grep "define NSM_API_VERSION_MAJOR" "src/nsmd.cpp" | cut -d ' ' -f 3)
_MINORAPI=$(grep "define NSM_API_VERSION_MINOR" "src/nsmd.cpp" | cut -d ' ' -f 3)
APIVERSION=$_MAJORAPI"."$_MINORAPI

#Present data to confirm write-action
echo "Root: $ROOT"
echo "Version: $VERSION"
echo "API Version: $APIVERSION"
read -p "Is parsed data correct? Continue by writing files? [y|n] " -n 1 -r
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    echo
    echo "Abort"
    exit 1
fi
echo

#Package Version Number
cd "$ROOT/docs/src"
sed -i '/^\:revnumber.*/c\:revnumber: '$VERSION index.adoc #Find the revnumber line and replace with entire new line

#API Version Number
cd "$ROOT/docs/src/api"
sed -i '/^\:revnumber.*/c\:revnumber: '$APIVERSION index.adoc #Find the revnumber line and replace with entire new line


#Generate README.md
cd "$ROOT/docs/src"
cat "readme-00.md" "readme-01.md" "readme-02.md" > "$ROOT/README.md"


#Generate website and documentation with Asciidoctor
cd "$ROOT/docs"
mkdir -p "api"
asciidoctor src/index.adoc -o index.html
asciidoctor src/api/index.adoc -o api/index.html
