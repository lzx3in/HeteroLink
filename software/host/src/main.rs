use anyhow::Result;

mod domain;
mod events;
mod protocol;
mod core;
mod storage;
mod services;
mod infra;
mod workers;
mod api;
mod app;

#[cfg(feature = "simulation")]
mod sim;

#[tokio::main]
async fn main() -> Result<()> {
    tracing_subscriber::fmt()
        .with_max_level(tracing::Level::INFO)
        .init();

    app::builder::AppBuilder::from_args().build_and_run().await
}
