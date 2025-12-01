/* 
   Arquivo: server_threaded.rs 
   Dependências: Nenhuma (apenas std)
*/
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::thread;

fn handle_client(mut stream: TcpStream) {
    let mut buffer = [0; 512];
    // Lê dados do cliente
    match stream.read(&mut buffer) {
        Ok(size) if size > 0 => {
            let received = String::from_utf8_lossy(&buffer[..size]);
            // println!("Recebido: {}", received); // Comentado para teste de stress

            // Responde ao cliente
            let response = "I got your message";
            if let Err(e) = stream.write(response.as_bytes()) {
                eprintln!("Erro ao escrever: {}", e);
            }
        }
        Ok(_) => {} // Conexão fechada
        Err(e) => eprintln!("Erro ao ler: {}", e),
    }
    // O socket fecha automaticamente quando 'stream' sai de escopo
}

fn main() {
    let listener = TcpListener::bind("0.0.0.0:51482").expect("Não foi possível iniciar servidor");
    println!("Servidor Threaded rodando na porta 51482...");

    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                // Cria uma nova thread para cada conexão
                thread::spawn(|| {
                    handle_client(stream);
                });
            }
            Err(e) => {
                eprintln!("Erro na conexão: {}", e);
            }
        }
    }
}