#!/bin/bash

# Based on find-provides of rpm-mageia-setup, GPLv2+

export LC_ALL=C

for file in "$@"; do
  soname=$(objdump -p $file 2>/dev/null | awk '/SONAME/ {print $2}')
  if [ "$soname" != "" ]; then
    echo $soname
    objdump -p $f 2>/dev/null | awk '
      BEGIN { START=0 ; }
      /Version definitions:/ { START=1; }
      /^[0-9]/ && (START==1) { print $4; }
      /^$/ { START=0; }
    ' | \
      grep -v $soname | \
      while read symbol ; do
        echo "$soname($symbol)"
      done
  fi
done

