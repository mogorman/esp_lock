#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <math.h>

#include <PubSubClient.h>


const char* ssid = "jingle_bells";
const char* password = "0123456789abcdef";
const char* host = "frontdoor";
const char* mqtt_server = "home.rldn.net";

#define LED 1

#define AIN1 4
#define AIN2 16
#define PWMA 5

#define SW1A 3
#define SW1B 12

#define SW2A 13
#define SW2B 14

#define DOOR A0

#define LOCK_OPEN 0
#define LOCK_CLOSED 1

#define MOTOR_CW 0
#define MOTOR_CCW 1

#define MOTOR_SPEED 200

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
uint8_t lock_state;


int16_t test_var = 0;
int16_t toggle = 0;
unsigned long timer = 0;


void unlock();
void lock();
void reset_lock();
void stop_motor();
void move_motor(uint8_t dir);

void callback(char* topic, byte* payload, unsigned int length) {
 
 // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    lock();
  } else {
    digitalWrite(LED, HIGH);  // Turn the LED off by making the voltage HIGH
    unlock();
  }

}


void setup() {
  pinMode(LED, OUTPUT);
  
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);

  pinMode(SW1A, INPUT);
  pinMode(SW1B, INPUT);
  pinMode(SW2A, INPUT);
  pinMode(SW2B, INPUT);
  pinMode(DOOR, INPUT_PULLUP);
  
  timer = millis();
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  ArduinoOTA.setPassword((const char *)"123");
  ArduinoOTA.setHostname(host);
  ArduinoOTA.onStart([]() {
//    digitalWrite(LED, LOW);
     asm("nop");
  });
  ArduinoOTA.onEnd([]() {
//    digitalWrite(LED, LOW);
    asm("nop");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
//    digitalWrite(LED,progress%2);
     asm("nop");
  });
  ArduinoOTA.onError([](ota_error_t error) {
//    digitalWrite(LED, LOW);
    asm("nop");
//    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
//    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
//    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
//    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
//    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
//    // Attempt to connect
    if (client.connect("ESP8266Client")) {
//      // Once connected, publish an announcement...
      client.publish("door_out", "reconnected");
      // ... and resubscribe
      client.subscribe("door_in");
    } else {
      delay(5000);
    }
  }
}

void loop() {
    for(int i = 0 ; i < 10; i++) {
       ArduinoOTA.handle();
    }
    if (!client.connected()) {
      reconnect();
    }
    client.loop();

    long now = millis();
    if (now - lastMsg > 500) {
      int sw1a, sw1b, sw2a, sw2b, door;
      sw1a = digitalRead(SW1A);
      sw1b = digitalRead(SW1B);
      sw2a = digitalRead(SW2A);
      sw2b = digitalRead(SW2B);
      door = digitalRead(DOOR);
      lastMsg = now;
      ++value;
      snprintf (msg, 75, "Return: #%ld: 1A=%ld 1B=%ld 2A=%ld 2B=%ld D=%ld s=%ld", value, sw1a, sw1b, sw2a, sw2b, door, lock_state);
      client.publish("door_out", msg);
    }
}



// move lock around

void reset_lock() {
    // Move motor to reset its position
  move_motor(MOTOR_CCW);
  int sw1a, sw1b, sw2a, sw2b;
  do
  {
    sw1a = digitalRead(SW1A);
    sw1b = digitalRead(SW1B);
    sw2a = digitalRead(SW2A);
    sw2b = digitalRead(SW2B);
  }
  while ( !((sw2a == 1) && (sw2b == 1)));
  stop_motor();
  lock_state = LOCK_OPEN;
}

void move_motor(uint8_t dir)
{
    if ( dir ) {
      digitalWrite(AIN1, HIGH);
      digitalWrite(AIN2, LOW);
    } else {
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, HIGH);
    }
    analogWrite(PWMA, MOTOR_SPEED);
}

void stop_motor()
{
    analogWrite(PWMA, 0);
}

void lock()
{
  
  int sw1a, sw1b, sw2a, sw2b;
  // Move motor to lock the deadbolt
  move_motor(MOTOR_CW);
  do
  {
    sw1a = digitalRead(SW1A);
    sw1b = digitalRead(SW1B);
    sw2a = digitalRead(SW2A);
    sw2b = digitalRead(SW2B);
  }
  while ( !((sw1a == 0) && (sw1b == 1) && 
            (sw2a == 0) && (sw2b == 1)) );
  stop_motor();
  delay(100);
  
  // Move motor back to starting position
  move_motor(MOTOR_CCW);
  do
  {
    sw1a = digitalRead(SW1A);
    sw1b = digitalRead(SW1B);
    sw2a = digitalRead(SW2A);
    sw2b = digitalRead(SW2B);
  }
  while ( !((sw2a == 1) && (sw2b == 1)) );
  stop_motor();
  lock_state = LOCK_OPEN;
}

void unlock()
{
  
  int sw1a, sw1b, sw2a, sw2b;
  // Move motor to lock the deadbolt
  move_motor(MOTOR_CCW);
  do
  {
    sw1a = digitalRead(SW1A);
    sw1b = digitalRead(SW1B);
    sw2a = digitalRead(SW2A);
    sw2b = digitalRead(SW2B);
  }
  while ( !((sw1a == 1) && (sw1b == 0) && 
            (sw2a == 1) && (sw2b == 0) ));
  stop_motor();
  delay(100);
  
  // Move motor back to starting position
  move_motor(MOTOR_CW);
  do
  {
    sw1a = digitalRead(SW1A);
    sw1b = digitalRead(SW1B);
    sw2a = digitalRead(SW2A);
    sw2b = digitalRead(SW2B);
  }
  while ( !((sw2a == 1) && (sw2b == 1)) );
  stop_motor();
  lock_state = LOCK_CLOSED;
}
