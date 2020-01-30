/*
DHCP exmaple code used to request ip adderess and repeatedly renew it vias Ethernet.maintain()

 */

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <ArduinoHttpServer.h>


// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {
  0x84, 0x0D, 0x8E, 0x34, 0x22, 0xB9
};
byte DHCP_ip_status;//used to check ip renewal status from DHCP
EthernetServer server(80);//create a server listinng for connections via HTTP (port 80) 
String response_body;//used for sending reponse body
String http_content_type;//used to determine http reply content type. csv for file downloads or html
File file;

void setup() {

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  if (!SD.begin(4)) {
    Serial.println("SD initialization failed!");
    while (1);
  }
  Serial.println("Creating csv files for sensors");
  create_file("sensor1.csv");
  create_file("sensor2.csv");


  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(1);
    }
  }
  // print your local IP address:
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());

  //Open the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());


}

void loop() {
  // listen for incoming clients
  EthernetClient ethernet_client = server.available();
  if (ethernet_client) {
    ArduinoHttpServer::StreamHttpRequest<1023> httpRequest(ethernet_client);// Connected to client. Allocate and initialize StreamHttpRequest object.
    
    if (httpRequest.readRequest())// Parse the request. return 1 when request has been broken down
      {
      //retrive requesr method i.e. PUT or GET
       ArduinoHttpServer::MethodEnum method( ArduinoHttpServer::MethodInvalid );
       method = httpRequest.getMethod();
       //GET Request via Ethernet
       if( method == ArduinoHttpServer::MethodGet )//GET request from web client
        {   
            //Print URL request from client
            Serial.println("GET Request Received from Web browser");
            Serial.print("data varable selected: ");
            Serial.println( httpRequest.getResource()[0] );

            //send requested data
            response_body= get_response(httpRequest.getResource()[0],httpRequest.getResource()[1]);//response body chosen by URL sent using response_bodyt function
            ArduinoHttpServer::StreamHttpReply httpReply(ethernet_client,http_content_type);//create http reply object
            httpReply.send(response_body);
         }
        //PUT Request via Ethernet
        else if( method == ArduinoHttpServer::MethodPut )
         {
         }
         else{
           Serial.println("Neither GET or PUT");
         }
      }
      else{//Error
         // HTTP parsing failed. Client did not provide correct HTTP data or
         // client requested an unsupported feature.
         ArduinoHttpServer::StreamHttpErrorReply httpReply(ethernet_client, httpRequest.getContentType());
         httpReply.send(httpRequest.getErrorDescrition());
      }

    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    ethernet_client.stop();
    Serial.println("client disconnected");
  }
  DHCP_ip_status=maintain_ip();//maintain ip address via DHCP
}
String get_response(String url_1, String url_2){//function selects correct response body, dependant on the full request URL
  String download_file="";
  String body="";
  char data_on_file;
  if(url_1=="sensor1"){
    http_content_type="text/html";
    body="<p>page will show information on sensor1 </p>";
    body+="<p><a href = \'/sensor1.csv\' target= '_blank'> Click here to download file </a></p>";
    body+="<p><a href = \''> Click here to go back to homepage</a></p>";
    return body;
  }
  else if (url_1=="sensor1.csv"){
    http_content_type="text/csv";
    file=SD.open("sensor1.csv");
    if(file){
      while(file.available()){
        data_on_file=file.read();
        download_file.concat(String(data_on_file));
      }
    }
    return download_file;
    }
  else if (url_1=="faults"){
    http_content_type="text/html";
    return String("This Page will show system faults i.e. lost connection with sensor" );
    }
  else if (url_1=="string"){
    http_content_type="text/html";
    return String("string" );
    }
  else if (url_1==""){
    http_content_type="text/html";
    body="<p>Hompage with links to each sensor in html. will also show latest measurements for each sensor</p>" ;
    body+="<p><a href = \'/sensor1\'> Click here for sensor 1 information</a></p>";
    body+="<p><a href = \'/faults\'> Click here for faults </a></p>" ;
    return body;
    } 
  else{
    http_content_type="text/html";
    return String("Incorrect URL. Enter http://" + String(Ethernet.localIP()) +" to go back" );
  }
}
void create_file(String filename){//function creates a file and adds the table header if required
  char data_on_file;//used to test first char in file
  Serial.print("Creating File: ");
  Serial.println(filename);
  //check to see if file exists
  if(SD.exists(filename)){
    file=SD.open(filename);
    //file exists
    if(file){//if opened Successful
      Serial.print("File Exists. ");
      data_on_file=file.read();//read first char 
      if(data_on_file=='D'){  //header Exists
        Serial.println("Headers exist. ");
      }
      else{//no header
        file.println("Date,Time,Temperature(Celcius),Humiditiy(percent),CO2(ppm),Light,PIR Event,Motion Event");
        Serial.println("Headers did not existed. Added Headers");
      }
      file.close();
    }
    else{//Error
      Serial.println("Error Opening file. File has been created");
    }
  }
  else{ //no File exists
    file=SD.open(filename,FILE_WRITE);
      if(file){//file opened Successful
        Serial.println("File did not exist, File Created");
        file.println("Date,Time,Temperature(Celcius),Humiditiy(percent),CO2(ppm),Light,PIR Event,Motion Event");
        file.close();
      }
      else{
        Serial.println("Error Opening file. File not created");
      }
  }
}

byte maintain_ip(void){
  //maintain ip address
  byte status;
  status=Ethernet.maintain();
  switch (status) {
    case 1:
      //renewed fail
      Serial.println("Error: renewed fail");
      break;

    case 2:
      //renewed success
      Serial.println("Renewed success");
      //print your local IP address:
      Serial.print("My IP address: ");
      Serial.println(Ethernet.localIP());
      break;

    case 3:
      //rebind fail
      Serial.println("Error: rebind fail");
      break;

    case 4:
      //rebind success
      Serial.println("Rebind success");
      //print your local IP address:
      Serial.print("My IP address: ");
      Serial.println(Ethernet.localIP());
      break;

    default:
      //nothing happened
      break;
  }
  return status;


}