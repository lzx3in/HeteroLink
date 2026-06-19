use axum::extract::ws::{WebSocket, WebSocketUpgrade, Message};
use axum::extract::State;
use axum::response::IntoResponse;
use futures::{SinkExt, StreamExt};
use tracing::info;

use crate::api::AppState;

pub async fn ws_handler(
    ws: WebSocketUpgrade,
    State(state): State<AppState>,
) -> impl IntoResponse {
    ws.on_upgrade(move |socket| handle_ws_connection(socket, state))
}

async fn handle_ws_connection(socket: WebSocket, state: AppState) {
    let (mut sender, mut receiver) = socket.split();
    let mut rx = state.ws_broadcast.subscribe();

    info!("WebSocket client connected");

    // 发送任务：从 broadcast 接收并推送到 WebSocket 客户端
    let send_task = tokio::spawn(async move {
        while let Ok(msg) = rx.recv().await {
            if let Ok(json) = serde_json::to_string(&msg) {
                if sender.send(Message::Text(json.into())).await.is_err() {
                    break;
                }
            }
        }
    });

    // 接收任务：处理客户端发来的消息
    let recv_task = tokio::spawn(async move {
        while let Some(Ok(msg)) = receiver.next().await {
            match msg {
                Message::Text(text) => {
                    // 可解析客户端命令，当前仅做日志
                    tracing::debug!("WS client message: {}", text);
                }
                Message::Close(_) => break,
                _ => {}
            }
        }
    });

    // 任一任务结束则关闭连接
    tokio::select! {
        _ = send_task => {},
        _ = recv_task => {},
    }

    info!("WebSocket client disconnected");
}
