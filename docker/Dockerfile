FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV CMAKE_GENERATOR=Ninja
RUN apt-get update -y
RUN apt-get install -y build-essential make git clang libpng-dev libuv1-dev m4 zlib1g zlib1g-dev libprotobuf-dev protobuf-compiler libprotoc-dev libgoogle-perftools-dev libboost-all-dev pkg-config cpuset cpufrequtils xvfb gnuplot npm ca-certificates curl gnupg lsb-release libssl-dev apache2-utils jq libseccomp-dev gawk wget
RUN apt-get install -y python3-dev python3-venv
RUN apt-get install -y python3-pip python-is-python3
RUN apt-get install -y python3-six scons
RUN apt-get install -y cmake emacs-nox
RUN apt-get install -y gdb lldb
RUN apt-get install -y strace

