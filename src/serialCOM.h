#include <SoftwareSerial.h>

//Serial hardware

String  GetSerialCommand(int secondsTimout){
  String SerialCommand="";
  byte incomingByte=0;
  secondsTimout=secondsTimout*1000;
  int tick=0;
  if (Serial.available()!=0){
    while(incomingByte != 10 && incomingByte != 13 && tick<=secondsTimout){
      if (Serial.available()!=0)
      {
        incomingByte = Serial.read();//le e consome o byte lido
        //Serial.write(incomingByte);//escreve o byte lido
        Serial.flush();
        SerialCommand+=char(incomingByte);
      }
      if(incomingByte == 10 || incomingByte == 13 ) {
        Serial.flush();
        return SerialCommand;
      }
      tick++;
      delay(1);
    }
  }
}

String  GetSerialCommand(){
  String SerialCommand="";
  byte incomingByte=0;

  int tick=0;
  if (Serial.available()!=0){
    while(incomingByte != 13){
      if (Serial.available()!=0)
      {
        incomingByte = Serial.read();//le e consome o byte lido
        //Serial.write(incomingByte);//escreve o byte lido
        Serial.flush();
        SerialCommand+=char(incomingByte);
      }
      if(incomingByte == 10 || incomingByte == 13 ) {
        Serial.flush();
        return SerialCommand;
      }
    }
  }
}



  //response to default serial
String GetSwSerialResponse(SoftwareSerial& swSerial){
  String SerialCommand="";
  byte incomingByte=0;
  while(swSerial.available()!=0){
    incomingByte = swSerial.read();//le e consome o byte lido
    SerialCommand+=char(incomingByte);
  }
  return SerialCommand;
}
