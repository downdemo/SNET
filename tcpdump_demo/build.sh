#!/bin/bash -ex
root=`git rev-parse --show-toplevel`
demo_dir=${root}/tcpdump_demo
debug_dir=${root}/debug
rm -rf ${debug_dir}
mkdir -p ${debug_dir}
g++ -std=c++20 -g ${demo_dir}/server.cpp -o ${debug_dir}/server
g++ -std=c++20 -g ${demo_dir}/client.cpp -o ${debug_dir}/client
