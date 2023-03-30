/*
  Heltec.LoRa Multiple Communication

  This example provide a simple way to achieve one to multiple devices
  communication.

  Each devices send datas in broadcast method. Make sure each devices
  working in the same BAND, then set the localAddress and destination
  as you want.
  
  Sends a message every half second, and polls continually
  for new incoming messages. Implements a one-byte addressing scheme,
  with 0xFD as the broadcast address. You can set this address as you
  want.

  Note: while sending, Heltec.LoRa radio is not listening for incoming messages.
  
  by Aaron.Lee from HelTec AutoMation, ChengDu, China
  成都惠利特自动化科技有限公司
  www.heltec.cn
  
  this project also realess in GitHub:
  https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
*/
#include "heltec.h"
#include "images.h"

#include <SPI.h>
#include <Wire.h>  

#define LORA_SCK     5    
#define LORA_MISO    19   
#define LORA_MOSI    27 
#define LORA_SS      18  
#define LORA_RST     14   
#define LORA_DI0     26  
#define LORA_BAND    915E6

#include <SPI.h>
#include <MFRC522.h>

#define RFID_SDA 5 
#define RFID_SCK 18 
#define RFID_MOSI 23
#define RFID_MISO 19
#define RFID_RST 22

MFRC522 mfrc522(RFID_SDA, RFID_RST);  // Create MFRC522 instance

int current_spi = -1; // -1 - NOT STARTED   0 - RFID   1 - LORA

String outgoing;              // outgoing message

//MOVIL: 0xAA = 170
//FIJA: 0xFA = 250
byte localAddress = 0xFA;     // address of this device
byte destination = 0xAA;      // destination to send to

byte msgCount = 0;            // count of outgoing messages

unsigned long previousTime1 = 0;
unsigned long previousTime2 = 0; 

void uacj_logo(){
  Heltec.display->clear();
  Heltec.display->drawXbm(0, 5, uacj_logo_width, uacj_logo_height, uacj_logo_bits);
  Heltec.display->display();
}

void setup()
{
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, LORA_BAND /*long BAND*/);

  Heltec.display->init();
  //Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10);
  uacj_logo();
  delay(1500);
  Heltec.display->clear();

  Heltec.display->drawString(0, 0, "Heltec.LoRa Initial success!");
  Heltec.display->drawString(0, 10, "Wait for incoming data...");
  
  delay(1000);
}

void loop()
{
  unsigned long currentTime = millis();

  if (currentTime - previousTime1 >= 1000) {
    Serial.println("Reading card...");
    RFID_check();
    previousTime1 = currentTime;
  }
  
  if (currentTime - previousTime2 >= 5000) {  
    spi_select(1);
    String message = "Hola, soy #" + String(localAddress) + "! - " + String(msgCount);   // send a message
    sendMessage(message);
    Serial.println("Enviando: " + message);
    onReceive(LoRa.parsePacket());
    previousTime2 = currentTime;
  }  
  
  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}

int RFID_check() {
  spi_select(0);
  if (mfrc522.PICC_IsNewCardPresent()){  
    //Seleccionamos una tarjeta
    if (mfrc522.PICC_ReadCardSerial()) 
    {
      // Enviamos serialemente su UID
      Serial.print("Card UID:");
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);   
      } 
      Serial.println();
      // Terminamos la lectura de la tarjeta  actual
      mfrc522.PICC_HaltA();         
    }
	}

  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    delay(100);
    return false;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return false;
  }
  
  return true;
}

void spi_select(int which) {
     if (which == current_spi) return;
     SPI.end();
     
     switch(which) {
        case 0:
          SPI.begin(RFID_SCK, RFID_MISO, RFID_MOSI);
          mfrc522.PCD_Init();   
        break;
        case 1:
          SPI.begin(LORA_SCK,LORA_MISO,LORA_MOSI,LORA_SS);
          LoRa.setPins(LORA_SS,LORA_RST,LORA_DI0);          
        break;
     }

     current_spi = which;
}

void sendMessage(String outgoing){
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize){
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  while (LoRa.available()){
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()){
    // check length for error
    Serial.println("Error: message length does not match length");
    return;
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress) {
    Serial.println("This message is not for me.");
    return;
  }
  
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->drawString(0, 0, "RSSI: " + String(LoRa.packetRssi()) + " - SNR: " + String(LoRa.packetSnr(), 1));
  Heltec.display->drawString(0, 15, "Distancia: " + String(pow(10, ((-69 - LoRa.packetRssi()) / (10 * LoRa.packetSnr()))), 1) + "m");
  Serial.println("# of Packet: " + String(incomingMsgId) + " - Length of Packet: " + String(incomingLength));  
  Heltec.display->drawStringMaxWidth(0 , 30, 128, "MSG: " + String(incoming));
  Heltec.display->drawString(0 , 50, "ESTACION FIJA");
  Heltec.display->display();

  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println("Distancia: " + String(pow(10, ((-69 - LoRa.packetRssi()) / (10 * LoRa.packetSnr()))), 1) + "m");
  Serial.println();
}