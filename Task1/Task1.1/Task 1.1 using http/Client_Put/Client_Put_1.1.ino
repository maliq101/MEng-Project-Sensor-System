/*
Simple Program that sends a PUT Request to the arduino server using HTTP
Code works with Server_PUT_GET program, and sends test string, int, float and char variables
 */
#include <ArduinoHttpClient.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
/////// Wifi Settings ///////
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

char serverAddress[] = "192.168.4.1";  // server address
int port = 80;//port number for HTTP

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);//create http client which connects to server wifi
int status = WL_IDLE_STATUS;

//test variables used for PUT testing
String test_string="test data from client";//strng
char test_char[]={48,49,50,51};//[0,1,2,3] in ascii
float test_float=3.142; 
int test_int=100;

void setup() {
  Serial.begin(9600);
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
  }

  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

void loop() {
  Serial.println("making PUT request");
  String contentType = "text/plain";
  String putData = String("String:"+ test_string + "\n" + "int:" + test_int + "\n"  + "float:" + test_float + "\n" + "test_char_array:" + test_char[2]);

  client.put("/", contentType, putData);


  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
  Serial.println("Wait five seconds");
  delay(5000);
}
