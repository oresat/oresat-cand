#!/bin/bash -e
# While the '-no path "..."' could be done with a .clang-format-ignore file,
# a .clang-format-ignore file will only be used by clang-format v18.0.1 or newer.
#
# This method supports older clang-format versions.
find . -wholename "*.[c/h]" -not -path "./src/canopenlinux/CANopenLinux/*" -not -path "./build/*" | xargs clang-format -i $@
