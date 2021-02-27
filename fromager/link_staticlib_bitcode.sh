#!/bin/sh
set -e

dir=$PWD/link_staticlib_bitcode_$$
input="$PWD/$1"
output="$PWD/$2"

mkdir -p "$dir"
(cd "$dir" && ar x "$input")
llvm-link-9 "$dir"/* -o "$output"
rm -rf "$dir"
