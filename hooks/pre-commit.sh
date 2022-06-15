#!/bin/sh

# Prefer gmake over make to get GNU make on non-GNU userland systems if present
if command -v gmake 2>/dev/null; then
    make_cmd=gmake
else
    make_cmd=make
fi

$make_cmd syntax-check
