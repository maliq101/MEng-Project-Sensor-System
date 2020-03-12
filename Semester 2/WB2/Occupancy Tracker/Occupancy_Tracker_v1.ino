
/*
This program includes the measuring capabilities of the TE Ambimate sensor with the previous client code
Periodic communication with the server via HTTP 

sensor communication using i2c
request all mesaured data, which is reaches in a char array

RED=VCC(TOP RIGHT ON sensor)
BLack=GND
YELLOW=SCL
BLUE=SDA

*/
#include <ArduinoHttpClient.h>
#include <WiFiNINA.h>
#include "Sensor_Config.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
/////// Wifi Settings ///////
char ssid[] = SERVER_2_SSID;
char pass[] = SECRET_PASS;

///Create wificlient object, HTTP client object and relevant variables
char serverAddress[] = SERVER_2_IP;  // server address
int port = 80;//port number for HTTP
WiFiClient wifi;//Create a wifi Client object called wifi.
HttpClient client = HttpClient(wifi, serverAddress, port);//create http client object  using the wifi client object
int wifi_status = WL_IDLE_STATUS;
int http_status_code = client.responseStatusCode();
String http_response = client.responseBody();
String http_content_type;
String http_put_data;
String sensor_number = SENSOR_6;//sensor number sent as http url


int occupancy=0;
volatile int occupancy_vol;
int sample_time = 120000;//2 min
void setup() {
  Serial.begin(9600);
  pinMode(0,OUTPUT);//RED no commincation with server
  pinMode(1,OUTPUT);//RED Low Battery
  pinMode(2,OUTPUT);//Green commincation with sensor IC
  pinMode(3,OUTPUT);//Green commincation with Server

  pinMode(4,INPUT);
  pinMode(5,INPUT);

  attachInterrupt(digitalPinToInterrupt(5), increase, CHANGE);
  attachInterrupt(digitalPinToInterrupt(6), decrease, CHANGE);
  connect_to_wifi();
}

void loop() {
  occupancy=occupancy_vol;
  led(occupancy);
  put_request();
  delay(sample_time);
}

void led(int i){
}
void connect_to_wifi(void){
  while ( wifi_status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);
    digitalWrite(5,HIGH);
    // Connect to WPA/WPA2 network:
    wifi_status = WiFi.begin(ssid, pass);
  }
  digitalWrite(5,LOW);

  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

}
void reconnect_to_wifi(void){
  while ( wifi_status != WL_CONNECTED) {
    digitalWrite(0,HIGH);
    wifi_status = WiFi.begin(ssid, pass);// Connect to WPA/WPA2 network:
  }
  digitalWrite(0,LOW);
}
void put_request(){
  Serial.println("making PUT request");
  http_content_type = "text/plain";
  http_put_data = String(occupancy);
  if( wifi.connect(serverAddress,port)){
    if(wifi.connected()){
      //client connected to the server
      client.put(sensor_number, http_content_type, http_put_data);
      http_status_code = client.responseStatusCode();
      http_response = client.responseBody();
    }
  }
  else{
    //print error message then wait 1 second  then attempt to connect to the wifi network
    digitalWrite(5,HIGH);
    wifi_error();
    wifi.flush();
    wifi.stop();// disconnect the wifi client
    WiFi.disconnect();//disconnect from the wifi network
    delay(1000);
    Serial.println("Attempting to reconnect");
    reconnect_to_wifi();
  }
}
void wifi_error(void){
  String error_code;
  wifi_status=wifi.status();
  Serial.println("Error with WiFi connection");
  switch (wifi_status)
  {
  case 0:
    error_code="WiFi idle";
    break;
  case 1:
    error_code="No SSID available";
    break;
  case 4:
    error_code="Connection Failed";
    break;
  case 5:
    error_code="Connection Lost";
    break;
  case 6:
    error_code="Disconnected from WiFi";
    break;
  default:
    break;
  }
  Serial.print("Error: ");
  Serial.println(error_code);
}
void increase(void){
  occupancy_vol++;
}
void decrease(void){
  occupancy_vol--;
}