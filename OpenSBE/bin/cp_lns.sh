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
# File   : cp_lns.sh
# Author : Karen Arutyunov <karen@ipmce.ru>

usage="Usage: cp_lns.sh TARGET [LINK_NAME]"

if test "$#" -eq "0"; then
  echo "cp_lns.sh: missing TARGET argument" 1>&2
  echo "$usage" 1>&2
  exit 1
fi

if test "$#" -eq "1"; then
# Link has same name as target
  exit 0
fi

target=$1
shift

if test "$#" -eq "1"; then

# Usage: cp_lns.sh TARGET LINK_NAME

  link=$1

  if test -d $link; then
    cmd="cp -r -p"
  else
    cmd="cp -p"
  fi

  link_dir=`dirname $link`
  link_file_name=`echo $link|sed -e "s%^.*/\(.*\)$%\1%"`

  cd $link_dir
  $cmd "$target" "$link_file_name"

  exit 0
fi

# Usage: cp_lns.sh TARGET... DIRECTORY_NAME

echo "cp_lns.sh: usage 'cp_lns.sh TARGET... \
DIRECTORY_NAME' is not implemented yet" 1>&2

exit 1
