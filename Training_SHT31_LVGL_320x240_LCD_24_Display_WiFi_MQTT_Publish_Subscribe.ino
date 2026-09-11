#include <TonyS_X1.h>
#include <TonyS_X1_ExternalModule.h>
#include <PubSubClient.h>
#include <WiFi.h>

#define mqttPublishTopic "node99/temp&humi"   //<==========แก้ให้ไม่เหมือนกัน
#define mqttSubscribeTopic "node99/relay"     //<==========แก้ให้ไม่เหมือนกัน
#define clientName "TSN99"                    //<==========แก้ให้ไม่เหมือนกัน ต้องไม่มีอักขระพิเศษ และไม่ยาวไป

const char* ssid = "Your SSID";               //<==========ชื่อ Wi-Fi
const char* password = "Your Password";       //<==========รหัสผ่าน Wi-Fi

const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;

TonySHT31 sht31;
TonyTFT tft = TonyTFT(SLOT1, TFT_240_320);

unsigned long lastMeasure = 0;
unsigned long interval = 3000;
unsigned long now = 0;

lv_obj_t * label_Temp;
lv_obj_t * label_Humi;
lv_obj_t * lmeter_Temp;
lv_obj_t * lmeter_Humi;
lv_obj_t * chart;
lv_chart_series_t * ser_Temp;
lv_chart_series_t * ser_Humi;

WiFiClient client;
PubSubClient mqtt(client);

void setup() {
  Serial.begin(115200);
  Tony.begin();
  sht31.begin(0x44);

  tft.init(240, 320);
  tft.setRotation(2);
  tft.setCursor(0, 0);
  tft.fillScreen(BLACK);
  tft.setTextSize(2);
  tft.setTextColor(GREEN);
  tft.print("Connecting to ");
  tft.println(ssid);

  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(250);
    Serial.print(".");
    tft.print(".");
  }
  Serial.println("");
  Serial.println("Wi-Fi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  tft.println("");
  tft.println("Wi-Fi conected");
  tft.println("Connecting to MQTT");

  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setCallback(callback);
  
  mqtt.connect(clientName);
  Serial.print("Connecting to MQTT...");
  while(!mqtt.connected()){
    delay(250);
    Serial.print(".");
    tft.print(".");
  }
  mqtt.subscribe(mqttSubscribeTopic);
  Serial.println("");
  Serial.println("MQTT connected");
  tft.println("");
  tft.println("MQTT conected");

  tft.fillScreen(BLACK);

  lvglSetup();

  Tony.pinMode(Relay_1, OUTPUT);
  Tony.pinMode(Relay_2, OUTPUT);
}

void loop() {
  mqtt.loop();
  now = millis();
  if (now - lastMeasure > interval){
    lastMeasure = now;
    float t = sht31.readTemperature();
    float h = sht31.readHumidity();

    String temp = String(t, 2);
    String humi = String(h, 2);

    if(!isnan(t) && !isnan(h)){
      Serial.print("Temperature = ");
      Serial.print(t);
      Serial.println(" *c");

      Serial.print("Humidity = ");
      Serial.print(h);
      Serial.println(" %");

      lv_label_set_text(label_Temp, temp.c_str());
      lv_lmeter_set_value(lmeter_Temp, t);
      lv_chart_set_next(chart, ser_Temp, t);

      lv_label_set_text(label_Humi, humi.c_str());
      lv_lmeter_set_value(lmeter_Humi, h);
      lv_chart_set_next(chart, ser_Humi, h);

      String toPublish = String(t) + "," + String(h);
      mqtt.publish(mqttPublishTopic, toPublish.c_str());
    }

    else{
      Serial.println("Failed to read data from SHT31");
      return;
    }
  
    Serial.println("");
  }
  //delay(3000);
}

void lvglSetup(){
  lvglInit(&tft);
  startLvglHandle();

  lv_theme_t *themes = lv_theme_alien_init(20, NULL);
  lv_theme_set_current(themes);

  lv_obj_t * cont_Temp = lv_cont_create(lv_disp_get_scr_act(NULL), NULL);
  lv_obj_set_size(cont_Temp, 95, 110);
  lv_obj_align(cont_Temp, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 30);

  lv_obj_t * cont_Humi = lv_cont_create(lv_disp_get_scr_act(NULL), NULL);
  lv_obj_set_size(cont_Humi, 95, 110);
  lv_obj_align(cont_Humi, NULL, LV_ALIGN_IN_TOP_RIGHT, -20, 30);

  lmeter_Temp = lv_lmeter_create(cont_Temp, NULL);
  lv_lmeter_set_range(lmeter_Temp, 0, 100);                   /*Set the range*/
  lv_lmeter_set_value(lmeter_Temp, 80);                       /*Set the current value*/
  lv_lmeter_set_scale(lmeter_Temp, 240, 21);                  /*Set the angle and number of lines*/
  lv_obj_set_size(lmeter_Temp, 80, 80);
  lv_obj_align(lmeter_Temp, NULL, LV_ALIGN_CENTER, 0, 0);

  lmeter_Humi = lv_lmeter_create(cont_Humi, NULL);
  lv_lmeter_set_range(lmeter_Humi, 0, 100);                   /*Set the range*/
  lv_lmeter_set_value(lmeter_Humi, 20);                       /*Set the current value*/
  lv_lmeter_set_scale(lmeter_Humi, 240, 21);                  /*Set the angle and number of lines*/
  lv_obj_set_size(lmeter_Humi, 80, 80);
  lv_obj_align(lmeter_Humi, NULL, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t * symbol_Temp = lv_label_create(cont_Temp, NULL);
  lv_label_set_text(symbol_Temp, "T");
  lv_obj_align(symbol_Temp, NULL, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t * symbol_Humi = lv_label_create(cont_Humi, NULL);
  lv_label_set_text(symbol_Humi, "H");
  lv_obj_align(symbol_Humi, NULL, LV_ALIGN_CENTER, 0, 0);
    
  label_Temp = lv_label_create(cont_Temp, NULL);
  lv_label_set_text(label_Temp, "00.00");
  lv_obj_align(label_Temp, NULL, LV_ALIGN_CENTER, 0, 40);

  label_Humi = lv_label_create(cont_Humi, NULL);
  lv_label_set_text(label_Humi, "00.00");
  lv_obj_align(label_Humi, NULL, LV_ALIGN_CENTER, 0, 40);

  /*Create a chart*/
  chart = lv_chart_create(lv_scr_act(), NULL);
  lv_obj_set_size(chart, 200, 150);
  lv_obj_align(chart, NULL, LV_ALIGN_CENTER, 0, 60);
  lv_chart_set_type(chart, LV_CHART_TYPE_POINT | LV_CHART_TYPE_LINE);            /*Show lines and points too*/
  lv_chart_set_series_width(chart, 4);                                          /*Line width and point radious*/
  lv_chart_set_range(chart, 0, 100);
  
  /*Add two data series*/
  ser_Temp = lv_chart_add_series(chart, LV_COLOR_RED);
  ser_Humi = lv_chart_add_series(chart, LV_COLOR_CYAN);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message: ");
   
  String message = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  if(String(topic) == mqttSubscribeTopic){
    if(message == "RELAY#1 ON"){
      Tony.digitalWrite(Relay_1, HIGH);
      Serial.println(" ==> RELAY#1 IS ON");
    }
    else if(message == "RELAY#1 OFF"){
      Tony.digitalWrite(Relay_1, LOW);
      Serial.println(" ==> RELAY#1 IS OFF");
    }
    else if(message == "RELAY#2 ON"){
      Tony.digitalWrite(Relay_2, HIGH);
      Serial.println(" ==> RELAY#2 IS ON");
    }
    else{
      Tony.digitalWrite(Relay_2, LOW);
      Serial.println(" ==> RELAY#2 IS OFF");
    }
  } 
  Serial.println(""); 
}
