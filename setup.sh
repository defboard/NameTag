#! /usr/bin/env bash

# Secure boot signing key
fname=secure_boot_signing_key.esp32c3.pem
if [[ -e "$fname" ]]; then
    echo "$fname: Exists"
else
    echo "$fname: Creating"
    ./idf.sh secure-generate-signing-key --version 2 "$fname"
fi
