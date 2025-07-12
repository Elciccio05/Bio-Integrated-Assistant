#include <Arduino.h>
#include "RTClib.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WebServer.h>

RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Domenica", "Lunedì", "Martedì", "Mercoledì", "Giovedì", "Venerdì", "Sabato"};

const int oneWireBus = 10;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

int alarmHour = 7;
int alarmMinute = 30;
bool svegliaAttiva = true;

#define MOTOR_LEFT 1
#define MOTOR_RIGHT 2

const char* ssid = "BIA";
const char* password = "bia_network";

WebServer server(80);

void suonaSveglia(){
  int currentTime = millis();
  while ((millis() - currentTime) < 60000){
    digitalWrite(MOTOR_LEFT, HIGH);
    digitalWrite(MOTOR_RIGHT, HIGH);

    delay(5000);

    digitalWrite(MOTOR_LEFT, LOW);
    digitalWrite(MOTOR_RIGHT, LOW);

    delay(5000);
  }
}

void handleRoot() {
  sensors.requestTemperatures();
  float temperaturaC = sensors.getTempCByIndex(0);

  String colore = "black";
  String avviso = "";

  if (temperaturaC < 25.0){
    colore = "deepskyblue";
    avviso = "<p style='color:deepskyblue;'>Attenzione: temperatura interna ridotta</p>";
  } else if (temperaturaC > 35.0) {
    colore = "red";
    avviso = "<p style='color:red;'>Attenzione: temperatura interna eccessiva</p>";
  }

  String page = R"rawliteral(
    <html>
      <head>
        <title>Sveglia</title>
      </head>
      <body>
        <h2>Temperatura attuale</h2>
  )rawliteral";

  page += "<h1 style='color:" + colore + "; font-size: 48px;'>";
  page += String(temperaturaC, 1) + " &deg;C</h1>";
  page += avviso;
  
  page += R"rawliteral(
        <hr>
        <h2>Imposta sveglia</h2>
        <form action="/set-alarm">
          Ora: <input type="number" name="hour" min="0" max="23"><br>
          Minuto: <input type="number" name="minute" min="0" max="59"><br>
          <input type="submit" value="Salva">
        </form>
        <hr>
        <h3>Stato sveglia: 
  )rawliteral";

  page += (svegliaAttiva ? "ATTIVA" : "DISATTIVATA");

  page += R"rawliteral(</h3>
        <form action="/disattiva">
          <input type="submit" value="Disattiva sveglia">
        </form>
  )rawliteral";

  if (!svegliaAttiva){
    page += R"rawliteral(
        <form action="/attiva">
          <input type="submit" value="Riattiva sveglia">
        </form>
    )rawliteral";
  }

  page += R"rawliteral(
      </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", page);
}

void handleSetAlarm() {
  if (server.hasArg("hour") && server.hasArg("minute")) {
    alarmHour = server.arg("hour").toInt();
    alarmMinute = server.arg("minute").toInt();
    Serial.printf("Sveglia impostata: %02d:%02d\n", alarmHour, alarmMinute);
    server.send(200, "text/html", "<h1>Sveglia salvata!</h1><a href='/'>Torna</a>");
  } else {
    server.send(400, "text/plain", "Parametri mancanti.");
  }
}

void handleDisattiva() {
  svegliaAttiva = false;
  //Serial.println("Sveglia disattivata manualmente.");
  server.send(200, "text/html", "<h1>Sveglia disattivata!</h1><a href='/'>Torna</a>");
}

void handleAttiva() {
  svegliaAttiva = true;
  //Serial.println("Sveglia riattivata manualmente.");
  server.send(200, "text/html", "<h1>Sveglia riattivata!</h1><a href='/'>Torna</a>");
}

void setup() {
  Serial.begin(115200);

  //----RTC SETUP----
  if (!rtc.begin()){
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()){
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  //----TEMPERATURE SENSOR SETUP----
  sensors.begin();

  //----MOTORS SETUP----
  pinMode(MOTOR_LEFT, OUTPUT);
  pinMode(MOTOR_RIGHT, OUTPUT);

  digitalWrite(MOTOR_LEFT, LOW);
  digitalWrite(MOTOR_RIGHT, LOW);

  //----WEBSERVER SETUP----
  WiFi.softAP(ssid, password);

  server.on("/", handleRoot);
  server.on("/set-alarm", handleSetAlarm);
  server.on("/disattiva", handleDisattiva);
  server.on("/attiva", handleAttiva);
  server.begin();

}

void loop() {
  DateTime now = rtc.now();

  int currentHour = now.hour();
  int currentMinute = now.minute();

  if (svegliaAttiva && currentHour == alarmHour && currentMinute == alarmMinute){
    suonaSveglia();
  }

  // Getting each time field in individual variables
  // And adding a leading zero when needed
  //String yearStr = String(now.year(), DEC);
  //String monthStr = (now.month() < 10 ? "0" : "") + String(now.month(), DEC);
  //String dayStr = (now.day() < 10 ? "0" : "") + String(now.day(), DEC);
  //String hourStr = (now.hour() < 10 ? "0" : "") + String(now.hour(), DEC); 
  //String minuteStr = (now.minute() < 10 ? "0" : "") + String(now.minute(), DEC);
  //String secondStr = (now.second() < 10 ? "0" : "") + String(now.second(), DEC);
  //String dayOfWeek = daysOfTheWeek[now.dayOfTheWeek()];

  //sensors.requestTemperatures(); 
  //float temperatureC = sensors.getTempCByIndex(0);

  server.handleClient();

  delay(1000);
}