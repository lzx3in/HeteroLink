use rumqttc::{MqttOptions, AsyncClient, QoS};
use std::time::Duration;

#[tokio::test]
async fn test_mqtt_connect() {
    let mut opts = MqttOptions::new("test-heterolink", "broker.emqx.io", 1883u16);
    opts.set_keep_alive(Duration::from_secs(5));
    opts.set_clean_session(true);
    
    let (client, mut eventloop) = AsyncClient::new(opts, 10);
    
    println!("Starting poll...");
    for i in 0..10 {
        match eventloop.poll().await {
            Ok(event) => println!("Event {}: {:?}", i, event),
            Err(e) => println!("Error {}: {}", i, e),
        }
    }
}
