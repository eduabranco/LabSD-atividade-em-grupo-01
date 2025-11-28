#!/bin/bash

# Script helper para facilitar o uso do projeto com Docker

set -e

USAGE="
Uso: ./docker-helper.sh [comando]

Comandos disponíveis:
  start          - Inicia todos os containers (docker-compose up)
  stop           - Para todos os containers
  build          - Recompila as imagens sem cache
  logs           - Mostra logs em tempo real
  status         - Verifica status dos containers
  server-logs    - Mostra apenas logs do servidor
  client-logs    - Mostra apenas logs do cliente
  clean          - Remove containers e imagens
  help           - Mostra esta mensagem
"

case \"${1:-help}\" in
  start)
    echo \"🚀 Iniciando containers...\"
    docker-compose up --build
    ;;
  
  stop)
    echo \"⛔ Parando containers...\"
    docker-compose down
    ;;
  
  build)
    echo \"🔨 Recompilando imagens...\"
    docker-compose build --no-cache
    ;;
  
  logs)
    echo \"📋 Mostrando logs em tempo real...\"
    docker-compose logs -f
    ;;
  
  status)
    echo \"📊 Status dos containers:\"
    docker-compose ps
    ;;
  
  server-logs)
    echo \"📋 Logs do servidor:\"
    docker-compose logs -f server
    ;;
  
  client-logs)
    echo \"📋 Logs do cliente:\"
    docker-compose logs -f client
    ;;
  
  clean)
    echo \"🧹 Limpando containers e imagens...\"
    docker-compose down -v
    docker rmi rust_compiler_server rust_compiler_client 2>/dev/null || true
    echo \"✅ Limpeza concluída\"
    ;;
  
  help|*)
    echo \"$USAGE\"
    ;;
esac
