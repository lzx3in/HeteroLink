use crate::events::DomainEvent;
use crate::infra::time_util::now_hms;

/// 封装事件广播通道，提供领域事件的发布能力
///
/// 与传输协议无关：不依赖 `api` 层的任何类型。
#[derive(Clone)]
pub struct EventBus {
    tx: tokio::sync::broadcast::Sender<DomainEvent>,
}

impl EventBus {
    pub fn new(capacity: usize) -> Self {
        let (tx, _) = tokio::sync::broadcast::channel(capacity);
        Self { tx }
    }

    pub fn subscribe(&self) -> tokio::sync::broadcast::Receiver<DomainEvent> {
        self.tx.subscribe()
    }

    /// 发布一个领域事件
    pub fn emit(&self, event: DomainEvent) {
        let _ = self.tx.send(event);
    }

    /// 便捷方法：发布日志事件
    pub fn emit_log(&self, message: impl Into<String>) {
        let _ = self.tx.send(DomainEvent::Log {
            message: message.into(),
            timestamp: now_hms(),
        });
    }

    /// 便捷方法：发布更新可用事件
    pub fn emit_update_available(&self, version: impl Into<String>, body: impl Into<String>) {
        let _ = self.tx.send(DomainEvent::UpdateAvailable {
            version: version.into(),
            body: body.into(),
        });
    }
}
