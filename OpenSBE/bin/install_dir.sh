#!/bin/sh
# Open Software Building Environment (OpenSBE, OSBE)
# Copyright (C) 2001-2002 Boris Kolpackov
# Copyright (C) 2001-2004 Pavel Gubin, Karen Arutyunov
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
#
# File   : install_dir.sh
# Author : Karen Arutyunov <karen@ipmce.ru>

cpprog="${CPPROG-cp}"
mkdirprog="${MKDIRPROG-mkdir}"
findprog="${FINDPROG-find}"
rmprog="${RMPROG-rm}"

srcdir="$1"
destdir="$2"
names=`echo "$3" | sed -e "s/\*\|\?\|\[\|\]/\\\\\&/g"`
if [ $# = 2 ]; then
  shift 2
else
  shift 3
fi

usage="Usage: install_dir.sh <source-dir> <dest-dir> \
[<name-patterns> [<extra-find-options>]]"

if test -z "$srcdir"; then
  echo "install_dir.sh: no source directory specified"
  echo "$usage"
  exit 1
fi

if test -z "$destdir"; then
  echo "install_dir.sh: no destination directory specified"
  echo "$usage"
  exit 1
fi

cd "$srcdir"

if test $? -ne 0; then
  exit 1
fi

file_list=""

if test -n "$names"; then
  for name in $names; do

    eval name="$name"

    if [ $# != 0 ]; then
      files=\
`$findprog . \( "$@" \) -a -name "$name" -a \( -type l -o -type f \)`
    else
      files=\
`$findprog . -name "$name" -a \( -type l -o -type f \)`
    fi

    echo "$name: $files"

    if test $? -ne 0; then
      exit 1
    fi

    file_list="$file_list $files"
  done
else
    if [ $# != 0 ]; then
      file_list=`$findprog . \( "$@" \) -a \( -type l -o -type f \)`
    else
      file_list=`$findprog . \( -type l -o -type f \)`
    fi
fi

for file in $file_list; do
  dst="$destdir/$file"
  echo "  $srcdir/$file -> $dst"

  dstdir=`echo $dst | sed -e 's,[^/]*$,,;s,/$,,;s,^$,.,'`
  $mkdirprog -p "$dstdir"

  if test $? -ne 0; then
    exit 1
  fi

  $rmprog -f "$dst"
  $cpprog -f "$file" "$dst"

  if test $? -ne 0; then
    exit 1
  fi

done
