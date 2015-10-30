#!/bin/bash

PROJECT="lximage-qt"
version="$1"
prefix=$PROJECT-$version
shift

if [[ -z $version ]]; then
	>&2 echo "USAGE: $0 <tag>"
	exit 1
fi

mkdir -p "dist/$version"
echo "Creating $prefix.tar.gz"
git archive -9 --format tar.gz $version --prefix="$prefix/" > "dist/$version/$prefix.tar.gz"
gpg --armor --detach-sign "dist/$version/$prefix.tar.gz"
echo "Creating $prefix.tar.xz"
git archive -9 --format tar.xz $version --prefix="$prefix/" > "dist/$version/$prefix.tar.xz"
gpg --armor --detach-sign "dist/$version/$prefix.tar.xz"
cd "dist/$version"

sha1sum --tag *.tar.gz *.tar.xz >> CHECKSUMS
sha256sum --tag *.tar.gz *.tar.xz >> CHECKSUMS

cd ..
echo "Uploading to lxqt.org..."

scp -r "$version" "downloads.lxqt.org:/srv/downloads.lxqt.org/$PROJECT/"
