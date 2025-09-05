#!/bin/bash

# Generate lib for Linux to x86_64 architecture

mkdir -p dist/lib/linux/x86_64

cd dist/lib/linux/x86_64

cmake ../../../.. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DRCNET_BUILD_EXAMPLE=OFF
cmake --build .

cmake ../../../.. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DRCNET_BUILD_EXAMPLE=OFF
cmake --build .

echo -e "\033[32m \n Generate lib RCNETCore for Linux x86_64 to Release/Debug generated successfully , go to the dist/lib/linux/x86_64 directory... \n\033[0m"