
/*
Code modified from Example Arduino Code
 */

#include <SPI.h>
#include <WiFiNINA.h>

/*#include <WiFiServer.h>
#include <WiFi.h>
#include <WiFiUdp.h>*/
#include "arduino_secrets.h" 

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                // your network key Index number (needed only for WEP)

int led =  LED_BUILTIN;
bool client_led=0;
int status = WL_IDLE_STATUS;
byte read_mac1;
char read_mac2;
int length_of_data;
byte client_mac_address[6];
WiFiServer server(80);
IPAddress client_ip;
IPAddress sensor_ip(192,168,4,2);//ip address of sensor client
byte mac[6];//store mac address of connected device

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Access Point Web Server");

  pinMode(led, OUTPUT);      // set the LED pin mode

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }

  // by default the local IP address of will be 192.168.4.1
  // you can override it with the following:
  // WiFi.config(IPAddress(10, 0, 0, 1));

  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Create open network. Change this line if you want to create an WEP network:
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    // don't continue
    while (true);
  }

  // wait 10 seconds for connection:
  delay(10000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();
}


void loop() {
  // compare the previous status to the current status
  if (status != WiFi.status()) {
    // it has changed update the variable
    status = WiFi.status();
    if (status == WL_AP_CONNECTED) {
      // a device has connected to the AP
      Serial.println("Device connected to AP");
    } else {
      // a device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }
  WiFiClient client = server.available();
  if (client.connected()) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    Serial.print("Device IP address: ");
    client_ip=client.remoteIP();//get ip address of connected client
    Serial.println(client_ip);
    //Otain MAC Address
    for(int i=5;i>=0;i--){
        length_of_data=client.available();
        byte dummy[length_of_data];
        int read_data[length_of_data];
        Serial.println(length_of_data);
      while(client.available()){
        for(int i=0; i<length_of_data; i++){
          dummy[i]=client.read();
          int j = convert_ansii(dummy[i]);
          read_data[i]=j;
        }
        //read_mac2=read_mac1;
        //Serial.print(read_mac2);
      }
      client_mac_address[i]=(read_data[0]*100)+(read_data[1]*10)+read_data[2];
      Serial.println(client_mac_address[i]);
    }
     //int final_value= (read_data[0]*100)+(read_data[1]*10)+read_data[2];
    //Serial.println(final_value);    
    Serial.print("MAC address: ");
    WiFi.BSSID(mac);
      Serial.print(mac[5],HEX);
      Serial.print(":");
      Serial.print(mac[4],HEX);
      Serial.print(":");
      Serial.print(mac[3],HEX);
      Serial.print(":");
      Serial.print(mac[2],HEX);
      Serial.print(":");
      Serial.print(mac[1],HEX);
      Serial.print(":");
      Serial.println(mac[0],HEX);   
    if(client_ip==sensor_ip){//sesnor client connected
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected()) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte, then
          Serial.print("Client value read as:");
          Serial.println(c);                   // print it out the serial monitor
          client.flush(); 
          client.stop();
          Serial.println("Client Disconnected");
        }
    }
   }
   else{//Laptop client connected
    Serial.println("new client");
    Serial.println("Laptop client");
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");
          }
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    client.stop();
    Serial.println("Client Disconnected");
   }
   }
   
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}

int sort_out_numbers(int *ptr, int number_length){
  if(number_length==3){
    int i=(*ptr*100);
    ptr++;
    i+=*ptr*10;
    ptr++;
    i+=*ptr;
    return i;
  }
}

int convert_ansii( byte i){
  switch (i){
    case 48:
    return 0;
    case 49:
    return 1;
    case 50:
    return 2;
    case 51:
    return 3;
    case 52:
    return 4;
    case 53:
    return 5;
        case 54:
    return 6;
    case 55:
    return 7;
        case 56:
    return 8;
    case 57:
    return 9;
  }