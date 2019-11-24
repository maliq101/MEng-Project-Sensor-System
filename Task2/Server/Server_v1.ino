/*
Sensor sends data using PUT HTTP Reuest over Wifi.
Data briefly stored in struct 
Permanent storage in csv file on sd card
RTC functionallity added 

*/

#include <ArduinoHttpServer.h>
#include <SPI.h> //
#include <WiFiNINA.h>
#include <RTCZero.h>
#include <SD.h>

char ssid[] = "";        // your network SSID (name)
char pass[] = "";    // your network password (use for WPA, or use as key for WEP)
WiFiServer server(80);//port for HTTP
char clientAddress[] = "192.168.4.2";  // connected client address
int port = 80;//port number for HTTP
int wifi_status = WL_IDLE_STATUS;

struct 
{
  String number;
  String body_string;
  float temperature;
  float humidity;
  unsigned int light;
  unsigned int co2;
  unsigned int voc;
  String pir_event;
  String motion_event;
}current_sensor;
String response_body;//used for sending reponse body

struct last_contact_time
{
  byte seconds = 20;
  byte minutes = 59;
  byte hours = 23;
  byte day = 19;
  byte month = 12;
  byte year = 19;
}sensor1,sensor2;


RTCZero rtc;//create an rtc object
const byte seconds = 20;
const byte minutes = 59;
const byte hours = 23;
const byte day = 19;
const byte month = 12;
const byte year = 19;

const int chipSelect = 4;
File file;
void setup() {
  //Serial
  Serial.begin(9600);  //Initialize serial and wait for port to open:
  //RTC
  rtc.begin();//initialize RTC
  rtc.setTime(hours,minutes,seconds);
  rtc.setDate(day,month,year);
  //SD
  if (!SD.begin(4)) {
    Serial.println("SD initialization failed!");
    while (1);
  }
    //GPIO
  pinMode(0,OUTPUT);//RED Communication with sensor via wifi
  pinMode(2,OUTPUT);//green GET request
  pinMode(3,OUTPUT);//red PUT request
 // pinMode(5,OUTPUT);//RED


  //Wifi
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

  delay(10000);// wait 10 seconds for connection:
  server.begin();// start the web server on port 80
  printWiFiStatus();// you're connected now, so print out the status  
  Serial.println("Creating csv files for sensors");
  create_file("sensor1.csv");
  create_file("sensor2.csv");
}

void loop()
{
  digitalWrite(0,LOW);
  WiFiClient client( server.available() );//making a wifi client called client
  if (client.connected())
  {
    digitalWrite(0,HIGH);
    ArduinoHttpServer::StreamHttpRequest<1023> httpRequest(client);// Connected to client. Allocate and initialize StreamHttpRequest object.

    
    if (httpRequest.readRequest())// Parse the request. return 1 when request has been broken down
      {

       ArduinoHttpServer::MethodEnum method( ArduinoHttpServer::MethodInvalid );
       method = httpRequest.getMethod();

        ArduinoHttpServer::StreamHttpReply httpReply(client,httpRequest.getContentType());//create http reply object
        if( method == ArduinoHttpServer::MethodGet )//GET request from web client
        {   
            digitalWrite(2,HIGH);
            //Print URL request from client
            Serial.println("GET Request Received from Web browser");
            Serial.print("data varable selected: ");
            Serial.println( httpRequest.getResource()[0] );

            //send requested data
            response_body= get_response(httpRequest.getResource()[0]);//response body chosen by URL sent using response_bodyt function
            httpReply.send(response_body);
            digitalWrite(2,LOW);
         }
         else if( method == ArduinoHttpServer::MethodPut )//Put Request. print recieved data
         {
           digitalWrite(3,HIGH);
           Serial.println("PUT Request Received");
           current_sensor.number = httpRequest.getResource()[0];//sensor number used for determing what file to store data in
           current_sensor.body_string = httpRequest.getBody();//retreive the end of url determining data sent by sensor
           buffer_data();//retreive measured data from body string and store it in buffer struct
           response_body="Data Received by Server";//
           httpReply.send(response_body); //send ok status
           Serial.println("Recieved PUT String Body: ");
           Serial.println(current_sensor.body_string);
           store_data();//function stores temporary data stored in sturct to permenant file in SD card
           update_sensor_last_contact_time();//update the last confirmed Communication time for the current_sensor
          //print recieved data and send ok http repsonse to client
          digitalWrite(3,LOW);

         }
         else{
           Serial.println("Neither GET or PUT");
         }
      }
      else{//Error
         // HTTP parsing failed. Client did not provide correct HTTP data or
         // client requested an unsupported feature.
         ArduinoHttpServer::StreamHttpErrorReply httpReply(client, httpRequest.getContentType());
         httpReply.send(httpRequest.getErrorDescrition());
      }
   client.stop();
   check_sensor_last_contact_time(1);//check to see if any sensors have been disconnected
   check_sensor_last_contact_time(2);
   }

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
void buffer_data(void){//retrieve data sent by sensor and store it in buffer struct

  //retreive each measurement from body string
  //temperature + \n + humidity + \n + \n + co2 + \n + light
  String data1=current_sensor.body_string;
  String data2=data1;
  String temp;
  temp= data1.substring(0,data2.indexOf("\n"));
  current_sensor.temperature = temp.toFloat();
  int length = temp.length();
  length++;
  
  temp = data1.substring(length,data2.indexOf("\n",length));
  current_sensor.humidity = temp.toFloat();
  length += temp.length();
  length++;

  temp = data1.substring(length,data2.indexOf("\n",length));
  current_sensor.co2 = temp.toInt();
  length += temp.length();
  length++;

  temp = data1.substring(length,data2.indexOf("\n",length));
  current_sensor.light = temp.toInt();
  length += temp.length();
  length++;

  temp = data1.substring(length,data2.indexOf("\n",length));
  current_sensor.pir_event = temp;
  length += temp.length();
  length++;

  temp = data1.substring(length,data2.indexOf("\n",length));
  current_sensor.motion_event = temp;
  length += temp.length();
  length++;
  
}
void store_data(void){
  String filename ="";
  String data = "";
  int sensor_number = current_sensor.number.toInt();
  switch (sensor_number)
  {
  case 1://data recieved from sensor 1
    filename="sensor1.csv";
    break;
  case 2://data recieved from sensor 2
    filename="sensor2.csv"; 
    break;
  default:
    filename= "no file selected";
    break;
  }
  Serial.print("File Selected: ");
  Serial.println(filename);
  data = String(rtc.getDay()) + "/" + String(rtc.getMonth()) + "/" + String(rtc.getYear()) + ","+ String(rtc.getHours()) + ":" + String(rtc.getMinutes()) + ":" + String(rtc.getSeconds()) + "," + String(current_sensor.temperature) + "," + String(current_sensor.humidity) + "," + String(current_sensor.co2) + "," + String(current_sensor.light) + "," + current_sensor.pir_event + "," + current_sensor.motion_event;
  Serial.print("Date to be written to SD: " );
  Serial.println(data);
  file=SD.open(filename,FILE_WRITE);
  if(file){//if file opened Successful
    file.println(data);
    file.close();
  }
  else{//Error
    Serial.println("Error writing data to file");
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
void print_date(void){
  print2digits(rtc.getDay());
  Serial.print("/");
  print2digits(rtc.getMonth());
  Serial.print("/");
  print2digits(rtc.getYear());
}
void print_time(void){
  print2digits(rtc.getHours());
  Serial.print(":");
  print2digits(rtc.getMinutes());
  Serial.print(":");
  print2digits(rtc.getSeconds());
}
void print2digits(int number) {
  if (number < 10) {
    Serial.print("0"); // print a 0 before if the number is < than 10
  }
  Serial.print(number);
}
void update_sensor_last_contact_time(void){
    int sensor_number = current_sensor.number.toInt();
    //select currect last_contact struct
    switch (sensor_number)
    {
    case 1:
      sensor1.seconds=rtc.getSeconds();
      sensor1.minutes=rtc.getMinutes();
      sensor1.hours=rtc.getHours();
      sensor1.day=rtc.getDay();
      sensor1.month=rtc.getMonth();
      sensor1.year=rtc.getYear();
      break;
    case 2:
      sensor2.seconds=rtc.getSeconds();
      sensor2.minutes=rtc.getMinutes();
      sensor2.hours=rtc.getHours();
      sensor2.day=rtc.getDay();
      sensor2.month=rtc.getMonth();
      sensor2.year=rtc.getYear();
      break;
    default:
      break;
    }
  Serial.println("Last contact time of Sensor" + String(sensor_number) + " has been updated");
}
void check_sensor_last_contact_time(int sensor_number){
  signed int last_minute;//last Successful Communication minute
  signed int current_minute=rtc.getMinutes();
  signed int temp;
  switch (sensor_number)
  {
  case 1:
    last_minute=sensor1.minutes;
    break;
  case 2:
    last_minute=sensor2.minutes;
    break;
  default:
    break;
  }
  Serial.print("Contact status of Sensor " + String(sensor_number));
  if((current_minute<1)&&(last_minute>1)){
    //acounting for minute overflow
    temp=current_minute+60;
  }
  else{
    temp=current_minute;
  }
  if(((temp-last_minute)<=2)&&((temp-last_minute)>=0)){
    Serial.println(": OK");   
  }
  else{
    //lost contact with sensor
    Serial.println(": Lost contact");
    switch (sensor_number)
    {
      case 1:
        Serial.println("Last Successful contact at: " + String(sensor1.day) + "/" + String(sensor1.month) + "/" + String(sensor1.year) + "   "+ String(sensor1.hours) + ":" + String(sensor1.minutes) + ":" + String(sensor1.seconds));
        break;
      case 2:
        Serial.println("Last Successful contact at: " + String(sensor2.day) + "/" + String(sensor2.month) + "/" + String(sensor2.year) + "   "+ String(sensor2.hours) + ":" + String(sensor2.minutes) + ":" + String(sensor2.seconds));
        break;
      default:
        break;
    }
  }
}