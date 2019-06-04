#include <Wire.h>
#include <WiFi.h>

const char* ssid = "ATT8aur9F7_EXT";
const char* password = "Lazloian1";

WiFiServer server(80);

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

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wifi Connected");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() 
{
  WiFiClient client = server.available();

  if (client)
  {
    Serial.println("New Client");
    String currentLine = "";

    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        Serial.write(c);

        if (c == '\n')
        {
          if (currentLine.length() == 0)
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            /*if (isSleeping())
            {
              client.print("She is sleeping");
            }
            else
            {
              client.print("She is awake");
            }*/

            client.print(String(getAccel()));
            
            client.println();
            break;
          }
          else
          {
            currentLine = "";
          }
        }
        else if (c != '\r')
        {
          currentLine += c;
        }
      }
    }
    client.stop();
    Serial.println("Client Disconnected.");
  }
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



























