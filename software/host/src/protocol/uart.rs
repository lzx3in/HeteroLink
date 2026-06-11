use crate::protocol::{Frame, TelemetryData};
use anyhow::Result;
use std::io::Read;
use std::sync::{Arc, Mutex};
use std::time::Duration;
use tokio::sync::mpsc;
use tracing::{error, info};

/// UART 通道配置
#[derive(Debug, Clone)]
pub struct UartConfig {
    pub port_name: String,
    pub baud_rate: u32,
    pub data_bits: u8,
    pub stop_bits: u8,
    pub parity: u8,
}

impl Default for UartConfig {
    fn default() -> Self {
        Self {
            port_name: String::new(),
            baud_rate: 921600,
            data_bits: 8,
            stop_bits: 1,
            parity: 0,
        }
    }
}

/// UART 通道事件
#[derive(Debug)]
pub enum UartEvent {
    Connected(String),
    Disconnected(String),
    TelemetryReceived(u8, TelemetryData),
    ErrorReceived(u8, u8),
    Error(String),
}

/// UART 通道
pub struct UartChannel {
    config: UartConfig,
    connected: Arc<Mutex<bool>>,
    device_id: u8,
}

impl UartChannel {
    pub fn new(config: UartConfig) -> Self {
        Self {
            config,
            connected: Arc::new(Mutex::new(false)),
            device_id: 0,
        }
    }

    pub fn set_device_id(&mut self, device_id: u8) {
        self.device_id = device_id;
    }

    pub fn is_connected(&self) -> bool {
        *self.connected.lock().unwrap()
    }

    pub fn connect(&mut self, event_tx: mpsc::Sender<UartEvent>) -> Result<()> {
        let serial_builder = serialport::new(&self.config.port_name, self.config.baud_rate)
            .timeout(Duration::from_millis(100))
            .data_bits(match self.config.data_bits {
                5 => serialport::DataBits::Five,
                6 => serialport::DataBits::Six,
                7 => serialport::DataBits::Seven,
                _ => serialport::DataBits::Eight,
            })
            .stop_bits(match self.config.stop_bits {
                2 => serialport::StopBits::Two,
                _ => serialport::StopBits::One,
            })
            .parity(match self.config.parity {
                1 => serialport::Parity::Odd,
                2 => serialport::Parity::Even,
                _ => serialport::Parity::None,
            });

        let port = serial_builder.open().map_err(|e| {
            anyhow::anyhow!("Failed to open serial port {}: {}", self.config.port_name, e)
        })?;

        let connected = self.connected.clone();
        *connected.lock().unwrap() = true;

        let port_name = self.config.port_name.clone();
        info!("UART connected: {} @ {}", port_name, self.config.baud_rate);
        let _ = event_tx.blocking_send(UartEvent::Connected(port_name.clone()));

        // 启动读取线程
        std::thread::spawn(move || {
            let mut port = port;
            let mut read_buf = Vec::new();
            let mut byte_buf = [0u8; 1];

            loop {
                match port.read_exact(&mut byte_buf) {
                    Ok(_) => {
                        read_buf.push(byte_buf[0]);
                        // 尝试解析帧
                        if read_buf.len() >= 8 {
                            if let Ok(frame) = Frame::decode(&read_buf) {
                                let consumed = 6 + frame.length as usize + 2;
                                read_buf.drain(..consumed);

                                if frame.command == 0x10 {
                                    let telemetry = Frame::parse_telemetry(&frame.payload);
                                    let _ = event_tx.blocking_send(UartEvent::TelemetryReceived(
                                        frame.device_id, telemetry,
                                    ));
                                } else if frame.command == 0xFF {
                                    if let Some(&err) = frame.payload.first() {
                                        let _ = event_tx.blocking_send(UartEvent::ErrorReceived(
                                            frame.device_id, err,
                                        ));
                                    }
                                }
                            } else if read_buf.len() > 256 {
                                read_buf.clear();
                            }
                        }
                    }
                    Err(ref e) if e.kind() == std::io::ErrorKind::TimedOut => {
                        // 超时，继续等待
                    }
                    Err(e) => {
                        error!("UART read error: {}", e);
                        let _ = event_tx.blocking_send(UartEvent::Error(e.to_string()));
                        *connected.lock().unwrap() = false;
                        let _ = event_tx.blocking_send(UartEvent::Disconnected(port_name));
                        break;
                    }
                }
            }
        });

        Ok(())
    }

    pub fn disconnect(&mut self) {
        *self.connected.lock().unwrap() = false;
        info!("UART disconnected");
    }

    pub fn send_frame(&self, _frame: &Frame) -> Result<()> {
        if !self.is_connected() {
            return Err(anyhow::anyhow!("Not connected"));
        }
        // 实际发送需要持有 port 引用，这里简化处理
        Ok(())
    }

    pub fn send_heartbeat(&self, device_id: u8) -> Result<()> {
        let frame = Frame::create_heartbeat(device_id);
        self.send_frame(&frame)
    }
}
