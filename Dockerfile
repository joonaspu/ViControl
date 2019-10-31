FROM ubuntu:18.04

# Install dependencies
RUN apt update && apt upgrade && apt install -y git && \
    apt install -y  autoconf \
                    automake \
                    autopoint \
                    bash \
                    bison \
                    bzip2 \
                    flex \
                    g++ \
                    g++-multilib \
                    gettext \
                    git \
                    gperf \
                    intltool \
                    libc6-dev-i386 \
                    libgdk-pixbuf2.0-dev \
                    libltdl-dev \
                    libssl-dev \
                    libtool-bin \
                    libxml-parser-perl \
                    lzip \
                    make \
                    openssl \
                    p7zip-full \
                    patch \
                    perl \
                    pkg-config \
                    python \
                    ruby \
                    sed \
                    unzip \
                    wget \
                    xz-utils

WORKDIR /
RUN git clone https://github.com/mxe/mxe.git

# Build MXE
WORKDIR /mxe
RUN make cc protobuf
ENV PATH="/mxe/usr/bin:${PATH}"

# Add env variable for the built protoc
ENV MXE_PROTOC="/mxe/usr/x86_64-pc-linux-gnu/bin/protoc"

VOLUME /build
WORKDIR /build

CMD ["make", "windows"]