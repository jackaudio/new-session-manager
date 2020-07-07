#!/bin/sh

#The documentation is built statically and does not belong to the normal build process.
#Updating is part of the development process, not compiling or packaging.
#Run this before a release, or any time you want to update the docs or the README.

#This script takes common snippets of information and updates or generates source info files from
#them.
# parse src/nsmd.cpp for API version, insert into /docs/src/api/index.adoc
# parse src/nsmd.cpp for package version, insert into /meson.build and /docs/src/index.adoc
# generate /README.md  (shares text with manual index)
# generate manpages
# convert all .adoc files to html in /docs/  (This enables github to directly present this dir as website)
#
#We do _not_ change the copyright date in files license-headers.
#They only exist to mark to year of the fork. In the future dates might be removed completely.

set -e  #Stop script on errors
set -u  #Trace unset variables as an error.

#Change pwd to root dir
parent_path=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "$parent_path"/../..
[ -f "CHANGELOG" ] || ( echo "not in the root dir"; exit 1 ) #assert correct dir
[ -f "build/nsmd" ] || ( echo "no build/ dir with binaries"; exit 1 ) #assert build was made, for manpages

#Gather data
ROOT=$(pwd) #save for later
VERSION=$(grep "define VERSION_STRING" "src/nsmd.cpp" | cut -d ' ' -f 3) #Get version as "1.4" string
VERSION="${VERSION%\"}"  #Remove "
VERSION="${VERSION#\"}"  #Remove "

_MAJORAPI=$(grep "define NSM_API_VERSION_MAJOR" "src/nsmd.cpp" | cut -d ' ' -f 3)
_MINORAPI=$(grep "define NSM_API_VERSION_MINOR" "src/nsmd.cpp" | cut -d ' ' -f 3)
APIVERSION=$_MAJORAPI"."$_MINORAPI

#Present data to confirm write-action
echo "Root: $ROOT"
echo "Version: $VERSION"
echo "API Version: $APIVERSION"
echo "Please make sure that your meson build dir is up to date for manpage generation"
read -p "Is parsed data correct? Continue by writing files? [y|n] " -n 1 -r
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    echo
    echo "Abort"
    exit 1
fi
echo
echo

echo "Update meson.build version number"
cd "$ROOT"
sed -i "/^version :.*/c\version : '$VERSION'," meson.build #Find the version line and replace with entire new line

echo "Update docs to programs version number"
cd "$ROOT/docs/src"
sed -i '/^\:revnumber.*/c\:revnumber: '$VERSION index.adoc #Find the revnumber line and replace with entire new line

echo "Update API document to API version number"
cd "$ROOT/docs/src/api"
sed -i '/^\:revnumber.*/c\:revnumber: '$APIVERSION index.adoc #Find the revnumber line and replace with entire new line


echo "Generate README from snippets"
cd "$ROOT/docs/src"
cat "readme-00.md" "readme-01.md" "readme-02.md" > "$ROOT/README.md"


echo "Generate website and documentation with Asciidoctor using README snippets"
echo "  We generate directly into docs/ and not into e.g. docs/out because github can read docs/ directly."
cd "$ROOT/docs/"
mkdir -p "api"
asciidoctor src/index.adoc -o index.html
asciidoctor src/api/index.adoc -o api/index.html


echo "Generate all manpages"
cd "$ROOT/docs/src" #We tested earlier that a build-dir exists

help2man ../../build/nsmd --version-string="nsmd Version $VERSION" --no-info --include manpage-common.h2m > nsmd.1
help2man ../../build/nsm-legacy-gui --version-string="nsm-legacy-gui Version $VERSION" --no-info --include manpage-common.h2m > nsm-legacy-gui.1
help2man ../../build/nsm-legacy-gui --version-string="nsm-legacy-gui Version $VERSION" --no-info --include manpage-common.h2m > non-session-manager.1
help2man ../../build/nsm-proxy  --version-string="nsm-proxy Version $VERSION" --no-info --include manpage-common.h2m > nsm-proxy.1
help2man ../../build/nsm-proxy-gui  --version-string="nsm-proxy-gui Version $VERSION" --no-info --include manpage-common.h2m > nsm-proxy-gui.1
help2man ../../build/jackpatch  --version-string="jackpatch Version $VERSION" --no-info --include manpage-common.h2m > jackpatch.1

echo
echo "Finished. You need to commit your changes to git manually"
