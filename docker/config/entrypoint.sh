#!/bin/bash
set -x
set -e

cd /app/tools/config/
echo $HOME

export DOTNET_CLI_HOME="/tmp/DOTNET_CLI_HOME"

shopt -s globstar
rm -rf **/bin **/obj **/out/*
dotnet restore
dotnet publish -c Release -o out
