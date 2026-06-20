use axum::extract::ws::{WebSocket, WebSocketUpgrade, Message};
use axum::extract::State;
use axum::response::IntoResponse;
use futures::{SinkExt, StreamExt};
use tracing::info;

use crate::api::broadcast::WsMessage;
use crate::api::AppState;

pub async fn ws_handler(
    ws: WebSocketUpgrade,
    State(state): State<AppState>,
) -> impl IntoResponse {
    ws.on_upgrade(move |socket| handle_ws_connection(socket, state))
}

async fn handle_ws_connection(socket: WebSocket, state: AppState) {
    let (mut sender, mut receiver) = socket.split();
    let mut rx = state.event_bus.subscribe();

    info!("WebSocket client connected");

    // 发送任务：DomainEvent → WsMessage → JSON → WebSocket
    let send_task = tokio::spawn(async move {
        while let Ok(domain_event) = rx.recv().await {
            if let Some(ws_msg) = WsMessage::from_domain(&domain_event) {
                if let Ok(json) = serde_json::to_string(&ws_msg) {
                    if sender.send(Message::Text(json)).await.is_err() {
                        break;
                    }
                }
            }
        }
    });

    // 接收任务：处理客户端发来的消息
    let recv_task = tokio::spawn(async move {
        while let Some(Ok(msg)) = receiver.next().await {
            match msg {
                Message::Text(text) => {
                    tracing::debug!("WS client message: {}", text);
                }
                Message::Close(_) => break,
                _ => {}
            }
        }
    });

    tokio::select! {
        _ = send_task => {},
        _ = recv_task => {},
    }

    info!("WebSocket client disconnected");
}
