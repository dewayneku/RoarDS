#!/bin/bash

if [[ ! -x RoarDS.exe ]]; then
	echo "Run this script from the directory you built RoarDS."
	exit 1
fi

mkdir -p dist

for lib in $(ldd RoarDS.exe | grep mingw | sed "s/.*=> //" | sed "s/(.*)//"); do
	cp "${lib}" dist
done

cp RoarDS.exe dist
windeployqt dist
