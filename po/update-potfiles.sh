#!/bin/sh

echo "# DO NOT EDIT.  This file is automatically generated."
echo "# List of source files which contain translatable strings."
echo "[encoding: UTF-8]"
files=`find .. \( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.ui' -o -name '*.schemas.in' -o -name "*.gschema.xml.in" -o -name '*.desktop.in.in' -o -name '*.extension.in.in' \) -printf "%P\n" | sort`
for f in $files; do
	case $f in
        build/*) ;;
	*.ui) echo "[type: gettext/glade]$f" ;;
	*) echo $f
	esac
done
