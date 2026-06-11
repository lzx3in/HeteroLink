use crate::protocol::crc16;

/// 帧头魔数
pub const FRAME_HEADER: u16 = 0xAA55;

/// 命令字定义
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Command {
    Heartbeat  = 0x01,
    ConfigReq  = 0x02,
    ConfigRsp  = 0x03,
    Telemetry  = 0x10,
    Control    = 0x20,
    LogUpload  = 0x30,
    Error      = 0xFF,
}

impl TryFrom<u8> for Command {
    type Error = ();
    fn try_from(v: u8) -> Result<Self, ()> {
        match v {
            0x01 => Ok(Command::Heartbeat),
            0x02 => Ok(Command::ConfigReq),
            0x03 => Ok(Command::ConfigRsp),
            0x10 => Ok(Command::Telemetry),
            0x20 => Ok(Command::Control),
            0x30 => Ok(Command::LogUpload),
            0xFF => Ok(Command::Error),
            _ => Err(()),
        }
    }
}

/// 错误码
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum ErrorCode {
    None        = 0x00,
    InvalidCmd  = 0x01,
    InvalidLen  = 0x02,
    CrcError    = 0x03,
    Timeout     = 0x04,
    DeviceBusy  = 0x05,
    Unknown     = 0xFF,
}

impl TryFrom<u8> for ErrorCode {
    type Error = ();
    fn try_from(v: u8) -> Result<Self, ()> {
        match v {
            0x00 => Ok(ErrorCode::None),
            0x01 => Ok(ErrorCode::InvalidCmd),
            0x02 => Ok(ErrorCode::InvalidLen),
            0x03 => Ok(ErrorCode::CrcError),
            0x04 => Ok(ErrorCode::Timeout),
            0x05 => Ok(ErrorCode::DeviceBusy),
            0xFF => Ok(ErrorCode::Unknown),
            _ => Err(()),
        }
    }
}

/// 协议帧结构
#[derive(Debug, Clone)]
pub struct Frame {
    pub header: u16,
    pub device_id: u8,
    pub command: u8,
    pub length: u16,
    pub payload: Vec<u8>,
    pub crc: u16,
}

impl Default for Frame {
    fn default() -> Self {
        Self {
            header: FRAME_HEADER,
            device_id: 0,
            command: 0,
            length: 0,
            payload: Vec::new(),
            crc: 0,
        }
    }
}

impl Frame {
    pub fn encode(&self) -> Vec<u8> {
        let total_len = 8 + self.payload.len();
        let mut buf = Vec::with_capacity(total_len);
        buf.extend_from_slice(&self.header.to_le_bytes());
        buf.push(self.device_id);
        buf.push(self.command);
        let len = self.payload.len() as u16;
        buf.extend_from_slice(&len.to_le_bytes());
        buf.extend_from_slice(&self.payload);
        let crc = crc16(&buf[2..]);
        buf.extend_from_slice(&crc.to_le_bytes());
        buf
    }

    pub fn decode(data: &[u8]) -> Result<Self, &'static str> {
        if data.len() < 8 {
            return Err("Frame too short");
        }
        let header = u16::from_le_bytes([data[0], data[1]]);
        if header != FRAME_HEADER {
            return Err("Invalid header");
        }
        let device_id = data[2];
        let command = data[3];
        let len = u16::from_le_bytes([data[4], data[5]]) as usize;
        if data.len() < 8 + len {
            return Err("Incomplete frame");
        }
        let payload = data[6..6 + len].to_vec();
        let crc = u16::from_le_bytes([data[6 + len], data[7 + len]]);
        let frame = Self { header, device_id, command, length: len as u16, payload, crc };
        if !frame.verify_crc() {
            return Err("CRC check failed");
        }
        Ok(frame)
    }

    pub fn verify_crc(&self) -> bool {
        let mut buf = Vec::new();
        buf.push(self.device_id);
        buf.push(self.command);
        buf.extend_from_slice(&self.length.to_le_bytes());
        buf.extend_from_slice(&self.payload);
        crc16(&buf) == self.crc
    }

    pub fn parse_telemetry(payload: &[u8]) -> TelemetryData {
        if payload.len() < 8 {
            return TelemetryData { timestamp: 0, channels: vec![] };
        }
        let timestamp = u32::from_le_bytes([payload[0], payload[1], payload[2], payload[3]]);
        let channel_count = (payload.len() - 4) / 4;
        let mut channels = Vec::with_capacity(channel_count);
        for i in 0..channel_count {
            let offset = 4 + i * 4;
            let bits = u32::from_le_bytes([
                payload[offset], payload[offset + 1],
                payload[offset + 2], payload[offset + 3],
            ]);
            channels.push(f32::from_bits(bits));
        }
        TelemetryData { timestamp, channels }
    }

    pub fn create_heartbeat(device_id: u8) -> Self {
        Self { device_id, command: Command::Heartbeat as u8, ..Self::default() }
    }

    pub fn create_control_command(device_id: u8, cmd_type: u8, payload: &[u8]) -> Self {
        let mut p = Vec::with_capacity(1 + payload.len());
        p.push(cmd_type);
        p.extend_from_slice(payload);
        Self {
            device_id,
            command: Command::Control as u8,
            length: p.len() as u16,
            payload: p,
            ..Self::default()
        }
    }
}

/// 遥测数据结构
#[derive(Debug, Clone, Default, serde::Serialize)]
pub struct TelemetryData {
    pub timestamp: u32,
    pub channels: Vec<f32>,
}
