
# Stage 1: Build C++ backend
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libasio-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j2

# Stage 2: Run backend
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy compiled executable
COPY --from=builder /app/build/inventory_server ./inventory_server

# Copy JSON storage directory
COPY --from=builder /app/storage ./storage

# Render default port
ENV PORT=10000

EXPOSE 10000

# Start Crow server
CMD ["./inventory_server"]
