#!/bin/bash
set -x
set -e

if ! getent passwd "$(id -u)" >/dev/null; then
    echo "trx:x:$(id -u):$(id -g)::/app:/sbin/nologin" >>/etc/passwd
fi

cd /app/tools/installer/

export DOTNET_CLI_HOME="/tmp/DOTNET_CLI_HOME"

shopt -s globstar
rm -rf **/bin **/obj **/out/*
dotnet publish TRX_Installer -c Release -o out
