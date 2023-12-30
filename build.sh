#!/bin/bash
cmd="msbuild.exe clicker/clicker/clicker.vcxproj /p:Configuration=Release /p:Platform=x64"
echo ${cmd}
eval ${cmd}