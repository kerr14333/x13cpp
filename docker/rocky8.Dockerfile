# X13cpp build/test image — Rocky Linux 8 (RHEL 8 ABI).
#
# PURPOSE: this image pins the OLDEST toolchain we support. Rocky 8's default
# compiler is GCC 8.5, whose C++ standard ceiling is C++17. We deliberately do
# NOT enable gcc-toolset (which would provide a newer GCC) — the whole point of
# this image is to prove the port builds and passes parity with a stock GCC 8.5
# / C++17 toolchain, so the codebase can never accidentally require C++20.
#
# Build:  docker build -f docker/rocky8.Dockerfile -t x13cpp-rocky8 .
# Use:    docker run --rm -v "$PWD":/work x13cpp-rocky8 <cmd>
FROM rockylinux:8

# gcc-c++      -> g++ 8.5 (C++17 ceiling; do NOT install gcc-toolset-*)
# gcc-gfortran -> gfortran, to build the Fortran oracle for parity
# cmake        -> Rocky 8 AppStream ships CMake >= 3.20 (supports presets, which
#                 need >= 3.19). If a future preset needs a newer CMake than
#                 AppStream provides, install cmake3 from EPEL instead:
#                   dnf install -y epel-release && dnf install -y cmake3
#                 and invoke `cmake3`. We document that here rather than pulling
#                 EPEL by default, to keep the image close to a stock RHEL 8.
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

# Fail the build loudly if the toolchain is not what we expect.
RUN set -eux; \
    g++ --version; \
    gfortran --version; \
    cmake --version; \
    python3 --version; \
    g++ -dumpversion | grep -q '^8\.' || (echo "expected GCC 8.x on Rocky 8" && exit 1)

WORKDIR /work

# CI-friendly: default to an interactive shell; GitHub Actions overrides the
# command per step. Accepts an arbitrary command via `docker run ... <cmd>`.
CMD ["/bin/bash"]
