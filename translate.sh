#!/bin/sh

for file in *.{h,c}; do
	[ -f ${file}.de ] && continue

	./translate.lua ${file} ${file}.de
done

for file in *.de; do
	cp -f ${file} ${file%.de}
done
