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
  byte seconds;
  byte minutes;
  byte hours;
  byte day;
  byte month;
  byte year;
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
}sensor4,sensor5,sensor6;

RTCZero rtc;//create an rtc object
const byte seconds = 00;
const byte minutes = 55;
const byte hours = 23;
const byte day = 07;
const byte month = 02;
const byte year = 20;

const int chipSelect = 4;
File file;
void setup() {
  //RTC
  rtc.begin();//initialize RTC
  rtc.setTime(hours,minutes,seconds);
  rtc.setDate(day,month,year);
  //SD
  if (!SD.begin(4)) {
    while (1);
  }
    //GPIO
  pinMode(0,OUTPUT);//RED Communication with sensor via wifi
  pinMode(2,OUTPUT);//green GET request
  pinMode(3,OUTPUT);//red PUT request
 // pinMode(5,OUTPUT);//RED

  //Wifi
  //while (!Serial) {
  //  ; // wait for serial port to connect. Needed for native USB port only
  //}
  // Create open network. Change this line if you want to create an WEP network:
  wifi_status = WiFi.beginAP(ssid);
  if (wifi_status != WL_AP_LISTENING) {
    // don't continue
    while (true);
  }

  delay(10000);// wait 10 seconds for connection:
  server.begin();// start the web server on port 80
  create_file("sensor4.csv");
  create_file("sensor4F.csv");
  create_file("sensor5.csv");
  create_file("sensor5F.csv");
  create_file("sensor6.csv");
  create_file("sensor6F.csv");
  sensor4.location=LOCATION_4;
  sensor5.location=LOCATION_5;
  sensor6.location=LOCATION_6;
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

           current_sensor.number = httpRequest.getResource()[0];//sensor number used for determing what file to store data in
           current_sensor.body_string = httpRequest.getBody();//retreive the end of url determining data sent by sensor
           buffer_data();//retreive measured data from body string and store it in buffer struct
           response_body="Data Received by Server";//
           httpReply.send(response_body); //send ok status
           store_data();//function stores temporary data stored in sturct to permenant file in SD card
           update_sensor_last_contact_time();//update the last confirmed Communication time for the current_sensor
          //print recieved data and send ok http repsonse to client
           digitalWrite(0,LOW);
           check_sensor_last_contact_time(4);//check to see if any sensors have been disconnected
           sensor4.contact_time= String(sensor4.day) + "/" + String(sensor4.month) + "/" + String(sensor4.year) + "   "+ String(sensor4.hours) + ":" + String(sensor4.minutes) + ":" + String(sensor4.seconds);
           check_sensor_last_contact_time(5);
           sensor5.contact_time= String(sensor5.day) + "/" + String(sensor5.month) + "/" + String(sensor5.year) + "   "+ String(sensor5.hours) + ":" + String(sensor5.minutes) + ":" + String(sensor5.seconds);
           check_sensor_last_contact_time(6);
           sensor6.contact_time= String(sensor6.day) + "/" + String(sensor6.month) + "/" + String(sensor6.year) + "   "+ String(sensor6.hours) + ":" + String(sensor6.minutes) + ":" + String(sensor6.seconds);
         if(sensor4.status=="Error"||sensor5.status=="Error"||sensor6.status=="Error"){
           digitalWrite(1,HIGH);
            }
         else{
           digitalWrite(1,LOW);
         }
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
  if(url_1=="sensor4"){
    http_content_type="text/html";
    body ="<p>Latest measurements from Sensor 4</p>";
    body+="<table> <tr> <th> Date and Time </th> <th> Temperature (C)</th> <th> Humiditiy (Percent) </th> <th> Light (Lux) </th> <th> CO2 (ppm) </th> <th> PIR Event </th> <th> Motion Event </th> <th> Battery Voltage (V) </th></tr>";
    body+="<tr> <td>" + sensor4.contact_time + "</td> <td>" + sensor4.temperature + "</td> <td>"+ sensor4.humidity + "</td> <td>"+ sensor4.light + "</td> <td>"+ sensor4.co2 + "</td> <td>"+ sensor4.pir_event + "</td> <td>"+ sensor4.motion_event + "</td> <td>" + sensor4.bat_volt + "</td> </tr> </table>";
    body+="<p><a href = \'/sensor4.csv\' target= '_blank'> Click here for .csv file </a></p>";
    body+="<p><a href = \'homepage\'> Click here to go back to homepage</a></p>";
    return body;
  }
  else if(url_1=="sensor5"){
    http_content_type="text/html";
    body ="<p>Latest measurement from Sensor 5</p>";
    body+="<table> <tr> <th> Date and Time </th> <th> Temperature (C)</th> <th> Humiditiy (Percent) </th> <th> Light (Lux) </th> <th> CO2 (ppm) </th> <th> PIR Event </th> <th> Motion Event </th> <th> Battery Voltage (V) </th> </tr>";
    body+="<td>" + sensor5.contact_time + "</td> <td>" + sensor5.temperature + "</td> <td>"+ sensor5.humidity + "</td> <td>"+ sensor5.light + "</td> <td>"+ sensor5.co2 + "</td> <td>"+ sensor5.pir_event + "</td> <td>"+ sensor5.motion_event + "</td> <td>" + sensor5.bat_volt + "</td> </tr> </table>";
    body+="<p><a href = \'/sensor5.csv\' target= '_blank'> Click here for .csv file </a></p>";
    body+="<p><a href = \'homepage\'> Click here to go back to homepage</a></p>";
    return body;
  }
  else if(url_1=="sensor6"){
    http_content_type="text/html";
    body ="<p>Latest measurements from Sensor 6</p>";
    body+="<table> <tr> <th> Date and Time </th> <th> Temperature (C)</th> <th> Humiditiy (Percent) </th> <th> Light (Lux) </th> <th> CO2 (ppm) </th> <th> PIR Event </th> <th> Motion Event </th> <th> Battery Voltage (V) </th></tr>";
    body+="<tr> <td>" + sensor6.contact_time + "</td> <td>" + sensor6.temperature + "</td> <td>"+ sensor6.humidity + "</td> <td>"+ sensor6.light + "</td> <td>"+ sensor6.co2 + "</td> <td>"+ sensor6.pir_event + "</td> <td>"+ sensor6.motion_event + "</td> <td>" + sensor6.bat_volt + "</td> </tr> </table>";
    body+="<p><a href = \'/sensor6.csv\' target= '_blank'> Click here for .csv file </a></p>";
    body+="<p><a href = \'homepage\'> Click here to go back to homepage</a></p>";
    return body;
  }
  else if (url_1=="sensor4.csv"){
    http_content_type="text/csv";
    file=SD.open("sensor4.csv");
    if(file){
      while(file.available()){
        data_on_file=file.read();
        download_file.concat(String(data_on_file));
      }
    }
    SD.remove("sensor4.csv");
    create_file("sensor4.csv");
    return download_file;
  }
  else if (url_1=="sensor5.csv"){
    http_content_type="text/csv";
    file=SD.open("sensor5.csv");
    if(file){
      while(file.available()){
        data_on_file=file.read();
        download_file.concat(String(data_on_file));
      }
    }
    SD.remove("sensor5.csv");
    create_file("sensor5.csv");
    return download_file;
  }
    else if (url_1=="sensor6.csv"){
    http_content_type="text/csv";
    file=SD.open("sensor6.csv");
    if(file){
      while(file.available()){
        data_on_file=file.read();
        download_file.concat(String(data_on_file));
      }
    }
    SD.remove("sensor6.csv");
    create_file("sensor6.csv");
    return download_file;
  }
  else if (url_1==""){
    http_content_type="text/html";
    body="<p>Summary of all Sensors</p>" ;
    body+="<p><a href = \'/sensor4\'> Click here for sensor 4 information</a></p>";
    body+="<p><a href = \'/sensor5\'> Click here for sensor 5 information</a></p>";
    body+="<p><a href = \'/sensor6\'> Click here for sensor 6 information</a></p>";
    body+="<table> <tr> <th> Sensor Number </th> <th> Location </th> <th> Status </th> <th> Time of Last Communication </th> <th> Battery Voltage (V) </th> <th> File Size (B) </th> </tr>";
    body+="<tr> <td> 4 </td> <td>" + sensor4.location + "</td> <td>" + sensor4.status + "</td> <td>" + sensor4.contact_time + "</td> <td>" + sensor4.bat_volt +"</td> <td>" + sensor4.file_size + "</td> </tr>";
    body+="<tr> <td> 5 </td> <td>" + sensor5.location + "</td> <td>" + sensor5.status + "</td> <td>" + sensor5.contact_time + "</td> <td>" + sensor5.bat_volt +"</td> <td>" + sensor5.file_size + "</td> </tr>";
    body+="<tr> <td> 6 </td> <td>" + sensor6.location + "</td> <td>" + sensor6.status + "</td> <td>" + sensor6.contact_time + "</td> <td>" + sensor6.bat_volt +"</td> <td>" + sensor6.file_size + "</td> </tr> </table>";

    return body;
    } 
  else if (url_1=="homepage"){
    http_content_type="text/html";
    body="<p>Summary of all Sensors</p>" ;
    body+="<p><a href = \'/sensor4\'> Click here for sensor 4 information</a></p>";
    body+="<p><a href = \'/sensor5\'> Click here for sensor 5 information</a></p>";
    body+="<p><a href = \'/sensor6\'> Click here for sensor 6 information</a></p>";
    body+="<table> <tr> <th> Sensor Number </th> <th> Location </th> <th> Status </th> <th> Time of Last Communication </th> <th> Battery Voltage (V) </th> <th> File Size (B) </th> </tr>";
    body+="<tr> <td> 4 </td> <td>" + sensor4.location + "</td> <td>" + sensor4.status + "</td> <td>" + sensor4.contact_time + "</td> <td>" + sensor4.bat_volt +"</td> <td>" + sensor4.file_size + "</td> </tr>";
    body+="<tr> <td> 5 </td> <td>" + sensor5.location + "</td> <td>" + sensor5.status + "</td> <td>" + sensor5.contact_time + "</td> <td>" + sensor5.bat_volt +"</td> <td>" + sensor5.file_size + "</td> </tr>";
    body+="<tr> <td> 6 </td> <td>" + sensor6.location + "</td> <td>" + sensor6.status + "</td> <td>" + sensor6.contact_time + "</td> <td>" + sensor6.bat_volt +"</td> <td>" + sensor6.file_size + "</td> </tr> </table>";
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
  byte minute=rtc.getMinutes();
  byte hour=rtc.getHours();
  int sensor_number = current_sensor.number.toInt();
  switch (sensor_number)
  {
  case 4://data recieved from sensor 1
    filename="sensor4.csv";
    filenameF="sensor4F.csv";
    sensor4.temperature= String(current_sensor.temperature);
    sensor4.humidity= String(current_sensor.humidity);
    sensor4.light= String(current_sensor.light);
    sensor4.co2 = String(current_sensor.co2);
    sensor4.pir_event= String(current_sensor.pir_event);
    sensor4.motion_event= String(current_sensor.motion_event);
    sensor4.bat_volt=String(current_sensor.bat_volt);

    file=SD.open(filename,FILE_READ);
    sensor4.file_size=file.size();
    file.close();
    break;
  case 5://data recieved from sensor 2
    filename="sensor5.csv"; 
    filenameF="sensor5F.csv";
    sensor5.temperature= String(current_sensor.temperature);
    sensor5.humidity= String(current_sensor.humidity);
    sensor5.light= String(current_sensor.light);
    sensor5.co2 = String(current_sensor.co2);
    sensor5.pir_event= String(current_sensor.pir_event);
    sensor5.motion_event= String(current_sensor.motion_event);
    sensor5.bat_volt=String(current_sensor.bat_volt);
    
    file=SD.open(filename,FILE_READ);
    sensor5.file_size=file.size();
    file.close();
    break;
  case 6://data recieved from sensor 3
    filename="sensor6.csv";
    filenameF="sensor6F.csv";
    sensor6.temperature= String(current_sensor.temperature);
    sensor6.humidity= String(current_sensor.humidity);
    sensor6.light= String(current_sensor.light);
    sensor6.co2 = String(current_sensor.co2);
    sensor6.pir_event= String(current_sensor.pir_event);
    sensor6.motion_event= String(current_sensor.motion_event);
    sensor6.bat_volt=String(current_sensor.bat_volt);

    file=SD.open(filename,FILE_READ);
    sensor6.file_size=file.size();
    file.close();
    break;
  default:
    break;
  }
  if((hour==23&&minute>=55)||(hour==0&&minute<5)){
    data= String(rtc.getDay()) + "/" + String(rtc.getMonth()) + "/" + String(rtc.getYear()) + ","; 
  }
  else{
    data=",";
  }
  data += String(rtc.getHours()) + ":" + String(rtc.getMinutes()) + ":" + String(rtc.getSeconds()) + "," + String(current_sensor.temperature) + "," + String(current_sensor.humidity) + "," + String(current_sensor.co2) + "," + String(current_sensor.light) + "," + current_sensor.pir_event + "," + current_sensor.motion_event + "," + current_sensor.bat_volt;
  //store in short file
  file=SD.open(filename,FILE_WRITE);

    file.println(data);
    file.close();

  //store in full file
  file=SD.open(filenameF,FILE_WRITE);

    file.println(data);
    file.close();
  
}
void create_file(String filename){//function creates a file and adds the table header if required
  char data_on_file;//used to test first char in file
    //check to see if file exists
  if(SD.exists(filename)){
    file=SD.open(filename);
    //file exists
    if(file){//if opened Successful
      data_on_file=file.read();//read first char 
      if(data_on_file!='D'){//no header Exists
        file.println("Date,Time,Temperature (Celcius),Humiditiy (percent),CO2 (ppm),Light (Lux) ,PIR Event,Motion Event,Battery Voltage (V)");
      }
    }
    file.close();
  }
  else{ //no File exists
    file=SD.open(filename,FILE_WRITE);
      if(file){//file opened Successful
        file.println("Date,Time,Temperature (Celcius),Humiditiy (percent),CO2 (ppm),Light (ppm),PIR Event,Motion Event,Battery Voltage (V)");
      }
  file.close();
  }
}
void update_sensor_last_contact_time(void){
    int sensor_number = current_sensor.number.toInt();
    //select currect last_contact struct
    switch (sensor_number)
    {
    case 4:
      sensor4.seconds=rtc.getSeconds();
      sensor4.minutes=rtc.getMinutes();
      sensor4.hours=rtc.getHours();
      sensor4.day=rtc.getDay();
      sensor4.month=rtc.getMonth();
      sensor4.year=rtc.getYear();
      break;
    case 5:
      sensor5.seconds=rtc.getSeconds();
      sensor5.minutes=rtc.getMinutes();
      sensor5.hours=rtc.getHours();
      sensor5.day=rtc.getDay();
      sensor5.month=rtc.getMonth();
      sensor5.year=rtc.getYear();
      break;
    case 6:
      sensor6.seconds=rtc.getSeconds();
      sensor6.minutes=rtc.getMinutes();
      sensor6.hours=rtc.getHours();
      sensor6.day=rtc.getDay();
      sensor6.month=rtc.getMonth();
      sensor6.year=rtc.getYear();
      break;
    default:
      break;
    }
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
  case 4:
    last_minute=sensor4.minutes;
    file_size=sensor4.file_size;
    volt=sensor4.bat_volt.toFloat();
    break;
  case 5:
    last_minute=sensor5.minutes;
    file_size=sensor5.file_size;
    volt=sensor5.bat_volt.toFloat();
    break;
  case 6:
    last_minute=sensor6.minutes;
    file_size=sensor6.file_size;
    volt=sensor6.bat_volt.toFloat();
    break;
  default:
    break;
  }
  if((current_minute<1)&&(last_minute>1)){
    //acounting for minute overflow
    temp=current_minute+60;
  }
  else{
    temp=current_minute;
  }
  if((((temp-last_minute)<=2)&&((temp-last_minute)>=0))&&(volt>=2)&&(file_size<=6000)){
    status="OK";
  }
  else{
    //lost contact with sensor
    status="Error";
  }
 switch (sensor_number)
  {
  case 4:
    sensor4.status=status;
    break;
  case 5:
    sensor5.status=status;
    break;
  case 6:
    sensor6.status=status;
    break;
  default:
    break;
  }
}