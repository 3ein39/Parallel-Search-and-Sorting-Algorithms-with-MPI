#!/bin/bash
# Get per-core timing information
export TIMEFORMAT="%R %U %S"
time "$@"
