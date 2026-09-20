FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# ============================================================
# System dependencies
# ============================================================

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libssl-dev \
    libpq-dev \
    libdrogon-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    libsqlite3-dev \
    default-libmysqlclient-dev \
    libbrotli-dev \
    libhiredis-dev \
    libyaml-cpp-dev \
    && rm -rf /var/lib/apt/lists/*

# ============================================================
# Drogon Ubuntu package compatibility
# ============================================================

RUN ln -s /usr/lib/x86_64-linux-gnu/libmysqlclient.so \
    /usr/lib/x86_64-linux-gnu/libmariadbclient.so

# ============================================================
# Build libpqxx 7.8.1
# ============================================================

WORKDIR /tmp

RUN git clone --depth 1 --branch 7.8.1 \
    https://github.com/jtv/libpqxx.git \
    libpqxx

RUN cd /tmp/libpqxx \
    && mkdir build \
    && cd build \
    && cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_STANDARD=20 \
        -DCMAKE_CXX_STANDARD_REQUIRED=ON \
        -DBUILD_TEST=OFF \
        -DSKIP_BUILD_TEST=ON \
    && cmake --build . -j$(nproc) \
    && cmake --install .

# ============================================================
# KaviyaMart
# ============================================================

WORKDIR /app

COPY . .

RUN mkdir -p build \
    && cd build \
    && cmake .. \
    && cmake --build . --config Release

# ============================================================
# Runtime environment
# ============================================================

ENV PORT=8080
ENV FRONTEND_DIR=/app/frontend
ENV UPLOAD_DIR=/app/frontend/uploads

EXPOSE 8080

CMD ["/app/build/KaviyaMart"]