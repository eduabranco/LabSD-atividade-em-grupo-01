FROM debian:bookworm-slim

# Install build essentials and dependencies for GTK3
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    curl \
    git \
    libgtk-3-dev \
    pkg-config \
    x11-apps \
    libxcb-render0-dev \
    libxcb-shape0-dev \
    libxcb-xfixes0-dev \
    libxkbcommon-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Rust
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

# Set working directory
WORKDIR /app/client

# Copy client source code
COPY client_gui.c .

# Compile the client
RUN gcc client_gui.c -o client_gui `pkg-config --cflags --libs gtk+-3.0`

# Set environment for X11 display
ENV DISPLAY=:0

# Default command - runs the client
CMD ["./client_gui"]
