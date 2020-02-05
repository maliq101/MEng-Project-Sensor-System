#include <ArduinoLowPower.h>

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
#include "config.h"

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
  float bat_volt;
  unsigned int light;
  unsigned int co2;
  unsigned int voc;
  byte pir_event;
  byte motion_event;
}current_sensor;//struct use for temporary storage of measurements
String response_body;//used for sending reponse body
String http_content_type;//used to determine http reply content type. csv for file downloads or html

struct sensor_information // struct used for stroing all information releveant for each sensor unit
{
  byte seconds = 20;
  byte minutes = 59;
  byte hours = 23;
  byte day = 19;
  byte month = 12;
  byte year = 19;
  String contact_time ="";
  String status;
  String location;
  String temperature;
  String humidity;
  String light;
  String co2;
  String voc;
  String pir_event;
  String motion_event;   
  String bat_volt;
  unsigned long file_size;
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
  create_file("sensor1F.csv");
  create_file("sensor2.csv");
  create_file("sensor2F.csv");
  sensor1.location=LOCATION_1;
  sensor2.location=LOCATION_2;
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
        if( method == ArduinoHttpServer::MethodGet )//GET request from web client
        {   
            digitalWrite(2,HIGH);
            //Print URL request from client
            Serial.println("GET Request Received from Web browser");
            Serial.print("data varable selected: ");
            Serial.println( httpRequest.getResource()[0] );

            //send requested data
            response_body= get_response(httpRequest.getResource()[0]);//response body chosen by URL sent using response_bodyt function
            ArduinoHttpServer::StreamHttpReply httpReply(client,http_content_type);//create http reply object
            httpReply.send(response_body);
            digitalWrite(2,LOW);
         }
         else if( method == ArduinoHttpServer::MethodPut )//Put Request. print recieved data
         {
           ArduinoHttpServer::StreamHttpReply httpReply(client,"text/plain");//create http reply object
           digitalWrite(0,HIGH);
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
           digitalWrite(0,LOW);
           check_sensor_last_contact_time(1);//check to see if any sensors have been disconnected
           sensor1.contact_time= String(sensor1.day) + "/" + String(sensor1.month) + "/" + String(sensor1.year) + "   "+ String(sensor1.hours) + ":" + String(sensor1.minutes) + ":" + String(sensor1.seconds);
           check_sensor_last_contact_time(2);
           sensor2.contact_time= String(sensor2.day) + "/" + String(sensor2.month) + "/" + String(sensor2.year) + "   "+ String(sensor2.hours) + ":" + String(sensor2.minutes) + ":" + String(sensor2.seconds);
         if(sensor1.status=="Error"||sensor2.status=="Error"){
           digitalWrite(1,HIGH);
            }
         else{
           digitalWrite(1,LOW);
         }
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
  }
}
String get_response(String url_1){//function selects correct response body, dependant on the full request URL
  String download_file="";//string stores .csv file to be returned
  String body="";//string stores HTML code to be returned
  char data_on_file;
  if(url_1=="sensor1"){
    http_content_type="text/html";
    body ="<p>Latest measurements from Sensor 1</p>";
    body+="<table> <tr> <th> Date and Time </th> <th> Temperature (C)</th> <th> Humiditiy (Percent) </th> <th> Light (Lux) </th> <th> CO2 (ppm) </th> <th> PIR Event </th> <th> Motion Event </th> <th> Battery Voltage (V) </th></tr>";
    body+="<tr> <td>" + sensor1.contact_time + "</td> <td>" + sensor1.temperature + "</td> <td>"+ sensor1.humidity + "</td> <td>"+ sensor1.light + "</td> <td>"+ sensor1.co2 + "</td> <td>"+ sensor1.pir_event + "</td> <td>"+ sensor1.motion_event + "</td> <td>" + sensor1.bat_volt + "</td> </tr> </table>";
    body+="<p><a href = \'/sensor1.csv\' target= '_blank'> Click here for .csv file </a></p>";
    body+="<p><a href = \'homepage\'> Click here to go back to homepage</a></p>";
    return body;
  }
  if(url_1=="sensor2"){
    http_content_type="text/html";
    body ="<p>Latest measurement from Sensor 2</p>";
    body+="<table> <tr> <th> Date and Time </th> <th> Temperature (C)</th> <th> Humiditiy (Percent) </th> <th> Light (Lux) </th> <th> CO2 (ppm) </th> <th> PIR Event </th> <th> Motion Event </th> <th> Battery Voltage (V) </th> </tr>";
    body+="<td>" + sensor2.contact_time + "</td> <td>" + sensor2.temperature + "</td> <td>"+ sensor2.humidity + "</td> <td>"+ sensor2.light + "</td> <td>"+ sensor2.co2 + "</td> <td>"+ sensor2.pir_event + "</td> <td>"+ sensor2.motion_event + "</td> <td>" + sensor2.bat_volt + "</td> </tr> </table>";
    body+="<p><a href = \'/sensor2.csv\' target= '_blank'> Click here for .csv file </a></p>";
    body+="<p><a href = \'homepage\'> Click here to go back to homepage</a></p>";
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
    SD.remove("sensor1.csv");
    create_file("sensor1.csv");
    return download_file;
  }
  else if (url_1=="sensor2.csv"){
    http_content_type="text/csv";
    file=SD.open("sensor2.csv");
    if(file){
      while(file.available()){
        data_on_file=file.read();
        download_file.concat(String(data_on_file));
      }
    }
    SD.remove("sensor2.csv");
    create_file("sensor2.csv");
    return download_file;
  }
  else if (url_1==""){
    http_content_type="text/html";
    body="<p>Summary of all Sensors</p>" ;
    body+="<p><a href = \'/sensor1\'> Click here for sensor 1 information</a></p>";
    body+="<p><a href = \'/sensor2\'> Click here for sensor 2 information</a></p>";
    body+="<table> <tr> <th> Sensor Number </th> <th> Location </th> <th> Status </th> <th> Time of Last Communication </th> <th> Battery Voltage (V) </th> <th> File Size (B) </td> </tr>";
    body+="<tr> <td> 1 </td> <td>" + sensor1.location + "</td> <td>" + sensor1.status + "</td> <td>" + sensor1.contact_time + "</td> <td>" + sensor1.bat_volt +"</td> <td>" + sensor1.file_size + "</td> </tr>";
    body+="<tr> <td> 2 </td> <td>" + sensor2.location +  "</td> <td>" + sensor2.status + "</td> <td>" + sensor2.contact_time + "</td> <td>" + sensor2.bat_volt +"</td> <td>" + sensor2.file_size + "</td> </tr> </table>";

    return body;
    } 
  else if (url_1=="homepage"){
    http_content_type="text/html";
    body="<p>Summary of all Sensors</p>" ;
    body+="<p><a href = \'/sensor1\'> Click here for sensor 1 information</a></p>";
    body+="<p><a href = \'/sensor2\'> Click here for sensor 2 information</a></p>";
    body+="<table> <tr> <th> Sensor Number </th> <th> Location </th> <th> Status </th> <th> Time of Last Communication </th> <th> Battery Voltage (V) </th> <th> File Size (B) </td> </tr>";
    body+="<tr> <td> 1 </td> <td>" + sensor1.location + "</td> <td>" + sensor1.status + "</td> <td>" + sensor1.contact_time + "</td> <td>" + sensor1.bat_volt +"</td> <td>" + sensor1.file_size + "</td> </tr>";
    body+="<tr> <td> 2 </td> <td>" + sensor2.location +  "</td> <td>" + sensor2.status + "</td> <td>" + sensor2.contact_time + "</td> <td>" + sensor2.bat_volt +"</td> <td>" + sensor2.file_size + "</td> </tr> </table>";

    return body;
    } 
  else{
    http_content_type="text/html";
    body="An Incorrect URL has been Entered";
    body+="<p><a href = \'homepage\'> Click here to return to Homepage";
    return body;
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
  if(temp=="PIR EVENT"){
    current_sensor.pir_event=1;
  }
  else{
    current_sensor.pir_event=0;
  }
  length += temp.length();
  length++;

  temp = data1.substring(length,data2.indexOf("\n",length));
  if(temp=="MOTION_EVENT EVENT"){
    current_sensor.motion_event=1;
  }
  else{
    current_sensor.motion_event=0;
  }
  length += temp.length();
  length++;

  temp = data1.substring(length,data2.indexOf("\n",length));
  current_sensor.bat_volt = temp.toFloat();
  length += temp.length();
  length++;
  
}
void store_data(void){
  String filename ="";//file of most recent data not sent 
  String filenameF="";//full file
  String data = "";
  byte previous_day;
  byte day=rtc.getDay();
  int sensor_number = current_sensor.number.toInt();
  switch (sensor_number)
  {
  case 1://data recieved from sensor 1
    filename="sensor1.csv";
    filenameF="sensor1F.csv";
    sensor1.temperature= String(current_sensor.temperature);
    sensor1.humidity= String(current_sensor.humidity);
    sensor1.light= String(current_sensor.light);
    sensor1.co2 = String(current_sensor.co2);
    sensor1.pir_event= String(current_sensor.pir_event);
    sensor1.motion_event= String(current_sensor.motion_event);
    sensor1.bat_volt=String(current_sensor.bat_volt);
    previous_day=sensor1.day;

    file=SD.open(filename,FILE_READ);
    sensor1.file_size=file.size();
    file.close();
    break;
  case 2://data recieved from sensor 2
    filename="sensor2.csv"; 
    filenameF="sensor2F.csv";
    sensor2.temperature= String(current_sensor.temperature);
    sensor2.humidity= String(current_sensor.humidity);
    sensor2.light= String(current_sensor.light);
    sensor2.co2 = String(current_sensor.co2);
    sensor2.pir_event= String(current_sensor.pir_event);
    sensor2.motion_event= String(current_sensor.motion_event);
    sensor2.bat_volt=String(current_sensor.bat_volt);
    previous_day=sensor2.day;
    
    file=SD.open(filename,FILE_READ);
    sensor1.file_size=file.size();
    file.close();
    break;
  default:
    filename= "no file selected";
    filenameF= "no file selected";
    break;
  }
  Serial.print("File Selected: ");
  Serial.print(filename);
  Serial.print(", ");
  Serial.println(filenameF);
  if(day!=previous_day){
    data= String(rtc.getDay()) + "/" + String(rtc.getMonth()) + "/" + String(rtc.getYear()) + ","; 
  }
  else{
    data=",";
  }
  data += String(rtc.getHours()) + ":" + String(rtc.getMinutes()) + ":" + String(rtc.getSeconds()) + "," + String(current_sensor.temperature) + "," + String(current_sensor.humidity) + "," + String(current_sensor.co2) + "," + String(current_sensor.light) + "," + current_sensor.pir_event + "," + current_sensor.motion_event + "," + current_sensor.bat_volt;
  Serial.print("Date to be written to SD: " );
  Serial.println(data);
  //store in short file
  file=SD.open(filename,FILE_WRITE);
  if(file){//if file opened Successful
    file.println(data);
    file.close();
    Serial.println("success");
  }
  else{//Error
    Serial.println("Error writing data to file");
  }
  //store in full file
  file=SD.open(filenameF,FILE_WRITE);
  if(file){//if file opened Successful
    file.println(data);
    file.close();
    Serial.println("success");
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
        file.println("Date,Time,Temperature (Celcius),Humiditiy (percent),CO2 (ppm),Light (Lux) ,PIR Event,Motion Event,Battery Voltage (V)");
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
        file.println("Date,Time,Temperature (Celcius),Humiditiy (percent),CO2 (ppm),Light (ppm),PIR Event,Motion Event,Battery Voltage (V)");
        file.close();
      }
      else{
        Serial.println("Error Opening file. File not created");
      }
  }
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
  float volt;
  unsigned long file_size;
  String status="";
  switch (sensor_number)
  {
  case 1:
    last_minute=sensor1.minutes;
    file_size=sensor1.file_size;
    volt=sensor1.bat_volt.toFloat();
    break;
  case 2:
    last_minute=sensor2.minutes;
    file_size=sensor2.file_size;
    volt=sensor2.bat_volt.toFloat();
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
  if((((temp-last_minute)<=2)&&((temp-last_minute)>=0))&&(volt>=2)&&(file_size<=6000)){
    Serial.println(": OK");
    status="OK";
  }
  else{
    //lost contact with sensor
    Serial.println(": Lost contact");
    status="Error";
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
  Serial.println("/////////");
  Serial.println();
  }
  switch (sensor_number)
  {
  case 1:
    sensor1.status=status;
    break;
  case 2:
    sensor2.status=status;
    break;
  default:
    break;
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

  Serial.print("MAC Address:" );
  byte mac[6];
  WiFi.macAddress(mac);
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
  Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();

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