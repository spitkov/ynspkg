#!/bin/bash

# Exit on error
set -e

CURL_VERSION="8.6.0"
BUILD_DIR="$(pwd)/deps"
INSTALL_DIR="$(pwd)/deps/install"

# Create directories
mkdir -p "$BUILD_DIR"
mkdir -p "$INSTALL_DIR"

# Download and extract curl
cd "$BUILD_DIR"
wget "https://curl.se/download/curl-${CURL_VERSION}.tar.gz"
tar xf "curl-${CURL_VERSION}.tar.gz"
cd "curl-${CURL_VERSION}"

# Configure with minimal features
./configure \
    --prefix="$INSTALL_DIR" \
    --disable-shared \
    --enable-static \
    --disable-ldap \
    --disable-ldaps \
    --disable-rtsp \
    --disable-dict \
    --disable-telnet \
    --disable-tftp \
    --disable-pop3 \
    --disable-imap \
    --disable-smtp \
    --disable-gopher \
    --disable-smb \
    --disable-mqtt \
    --without-librtmp \
    --without-libidn2 \
    --without-libpsl \
    --without-nghttp2 \
    --without-libssh2 \
    --without-zstd \
    --without-brotli \
    --without-gssapi \
    --with-openssl \
    CFLAGS="-fPIC"

# Build and install
make -j$(nproc)
make install 