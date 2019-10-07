// Changes made to the original example file fom TE. 
//Version B
//Please refer to 114-13315 for connection diagram
#include <Wire.h>

unsigned char buf[20];

unsigned char opt_sensors;
int incomingByte = 0;
int loopCount = 0;
char showTemp = 0, showHum = 0, showLight = 0, showSound = 0, degCorf = 0, showCO2 = 0, showVOC = 0, showPwr = 0, showEvents = 0;
String sampPeriodTxt;

float sampPeriod = 1;

void setup() {
  // put your setup code here, to run once:
  Wire.begin(); // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
}

void restart_info() {

  // Data and basic information are acquired from the module
  Wire.beginTransmission(0x2A); // transmit to device
  Wire.write(byte(0x80));       // sends instruction to read firmware version
  Wire.endTransmission();       // stop transmitting
  Wire.requestFrom(0x2A, 1);    // request 1 byte from slave device
  unsigned char fw_ver = Wire.read(); // receive a byte

  Wire.beginTransmission(0x2A); // transmit to device
  Wire.write(byte(0x81));       // sends instruction to read firmware subversion
  Wire.endTransmission();       // stop transmitting
  Wire.requestFrom(0x2A, 1);    // request 1 byte from slave device
  unsigned char fw_sub_ver = Wire.read(); // receive a byte

  Wire.beginTransmission(0x2A); // transmit to device
  Wire.write(byte(0x82));       // sends instruction to read optional sensors byte
  Wire.endTransmission();       // stop transmitting
  Wire.requestFrom(0x2A, 1);    // request 1 byte from slave device
  opt_sensors = Wire.read(); // receive a byte

  delay(1000);

  Serial.print("AmbiMate sensors: 4 core");
  if (opt_sensors & 0x01)
    Serial.print(" + CO2");
  if (opt_sensors & 0x04)
    Serial.print(" + Audio");
  Serial.println(" ");

  Serial.print("AmbiMate Firmware version ");
  Serial.print(fw_ver);
  Serial.print(".");
  Serial.println(fw_sub_ver);
  Serial.println("Arduino ino ver: A.2");
  Serial.println("");
  Serial.println("");

    sampPeriod = 0.5;
  sampPeriod = sampPeriod * 1000;   // convert to mSecs
}

//Top line of headings are printed using the following
void printLabels(void) {

  Serial.println(" ");
  Serial.println(" ");
  // Construct the first line of the headings
    Serial.print("Temperature\t");
    Serial.print("Humidity\t");
    Serial.print("Light\t");
    Serial.print("eCO2\t");
    Serial.print("VOC\t");
    Serial.print("Power\t");
    Serial.print("Event\n");
  // Construct the second line of the headings
    Serial.print("C\t\t");
    Serial.print("%\t\t");
    Serial.print("Lux\t");
    Serial.print("PPM\t");
    Serial.print("PPB\t");
    Serial.print("volts\t");
  Serial.print("\n");
}

//Loop starts here
void loop() {

  if (loopCount == 0)
  {
    restart_info();
    loopCount = 1;
  }
  if (loopCount == 1)
  {
    printLabels();
    loopCount = 2;
  }
  // All sensors except the CO2 sensor are scanned in response to this command
  Wire.beginTransmission(0x2A); // transmit to device
  // Device address is specified in datasheet
  Wire.write(byte(0xC0));       // sends instruction to read sensors in next byte
  if (opt_sensors & 0x01)       // If gas sensor is installed, include it in the data request
    Wire.write(byte(0x7F));    // 0x7F indicates to read all connected sensors
  else
    Wire.write(byte(0x3F));    // exclude gas sensor from data request
  // 0xFF indicates to read all connected sensors
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
    buf[i] = Wire.read(); // receive a byte as character
    i++;
  }

  // convert the raw data to engineering units
  unsigned int status = buf[0];
  float temperatureC = (buf[1] * 256.0 + buf[2]) / 10.0;
  float temperatureF = ((temperatureC * 9.0) / 5.0) + 32.0;
  float Humidity = (buf[3] * 256.0 + buf[4]) / 10.0;
  unsigned int light = (buf[5] * 256.0 + buf[6]);
  unsigned int audio = (buf[7] * 256.0 + buf[8]);
  float batVolts = ((buf[9] * 256.0 + buf[10]) / 1024.0) * (3.3 / 0.330);
  unsigned int co2_ppm = (buf[11] * 256.0 + buf[12]);
  unsigned int voc_ppm = (buf[13] * 256.0 + buf[14]);
      Serial.print(temperatureC, 1);
    Serial.print("\t\t");

    Serial.print(Humidity, 1);
    Serial.print("\t\t");

    Serial.print(light);
    Serial.print("\t");

      Serial.print(co2_ppm);
      Serial.print("\t");
   
      Serial.print(voc_ppm);
      Serial.print("\t");

    Serial.print(batVolts);
    Serial.print("\t");
 
    if (status & 0x80)
      Serial.print("  PIR_EVENT");
    if (status & 0x02)
      Serial.print("  AUDIO_EVENT");
    if (status & 0x01)
      Serial.print("  MOTION_EVENT");
 
  Serial.print("\n");


  // all sensors except the CO2\VOC sensor are scanned at this rate.
  // CO2/VOC sensor is only updated in AmbiMate every 60 seconds
  delay(sampPeriod - 100);

  incomingByte = Serial.read();

  if ((incomingByte == 'R') || (incomingByte == 'r'))
  {
    //Serial.print("Got R\n");
    Serial.print("\n");
    Serial.print("\n");
    loopCount = 0;
  }

}
