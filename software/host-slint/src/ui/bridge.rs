use crate::core::{DeviceInfo, AlarmRecord, AlarmLevel, ChannelStats};
use crate::protocol::TelemetryData;
use slint::{ModelRc, SharedString, VecModel, Model};

slint::include_modules!();

/// UI 桥接器 - 连接 Rust 后端和 Slint 前端
#[allow(dead_code)]
pub struct UiBridge {
    pub ui: AppUI,
}

impl UiBridge {
    pub fn new(ui: AppUI) -> Self {
        Self { ui }
    }

    #[allow(dead_code)]
    pub fn update_devices(&self, devices: &[DeviceInfo]) {
        let model = VecModel::from(
            devices.iter().map(|d| DeviceItem {
                id: d.id.clone().into(),
                name: d.name.clone().into(),
                connected: d.connected,
                online: d.online,
                connection_type: d.connection_type.clone().into(),
                port: d.port.clone().into(),
            }).collect::<Vec<_>>()
        );
        self.ui.set_devices(ModelRc::new(model));
    }

    #[allow(dead_code)]
    pub fn update_telemetry(&self, data: &[TelemetryData]) {
        let model = VecModel::from(
            data.iter().map(|d| TelemetrySnapshot {
                timestamp: d.timestamp as i32,
                channel_count: d.channels.len() as i32,
                ch0: d.channels.get(0).copied().unwrap_or(0.0),
                ch1: d.channels.get(1).copied().unwrap_or(0.0),
                ch2: d.channels.get(2).copied().unwrap_or(0.0),
                ch3: d.channels.get(3).copied().unwrap_or(0.0),
            }).collect::<Vec<_>>()
        );
        self.ui.set_telemetry_data(ModelRc::new(model));
    }

    #[allow(dead_code)]
    pub fn update_stats(&self, stats: &[(i32, ChannelStats)]) {
        let model = VecModel::from(
            stats.iter().map(|(ch, s)| StatsItem {
                channel_id: *ch,
                min: s.min,
                max: s.max,
                avg: s.avg,
                rms: s.rms,
                sample_count: s.sample_count as i32,
            }).collect::<Vec<_>>()
        );
        self.ui.set_channel_stats(ModelRc::new(model));
    }

    #[allow(dead_code)]
    pub fn update_alarms(&self, alarms: &[AlarmRecord]) {
        let model = VecModel::from(
            alarms.iter().map(|a| AlarmItem {
                device_id: a.device_id.clone().into(),
                channel_id: a.channel_id,
                level: match a.level {
                    AlarmLevel::Info => SharedString::from("INFO"),
                    AlarmLevel::Warning => SharedString::from("WARNING"),
                    AlarmLevel::Critical => SharedString::from("CRITICAL"),
                },
                value: a.value,
                message: a.message.clone().into(),
                timestamp: a.timestamp.format("%H:%M:%S").to_string().into(),
                acknowledged: a.acknowledged,
            }).collect::<Vec<_>>()
        );
        self.ui.set_alarm_records(ModelRc::new(model));
    }

    #[allow(dead_code)]
    pub fn update_available_ports(&self, ports: &[String]) {
        let model = VecModel::from(
            ports.iter().map(|p| SharedString::from(p.clone())).collect::<Vec<_>>()
        );
        self.ui.set_available_ports(ModelRc::new(model));
    }

    #[allow(dead_code)]
    pub fn add_log(&self, message: &str) {
        let current = self.ui.get_log_messages();
        let mut logs: Vec<SharedString> = current.iter().collect();
        logs.push(SharedString::from(message));
        if logs.len() > 100 {
            logs.drain(0..logs.len() - 100);
        }
        self.ui.set_log_messages(ModelRc::new(VecModel::from(logs)));
    }

    #[allow(dead_code)]
    pub fn update_status(&self, message: &str, connected: i32, online: i32) {
        self.ui.set_status_message(message.into());
        self.ui.set_connected_count(connected);
        self.ui.set_online_count(online);
    }

    #[allow(dead_code)]
    pub fn set_mqtt_connected(&self, connected: bool) {
        self.ui.set_mqtt_connected(connected);
    }

    #[allow(dead_code)]
    pub fn set_recording(&self, recording: bool) {
        self.ui.set_recording(recording);
    }
}
