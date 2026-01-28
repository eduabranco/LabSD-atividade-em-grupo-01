/* src/bin/compiler_server.rs */
use tokio::net::TcpListener;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use std::process::Command;
use std::fs;
use tokio::time::{timeout, Duration};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let listener = TcpListener::bind("0.0.0.0:51482").await?;
    println!("Servidor de Compilação (C) Seguro rodando na porta 51482...");

    loop {
        let (mut socket, _) = listener.accept().await?;

        tokio::spawn(async move {
            let mut buffer = [0; 4096];
            let n = match socket.read(&mut buffer).await {
                Ok(n) if n == 0 => return,
                Ok(n) => n,
                Err(_) => return,
            };

            let source_code = String::from_utf8_lossy(&buffer[..n]);
            
            let id = uuid::Uuid::new_v4(); 
            // NOTE: For better security, these should be inside a Docker container
            let filename = format!("/tmp/prog_{}.c", id);
            let output_bin = format!("/tmp/prog_{}", id);

            if let Err(_) = fs::write(&filename, source_code.as_bytes()) {
                let _ = socket.write_all(b"Erro interno: Falha ao salvar arquivo").await;
                return;
            }

            // Compile with timeout
            // Although compilation usually terminates, it's good practice
            let compile_output = Command::new("gcc")
                .arg(&filename)
                .arg("-o")
                .arg(&output_bin)
                .output();

            let response = match compile_output {
                Ok(output) => {
                    if output.status.success() {
                        // EXECUTION with 5 second timeout to prevent loops
                        // We use tokio::process::Command if we want async wait, 
                        // but std::process inside spawn_blocking or just timeout wrapper is easier here.
                        
                        let run_result = timeout(Duration::from_secs(5), async {
                             tokio::process::Command::new(&output_bin)
                                .output()
                                .await
                        }).await;

                        match run_result {
                            Ok(Ok(run_out)) => {
                                let mut result = String::from("--- Compilação: SUCESSO ---\n");
                                result.push_str("--- Saída do Programa ---\n");
                                result.push_str(&String::from_utf8_lossy(&run_out.stdout));
                                result.push_str(&String::from_utf8_lossy(&run_out.stderr));
                                result
                            },
                            Ok(Err(e)) => format!("Erro na execução: {}", e),
                            Err(_) => String::from("Erro: Tempo limite de execução excedido (5s)."),
                        }
                    } else {
                        let mut err = String::from("--- Erro de Compilação ---\n");
                        err.push_str(&String::from_utf8_lossy(&output.stderr));
                        err
                    }
                },
                Err(e) => format!("Falha ao invocar GCC: {}", e),
            };

            let _ = socket.write_all(response.as_bytes()).await;

            // Cleanup
            let _ = fs::remove_file(&filename);
            let _ = fs::remove_file(&output_bin);
        });
    }
}