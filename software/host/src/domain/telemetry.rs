/// 遥测数据帧 — 跨层共享的领域模型
#[derive(Debug, Clone, Default, serde::Serialize)]
pub struct TelemetryData {
    pub timestamp: u32,
    pub channels: Vec<f32>,
}
