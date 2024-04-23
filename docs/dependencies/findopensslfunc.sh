#!/bin/bash
# Find all .dylib files in any openssl directory and grep for EVP_MD_size
find /opt/homebrew/opt/ -type d -name "openssl*" -exec find {} -type f -name "*.dylib" \; | while read lib; do
    echo "Checking $lib for EVP_MD_size:"
    nm "$lib" | grep "EVP_MD_size" || echo "Not found in $lib"
done

