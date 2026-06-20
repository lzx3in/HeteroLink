/// 返回当前时间的 HH:MM:SS 格式字符串
pub fn now_hms() -> String {
    chrono::Local::now().format("%H:%M:%S").to_string()
}

/// 返回适用于文件命名的 YYYYMMDD_HHMMSS 格式字符串
pub fn now_datetime_file() -> String {
    chrono::Local::now().format("%Y%m%d_%H%M%S").to_string()
}
