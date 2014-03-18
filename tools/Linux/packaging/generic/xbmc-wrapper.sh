#!/bin/bash

script="$(readlink -f "$0")"
scriptdir="$(dirname "$script")"
depdir="$(readlink -f "$scriptdir/data/lib")"
xbmcdir="$(readlink -f "$scriptdir/data")"

export LD_LIBRARY_PATH="$depdir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export prefix="$xbmcdir"
export XBMC_HOME="$xbmcdir/share/xbmc"
export XBMC_BIN_HOME="$xbmcdir/lib/xbmc"
export PYTHONHOME="$xbmcdir"
export PYTHONOPTIMIZE=1
export PATH="$xbmcdir/bin:$PATH"

exec "$xbmcdir/bin/$(basename "$script")" "$@"

