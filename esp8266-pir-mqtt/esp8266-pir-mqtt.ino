#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// -------------
// Configuration
// -------------

const char *wifi_ssid = "YOUR_SSID_HERE";
const char *wifi_pass = "YOUR_PASSWORD_HERE";
const char *wifi_hostname = "ESP8266-PIR-Sensor";

const char *mqtt_server = "your-mqtt-server.com";
const char *mqtt_topic = "your/mqtt/topic";
const char *mqtt_payload_motion = "true";
const char *mqtt_payload_clear = "false";

const int mqtt_port = 1883;
const char *mqtt_user = NULL;
const char *mqtt_pass = NULL;
const char *mqtt_client_id = wifi_hostname;

const int sensor_pin = 5; // GPIO5 = D1 on a NodeMCU
const int sensor_interval_ms = 1000;

// --------------------
// End of configuration
// --------------------

WiFiClient wifi;
PubSubClient client(wifi);

bool motion_state = false;
int last_check_millis = 0;

void setup_wifi()
{
  delay(10);

  // settings to reduce wifi interference from NodeMCU to the PIR sensor
  wifi_set_phy_mode(PHY_MODE_11G);
  system_phy_set_max_tpw(8);
  WiFi.mode(WIFI_STA);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.hostname(wifi_hostname);
  WiFi.begin(wifi_ssid, wifi_pass);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_mqtt()
{
  client.setServer(mqtt_server, mqtt_port);
}

void setup_sensor()
{
  pinMode(sensor_pin, INPUT);
}

void reconnect_mqtt()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");

    if (client.connect(mqtt_client_id, mqtt_user, mqtt_pass))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void check_sensor()
{
  int new_state = digitalRead(sensor_pin);

  if (new_state == HIGH && !motion_state)
  {
    Serial.println("Motion detected!");
    motion_state = true;
    client.publish(mqtt_topic, mqtt_payload_motion);
  }
  else if (new_state == LOW && motion_state)
  {
    Serial.println("Motion cleared.");
    motion_state = false;
    client.publish(mqtt_topic, mqtt_payload_clear);
  }
}

void setup()
{
  Serial.begin(115200);

  setup_wifi();
  setup_mqtt();
  setup_sensor();
}

void loop()
{

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Lost WiFi Connection! Restarting...");
    ESP.restart();
  }

  if (!client.connected())
  {
    reconnect_mqtt();
  }
  client.loop();

  long now = millis();
  if (now - last_check_millis > sensor_interval_ms)
  {
    last_check_millis = now;

    check_sensor();
  }
}
