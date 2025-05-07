#!/bin/sh

# This script generates a C header file from the socat binary
# If socat.h already exists, it will not be generated again
# If socat.h does not exist:
#	- it will download the socat binary from github.com/ernw/static-toolbox
#	- then hexdump the binary to socat.h

generate_socat_h() {
	# if socat file do not exist, download it
	if [ ! -f socat ]; then
		echo "socat binary not found."
		echo "Download static socat binary from github.com/ernw/static-toolbox"
		wget https://github.com/ernw/static-toolbox/releases/download/socat-v1.7.4.4/socat-1.7.4.4-x86_64 -O socat
		if [ $? -ne 0 ]; then
			echo "Failed to download socat binary."
			exit 1
		fi
		echo "Download complete."
	fi
	echo "Hexdumping socat to socat.h"
	xxd -i socat > socat.h
}

# Check if socat.h exists
if [ ! -f ./socat.h ]; then
	echo "socat.h not found, generating it..."
	generate_socat_h
else
	echo "socat.h already exists, skipping generation."
fi