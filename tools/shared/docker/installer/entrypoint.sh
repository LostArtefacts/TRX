#!/bin/bash
set -x
set -e

if [[ -z "$1" || ( "$1" != "1" && "$1" != "2" && "$1" != "trx" ) ]]; then
    echo "Error: You must supply '1', '2', or 'trx' as an argument."
    exit 1
fi
TR_VERSION=$1

cd /app/tools/installer/

export DOTNET_CLI_HOME="/tmp/DOTNET_CLI_HOME"

shopt -s globstar
rm -rf **/bin **/obj **/out/*
if [[ "$TR_VERSION" == "trx" ]]; then
    dotnet publish TRX_Installer -c Release -o out
else
    dotnet publish TR${TR_VERSION}X_Installer -c Release -o out
fi
