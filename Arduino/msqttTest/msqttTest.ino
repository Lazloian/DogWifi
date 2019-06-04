#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

const char* ssid = "Student-N";
const char* password = "Cubs06StuLHS";

const char* mqtt_server = "10.220.74.14";

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[50];
int value = 0;

void setup()
{
  Serial.begin(9600);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi()
{
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);

  WiFi.begin(ssid, password);

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

void callback(char* topic, byte* message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  Serial.println();

  if (String(topic) == "esp32/output")
  {
    Serial.print("Changing output to ");

    if (messageTemp == "on")
    {
      Serial.println("on");
    }
    else if (messageTemp == "off")
    {
      Serial.println("off");
    }
  }
}


void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection... ");

    if (client.connect("ESP8266Client"))
    {
      Serial.println("connected");
      client.subscribe("esp32/output");
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

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }

  client.loop();

  long now = millis();
  if (now - lastMsg > 5000)
  {
    lastMsg = now;

    char tempString[8];
    dtostrf(10.0, 1, 2, tempString);

    client.publish("esp32/acceleration", tempString);
  }
}
