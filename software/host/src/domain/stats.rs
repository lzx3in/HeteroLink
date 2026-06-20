use serde::Serialize;

/// 通道统计数据
#[derive(Debug, Clone, Default, Serialize)]
pub struct ChannelStats {
    pub min: f32,
    pub max: f32,
    pub avg: f32,
    pub rms: f32,
    pub sample_count: u32,
}
