use eframe::egui;
use std::io::{Read, Write};
use std::net::TcpStream;

fn main() -> Result<(), eframe::Error> {
    let options = eframe::NativeOptions::default();
    eframe::run_native(
        "Cliente Compilador Rust",
        options,
        Box::new(|_cc| Box::new(CompilerApp::default())),
    )
}

struct CompilerApp {
    code: String,
    output: String,
    server_ip: String,
}

impl Default for CompilerApp {
    fn default() -> Self {
        Self {
            // Código inicial C padrão
            code: "#include <stdio.h>\n\nint main() {\n    printf(\"Ola Mundo do Linux Sockets!\\n\");\n    return 0;\n}".to_owned(),
            output: String::new(),
            server_ip: "127.0.0.1:51482".to_owned(),
        }
    }
}

impl CompilerApp {
    fn run_code(&mut self) {
        self.output = "Conectando ao servidor...".to_owned();
        
        // Lógica de Socket (Bloqueante para simplificar o exemplo GUI)
        match TcpStream::connect(&self.server_ip) {
            Ok(mut stream) => {
                // Envia o código
                if let Err(e) = stream.write_all(self.code.as_bytes()) {
                    self.output = format!("Erro ao enviar código: {}", e);
                    return;
                }
                
                // O servidor C/Rust implementado espera fechar a escrita para processar (EOF)
                // ou processa direto. Vamos garantir o shutdown da escrita para sinalizar fim.
                let _ = stream.shutdown(std::net::Shutdown::Write);

                // Lê a resposta
                let mut buffer = String::new();
                match stream.read_to_string(&mut buffer) {
                    Ok(_) => self.output = buffer,
                    Err(e) => self.output = format!("Erro ao ler resposta: {}", e),
                }
            },
            Err(e) => {
                self.output = format!("Não foi possível conectar em {}: {}", self.server_ip, e);
            }
        }
    }
}

impl eframe::App for CompilerApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("Compilador Remoto C (via Sockets)");

            ui.horizontal(|ui| {
                ui.label("Servidor IP:");
                ui.text_edit_singleline(&mut self.server_ip);
                
                if ui.button("EXECUTAR ▶").clicked() {
                    self.run_code();
                }
            });

            ui.separator();

            // Área de Edição (Código)
            ui.label("Editor de Código (C):");
            egui::ScrollArea::vertical().max_height(300.0).show(ui, |ui| {
                 ui.add(
                    egui::TextEdit::multiline(&mut self.code)
                        .font(egui::TextStyle::Monospace) // Fonte monoespaçada
                        .code_editor()
                        .desired_width(f32::INFINITY)
                );
            });

            ui.separator();

            // Área de Saída (Output)
            ui.label("Saída do Servidor:");
            egui::ScrollArea::vertical().show(ui, |ui| {
                ui.add(
                    egui::TextEdit::multiline(&mut self.output)
                        .font(egui::TextStyle::Monospace)
                        .desired_width(f32::INFINITY)
                        .interactive(false) // Apenas leitura
                );
            });
        });
    }
}