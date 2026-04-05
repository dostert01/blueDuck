# ─────────────────────────────────────────────────────────────────────────────
# Stage 1 — C++ build
#
# Ubuntu 24.04 ships GCC 13 and CMake 3.28 out of the box.
# Drogon is built from source so everything uses the same toolchain.
# ─────────────────────────────────────────────────────────────────────────────
FROM ubuntu:24.04 AS cpp-builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git ca-certificates \
        libjsoncpp-dev uuid-dev zlib1g-dev libssl-dev \
        libgit2-dev libpq-dev libcurl4-openssl-dev \
        libzip-dev libpugixml-dev nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Build and install Drogon
RUN git clone --depth 1 --branch v1.9.10 \
        https://github.com/drogonframework/drogon.git /tmp/drogon \
    && cd /tmp/drogon && git submodule update --init \
    && cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_CTL=OFF \
        -DBUILD_ORM=ON \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
    && cmake --build build -j"$(nproc)" \
    && cmake --install build \
    && rm -rf /tmp/drogon

WORKDIR /src
COPY CMakeLists.txt ./
COPY cmake/           cmake/
COPY interfaces/      interfaces/
COPY src/             src/
COPY plugins/         plugins/
COPY db/              db/

# libzip-dev ships a broken CMake config referencing tool binaries not in the
# package. Create stubs — we only need the library, not the CLI tools.
RUN for f in zipcmp zipmerge ziptool; do ln -sf /bin/true /usr/bin/$f; done

WORKDIR /build
RUN cmake /src \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTS=OFF \
    && cmake --build . -j"$(nproc)"


# ─────────────────────────────────────────────────────────────────────────────
# Stage 2 — Angular frontend build
# ─────────────────────────────────────────────────────────────────────────────
FROM node:20-alpine AS frontend-builder

WORKDIR /frontend

# Install dependencies first so this layer is cached independently of source
COPY frontend/package.json ./
# Use npm install so a lockfile is not required; produces one inside the layer
RUN npm install

COPY frontend/ ./
RUN npm run build


# ─────────────────────────────────────────────────────────────────────────────
# Stage 3 — Runtime image
#
# Same Ubuntu 24.04 base ensures ABI compatibility with the builder.
# Only runtime shared libraries are installed (no -dev headers).
# ─────────────────────────────────────────────────────────────────────────────
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        libgit2-1.7 \
        libpq5 \
        libcurl4t64 \
        libzip4t64 \
        libpugixml1v5 \
        libjsoncpp25 \
        libssl3t64 \
        libuuid1 \
        zlib1g \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Drogon runtime libraries from the builder
COPY --from=cpp-builder /usr/local/lib/libdrogon.so* /usr/local/lib/
COPY --from=cpp-builder /usr/local/lib/libtrantor.so* /usr/local/lib/
RUN ldconfig

# Create unprivileged user
RUN useradd --system --no-create-home --shell /sbin/nologin blueduck

# Application directories
RUN mkdir -p \
        /usr/lib/blueduck/plugins \
        /usr/share/blueduck/migrations \
        /var/lib/blueduck/www \
        /var/lib/blueduck/repos \
        /var/lib/blueduck/cache \
        /var/lib/blueduck/uploads \
        /etc/blueduck \
    && chown -R blueduck:blueduck \
        /var/lib/blueduck \
        /etc/blueduck

# Copy built artifacts from previous stages
COPY --from=cpp-builder  /build/bin/blueduck                      /usr/bin/blueduck
COPY --from=cpp-builder  /build/plugins/                          /usr/lib/blueduck/plugins/
COPY --from=cpp-builder  /src/db/migrations/                      /usr/share/blueduck/migrations/
COPY --from=frontend-builder \
     /frontend/dist/blueduck-frontend/browser/                    /var/lib/blueduck/www/

# Container-specific runtime config (DB host = "db", paths = container paths)
COPY docker/config.json   /etc/blueduck/config.json
COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

USER blueduck
WORKDIR /var/lib/blueduck

EXPOSE 8080

ENTRYPOINT ["/entrypoint.sh"]
