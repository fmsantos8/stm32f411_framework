FROM ubuntu:24.04

ENV LANG=C.UTF-8 LC_ALL=C.UTF-8

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential ca-certificates curl git python3 python3-serial xz-utils \
    && rm -rf /var/lib/apt/lists/*

# The official toolchain archive below targets a Linux x86_64 host.
RUN test "$(dpkg --print-architecture)" = amd64

ARG ARM_GNU_VERSION=14.3.rel1
ARG ARM_GNU_SHA256=8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd
RUN curl -fSL --retry 3 \
        "https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_GNU_VERSION}/binrel/arm-gnu-toolchain-${ARM_GNU_VERSION}-x86_64-arm-none-eabi.tar.xz" \
        -o /tmp/arm-gnu.tar.xz \
    && echo "${ARM_GNU_SHA256}  /tmp/arm-gnu.tar.xz" | sha256sum -c - \
    && mkdir /opt/arm-gnu-toolchain \
    && tar -xJf /tmp/arm-gnu.tar.xz -C /opt/arm-gnu-toolchain --strip-components=1 \
    && rm /tmp/arm-gnu.tar.xz

ARG CMAKE_VERSION=3.31.8
ARG CMAKE_SHA256=630615d8e98ac33eba7fbe472626dff5c899c85af3c024585ae109166a6909d0
RUN curl -fSL --retry 3 \
        "https://cmake.org/files/v3.31/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" \
        -o /tmp/cmake.tar.gz \
    && echo "${CMAKE_SHA256}  /tmp/cmake.tar.gz" | sha256sum -c - \
    && mkdir /opt/cmake \
    && tar -xzf /tmp/cmake.tar.gz -C /opt/cmake --strip-components=1 \
    && rm /tmp/cmake.tar.gz

ARG NINJA_VERSION=1.13.1
ARG NINJA_SHA256=f0055ad0369bf2e372955ba55128d000cfcc21777057806015b45e4accbebf23
RUN curl -fSL --retry 3 \
        "https://github.com/ninja-build/ninja/archive/refs/tags/v${NINJA_VERSION}.tar.gz" \
        -o /tmp/ninja.tar.gz \
    && echo "${NINJA_SHA256}  /tmp/ninja.tar.gz" | sha256sum -c - \
    && mkdir /tmp/ninja \
    && tar -xzf /tmp/ninja.tar.gz -C /tmp/ninja --strip-components=1 \
    && cd /tmp/ninja \
    && python3 configure.py --bootstrap \
    && install -m 755 ninja /usr/local/bin/ninja \
    && rm -rf /tmp/ninja /tmp/ninja.tar.gz

COPY tools/install_stm32f4_sdk.py /tmp/framework/tools/install_stm32f4_sdk.py
RUN python3 /tmp/framework/tools/install_stm32f4_sdk.py /opt/STM32CubeF4 \
    && rm -rf /tmp/framework

ENV STM32F4_SDK_DIR=/opt/STM32CubeF4
ENV PATH=/opt/arm-gnu-toolchain/bin:/opt/cmake/bin:${PATH}

CMD ["/bin/bash"]
