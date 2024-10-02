#!/bin/bash
set -x
set -e

export DOTNET_CLI_HOME="/tmp/DOTNET_CLI_HOME"
echo $HOME
shopt -s globstar

# Build the common lib DLL
cd /app/tools/config/
rm -rf **/bin **/obj
dotnet restore -p:EnableWindowsTargeting=true
dotnet publish -c Release -p:EnableWindowsTargeting=true

# Build the main executable
cd /app/tools/tr1/config/
rm -rf **/bin **/obj **/out/*
dotnet restore
dotnet publish -c Release -o out
