#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <HTTPClient.h>

#define WIFI_SSID "Samroy"
#define WIFI_PASSWORD "samroypassword"
#define SMTP_server "smtp.gmail.com"
#define SMTP_Port 465
#define sender_email "samroyj006@gmail.com"
#define sender_password "cjpckncyvdjepnoj"
#define Recipient_email "samroy963roshan@gmail.com"
#define Recipient_name ""

#define TRIGGER_PIN   7  // Pin connected to the trigger pin on the ultrasonic sensor
#define ECHO_PIN      8  // Pin connected to the echo pin on the ultrasonic sensor
#define MAX_DISTANCE  200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
#define BUZZER_PIN    6  // Pin connected to the buzzer

SoftwareSerial serial_connection(7, 8); // tx, rx
TinyGPSPlus gps;                        // GPS object to process the NMEA data
SMTPSession smtp;

void setup()
{
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");

  smtp.debug(1);

  Serial.println("GPS Start");
  serial_connection.begin(9600);
}

void loop()
{
  while (serial_connection.available())  
  {
    gps.encode(serial_connection.read());  
  }
  if (gps.location.isUpdated()) 
  {
    // Check if GPS location is near a specified location
    double targetLatitude = 12.88741893498203;
    double targetLongitude = 77.64154664898182;
    double distance = TinyGPSPlus::distanceBetween(gps.location.lat(), gps.location.lng(), targetLatitude, targetLongitude);

    if (distance < 250)
    {
      // Adjust UTC time to IST
      int hourIST = (gps.time.hour() + 5) % 24;  // Add 5 hours for IST, % 24 to handle overflow
      int minuteIST = (gps.time.minute() + 30) % 60;  // Add 30 minutes for IST, % 60 to handle overflow

      Serial.print("Time IST: ");
      if (hourIST < 10) Serial.print("0");
      Serial.print(hourIST);
      Serial.print(":");
      if (minuteIST < 10) Serial.print("0");
      Serial.print(minuteIST);
      Serial.print(":");
      if (gps.time.second() < 10) Serial.print(F("0"));
      Serial.print(gps.time.second());
      Serial.println("");

      sendEmail();

      // Display the location and route
      String mapUrl = "http://maps.google.com/maps?q=" + String(targetLatitude, 6) + "," + String(targetLongitude, 6);
      Serial.println("Map URL: " + mapUrl);

      HTTPClient http;
      http.begin(mapUrl);
      int httpCode = http.GET();
      if (httpCode > 0)
      {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK)
        {
          String payload = http.getString();
          Serial.println(payload);
        }
      }
      else
      {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    }
  }
}

void sendEmail() {
  ESP_Mail_Session session;
  session.server.host_name = SMTP_server;
  session.server.port = SMTP_Port;
  session.login.email = sender_email;
  session.login.password = sender_password;
  session.login.user_domain = "";

  if (!smtp.connect(&session)) {
    Serial.println("Error connecting to SMTP server");
    return;
  }

  SMTP_Message message;
  message.sender.name = "Samroy";
  message.sender.email = sender_email;
  message.subject = "Reminder Message";
  message.addRecipient(Recipient_name, Recipient_email);

  String htmlMsg = "<div style=\"color:#000000;\"><h1>Your Tablets are Running low...plss refill it</h1><p>Mail Generated from ESP32</p>";
  htmlMsg += "<p>Map Location: <a href=\"http://maps.google.com/maps?q=" + String(12.88741893498203, 6) + "," + String(77.64154664898182, 6) + "\">Click here</a></p></div>";
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.println("Error sending Email, " + smtp.errorReason());
  } else {
    Serial.println("Email sent successfully");
  }
}
