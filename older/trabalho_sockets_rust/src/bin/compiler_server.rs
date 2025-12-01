/* 
   Arquivo: compiler_server.rs
   Dependências: tokio = { version = "1", features = ["full"] }
   Dependências: uuid = { version = "1", features = ["v4"] } (Para nomes de arquivos únicos)
*/

use tokio::net::TcpListener;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use std::process::Command;
use std::fs;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let listener = TcpListener::bind("0.0.0.0:51482").await?;
    println!("Servidor de Compilação (C) rodando na porta 51482...");

    loop {
        let (mut socket, _) = listener.accept().await?;

        tokio::spawn(async move {
            let mut buffer = [0; 4096]; // Buffer maior para código fonte
            let n = match socket.read(&mut buffer).await {
                Ok(n) if n == 0 => return,
                Ok(n) => n,
                Err(_) => return,
            };

            let source_code = String::from_utf8_lossy(&buffer[..n]);
            
            // Gera nome único para evitar colisão entre clientes
            let id = uuid::Uuid::new_v4(); 
            let filename = format!("/tmp/prog_{}.c", id);
            let output_bin = format!("/tmp/prog_{}", id);

            // 1. Salva o código em disco
            if let Err(_) = fs::write(&filename, source_code.as_bytes()) {
                let _ = socket.write_all(b"Erro interno: Falha ao salvar arquivo").await;
                return;
            }

            // 2. Compila (gcc)
            let compile_output = Command::new("gcc")
                .arg(&filename)
                .arg("-o")
                .arg(&output_bin)
                .output();

            let response = match compile_output {
                Ok(output) => {
                    if output.status.success() {
                        // 3. Se compilou, executa
                        let run_output = Command::new(&output_bin).output();
                        match run_output {
                            Ok(run_out) => {
                                // Retorna stdout + stderr da execução
                                let mut result = String::from("--- Compilação: SUCESSO ---\n");
                                result.push_str("--- Saída do Programa ---\n");
                                result.push_str(&String::from_utf8_lossy(&run_out.stdout));
                                result.push_str(&String::from_utf8_lossy(&run_out.stderr));
                                result
                            },
                            Err(e) => format!("Erro na execução: {}", e),
                        }
                    } else {
                        // Retorna erro de compilação
                        let mut err = String::from("--- Erro de Compilação ---\n");
                        err.push_str(&String::from_utf8_lossy(&output.stderr));
                        err
                    }
                },
                Err(e) => format!("Falha ao invocar GCC: {}", e),
            };

            // Envia resposta
            let _ = socket.write_all(response.as_bytes()).await;

            // Limpeza (remove arquivos temporários)
            let _ = fs::remove_file(&filename);
            let _ = fs::remove_file(&output_bin);
        });
    }
}