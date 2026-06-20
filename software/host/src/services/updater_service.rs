use std::sync::Arc;
use tokio::sync::Mutex as TokioMutex;
use tracing::{info, warn};

use crate::infra::event_bus::EventBus;

/// 当前更新状态
#[derive(Debug, Clone, PartialEq)]
pub enum UpdateStatus {
    /// 空闲，尚未检查
    Idle,
    /// 正在检查更新
    Checking,
    /// 发现新版本
    Available {
        version: String,
        body: String,
        download_url: String,
    },
    /// 正在下载
    Downloading { progress: f32 },
    /// 已下载，等待重启应用
    Ready { version: String },
    /// 已是最新
    UpToDate,
    /// 检查/下载失败
    Error(String),
}

/// GitHub Release 元数据（从 API 解析）
#[derive(Debug, Clone)]
#[allow(dead_code)]
struct ReleaseInfo {
    tag_name: String,
    version: String,
    body: String,
    assets: Vec<ReleaseAsset>,
}

#[derive(Debug, Clone)]
#[allow(dead_code)]
struct ReleaseAsset {
    name: String,
    download_url: String,
}

/// 自动更新服务 — 从 GitHub Releases 检查并应用更新
#[derive(Clone)]
pub struct UpdaterService {
    inner: Arc<TokioMutex<UpdaterInner>>,
    event_bus: EventBus,
}

struct UpdaterInner {
    status: UpdateStatus,
    repo_owner: String,
    repo_name: String,
    current_version: String,
    bin_name: String,
}

impl UpdaterService {
    pub fn new(event_bus: EventBus) -> Self {
        Self {
            inner: Arc::new(TokioMutex::new(UpdaterInner {
                status: UpdateStatus::Idle,
                repo_owner: "lzx3in".to_string(),
                repo_name: "HeteroLink".to_string(),
                current_version: env!("CARGO_PKG_VERSION").to_string(),
                bin_name: "heterolink-host".to_string(),
            })),
            event_bus,
        }
    }

    /// 获取当前版本
    pub async fn current_version(&self) -> String {
        let inner = self.inner.lock().await;
        inner.current_version.clone()
    }

    /// 获取 EventBus 引用
    pub fn event_bus(&self) -> &EventBus {
        &self.event_bus
    }

    /// 获取当前更新状态
    pub async fn status(&self) -> UpdateStatus {
        let inner = self.inner.lock().await;
        inner.status.clone()
    }

    /// 检查 GitHub 是否有新版本
    pub async fn check_for_update(&self) -> UpdateStatus {
        {
            let mut inner = self.inner.lock().await;
            inner.status = UpdateStatus::Checking;
        }
        self.event_bus.emit_log("正在检查更新...");

        let (repo_owner, repo_name, current_version, bin_name) = {
            let inner = self.inner.lock().await;
            (
                inner.repo_owner.clone(),
                inner.repo_name.clone(),
                inner.current_version.clone(),
                inner.bin_name.clone(),
            )
        };

        // 在 blocking 线程执行 self_update 的同步 API
        let result = tokio::task::spawn_blocking(move || {
            Self::fetch_latest_release(&repo_owner, &repo_name, &current_version, &bin_name)
        })
        .await;

        let new_status = match result {
            Ok(Ok(release)) => {
                info!("New version available: {}", release.version);
                UpdateStatus::Available {
                    version: release.version.clone(),
                    body: release.body.clone(),
                    download_url: release
                        .assets
                        .first()
                        .map(|a| a.download_url.clone())
                        .unwrap_or_default(),
                }
            }
            Ok(Err(msg)) if msg == "up_to_date" => {
                info!("Already up to date");
                UpdateStatus::UpToDate
            }
            Ok(Err(msg)) => {
                warn!("Update check failed: {}", msg);
                UpdateStatus::Error(msg)
            }
            Err(e) => {
                warn!("Update check task failed: {}", e);
                UpdateStatus::Error(format!("任务失败: {}", e))
            }
        };

        {
            let mut inner = self.inner.lock().await;
            inner.status = new_status.clone();
        }
        new_status
    }

    /// 下载并应用更新（替换当前二进制）
    pub async fn apply_update(&self) -> Result<String, String> {
        let (repo_owner, repo_name, bin_name) = {
            let inner = self.inner.lock().await;
            match &inner.status {
                UpdateStatus::Available { version, .. } => {
                    info!("Applying update to version {}", version);
                }
                _ => return Err("没有可用的更新".to_string()),
            }
            (
                inner.repo_owner.clone(),
                inner.repo_name.clone(),
                inner.bin_name.clone(),
            )
        };

        {
            let mut inner = self.inner.lock().await;
            inner.status = UpdateStatus::Downloading { progress: 0.0 };
        }
        self.event_bus.emit_log("正在下载更新...");

        let result = tokio::task::spawn_blocking(move || {
            self_update::backends::github::Update::configure()
                .repo_owner(&repo_owner)
                .repo_name(&repo_name)
                .bin_name(&bin_name)
                .current_version(env!("CARGO_PKG_VERSION"))
                .show_download_progress(false)
                .no_confirm(true)
                .build()
                .map_err(|e| format!("构建更新器失败: {}", e))?
                .update()
                .map_err(|e| format!("更新失败: {}", e))
        })
        .await;

        match result {
            Ok(Ok(status)) => {
                let version = status.version().to_string();
                info!("Update applied successfully: {}", version);
                {
                    let mut inner = self.inner.lock().await;
                    inner.status = UpdateStatus::Ready {
                        version: version.clone(),
                    };
                }
                self.event_bus
                    .emit_log(&format!("更新已就绪 (v{})，请重启服务以应用", version));
                Ok(version)
            }
            Ok(Err(msg)) => {
                warn!("Apply update failed: {}", msg);
                {
                    let mut inner = self.inner.lock().await;
                    inner.status = UpdateStatus::Error(msg.clone());
                }
                Err(msg)
            }
            Err(e) => {
                let msg = format!("任务失败: {}", e);
                warn!("Apply update task failed: {}", msg);
                {
                    let mut inner = self.inner.lock().await;
                    inner.status = UpdateStatus::Error(msg.clone());
                }
                Err(msg)
            }
        }
    }

    /// 从 GitHub API 获取最新 Release 信息并比较版本
    fn fetch_latest_release(
        owner: &str,
        repo: &str,
        current: &str,
        bin_name: &str,
    ) -> Result<ReleaseInfo, String> {
        let releases = self_update::backends::github::ReleaseList::configure()
            .repo_owner(owner)
            .repo_name(repo)
            .build()
            .map_err(|e| format!("初始化 ReleaseList 失败: {}", e))?
            .fetch()
            .map_err(|e| format!("获取 Release 列表失败: {}", e))?;

        if releases.is_empty() {
            return Err("没有找到任何 Release".to_string());
        }

        let latest = &releases[0];
        let latest_version = latest
            .version
            .trim_start_matches('v')
            .to_string();

        // 比较版本号
        if latest_version == current {
            return Err("up_to_date".to_string());
        }

        // 简单语义版本比较
        if !Self::is_newer(&latest_version, current) {
            return Err("up_to_date".to_string());
        }

        // 查找当前平台的资产
        let target_asset = Self::find_platform_asset(&latest.assets, bin_name);

        Ok(ReleaseInfo {
            tag_name: latest.name.clone(),
            version: latest_version,
            body: latest.body.clone().unwrap_or_default(),
            assets: target_asset.into_iter().collect(),
        })
    }

    /// 查找匹配当前平台的 Release 资产
    fn find_platform_asset(
        assets: &[self_update::update::ReleaseAsset],
        bin_name: &str,
    ) -> Option<ReleaseAsset> {
        let target = self_update::get_target();

        // 资产命名约定: {bin_name}-{target}.{ext}
        // e.g. heterolink-host-x86_64-unknown-linux-gnu.tar.gz
        let asset = assets.iter().find(|a| {
            a.name.contains(bin_name) && a.name.contains(target)
        });

        asset.map(|a| ReleaseAsset {
            name: a.name.clone(),
            download_url: a.download_url.clone(),
        })
    }

    /// 简单的语义版本比较：a > b 则返回 true
    fn is_newer(a: &str, b: &str) -> bool {
        let parse = |s: &str| -> Vec<u32> {
            s.split('.')
                .filter_map(|p| p.parse().ok())
                .collect()
        };
        let va = parse(a);
        let vb = parse(b);
        for (x, y) in va.iter().zip(vb.iter()) {
            if x > y {
                return true;
            }
            if x < y {
                return false;
            }
        }
        va.len() > vb.len()
    }
}
