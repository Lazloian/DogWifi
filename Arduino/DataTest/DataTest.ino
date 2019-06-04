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

int acc_error = 0;

float rad_to_deg = 180/3.151592654;
float xAcc, yAcc, zAcc;
float xAcc_error, yAcc_error, zAcc_error;
float xAngle, yAngle;
float xAngle_error, yAngle_error;

float acceleration;
float lastAcceleration;
float acceleration_error;

boolean sleeping;

void setup()
{
  Wire.begin();

  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);

  Wire.beginTransmission(0x68);
  Wire.write(0x1C);
  Wire.write(0x10);
  Wire.endTransmission(true);
  
  Serial.begin(9600);

  if (acc_error == 0)
  {
    for (int i = 0; i < 200; i++)
    {
      Wire.beginTransmission(0x68);
      Wire.write(0x3B);
      Wire.endTransmission(false);
      Wire.requestFrom(0x68, 6, true);

      xAcc = (Wire.read()<<8|Wire.read())/4096.0;
      yAcc = (Wire.read()<<8|Wire.read())/4096.0;
      zAcc = (Wire.read()<<8|Wire.read())/4096.0;

      xAcc_error += xAcc;
      yAcc_error += yAcc;
      zAcc_error += zAcc;

      acceleration = sqrt(pow(xAcc, 2) + pow(yAcc, 2) + pow(zAcc, 2));
      acceleration_error += acceleration;

      xAngle_error = xAngle_error + ((atan((yAcc) / sqrt(pow((xAcc), 2) + pow((zAcc), 2))) * rad_to_deg));
      yAngle_error = yAngle_error + ((atan(-1 * (xAcc) / sqrt(pow((yAcc), 2) + pow((zAcc), 2))) * rad_to_deg));

      if (i == 199)
      {
        xAngle_error = xAngle_error / 200;
        yAngle_error = yAngle_error / 200;
        xAcc_error = xAcc_error / 200;
        yAcc_error = yAcc_error / 200;
        zAcc_error = zAcc_error / 200;
        acceleration_error = acceleration_error / 200;
        
        acc_error = 1;
      }
    }
  }

  lastAcceleration = sqrt(pow(xAcc, 2) + pow(yAcc, 2) + pow(zAcc, 2)) - acceleration_error;

  delay(10);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

float getAccel() 
{

  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 6, true);

  xAcc = (Wire.read()<<8|Wire.read())/4096.0 - xAcc_error;
  yAcc = (Wire.read()<<8|Wire.read())/4096.0 - yAcc_error;
  zAcc = (Wire.read()<<9|Wire.read())/4096.0 - zAcc_error;

  xAngle = (atan((yAcc) / sqrt(pow((xAcc), 2) + pow((zAcc), 2))) * rad_to_deg) - xAngle_error;
  yAngle = (atan(-1 * (xAcc) / sqrt(pow((yAcc), 2) + pow((zAcc), 2))) * rad_to_deg) - yAngle_error;

  return sqrt(pow(xAcc, 2) + pow(yAcc, 2) + pow(zAcc, 2)) - acceleration_error;
}

boolean isSleeping()
{
  acceleration =  getAccel();

  if (acceleration > lastAcceleration + 0.5 || acceleration < lastAcceleration - 0.5)
  {
    lastAcceleration = acceleration;
    return false;
  }
  else
  {
    return true;
  }
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
  if (now - lastMsg > 500)
  {
    lastMsg = now;
    float accel = getAccel();
    char accelString[8];
    Serial.println(accel);
    dtostrf(accel, 1, 2, accelString);

    client.publish("esp32/acceleration", accelString);
  }
}
