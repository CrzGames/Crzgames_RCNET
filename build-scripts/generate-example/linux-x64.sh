#!/bin/bash

# Generate lib and executable for Linux to x86_64 architecture

mkdir -p dist/project-example/linux/x86_64

cd dist/project-example/linux/x86_64

cmake ../../../.. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build .

echo -e "\033[32m \n Generate library and executable RCNETCore for Linux x86_64 to Debug generated successfully , go to the dist/lib/linux/x86_64 directory... \n\033[0m"
