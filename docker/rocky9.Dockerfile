# X13cpp build/test image — Rocky Linux 9 (RHEL 9 ABI).
#
# PURPOSE: the current mainstream Linux target. Rocky 9's default compiler is
# GCC 11, with full C++17 support (and C++20 available, though the project
# targets C++17 for Rocky 8 compatibility). CMake in Rocky 9 AppStream is >= 3.26.
#
# Build:  docker build -f docker/rocky9.Dockerfile -t x13cpp-rocky9 .
# Use:    docker run --rm -v "$PWD":/work x13cpp-rocky9 <cmd>
FROM rockylinux:9

# gcc-c++      -> g++ 11 (C++17 + C++20)
# gcc-gfortran -> gfortran, to build the Fortran oracle for parity
# cmake        -> Rocky 9 AppStream ships CMake >= 3.26 (presets fully supported)
# make, git, python3 -> build driver, VCS, test scripts
# diffutils, which   -> used by CI / parity comparison steps
RUN dnf -y update && \
    dnf -y install \
        gcc-c++ \
        gcc-gfortran \
        make \
        cmake \
        python3 \
        python3-pip \
        git \
        diffutils \
        which \
        && dnf clean all && rm -rf /var/cache/dnf

RUN set -eux; \
    g++ --version; \
    gfortran --version; \
    cmake --version; \
    python3 --version

WORKDIR /work

# CI-friendly: default to an interactive shell; GitHub Actions overrides the
# command per step. Accepts an arbitrary command via `docker run ... <cmd>`.
CMD ["/bin/bash"]
