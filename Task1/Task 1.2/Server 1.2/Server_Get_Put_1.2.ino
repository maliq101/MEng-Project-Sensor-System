/*
Program works with either desktop clinet or arduino client
If GET request received it will send a test int, float, char and string variables to connected clinet
the URL entered in the request will determine what data will be transmitted

if PUT request received it will store data variable
relevant information printed on serial monitor
*/


#include <ArduinoHttpServer.h>
#include <SPI.h> //
#include <WiFiNINA.h>

char ssid[] = "";        // your network SSID (name)
char pass[] = "";    // your network password (use for WPA, or use as key for WEP)

WiFiServer server(80);//port for HTTP
char clientAddress[] = "192.168.4.2";  // connected client address
int port = 80;//port number for HTTP
int status = WL_IDLE_STATUS;
//test variables for GET and PUT functionallity
int test_int=20;
float test_float=3.142;
String test_string=" Test data from server";
char test_char[]={'t','e','s','t','0','1',2,3};
String body;//used for sending reponse body


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
void loop()
{
   WiFiClient client( server.available() );//making a wifi client called client
   if (client.connected())
   {
            ArduinoHttpServer::StreamHttpRequest<1023> httpRequest(client);// Connected to client. Allocate and initialize StreamHttpRequest object.

      
      if (httpRequest.readRequest())// Parse the request. return 1 when request has been broken down
      {
        // Retrieve HTTP resource / URL requested
         //Serial.println( httpRequest.getResource().toString() );

         // Retrieve HTTP method.
         // E.g.: GET (1) / PUT (2)/ HEAD / DELETE / POST
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
            body= response_body(httpRequest.getResource()[0]);//response body chosen by URL sent using response_bodyt function
            httpReply.send(body);
         }
         else if( method == ArduinoHttpServer::MethodPut )//Put Request. print recieved data
         {
           Serial.println("PUT Request Received");
           Serial.println(httpRequest.getBody());//print getBody
           body="Data Received by Server";
           httpReply.send(body); //send ok status
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
String response_body(String url){//function selects correct response body, dependnat on the request URL
  if(url=="all"){
    //send all data varables
    return String("String: " + test_string + "\n" + "int: " + test_int + "\n" + "float: " + test_float + "\n" + "test_char: " + test_char[1] + "_" + test_char[5] );
  }
  else if (url=="int"){
    return String("int: " + String(test_int));
    }
  else if (url=="float"){
    return String("float " + String(test_float,3));
    }
  else if (url=="string"){
    return String("string" + test_string);
    }
  else if (url==""){
    return String("Successful connection:\nEnter the following URL to read Server data, i.e. http://192.168.4.1/all \n/all: for all varables \n/int: for int \n/String: for string \n/float: for float");
    } 
  else{
    return String("Incorrect URL. Enter http://192.168.4.1 to go back" );
  }
}
