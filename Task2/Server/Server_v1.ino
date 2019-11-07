/*
*/


#include <ArduinoHttpServer.h>
#include <SPI.h> //
#include <WiFiNINA.h>

char ssid[] = "";        // your network SSID (name)
char pass[] = "";    // your network password (use for WPA, or use as key for WEP)

WiFiServer server(80);//port for HTTP
char clientAddress[] = "192.168.4.2";  // connected client address
int port = 80;//port number for HTTP
int wifi_status = WL_IDLE_STATUS;

struct 
{
  int number;
  String data_url;
  String body_string;
  float temperature;
  float humidity;
  unsigned int light;
  unsigned int co2_ppm;
  unsigned int voc_ppm;
}current_sensor,sensor1;

String response_body;//used for sending reponse body


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Access Point Web Server");
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
  wifi_status = WiFi.beginAP(ssid, pass);
  if (wifi_status != WL_AP_LISTENING) {
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
void loop()
{
  WiFiClient client( server.available() );//making a wifi client called client
  if (client.connected())
  {
    ArduinoHttpServer::StreamHttpRequest<1023> httpRequest(client);// Connected to client. Allocate and initialize StreamHttpRequest object.

    
    if (httpRequest.readRequest())// Parse the request. return 1 when request has been broken down
      {

       ArduinoHttpServer::MethodEnum method( ArduinoHttpServer::MethodInvalid );
       method = httpRequest.getMethod();

        ArduinoHttpServer::StreamHttpReply httpReply(client,httpRequest.getContentType());//create http reply object
        if( method == ArduinoHttpServer::MethodGet )//GET request from web client
        {
            //Print URL request from client
            Serial.println("GET Request Received from Web browser");
            Serial.print("data varable selected: ");
            Serial.println( httpRequest.getResource()[0] );

            //send requested data
            response_body= get_response(httpRequest.getResource()[0]);//response body chosen by URL sent using response_bodyt function
            httpReply.send(response_body);
         }
         else if( method == ArduinoHttpServer::MethodPut )//Put Request. print recieved data
         {
           Serial.println("PUT Request Received");
           current_sensor.number = httpRequest.getResource()[0].toInt();
           current_sensor.data_url = httpRequest.getResource()[1];
           current_sensor.body_string = httpRequest.getBody();//retreive the end of url determining data sent by sensor
           buffer_data();//retreive measured data from body string and store it in buffer struct
           store_data();//function stores temporary data stored in sturct to permenant file in SD card
          
          
           
         /*
           Serial.println("String Body from sensor");
           Serial.println(current_sensor.body_string);
           Serial.println("URL sent");
           Serial.println(String(current_sensor.number) + "/" + current_sensor.data_url);
           Serial.print("temperature: ");
           Serial.println(current_sensor.temperature);
           Serial.print("humidity: ");
           Serial.println(current_sensor.humidity);
           Serial.print("co2: ");
           Serial.println(current_sensor.co2_ppm);
           Serial.print("light: ");
           Serial.println(current_sensor.light);
           Serial.println();
           */
          //print recieved data and send ok http repsonse to client
           response_body="Data Received by Server";//
           httpReply.send(response_body); //send ok status
         }
         else{
           Serial.println("Neither GET or PUT");
         }
      }
      else
      {
         // HTTP parsing failed. Client did not provide correct HTTP data or
         // client requested an unsupported feature.
         ArduinoHttpServer::StreamHttpErrorReply httpReply(client, httpRequest.getContentType());
         httpReply.send(httpRequest.getErrorDescrition());
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
String get_response(String url){//function selects correct response body, dependant on the full request URL
  if(url=="all"){
    //send all data varables
    return String("String: ");
  }
  else if (url=="int"){
    return String("int: ");
    }
  else if (url=="float"){
    return String("float " );
    }
  else if (url=="string"){
    return String("string" );
    }
  else if (url==""){
    return String("Successful connection:\nEnter the following URL to read Server data, i.e. http://192.168.4.1/all \n/all: for all varables \n/int: for int \n/String: for string \n/float: for float");
    } 
  else{
    return String("Incorrect URL. Enter http://192.168.4.1 to go back" );
  }
}
void put_response(void){//retrieve data sent by sensor and store it in buffer struct

  //retreive each measurement from body string
  //temperature->humidity->co2->light
  String data1=current_sensor.body_string;
  String data2=data1;
  String temp;
  
  temp= data1.substring(0,data2.indexOf("\n"));
  current_sensor.temperature = temp.toFloat();
  int length = temp.length();
  
  temp = data1.substring(length+1,data2.indexOf("\n",length+1));
  current_sensor.humidity = temp.toFloat();
  length += temp.length();

  temp = data1.substring(length+1,data2.indexOf("\n",length+1));
  current_sensor.co2_ppm = temp.toInt();
  length += temp.length();


  temp = data1.substring(length+1,data2.indexOf("\n",length+1));
  current_sensor.light = temp.toInt();
  length += temp.length();
}
void store_data(void){
  switch (current_sensor.number)
  {
  case 1://data recieved from sensor 1
    
    break;
  case 2://data recieved from sensor 2
  
  default:
    break;
  }
}
void update_file(int sensor){

}