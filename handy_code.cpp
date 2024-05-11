//////////////////////////////////////////////////////////////////////////////////
/*
็Handy Sense IoT Final source code For Super Ai Engineer
4/5/2024
Cook By 
Adi 400695 Senior dev
Ohm 404640 Optimize dev 
*/
//////////////////////////////////////////////////////////////////////////////////
#include <HandySense.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include "time.h"
#include "TimeBFarm.h"
#include <TridentTD_LineNotify.h>
#include <pub_topic.h>
#include "SHT31.h"
#include <Wire.h>
#include <ModbusMaster.h>

BFarmTime bfarmtime;
int soil_mois_2;
int soil_mois_1;


SHT31 sht;
float tempset = 35;
float humidset = 80;
float moistureset = 50;
float stop = 10;
float v;
int valve_1 = 0;
int valve_2 = 0;
int line_valve_1 = 0;
int line_valve_2 = 0;

unsigned long currentMillis;

unsigned long timemer_set;
unsigned long line_timemer_set;
unsigned long Wtop_timemer_set;
unsigned long Wdown_timemer_set;
unsigned long fan_timemer_set;

bool clock_Stat = false;
bool line_clock_stat = false;
bool Wtop_stat = false;
bool Wdown_stat = false;
bool fan_stat = false;


const char* Netpiemqtt_server = "broker.netpie.io";
const int Netpiemqtt_port = 1883;
const char* Netpiemqtt_username = "";
const char* Netpiemqtt_password = "";
const char* Netpiemqtt_Client = "";


float VDP(float tem ,float hum) //ค่าที่ต้นไม้ชอบ ถ้าดีจะอยู่ใน 0.9-1
{
    float vpd = (610.78/1000) * pow(2.71828,(tem*17.2614/(tem+237.37)))*(1-(hum/100));
    return vpd;
}
void setup() {

  Serial.println("initialization done...");
  WiFi.begin("IoT_2.4GHz", "SPAI40000"); //เชื่อม wifi
  Wire.begin();
  Wire.setClock(10000);
  ledcSetup(0, 5000, 8);
  ledcAttachPin(33, 0); 
  sht.begin(0x44);

  Serial.begin(115200);

  LED_WIFI = 26;
  LED_SERVER = 27;

  setPin_Relay(32, 33, 25, 26);
  setPin_ErrorSensor(19, 18, 5);

  type_RTC = 0;
  WiFi.begin("IoT_2.4GHz", "SPAI40000");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  LINE.setToken(String(""));
  Netpieclient.setServer(Netpiemqtt_server, Netpiemqtt_port);
}

bool v1 = false;
bool v2 = false;

void loop() 
{
    currentMillis = millis();

    if (WiFi.status() != WL_CONNECTED) { // เช็ค wifi
        WiFi.begin("IoT_2.4GHz", "SPAI40000");

        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
        }
    }

    if (!Netpieclient.connected()) { // เช็คตัว netpie
        while (!Netpieclient.connected()) {
        Serial.print("Attempting NETPIE2020 connection...");
        if (Netpieclient.connect(Netpiemqtt_Client, Netpiemqtt_username, Netpiemqtt_password)) 
            {
                Serial.println("NETPIE2020 connected");
                Netpieclient.subscribe("@private/#");
            } 
            else {
                Serial.print("failed, rc=");
                Serial.print(client.state());
                Serial.println("try again in 5 seconds");
                delay(5000);
            }
        }
    }

    Netpieclient.loop(); 
    
    sht.read();

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    if (ReadAnalog_from_MPC3424(3, 1758687.00,1031062.00, 0, 100) >= 100) // อ่านค่าความชื้นดิน ตัวที่ 1
    {
        soil_mois_1 = 100;
    } 
    else if (ReadAnalog_from_MPC3424(3, 1758687.00,1031062.00, 0, 100) <= 0) 
    {
        soil_mois_1 = 0;
    }
    else{
        soil_mois_1 = ReadAnalog_from_MPC3424(3, 1758687.00,1031062.00, 0, 100);
    }

    if (ReadAnalog_from_MPC3424(4, 853125.00,482375.00,0, 100) >= 100) // อ่านค่าความชื้นดิน ตัวที่ 2
    {
        soil_mois_2 = 100;
    } 
    else if (ReadAnalog_from_MPC3424(4, 853125.00,482375.00,0, 100) <= 0) 
    {
        soil_mois_2 = 0;
    }
    else{
        soil_mois_2 = ReadAnalog_from_MPC3424(4, 853125.00,482375.00,0, 100);
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    float temp = sht.getTemperature(); // อ่านค่าอุณหภูมิ
    float humid = sht.getHumidity(); // อ่านค่าความชื้น
    float vlower_set = 0.7 ;
    float vupper_set = 1.4 ;
    float soim = (soil_mois_1+soil_mois_2)/2;
    float v = VDP(temp,humid);


    // Serial print ค่าต่างๆ

    Serial.println(String("Temp: ")); 
    Serial.println(temp);
    Serial.println(String("Humid: "));
    Serial.println(humid);
    Serial.println(String("Soil Moisture: 1"));
    Serial.println(soil_mois_1);
    Serial.println(String("Soil Moisture: 2"));
    Serial.println(soil_mois_2);
    Serial.println(String("MeanSoil 1&2:"));
    Serial.println(soim);
    Serial.println(String("VDP :"));
    Serial.println(v);
//////////////////////////////////////////////////////////////////////////

    if(!clock_Stat){ // เช็ต time แค่ครั้งเดียว
        timemer_set = currentMillis;
        clock_Stat = true;
        Serial.print("timer stats");
        Serial.println(timemer_set);
    }

    
    if(!line_clock_stat){ // เช็ต time แค่ครั้งเดียว
        line_timemer_set = currentMillis;
        line_clock_stat = true;
        Serial.print("timer stats");
        Serial.println(timemer_set);
    }

    Serial.print("time count : ");
    Serial.println(currentMillis - timemer_set);

    if(clock_Stat && currentMillis - timemer_set > 30000 ){ // ส่งข้อมูลเข้า Netpie
        clock_Stat = false;
        pub_topic("soil_moisture_1", soil_mois_1);
        pub_topic("soil_moisture_2", soil_mois_2);
        pub_topic("temp", temp);
        pub_topic("humid", humid);
        pub_topic("valve_1", valve_1);
        pub_topic("valve_2", valve_2);
        valve_1 = 0;
        valve_2 = 0;
    }

    if(line_clock_stat && currentMillis - line_timemer_set > 1800000 ){ // ส่งข้อมูลเข้า Netpie
        line_clock_stat = false;

        LINE.notify(String("soil_moisture_1 : ") + String(soil_mois_1));
        LINE.notify(String("soil_moisture_2 : ") + String(soil_mois_2));
        LINE.notify(String("soil_mean : ") + String(soim));
        LINE.notify(String("temp : ") + String(temp));
        LINE.notify(String("humid : ") + String(humid));
         LINE.notify(String("VPD : ") + String(v));
        
        LINE.notify(String("how many time valve is open in 30m"));
        LINE.notify(String("valve_1 : ") + String(line_valve_1));
        LINE.notify(String("valve_2 : ") + String(line_valve_2));

        line_valve_1 = 0;
        line_valve_2 = 0;
    }
/////////////////////////////////////////////////////////////////////////////////
    if(temp > tempset || humid > humidset){
        Serial.println("fan task");
        if(!fan_stat){
            fan_timemer_set = currentMillis;
            fan_stat = true;
        }

        if(fan_stat && currentMillis - fan_timemer_set < 10000){
            dacWrite(25, 255);
        }
        else{
            fan_stat = false;
            dacWrite(25, 0);
        }
    }
    else{
        fan_stat = false;
        dacWrite(25, 0);
    }
    
    // fuzzy logic
    if ((v <= vlower_set ||  v >= vupper_set) && !(soil_mois_1 <= stop) && !(soil_mois_2 <= stop))
    {   
/////////////////////////////////////////////////////////////////////////////////////////// top only
        if (temp < tempset && humid < humidset && soim < moistureset)
        {
            Serial.println("task1");
            if(!Wtop_stat){
                Wtop_timemer_set = currentMillis;
                // Wdown_timemer_set = currentMillis;
                Wtop_stat = true;
                // Wdown_stat = true;
            }

            if(Wtop_stat && currentMillis - Wtop_timemer_set < 2000){
                 digitalWrite(const_relay_pin[0], HIGH);
                 if(!v1){
                     valve_1 += 1;
                     line_valve_1 += 1;
                     v1 = true;
                 }
                 
            }
            else{
                v1 = false;
                Wtop_stat = false;
                digitalWrite(const_relay_pin[0], LOW);
            }
            // if(Wdown_stat && currentMillis - Wtop_timemer_set > 2000 && currentMillis - Wdown_timemer_set < 4000){
            //     digitalWrite(const_relay_pin[3], HIGH);
            //     valve_2 += 1;
            //      if(!v2){
            //          valve_2 += 1;
            //          v2 = true;
            //      }
            // }
            // else if(!Wtop_stat){
            //     v2 = false;
            //     Wdown_stat = false;
            //     digitalWrite(const_relay_pin[3], LOW);
            // }
        }
        
///////////////////////////////////////////////////////////////////////////////////////////
        if (temp > tempset && humid < humidset && soim < moistureset)
        {
            Serial.println("task2");
            if(!Wtop_stat){
                Wtop_timemer_set = currentMillis;
                // Wdown_timemer_set = currentMillis;
                Wtop_stat = true;
                // Wdown_stat = true;
            }

            if(Wtop_stat && currentMillis - Wtop_timemer_set < 2000){
                 digitalWrite(const_relay_pin[0], HIGH);
                 if(!v1){
                    v1 = true;
                    valve_1 += 1;
                    line_valve_1 += 1;
                 }
            }
            else{
                v1 = false;
                Wtop_stat = false;
                digitalWrite(const_relay_pin[0], LOW);
            }
            // if(Wdown_stat && currentMillis - Wtop_timemer_set > 2000 && currentMillis - Wdown_timemer_set < 4000){
            //     digitalWrite(const_relay_pin[3], HIGH);
            //     if(!v2){
            //         valve_2 += 1;
            //         v2 = true;
            //     }
            // }
            // else if(!Wtop_stat){
            //     v2 = false;
            //     Wdown_stat = false;
            //     digitalWrite(const_relay_pin[3], LOW);
            // }
        }
///////////////////////////////////////////////////////////////////////////////////////////        
        if (temp > tempset && humid > humidset && soim > moistureset)
        {
            digitalWrite(const_relay_pin[0], LOW);
            digitalWrite(const_relay_pin[3], LOW);
            Serial.println("task3");
            if(!fan_stat){
                fan_timemer_set = currentMillis;
                fan_stat = true;
            }

            if(fan_stat && currentMillis - fan_timemer_set < 10000){
                dacWrite(25, 255);
            }
            else{
                fan_stat = false;
                dacWrite(25, 0);
            }
        }
///////////////////////////////////////////////////////////////////////////////////////////
        if (temp > tempset && humid < humidset && soim > moistureset)
        {
            digitalWrite(const_relay_pin[0], LOW);
            digitalWrite(const_relay_pin[3], LOW);

            Serial.println("task4");
            if(!fan_stat){
                fan_timemer_set = currentMillis;
                fan_stat = true;
            }

            if(fan_stat && currentMillis - fan_timemer_set < 10000){
                dacWrite(25, 255);
            }
            else{
                fan_stat = false;
                dacWrite(25, 0);
            }
        }
///////////////////////////////////////////////////////////////////////////////////////////
        if (humid > humidset && soim < moistureset )
        {
            Serial.println("task5");

            if(!fan_stat){
                fan_timemer_set = currentMillis;
                fan_stat = true;
            }

            if(fan_stat && currentMillis - fan_timemer_set < 10000){
                dacWrite(25, 255);
            }
            else{
                fan_stat = false;
                dacWrite(25, 0);
            }
              
            if(!Wdown_stat){
                // Wtop_timemer_set = currentMillis;
                Wdown_timemer_set = currentMillis;
                // Wtop_stat = true;
                Wdown_stat = true;
            }
            
            if(Wdown_stat && currentMillis - Wdown_timemer_set < 2000){
                 digitalWrite(const_relay_pin[3], HIGH);
                 if(!v2){
                    valve_2 += 1;
                    line_valve_2 += 1;
                    v2 = true;
                 }
            }
            else{
                v2 = false;
                Wdown_stat = false;
                digitalWrite(const_relay_pin[3], LOW);
            }
            // if(Wtop_stat && currentMillis - Wdown_timemer_set > 2000 && currentMillis - Wtop_timemer_set < 4000){
            //     digitalWrite(const_relay_pin[0], HIGH);
            //     if(!v1){
            //         valve_1 += 1;
            //         v1 = true;
            //     }
            // }
            // else if(!Wdown_stat){
            //     v1 = false;
            //     Wtop_stat = false;
            //     digitalWrite(const_relay_pin[0], LOW);
            // }
        }

        // if(!(soil_mois_1 <= stop) && !(soil_mois_2 <= stop)){
        //     Wtop_stat = false;
        //     Wdown_stat = false;
        //     digitalWrite(const_relay_pin[0], LOW);
        //     digitalWrite(const_relay_pin[3], LOW);
        // }
    }
    else if((soim < moistureset) && !(soil_mois_1 <= stop) && !(soil_mois_2 <= stop) ){
        if(!fan_stat){
                fan_timemer_set = currentMillis;
                fan_stat = true;
            }

            if(fan_stat && currentMillis - fan_timemer_set < 10000){
                dacWrite(25, 255);
            }
            else{
                fan_stat = false;
                dacWrite(25, 0);
            }
              
            if(!Wdown_stat){
                // Wtop_timemer_set = currentMillis;
                Wdown_timemer_set = currentMillis;
                // Wtop_stat = true;
                Wdown_stat = true;
            }
            
            if(Wdown_stat && currentMillis - Wdown_timemer_set < 2000){
                 digitalWrite(const_relay_pin[3], HIGH);
                 if(!v2){
                    valve_2 += 1;
                    line_valve_2 += 1;
                    v2 = true;
                 }
            }
            else{
                v2 = false;
                Wdown_stat = false;
                digitalWrite(const_relay_pin[3], LOW);
            }
    }
    else
    {
        Wtop_stat = false;
        Wdown_stat = false;
        digitalWrite(const_relay_pin[0], LOW);
        digitalWrite(const_relay_pin[3], LOW);
    }
    

}
