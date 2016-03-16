/*

  nodemcu pinout

  MAX

  3v3 - vcc
  gnd - gnd
  d6  - SCK
  d7  - CS
  d8  - SO

  LCD

  vin - vcc
  gnd - GND
  d1  - SCL
  d2  - SDA

  DIMMER

  gnd - GND
  d0  - MODE
  d4  - PWM
  d5  - 5V

*/

#include <PID_v1.h>

#include <LiquidCrystal_I2C.h>
#include <max6675.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

#define ssid "esp"
#define password "12345678"

#define CONFIG_CHANNEL 12345
#define CONFIG_APIKEY "ABCDEFGHIJKLMNOP"
#define LOG_APIKEY "QRSTUVWXYZABCDEF"

#define port 80
#define host "api.thingspeak.com"

LiquidCrystal_I2C lcd(0x27, 16, 2);

int thermoDO = 15;
int thermoCS = 13;
int thermoCLK = 12;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
int vccPin = 14;
int vccPin2 = 16;

const int led = 16;

const int dimpin = 2;

const int maxpwm = 512;

double output = 0;
double wanted = -1;
double current = -1;

long lastUpdate;
long lastUpload;

// 0 off, 1 on, 2 auto
double mode = 2;

double effect = 0;

double current_temps[10];

//Define the aggressive and conservative Tuning Parameters
//double aggKp = 40, aggKi = 0.2, aggKd = 1;
//double consKp = 80, consKi = 0.05, consKd = 0.25;

/*
  Starting PID Settings For Common Control Loops


  Loop Type  PB
  % Integral
  min/rep Integral
  rep/min Derivative
  min Valve Type
  Flow  50 to 500 0.005 to 0.05 20 to 200 none  Linear or Modified Percentage
  Liquid Pressure 50 to 500 0.005 to 0.05 20 to 200 none  Linear or Modified Percentage
  Gas Pressure  1 to 50 0.1 to 50 0.02 to 10  0.02 to 0.1 Linear
  Liquid Level  1 to 50 1 to 100  0.1 to 1  0.01 to 0.05  Linear or Modified Percentage
  Temperature 2 to 100  0.2 to 50 0.02 to 5 0.1 to 20 Equal Percentage
  Chromatograph 100 to 2000 10 to 120 0.008 to 0.1  0.1 to 20 Linear
*/

double Kp = 50;
double Ki = 2.5;
double Kd = 5.0;

//Specify the links and initial tuning parameters
PID myPID(&current, &output, &wanted, Kp, Ki, Kd, DIRECT);

void startclient() {

  Serial.println("starting http client");

  WiFi.mode(WIFI_STA);
  WiFi.begin(AP_SSID, AP_PWD);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not yet connected");
    delay(500);
  }
  Serial.println("WiFi connected");

  // Print the IP address
  Serial.println(WiFi.localIP());

}

void initLcd() {

  lcd.begin();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Initializing");

}




void setup(void) {

  initLcd();

  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  Serial.begin(9600);

  pinMode(vccPin, OUTPUT);
  digitalWrite(vccPin, HIGH);

  pinMode(dimpin, OUTPUT);

  pinMode(vccPin2, OUTPUT);
  digitalWrite(vccPin2, HIGH);

  Serial.println("I'm alive");

  Serial.println("MAX6675 test");
  // wait for MAX chip to stabilize
  delay(500);

  lastUpdate = millis();
  lastUpload = millis();

  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(0, maxpwm);

  startclient();

  for (int i = 0; i < 10; i++)
    current_temps[i] = -1;


}

int outputpct() {
  return 100 * output / maxpwm;
}


float getValue(String data, int index)
{
  int ix=0;
  int ix0=0;
  int ix1=0;
  data.replace("\"", "");
  int l = data.length() - 1;
  for (int i = 0; i <= l; i++) {
    if (data.charAt(i) == ',' || i==l) {
      ix++;
      i++;
      ix0=ix1;
      ix1=i-1;
      if (index!=ix) continue;
      String str = data.substring(ix0, ix1);
      int n = str.indexOf(":")+1;
      String vstr = str.substring(n);
//      Serial.println("found string " + String(ix) + " ix0=" + String(ix0) + " ix1=" + String(ix1) + " s='" + vstr + "'");
      return vstr.toFloat();
    }
  }
 return 0;
}

void getconfig() {

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // We now create a URI for the request
  String url = "/channels/" + String(CONFIG_CHANNEL) + "/feeds/last?key=" + String(CONFIG_APIKEY);
  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(10);
  String line;
  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    line = client.readStringUntil('\r');
    Serial.print("got from config : " + line);
  }

  // {"created_at":"2016-03-13T22:44:27Z","entry_id":40,"field1":"2","field2":"56","field3":"66","field4":"34.0","field5":"2.2","field6":"4.6"}

//String line = "{\"created_at\":\"2016-03-14T20:31:40Z\",\"entry_id\":61,\"field1\":\"2\",\"field2\":\"40\",\"field3\":\"73\",\"field4\":\"44.0\",\"field5\":\"2.2\",\"field6\":\"5.0\"}";

  Serial.println("got from config : '" + line + "'");

  if (line.length() == 0) return;

  mode = getValue(line, 3);
  effect = getValue(line, 4);
  wanted = getValue(line, 5);
  Kp = getValue(line, 6);
  Ki = getValue(line, 7);
  Kd = getValue(line, 8);

  Serial.println("got from config mode=" + String(mode) + " effect=" + String(effect) + " wanted=" + String(wanted)
                 + " Kp=" + String(Kp) + " Ki=" + String(Ki) + " Kd=" + String(Kd));

}

void upload() {

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // We now create a URI for the request
  String url = "/update?key=" + String(LOG_APIKEY) + "&field1=" + String(current) + "&field2=" + String(wanted) + "&field3=" + String(outputpct());
  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(10);

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    //Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");

}


void getTemp() {
  float current_reading = thermocouple.readCelsius();

  if (current_temps[0] == -1) {
    for (int i = 0; i < 10; i++)
      current_temps[i] = current_reading;
  } else {

    for (int i = 9; i > 0; i--)
      current_temps[i] = current_temps[i - 1];

    current_temps[0] = current_reading;
  }

  float sum = 0;
  for (int i = 0; i < 10; i++)
    sum += current_temps[i];

  current = sum / 10;

  Serial.println("current_reading=" + String(current_reading) + " current=" + String(current));

}





void updateLcd() {

  lcd.setCursor(0, 0);
  lcd.print("C " + String(current) + " ");
  lcd.print("W " + String(wanted));

  lcd.setCursor(0, 1);

  String o = String(outputpct());

  while (o.length() < 3)
    o = " " + o;

  int m = (int)mode;

  String mstr = "Off";
  if(m==1) mstr = "On ";
  else if(m==2) mstr = "PID";

  lcd.print("Mode " + mstr + "   " + o + "%");
}


void loop() {

  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    getTemp();

    if (mode == 0) {
      output = 0;
    } else if (mode == 1) {
      output = maxpwm * effect / 100;
    } else {

      if (wanted >= 0) {

        myPID.SetTunings(Kp, Ki, Kd);

        myPID.Compute();
      }
    }
    /*
      Duty Cycle              Amount of Dimming               Arduino Code
      0 - 5%                  100% Dim (Fully OFF)            analogWrite(0) to analogWrite(15)
      5%-40%:                 100% - 0% Dim                   analogWrite(15) to analogWrite(90)
      Greater than 40%:       0% Dim (Fully ON)               analogWrite(90) to analogWrite(255)
    */

    if (output < 0) {
      Serial.println("output=" + String(output) + " doesn't make sense, setting to 0");
      output = 0;
    }

    Serial.println("setting output to " + String(output) + "/" + maxpwm + "     " + String(outputpct()) + "%");

    analogWrite(dimpin, output);

    updateLcd();

    if (millis() - lastUpload > 10000) {
      lastUpload = millis();
      upload();
      getconfig();
    }

  }

  delay(1000);

}

