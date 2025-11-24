#!/bin/bash
set -e

cd /workspace/trabalho_sockets_rust

echo "Starting compiler server..."
cargo run --release --bin compiler_server &
SERVER_PID=$!

sleep 2

echo "Starting client GUI..."
cargo run --release --bin client_gui &
CLIENT_PID=$!

# Wait for both processes
wait $SERVER_PID $CLIENT_PID