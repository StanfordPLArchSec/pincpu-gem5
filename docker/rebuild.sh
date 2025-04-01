#!/bin/bash

script_dir=$(dirname ${BASH_SOURCE[0]})
cd $script_dir

docker build -t hfi .
docker save hfi -o hfi.tar
apptainer build -f hfi.sif docker-archive://hfi.tar
