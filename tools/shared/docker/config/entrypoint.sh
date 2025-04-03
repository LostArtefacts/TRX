#!/bin/bash
set -x
set -e

if [[ -z "$1" || ( "$1" != "1" && "$1" != "2" ) ]]; then
    echo "Error: You must supply '1' or '2' as an argument to decide which game to build."
    exit 1
fi
TR_VERSION=$1

export DOTNET_CLI_HOME="/tmp/DOTNET_CLI_HOME"
echo $HOME
shopt -s globstar

cd /app/tools/config/
rm -rf **/bin **/obj **/out/*
dotnet publish TR${TR_VERSION}X_ConfigTool -c Release -o out
