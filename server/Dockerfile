FROM debian:bookworm-slim

# Install build essentials and dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    curl \
    git \
    && rm -rf /var/lib/apt/lists/*

# Install Rust
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

# Set working directory
WORKDIR /app/server

# Copy server source code
COPY server_rust.c .

# Compile the server
RUN gcc server_rust.c -o server_rust

# Expose the port
EXPOSE 51482

# Default command - runs the server on port 51482
CMD ["./server_rust", "51482"]
