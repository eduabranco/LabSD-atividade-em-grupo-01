/* src/bin/client_gui.rs */
use eframe::egui;
use std::io::{Read, Write};
use std::net::TcpStream;
use std::sync::mpsc::{channel, Receiver, Sender};
use std::thread;

fn main() -> Result<(), eframe::Error> {
    let options = eframe::NativeOptions::default();
    eframe::run_native(
        "Cliente Compilador Rust (Async)",
        options,
        Box::new(|_cc| Box::new(CompilerApp::default())),
    )
}

struct CompilerApp {
    code: String,
    output: String,
    server_ip: String,
    // Channel to receive the result from the thread
    rx: Option<Receiver<String>>,
    is_loading: bool,
}

impl Default for CompilerApp {
    fn default() -> Self {
        Self {
            code: "#include <stdio.h>\n\nint main() {\n    printf(\"Ola Mundo Assincrono!\\n\");\n    return 0;\n}".to_owned(),
            output: String::new(),
            server_ip: "127.0.0.1:51482".to_owned(),
            rx: None,
            is_loading: false,
        }
    }
}

impl CompilerApp {
    fn run_code(&mut self) {
        self.output = "Conectando ao servidor...".to_owned();
        self.is_loading = true;

        let ip = self.server_ip.clone();
        let code = self.code.clone();
        
        let (tx, rx): (Sender<String>, Receiver<String>) = channel();
        self.rx = Some(rx);

        // Spawn a thread for blocking network I/O
        thread::spawn(move || {
            let result = match TcpStream::connect(&ip) {
                Ok(mut stream) => {
                    if let Err(e) = stream.write_all(code.as_bytes()) {
                        format!("Erro ao enviar código: {}", e)
                    } else {
                        let _ = stream.shutdown(std::net::Shutdown::Write);
                        let mut buffer = String::new();
                        match stream.read_to_string(&mut buffer) {
                            Ok(_) => buffer,
                            Err(e) => format!("Erro ao ler resposta: {}", e),
                        }
                    }
                },
                Err(e) => format!("Não foi possível conectar em {}: {}", ip, e),
            };
            // Send result back to main thread
            let _ = tx.send(result);
        });
    }
}

impl eframe::App for CompilerApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        // Check for results from the thread
        if let Some(rx) = &self.rx {
            if let Ok(result) = rx.try_recv() {
                self.output = result;
                self.is_loading = false;
                self.rx = None; // Clear channel
            }
        }

        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("Compilador Remoto C (Non-Blocking)");

            ui.horizontal(|ui| {
                ui.label("Servidor IP:");
                ui.text_edit_singleline(&mut self.server_ip);
                
                if ui.add_enabled(!self.is_loading, egui::Button::new("EXECUTAR ▶")).clicked() {
                    self.run_code();
                }
                
                if self.is_loading {
                    ui.spinner();
                }
            });

            ui.separator();
            ui.label("Editor de Código (C):");
            egui::ScrollArea::vertical().max_height(300.0).show(ui, |ui| {
                 ui.add(
                    egui::TextEdit::multiline(&mut self.code)
                        .font(egui::TextStyle::Monospace)
                        .code_editor()
                        .desired_width(f32::INFINITY)
                );
            });

            ui.separator();
            ui.label("Saída do Servidor:");
            egui::ScrollArea::vertical().show(ui, |ui| {
                ui.add(
                    egui::TextEdit::multiline(&mut self.output)
                        .font(egui::TextStyle::Monospace)
                        .desired_width(f32::INFINITY)
                        .interactive(false)
                );
            });
        });
        
        // Repaint periodically if loading to animate spinner and check thread
        if self.is_loading {
            ctx.request_repaint();
        }
    }
}