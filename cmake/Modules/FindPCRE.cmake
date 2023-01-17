# - Find PCRE library
# -*- cmake -*-
# 
# Find the Perl-Compatible Regular Expressions (PCRE) includes and
# library
#
# Sets the following variables:
#
#  PCRE_INCLUDE_DIR  - where to find headers
#  PCRE_LIBRARIES    - List of libraries to link against
#  PCRE_FOUND        - True if PCRE found.
#

FIND_PACKAGE(PkgConfig)

PKG_CHECK_MODULES(PCRE REQUIRED libpcre)