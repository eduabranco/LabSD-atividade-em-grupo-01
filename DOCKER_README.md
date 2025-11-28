# Docker Setup - Cliente Servidor Rust

Este projeto fornece containerização completa para a aplicação de compilação e execução remota de código Rust.

## Estrutura

```
.
├── docker-compose.yml          # Orquestração dos containers
├── server/
│   ├── Dockerfile             # Imagem do servidor
│   └── server_rust.c          # Código-fonte do servidor
├── client/
│   ├── Dockerfile             # Imagem do cliente
│   └── client_gui.c           # Código-fonte do cliente
└── DOCKER_README.md           # Este arquivo
```

## Pré-requisitos

- Docker
- Docker Compose
- Para usar a GUI do cliente: X11 Server (já vem com Linux, no Windows use WSL2 com VcXsrv)

## Como Executar

### 1. Iniciar os containers

```bash
docker-compose up --build
```

Este comando irá:
- Compilar a imagem do servidor
- Compilar a imagem do cliente
- Iniciar ambos os containers em uma rede compartilhada
- O servidor aguardará conexões na porta 51482
- O cliente conectará automaticamente ao servidor

### 2. Verificar status dos containers

```bash
docker-compose ps
```

### 3. Parar os containers

```bash
docker-compose down
```

### 4. Ver logs de um container específico

```bash
# Logs do servidor
docker-compose logs server

# Logs do cliente
docker-compose logs client

# Acompanhar logs em tempo real
docker-compose logs -f
```

## Comunicação entre Containers

- **Servidor**: Rodando na porta 51482 dentro da rede `compiler-network`
- **Cliente**: Conecta ao servidor usando o hostname `server` (resolvido automaticamente pelo Docker)
- Ambos estão na mesma rede Docker, permitindo comunicação direta pelo hostname

## Mudanças necessárias no cliente

Se estiver rodando o cliente GUI localmente contra um servidor em container, altere o IP de conexão:

- **IP Local**: `127.0.0.1` ou `localhost`
- **IP em Container**: `server` (nome do serviço Docker)
- **Porta**: `51482` (padrão)

## Personalizações

### Mudar a porta do servidor

Edite `docker-compose.yml`:

```yaml
server:
  ports:
    - "PORTA_HOST:51482"  # Mude PORTA_HOST para qualquer porta
```

### Recompilar apenas uma imagem

```bash
docker-compose build --no-cache server
docker-compose build --no-cache client
```

### Executar containers sem orquestração

**Servidor:**
```bash
docker build -t rust-server ./server
docker run -p 51482:51482 rust-server
```

**Cliente:**
```bash
docker build -t rust-client ./client
docker run -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix rust-client
```

## Troubleshooting

### Cliente GUI não aparece

- Verifique se o X11 está configurado corretamente: `echo $DISPLAY`
- No Windows (WSL2), inicie o VcXsrv antes
- Permita conexões X11: `xhost +local:`

### Conexão recusada entre containers

- Verifique se ambos estão na mesma rede: `docker network ls`
- Inspecione a rede: `docker network inspect <network_name>`

### Porta 51482 já em uso

Mude a porta em `docker-compose.yml` ou libere a porta:
```bash
sudo lsof -i :51482
sudo kill -9 <PID>
```

## Performance

- As imagens usam `debian:bookworm-slim` para minimizar tamanho
- Compilação é feita durante o build para evitar overhead em tempo de execução
- Ambos os containers compartilham a mesma rede bridge para latência mínima

## Segurança

Para uso em produção:

- Considere usar volumes nomeados em vez de bind mounts
- Implemente autenticação no servidor
- Use secrets do Docker para credenciais sensíveis
- Considere usar redes overlay se distribuir em múltiplas máquinas
