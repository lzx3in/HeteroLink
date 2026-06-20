use std::sync::Arc;

use crate::core::DataProcessor;
use crate::domain::error::HeteroLinkError;

/// 数据导出服务
///
/// 在内存中直接生成 CSV/JSON 字节流，不再写入临时文件中转。
#[derive(Clone)]
pub struct ExportService {
    data_processor: Arc<DataProcessor>,
}

impl ExportService {
    pub fn new(data_processor: Arc<DataProcessor>) -> Self {
        Self { data_processor }
    }

    /// 在内存中生成 CSV 字节
    pub async fn export_csv_bytes(&self, device_id: &str) -> Result<Vec<u8>, HeteroLinkError> {
        let data = self.data_processor.get_data(device_id).await;
        if data.is_empty() {
            return Err(HeteroLinkError::ExportError(format!(
                "No data for device: {}",
                device_id
            )));
        }

        let mut buf: Vec<u8> = Vec::new();
        {
            let mut writer = csv::Writer::from_writer(&mut buf);

            // 写入表头
            if let Some(first) = data.first() {
                let channel_count = first.channels.len();
                let mut header: Vec<String> = vec!["timestamp".to_string()];
                for i in 0..channel_count {
                    header.push(format!("channel{}", i));
                }
                let header_refs: Vec<&str> = header.iter().map(|s| s.as_str()).collect();
                writer.write_record(&header_refs)?;

                for item in &data {
                    let mut record = vec![item.timestamp.to_string()];
                    for ch in &item.channels {
                        record.push(ch.to_string());
                    }
                    let record_refs: Vec<&str> = record.iter().map(|s| s.as_str()).collect();
                    writer.write_record(&record_refs)?;
                }
            }
            writer.flush()?;
        }

        Ok(buf)
    }

    /// 在内存中生成 JSON 字节
    pub async fn export_json_bytes(&self, device_id: &str) -> Result<Vec<u8>, HeteroLinkError> {
        let data = self.data_processor.get_data(device_id).await;
        if data.is_empty() {
            return Err(HeteroLinkError::ExportError(format!(
                "No data for device: {}",
                device_id
            )));
        }
        Ok(serde_json::to_vec_pretty(&data)?)
    }
}
