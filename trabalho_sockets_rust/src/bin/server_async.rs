/* 
   Arquivo: server_async.rs 
   No Cargo.toml adicione: tokio = { version = "1", features = ["full"] }
*/

use tokio::net::TcpListener;
use tokio::io::{AsyncReadExt, AsyncWriteExt};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let listener = TcpListener::bind("0.0.0.0:51482").await?;
    println!("Servidor Async (Epoll) rodando na porta 51482...");

    loop {
        let (mut socket, _) = listener.accept().await?;

        // Tokio spawn cria uma "green thread" (tarefa leve), não uma thread do SO.
        tokio::spawn(async move {
            let mut buffer = [0; 1024];

            // Em um loop para manter a conexão viva se necessário (comportamento do epoll example)
            loop {
                let n = match socket.read(&mut buffer).await {
                    Ok(n) if n == 0 => return, // Conexão fechada
                    Ok(n) => n,
                    Err(e) => {
                        eprintln!("Erro socket: {}", e);
                        return;
                    }
                };

                // Responde
                if let Err(e) = socket.write_all(b"I got your message").await {
                    eprintln!("Erro ao escrever: {}", e);
                    return;
                }
                
                // No exemplo original C, ele fecha após responder. 
                // Aqui retornamos, o que fecha o socket (drop).
                return; 
            }
        });
    }
}