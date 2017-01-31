//Trata commnando enviado pela serial
// send <nome do parametro> <valor> <unidade>
// exemplo1: send T 25 *C
// exemplo2: send Porta aberta estatus
// exemplo3: send Voltagem 12 Volts

// Agora usando o gerenciador de conexão WiFiManagerK.
#include <FS.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include "konker.h"
//for LED id
#include "BlinkerID.h"
#include <Adafruit_Sensor.h>
#include "serialCOM.h"


extern "C" {
  #include "user_interface.h"
}


#define period 0.2 //tempo do loop



//Variaveis da Configuracao em Json
char api_key[17];
char device_id[17];
char mqtt_server[64];
char mqtt_port[5];
char mqtt_login[32];
char mqtt_pass[32];
char *mensagemjson;
char mensagemC[100];

//Variaveis Fisicas
int name_ok=0;

//String de command
String pubString;
char message_buffer[20];
int reconnectcount=0;
char *type;
char typeC[32];
String Stype;
int disconnected=0;

int marked=0;


//Ticker
Ticker timer;

int timeout=10000;
int tic=0;
int newComand=0;

WiFiManager wifiManager;



//Criando as variaveis dentro do WiFiManager
WiFiManagerParameter custom_api_key("api", "api key", api_key, 17);
WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 64);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
WiFiManagerParameter custom_mqtt_login("login", "mqtt login", mqtt_login, 32);
WiFiManagerParameter custom_mqtt_pass("password", "mqtt pass", mqtt_pass, 32);



// Funcao de Callback para o MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  int i;
  int state=0;

  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("] ");
  for (i = 0; i < length; i++) {
    msgBufferIN[i] = payload[i];
    Serial.print((char)payload[i]);
  }
  msgBufferIN[i] = '\0';
  strcpy(msgTopic, topic);
  received_msg = 1;
  Serial.println("");
}

//Definindo os objetos de Wifi e MQTT
WiFiClient espClient;
PubSubClient client("mqtt.demos.konkerlabs.net", 1883, callback,espClient);





// Setup do Microcontrolador: Vamos configurar o acesso serial, conectar no Wifi, configurar o MQTT e o GPIO do botao
void setup(){
  Serial.println("Setup");
  Serial.begin(115200);

/*
  String ssdName="Konker";
  int tentativas=10;
  WiFi.begin();//tenta connectar direto
  while (WiFi.status() != WL_CONNECTED && tentativas>0)
  {
    delay(1000);
    Serial.println("Conectando");
    Serial.print(".");
    tentativas--;
  }
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("");
    ssdName= blinkMyID("Konker");
    Serial.println("Não conectado, divulgando SSID de configuração.. "  + ssdName);
  }else{
    startBlinkID(0);// pass zero to stop
    //keep LED on
    digitalWrite(_STATUS_LED, LOW);

  }*/




  //------------------- Montando Sistema de arquivos e copiando as configuracoes  ----------------------
  spiffsMount(mqtt_server, mqtt_port, mqtt_login, mqtt_pass, device_id, device_type, api_key, in_topic, cmd_topic, data_topic, ack_topic, config_topic, connect_topic, config_period);

  //Criando as variaveis dentro do WiFiManager
  WiFiManagerParameter custom_api_key("api", "api key", api_key, 17);
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 64);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
  WiFiManagerParameter custom_mqtt_login("login", "mqtt login", mqtt_login, 32);
  WiFiManagerParameter custom_mqtt_pass("password", "mqtt pass", mqtt_pass, 32);


  //wifiManager.resetSettings();
  //------------------- Configuracao do WifiManager K ----------------------
  wifiManager.setTimeout(500);
  wifiManager.setBreakAfterConfig(1);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.addParameter(&custom_api_key);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_login);
  wifiManager.addParameter(&custom_mqtt_pass);


  //------------------- Caso conecte, copie os dados para o FS ----------------------
  if(!wifiManager.autoConnect(blinkMyID("Konker"))) {

    //Copiando parametros
    copyHTMLPar(api_key, mqtt_server, mqtt_port, mqtt_login, mqtt_pass, custom_api_key, custom_mqtt_server, custom_mqtt_port, custom_mqtt_login, custom_mqtt_pass);

    //Salvando Configuracao
    if (shouldSaveConfig) {
      saveConfigtoFile(api_key, device_id, mqtt_server, mqtt_port, mqtt_login, mqtt_pass);
    }
    delay(2500);
    ESP.reset();
  }

  //------------------- Caso tudo mais falhe copie os dados para o FS ----------------------
  //Copiando parametros
  copyHTMLPar(api_key, mqtt_server, mqtt_port, mqtt_login, mqtt_pass, custom_api_key, custom_mqtt_server, custom_mqtt_port, custom_mqtt_login, custom_mqtt_pass);

  //Salvando Configuracao
  if (shouldSaveConfig) {
    saveConfigtoFile(api_key,device_id, mqtt_server,mqtt_port,mqtt_login,mqtt_pass);
  }

  //------------------- Configurando MQTT e LED ----------------------
  /*if (espClient2.connect("http://svc.eg.konkerlabs.net", 5595))
  {
    delay(50);
    Serial.println("Conectado com o endereco: http://svc.eg.konkerlabs.net!!");
    espClient2.stop();
  }*/
  client.setServer(mqtt_server, atol(mqtt_port));
  client.setCallback(callback);
  delay(200);
  if (!client.connected()) {
    disconnected=1;
    reconnectcount = reconnect(client, api_key, mqtt_login, mqtt_pass);
    if (reconnectcount>10) resetsettings=1;
  }
  client.subscribe(in_topic);
  client.subscribe(config_topic);

  config_period_I=period;

  //-----------------------------------------------------------------------------------------

  timer.attach_ms(1000*config_period_I,sendMessage);



  Serial.println();
  Serial.println("Setup finalizado");
  Serial.println(config_period_I);

  startBlinkID(0);// pass zero to stop
  //keep LED on
  digitalWrite(_STATUS_LED, LOW);

}




// Loop com o programa principal
void loop(){
  delay(100);

  // Se desconectado, reconectar.
  if (!client.connected()) {
    disconnected = 1;
    reconnectcount = reconnect(client, api_key, mqtt_login, mqtt_pass);
    if (reconnectcount>10) resetsettings=1;
    else {
      client.subscribe(in_topic);
      client.subscribe(config_topic);
    }
  }

  client.loop();
  delay(10);


  if (resetsettings==1)
  {
    resetsettings=0;
    wifiManager.resetSettings();
    delay(500);
    ESP.restart();
  }

  if (disconnected==1)
  {
    mensagemjson = jsonMQTTmsgCONNECTED( device_id, "Connected!");
    //if (messageACK(client, connect_topic, mensagemjson, ack_topic, name_ok)) name_ok=0;
    if (messageACK(client, data_topic, mensagemjson, ack_topic, name_ok)) name_ok=0;
    disconnected = 0;
  }



  //Trata commnando enviado pela serial
  // send <nome do parametro> <valor> <unidade>
  // exemplo1: send T 25 *C
  // exemplo2: send Porta aberta estatus
  // exemplo3: send Voltagem 12 Volts
  String returnedCommand;
  returnedCommand=GetSerialCommand();
  if (returnedCommand.substring(0,4) == "send" && returnedCommand.length()>=10){
    int firstSpace=returnedCommand.indexOf(" ",1);
    int secondSpace=returnedCommand.indexOf(" ",firstSpace+1);
    int thirdSpace=returnedCommand.indexOf(" ",secondSpace+1);
    if(firstSpace==4 && secondSpace>=6 && thirdSpace>=8){
      String strLabel=returnedCommand.substring(firstSpace+1,secondSpace);
      String strValue=returnedCommand.substring(secondSpace+1,thirdSpace);
      String strUnit=returnedCommand.substring(thirdSpace+1,returnedCommand.length()-1);

      int valueisNumber=true;
      int valOK=true;
      int doubleQuote=false;
      int firstQuote=strValue.indexOf("'",0);
      if (firstQuote==-1){
        firstQuote=strValue.indexOf("\"",0);
        if(firstQuote>=0){
          doubleQuote=true;
        }
      }
      Serial.println(firstQuote);

      if (firstQuote>=0){
        int secondQuote=strValue.indexOf("'",firstQuote+1);
        if (secondQuote==-1){
          secondQuote=strValue.indexOf("\"",firstQuote+1);
          if(doubleQuote==true && firstQuote==-1){
            valOK=false;
          }
        }
        Serial.println(secondQuote);

        if(firstQuote==0 && secondQuote==strValue.length()-1){
          valueisNumber=false;
          strValue=strValue.substring(1,strValue.length()-1);
        }else if(firstQuote==-1 && secondQuote==-1){
          valueisNumber=true;
        }else{
          valOK=false;
          Serial.println("cmd_malformado_aspas");
        }
      }

      if(valOK){
        mensagemjson = jsonMQTTmsgDATA(device_id, strLabel.c_str(), strValue.c_str(), valueisNumber, strUnit.c_str());
        if (messageACK(client, data_topic, mensagemjson, ack_topic, name_ok)) name_ok=0;
        senddata=0;
      }
    }else{
      Serial.println("cmd_malformado");
    }
  }else if (returnedCommand!=""){
    Serial.println("cmd_n_reconhecido");
  }
  returnedCommand="";
  ///////////////////


  if (configured==1)
  {
    timer.detach();
    delay(200);
    timer.attach_ms(1000*config_period_I,sendMessage);
    configured=0;
  }


  if (received_msg==1) {
    if (String(msgTopic)==String(config_topic))
    {
      jsonMQTT_config_msg(msgBufferIN);
    }

    if (String(msgTopic)==String(in_topic))
    {
      type = jsonMQTT_in_msg(msgBufferIN);
      Serial.println("recebido:" + String(type));
      delay(200);
    }
    received_msg=0;
  }

}
