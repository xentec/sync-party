#!/bin/bash

die() {
	echo "$@"
	exit 1
}

pkg_check() {
	local fail=0
	for cmd; do
		
		if ! command -v "$cmd" > /dev/null; then 
			echo "$cmd is missing"
			fail=1
		fi
	done
	[ $fail -eq 1 ] && die "Please install the missing binaries"
}


pkg_check g++ cmake

mkdir -p build && cd build
cmake ..
make
