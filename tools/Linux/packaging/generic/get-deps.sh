#!/bin/bash

# Based on find-requires of rpm-mageia-setup, GPLv2+

export LC_ALL=C

for f in $@; do
    objdump -p $f | awk 'BEGIN { START=0; LIBNAME=""; }
        /^$/ { START=0; }
        /^Dynamic Section:$/ { START=1; }
        (START==1) && /NEEDED/ {
                print $2 ;
        }
        /^Version References:$/ { START=2; }
        (START==2) && /required from/ {
            sub(/:/, "", $3);
            LIBNAME=$3;
        }
        (START==2) && (LIBNAME!="") && ($4!="") {
            print LIBNAME "(" $4 ")";
        }
    '
done | sort -u | grep -v 'libsafe|libfakeroot'


