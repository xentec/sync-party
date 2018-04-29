#!/bin/bash

SP_DIR="$(dirname "$(readlink -fn "${BASH_SOURCE[0]}")")"

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

build() {
	for dir; do
	(
		set -e
		cd "$SP_DIR/$dir"
		mkdir -p build && cd build;
		cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$SP_DIR/bin"
		make
	)
	done
	
}

pkg_check cmake g++ avr-g++
build driver cortex
