// -------------  𝑳𝑰𝑩𝑹𝑬𝑹𝑰𝑨𝑺 -------------
#include "heltec.h"
#include "images.h"
// ------------- 𝑭𝑰𝑵 -------------

#define BAND 915E6

// ------------- 𝑽𝑨𝑹𝑰𝑨𝑩𝑳𝑬𝑺 𝑫𝑬 𝑩𝑶𝑻𝑶𝑵𝑬𝑺 -------------
#define BUTTON_PIN1 13 //button #1
#define BUTTON_PIN2 12 //button #2
#define BUTTON_PIN3 32 //button #3

#define SHORT_PRESS_TIME 500 // 500 milliseconds
#define LONG_PRESS_TIME 1500 // 1500 milliseconds

bool startup_pressed = true;
int lastState1 = LOW, lastState2 = LOW, lastState3 = LOW; // the previous state from the input pin
int currentState1, currentState2, currentState3; // the current reading from the input pin
unsigned long pressedTime1 = 0, pressedTime2 = 0, pressedTime3 = 0;
unsigned long releasedTime1 = 0, releasedTime2 = 0, releasedTime3 = 0;
// ------------- 𝑭𝑰𝑵 -------------

// ------------- 𝑽𝑨𝑹𝑰𝑨𝑩𝑳𝑬𝑺 𝑫𝑬 𝑪𝑶𝑴𝑼𝑵𝑰𝑪𝑨𝑪𝑰𝑶𝑵 𝑳𝒐𝑹𝒂 -------------
String outgoing;              // outgoing message

//ESTACION MOVIL: 0xAA = 170
//ESTACION FIJA: 0xFA = 250
byte localAddress = 0xAA;     // address of this device
byte destination = 0xFA;      // destination to send to

byte msgCount = 0;            // count of outgoing messages
long lastSendTime = 0;        // last send time
// ------------- 𝑭𝑰𝑵 -------------

void uacj_logo(){ // 𝑰𝑪𝑶𝑵𝑶 𝑫𝑬 𝑰𝑵𝑰𝑪𝑰𝑶
	Heltec.display -> clear();
	Heltec.display -> drawXbm(0,5,uacj_logo_width,uacj_logo_height,(const unsigned char *)uacj_logo_bits);
	Heltec.display -> display();
}

void setup()
{
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  
  //  𝑷𝑰𝑵𝑬𝑺 𝑫𝑬 𝑬𝑵𝑻𝑹𝑨𝑫𝑨
  pinMode(BUTTON_PIN1, INPUT_PULLUP);
  pinMode(BUTTON_PIN2, INPUT_PULLUP);
  pinMode(BUTTON_PIN3, INPUT_PULLUP);

  uacj_logo();

  Serial.print("Inicializando");
  for(int i = 0; i <= 5; i++){
    Serial.print(".");
    delay(500);
  }

  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Heltec.LoRa Initial success!");
  Heltec.display->drawString(0, 10, "Wait for incoming data...");

  Serial.println("\nHeltec.LoRa iniciado!");
  Serial.println("Modulo listo para recibir información...");
  
  delay(1000);
}

void loop()
{
  if (millis() - lastSendTime > 3000)
  {
    String message = "Hola, soy #" + String(localAddress) + " - " + String(msgCount);   // send a message
    sendMessage(message);
    lastSendTime = millis();            // timestamp the message
  }

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());

  botones();
}

void sendMessage(String outgoing)
{
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize)
{
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  while (LoRa.available())
  {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length())
  {   // check length for error
    Serial.println("Error: Message length does not match length...");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != destination) {
    Serial.println("Error: This message is not for me...");
    return;                             // skip rest of function
  }

  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->drawString(0, 0, "RSSI: " + String(LoRa.packetRssi()) + " - SNR: " + String(LoRa.packetSnr(), 1));
  Heltec.display->drawString(0, 15, "Distancia: " + String(pow(10, ((-69 - LoRa.packetRssi()) / (10 * LoRa.packetSnr()))), 1) + "m");
  //Serial.println("# of Packet: " + String(incomingMsgId) + " - Length of Packet: " + String(incomingLength));  
  Heltec.display->drawStringMaxWidth(0 , 30, 128, "MSG: " + String(incoming));
  Heltec.display->drawString(0 , 50, "ESTACION MOVIL");
  Heltec.display->display();

  Serial.println("Received from: 0x" + String(sender, HEX) + ", Message: " + incoming + ", RSSI: " + String(LoRa.packetRssi()) + ", SNR: " + String(LoRa.packetSnr()));
  /*
  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("SNR: " + String(LoRa.packetSnr()));
  Serial.println();
  */
}

void botones(){ 
  if (lastState1 == HIGH && digitalRead(BUTTON_PIN1) == LOW){
    pressedTime1 = millis();
  } else if (lastState1 == LOW && digitalRead(BUTTON_PIN1) == HIGH) {
    releasedTime1 = millis();
    long pressDuration1 = releasedTime1 - pressedTime1;

    if (pressDuration1 > SHORT_PRESS_TIME && !startup_pressed){
      Serial.println("Button #1 Pressed!");
    }
  }

  if (lastState2 == HIGH && digitalRead(BUTTON_PIN2) == LOW){
    pressedTime2 = millis();
  } else if (lastState2 == LOW && digitalRead(BUTTON_PIN2) == HIGH) {
    releasedTime2 = millis();
    long pressDuration2 = releasedTime2 - pressedTime2;

    if (pressDuration2 > SHORT_PRESS_TIME && !startup_pressed){
      Serial.println("Button #2 Pressed!");
    }
  }
  
  if (lastState3 == HIGH && digitalRead(BUTTON_PIN3) == LOW){
    pressedTime3 = millis();
  } else if (lastState3 == LOW && digitalRead(BUTTON_PIN3) == HIGH) {
    releasedTime3 = millis();
    long pressDuration3 = releasedTime3 - pressedTime3;

    if (pressDuration3 > SHORT_PRESS_TIME && pressDuration3 < LONG_PRESS_TIME){
      Serial.println("Button #3 SHORT Pressed!");
    } else if (pressDuration3 > LONG_PRESS_TIME && !startup_pressed){
      Serial.println("Button #3 LONG Pressed!");
    } 
  }

  // save the the last state
  lastState1 = digitalRead(BUTTON_PIN1);
  lastState2 = digitalRead(BUTTON_PIN2);
  lastState3 = digitalRead(BUTTON_PIN3);

  if(startup_pressed){
    delay(5000);
    startup_pressed = false;
  } 
}
