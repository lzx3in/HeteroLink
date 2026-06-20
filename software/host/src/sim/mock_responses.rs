/// Generate a mock JSON response for simulation mode.
pub fn simulate_response(command_json: &str) -> String {
    let cmd = serde_json::from_str::<serde_json::Value>(command_json)
        .ok()
        .and_then(|v| v.get("cmd").and_then(|c| c.as_str()).map(|s| s.to_string()))
        .unwrap_or_else(|| "unknown".to_string());

    let (status, message) = match cmd.as_str() {
        "start" => {
            let rate = serde_json::from_str::<serde_json::Value>(command_json)
                .ok()
                .and_then(|v| {
                    v.get("params")
                        .and_then(|p| p.get("sample_rate"))
                        .and_then(|r| r.as_i64())
                })
                .unwrap_or(1000);
            (
                "ok".to_string(),
                format!("Simulator: sampling started at {} Hz", rate),
            )
        }
        "stop" => (
            "ok".to_string(),
            "Simulator: sampling stopped".to_string(),
        ),
        "set_gpio" => {
            let (ch, vl) = serde_json::from_str::<serde_json::Value>(command_json)
                .ok()
                .map(|v| {
                    let c = v.get("channel").and_then(|x| x.as_i64()).unwrap_or(4);
                    let val = v.get("value").and_then(|x| x.as_i64()).unwrap_or(0);
                    (c, val)
                })
                .unwrap_or((4, 0));
            (
                "ok".to_string(),
                format!("Simulator: GPIO ch{} = {}", ch, vl),
            )
        }
        other => (
            "error".to_string(),
            format!("Simulator: unknown command '{}'", other),
        ),
    };

    let ts = chrono::Utc::now().timestamp_millis();
    serde_json::json!({
        "cmd": cmd,
        "status": status,
        "message": message,
        "timestamp": ts
    })
    .to_string()
}
