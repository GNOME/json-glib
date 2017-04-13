#!/bin/sh
test -n "$srcdir" || srcdir=$(dirname "$0")
test -n "$srcdir" || srcdir=.

cd $srcdir

PROJECT=json-glib
VERSION=$(git describe --abbrev=0)

NAME="${PROJECT}-${VERSION}"

rm -f "${NAME}.tar"
rm -f "${NAME}.tar.xz"

echo "Creating git tree archive…"
git archive --prefix="${NAME}/" --format=tar HEAD > ${NAME}.tar

echo "Compressing archive…"
xz -f "${NAME}.tar"
