
/*
This program includes the measuring capabilities of the TE Ambimate sensor with the previous client code
Periodic communication with the server via HTTP 

sensor communication using i2c
request all mesaured data, which is reaches in a char array

RED=VCC(TOP RIGHT ON sensor)
BLUE=GND
YELLOW=SCL
BLUE=SDA

*/

#include <ArduinoHttpClient.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"
#include <Wire.h>

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
/////// Wifi Settings ///////
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

///Create wificlient object, HTTP client object and relevant variables
char serverAddress[] = "192.168.4.1";  // server address
int port = 80;//port number for HTTP
WiFiClient wifi;//Create a wifi Client object called wifi.
HttpClient client = HttpClient(wifi, serverAddress, port);//create http client object  using the wifi client object
int wifi_status = WL_IDLE_STATUS;
int http_status_code = client.responseStatusCode();
String http_response = client.responseBody();
String http_content_type;
String http_put_data;
String http_url = "/1";//sensor number


//variables for sensor 
unsigned sensor_buf[20];//buffer used to retrive raw data from sensor
float temperature, humidity;
unsigned int sensor_status,light,co2_ppm,voc_ppm;
String pir_event="";
String motion_event="";

int sample_time=5000;//5 seconds

void setup() {
  pinMode(0,OUTPUT);//commuication with server
  pinMode(2,OUTPUT);//RED
  pinMode(3,OUTPUT);//commicating with sensor
  pinMode(5,OUTPUT);//RED
  //attachInterrupt(digitalPinToInterrupt(6), event_out, HIGH);
  Serial.begin(9600);
  Wire.begin();//begin i2c commincation for sensor
  connect_to_wifi();


}

void loop() {
  wifi_status=WiFi.status();
  while ( wifi_status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);
    digitalWrite(5,HIGH);
    // Connect to WPA/WPA2 network:
    wifi_status = WiFi.begin(ssid, pass);
  }
  digitalWrite(5,LOW);
  
  digitalWrite(3,HIGH);
  read_sensor();
  digitalWrite(3,LOW);
  digitalWrite(0,HIGH);
  put_request("data");
  digitalWrite(0,LOW);
  print_response();
  delay(sample_time);
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
void read_sensor(void){
// All sensors except the CO2 sensor are scanned in response to this command
  Wire.beginTransmission(0x2A); // transmit to device
  // Device address is specified in datasheet
  Wire.write(byte(0xC0));       // sends instruction to read sensors in next byte
  Wire.write(byte(0x7F));    // 0x7F indicates to read all connected sensors
  Wire.endTransmission();       // stop transmitting
  // Delay to make sure all sensors are scanned by the AmbiMate
  delay(100);

  Wire.beginTransmission(0x2A); // transmit to device
  Wire.write(byte(0x00));       // sends instruction to read sensors in next byte
  Wire.endTransmission();       // stop transmitting
  Wire.requestFrom(0x2A, 15);   // request bytes from slave device

  // Acquire the Raw Data
  unsigned int i = 0;
  while (Wire.available()) { // slave may send less than requested
    sensor_buf[i] = Wire.read(); // receive a byte as character
    i++;
  }

  // convert the raw data to engineering units
  sensor_status = sensor_buf[0];
  temperature = (sensor_buf[1] * 256.0 + sensor_buf[2]) / 10.0;
  humidity = (sensor_buf[3] * 256.0 + sensor_buf[4]) / 10.0;
  light = (sensor_buf[5] * 256.0 + sensor_buf[6]);
  co2_ppm = (sensor_buf[11] * 256.0 + sensor_buf[12]);
  voc_ppm = (sensor_buf[13] * 256.0 + sensor_buf[14]);
  if (sensor_status & 0x80){
    pir_event="PIR EVENT";
  }
  else{
    pir_event="";
  }
  if (sensor_status & 0x01){
    motion_event="MOTION_EVENT EVENT";
  }
  else{
    motion_event="";
  }
}
void put_request(String data){
  Serial.println("making PUT request");
  http_content_type = "text/plain";
  if(data == "display"){//send all data as String
    http_put_data = String("Temperature (Celsius): " + String(temperature) + "\n" + "Humidtiy:" + String(humidity) + "\n"  + "co2 (ppm)" + String(co2_ppm) + "\n" + "Light:" + String(light));
    http_url+=String("/display");
  }
  else if(data == "data"){
    http_put_data = String(String(temperature) + "\n" + String(humidity) + "\n"  +  String(co2_ppm) + "\n" + String(light) +"\n" + pir_event +"\n" +motion_event +"\n");
    http_url+=String("/data");  
  }
  wifi_status=WiFi.status();
  if( wifi_status == WL_CONNECTED){
  client.put(http_url, http_content_type, http_put_data);
  http_status_code = client.responseStatusCode();
  http_response = client.responseBody();
  }
}
void print_response(void){
  Serial.print("Status code: ");
  Serial.println(http_status_code);
  Serial.print("Response: ");
  Serial.println(String(http_response + "\n"));

  Serial.println("Measured Data: ");
  Serial.println(String("Temperature (Celsius): " + String(temperature) + "\n" + "Humidtiy:" + String(humidity) + "\n"  + "co2 (ppm)" + co2_ppm + "\n" + "Light:" + light + "\n" + "PIR Event" + pir_event + "\n" + "Motion Event" + motion_event + "\n"));

  Serial.println("Wait five seconds");
}
//void event_out(){
  //digitalWrite(2,HIGH);
  //detachInterrupt(digitalPinToInterrupt(6));
  //read_sensor();
  //put_request("data");


  
//}