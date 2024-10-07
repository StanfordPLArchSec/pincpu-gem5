FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update
RUN apt-get install -y build-essential scons m4 python3-dev python3-pip python3-six python3-setuptools python3-wheel git zlib1g-dev protobuf-compiler libprotobuf-dev libgoogle-perftools-dev libboost-all-dev libhdf5-dev libpng-dev pkg-config swig
RUN pip3 install future six
