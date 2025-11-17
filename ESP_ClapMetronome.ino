


#include <LiquidCrystal.h>
#include "pitches.h"
#include <Adafruit_MCP23017.h>
#include <ICUsingMCP23017.h>
#include <Alislahish_MAX9814.h>
#include <RTClib.h>

#if defined(ESP8266)
   #include <ESP8266WiFi.h>
   #include <ESP8266WebServer.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>

  #include <esp_task_wdt.h>
#endif
  
#include <WiFiClient.h>
#include <MQTTClient.h>

#define NLCD; //no LCD


//MQTT
const char* host = "yourIP";
const char* ssid = "yourSSID";
const char* password = "yourPassword";

WiFiClient net;
MQTTClient mqtt;

//PINS
const int SPEAKER_PIN = 8;
const int LED_PIN = 2;

//const int BUTTON_PIN = 7;


const int Tap_out_duration_ms = 5;

#if defined(ESP8266)
  const int THRESH_PIN = D0;
  const int TAP_OUT_PIN = D3;
const int knockSensor = A0;
  //MAX9814 GainControl
  const int CH_0_RA_PIN = D1;
  const int CH_0_GAIN_PIN = D2;

#elif defined(ESP32)
  const int THRESH_PIN = 26;
  const int TAP_OUT_PIN = 17;
const int knockSensor = 36;
  //MAX9814 GainControl
  const int CH_0_RA_PIN = 22;
  const int CH_0_GAIN_PIN = 21;

#endif




#ifndef NLCD
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

#endif

int threshold = 320;
const long debounceDelay = 50;
const boolean ACCENT = true;

int BPM = 80;
const int minBPM =50;
const int maxBPM =205;

unsigned long last;
unsigned long last_pulse;
unsigned long tdelay;
int signiture = 4;
int tap_out = 0;

long lastDebounceTime = 0;
int buttonState; 
int lastButtonState = LOW;
int reading;

const int samples = 32;
const int avgCount = 64;

int Vanalogread[samples];
int FloatingMax[avgCount];
int FloatingMin[avgCount];
int fMaxSum;
int fMinSum;
int fReadSum;

float TimeMax[avgCount];
float TimeTrig[avgCount];
float TimeMin[avgCount];

bool tapped = false;
bool minFound = false;
bool maxFound = false;
  
int i_fAvg;

const int pulse = 10;//microseconds
const int riseTlim=6;

const int taps_len = 64;
float taps[taps_len];
float tap_intervals[taps_len];

int next_tap = 0;
int total_taps = 0;

int taps_per_bar = 4;
int tap_tolerance_perc = 5;
int tap_tolMin_ms = 5;
int tap_tolMax_ms = 5;
int tap_expected =0;
int tap_expected_max =0;
int tap_fail_limit = 5;

int match_half =0;
int match_double =0;
int match_exact =0;
int taps_failed = 0;

int noteDuration;
int beat;

Alislahish_MAX9814 ch0(CH_0_GAIN_PIN, CH_0_RA_PIN);
Adafruit_MCP23017 mcp;
uint8_t ra = 0;
uint8_t gain = 0;
uint8_t mcpAddr = 0;
RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  Serial.println("initializing");

 #if defined(ESP8266)

  ESP.wdtDisable();
  ESP.wdtEnable(40000);


#elif defined(ESP32)
#define WDT_TIMEOUT 3
  esp_task_wdt_init(40000, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch

#endif

  Serial.println("Pins");
  pinMode(LED_PIN, OUTPUT); //
  pinMode(THRESH_PIN, OUTPUT); //
  pinMode(TAP_OUT_PIN, OUTPUT);
  //pinMode(SPEAKER_PIN, OUTPUT);
  //pinMode(BUTTON_PIN, INPUT);
  pinMode(A0, INPUT);
  digitalWrite(THRESH_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);

  rtc.begin();
  
  //
  Serial.println("MQTT");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  Serial.println("wiFi");
  mqtt.begin(host, net);
  
  #ifndef NLCD  
  Serial.println("LCD");
  lcd.begin(128, 64);
  #endif

  Serial.println("beat");
  beat = 0;
  BPM = 80;
/*
int tap_tolerance_perc = 5;
int tap_tolMin_ms = 5;
int tap_tolMax_ms = 5;
*/

  int tap_tolerance_bpm = 5*BPM/100;
  tap_fail_limit = taps_per_bar+1;
  
  Serial.println("tdelay");
  //calculate seconds per beat
  tdelay = 60000/BPM;
  tap_tolMin_ms = 60000/(BPM + tap_tolerance_bpm);
  tap_tolMax_ms = 60000/(BPM + tap_tolerance_bpm);
  
  Serial.println("last");
  last = millis();
  last_pulse =micros();
  tap_expected = millis() ;
  tap_expected_max = tap_expected + tap_tolMax_ms;
  // to calculate the note duration, take one second
  // divided by the note type.
  //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
  Serial.println("noteDuration");
  noteDuration = 1000 / 16;

  Serial.println("threshold");
  threshold = 50;
  fMaxSum = threshold*avgCount;
  fReadSum = threshold*samples;
  fMinSum = 0;
  i_fAvg=0;
  
  tapped = false;
  minFound = false;
  maxFound = false;

  mcp.begin(mcpAddr);
  //make the MAX9814s use the same MCP23017
  ch0.setMCP23017(true, &mcp, mcpAddr);
    
  Serial.println("initialzed");
}

void loop() {
  int elapsed = millis() - last;
  int pulsing = micros() - last_pulse; //also need to use it for AnalogReadPulse

//  if (pulsing>pulse) {
    reading = 0;
    const int readinterval=100;
    last_pulse = micros();
    int lastread = last_pulse;
    int MinRead = analogRead(A0);
    int MaxRead =analogRead(A0);

    int StartSample = last_pulse;
    
    for (int i=0; i<samples;i++){
      int rIndex=i;
      // int rIndex=((i+1)*(i_fAvg+1))-1;
      if (Vanalogread[rIndex]){
      fReadSum = fReadSum - Vanalogread[rIndex];
      }
      Vanalogread[rIndex] = analogRead(A0);
      float now =micros()/1000;
      fReadSum = fReadSum - Vanalogread[rIndex];
      
      if ((tapped == false) and (Vanalogread[rIndex]>threshold)){
        
        float risetime = now - TimeMin[i_fAvg];
        
        reading = Vanalogread[rIndex];
        if (risetime<riseTlim){
          TimeTrig[i_fAvg]=now;
          taps[next_tap] = now;

          float tap_intervals[taps_len-1];

          next_tap = ++next_tap % taps_len;
          total_taps++;
        
          tapped = true;
          digitalWrite(THRESH_PIN, LOW);
          //Serial.println("passing threshhold " + String(threshold) + " on Rising Edge ready for reinitialisation - risetime[ms]" + String(risetime));
          int tmpBPM = bpms();
          if ((tmpBPM>minBPM) and (tmpBPM<maxBPM)){
            int tap_tolerance_bpm = 5*BPM/100;
            if ((tmpBPM < (BPM - tap_tolerance_bpm)) or (tmpBPM > (BPM + tap_tolerance_bpm))) {
              BPM =tmpBPM;
              //reset_taps();
              tap_out=0;
              Serial.println("new Tempo: " + String(BPM));
              mqtt.publish("/Proberaum/Metronome/Tempo", String(BPM));
            }

          }

          //to observe continuiti of Tempo
          tap_expected_max = now + tap_tolMax_ms;
          tap_expected = now + tdelay;
          taps_failed = 0;
        }
      }

      if (tap_expected_max < now){
        taps_failed = taps_failed + 1;
        tap_expected_max = tap_expected + tap_tolMax_ms;
        tap_expected = now + tdelay;
        if (taps_failed > tap_fail_limit ) {
          //reset_taps();
          taps_failed = 0;
        }else
        {
          /*int match_half =0;
            int match_double =0;
            int match_exact =0; */

            if ((match_double > tap_fail_limit) and (match_double>(match_exact*2))) {
              BPM = BPM * 2;
              reset_taps();
            } else
            {
              if ((match_half > tap_fail_limit) and (match_half > (match_exact*2))) {
                BPM = BPM / 2;
                reset_taps();
              }
            }
        
        }
      }
      
      if (MaxRead<Vanalogread[rIndex]) {
        TimeMax[i_fAvg]=millis();      
        if (FloatingMin[i_fAvg]){
          fMaxSum = fMaxSum - FloatingMax[i_fAvg];
        }
        FloatingMax[i_fAvg] = Vanalogread[rIndex];
        fMaxSum =fMaxSum+FloatingMax[i_fAvg];

        MaxRead = FloatingMax[i_fAvg];

        //reduce Gain if Value is near clipping
        if (Vanalogread[rIndex] > 1000){
          if (gain > 0){
              gain--;
              ch0.setGain(static_cast<MAX9814Gain>(gain));
              Serial.println("decrease Gain: " + String(gain));
              mqtt.publish("/Proberaum/Metronome/Gain", String(gain));
           }
        }
      }

      if (MinRead>Vanalogread[rIndex]) {
        TimeMin[i_fAvg]=now;
        if (tapped == false){
          if (FloatingMin[i_fAvg]){
            fMinSum = fMinSum - FloatingMin[i_fAvg];
          }
          FloatingMin[i_fAvg] = Vanalogread[rIndex];
          fMinSum =fMinSum + FloatingMin[i_fAvg];
          MinRead = FloatingMin[i_fAvg];

          //prepare for next rising edge 
          MaxRead = FloatingMax[i_fAvg];
        }else
        if(Vanalogread[rIndex]<(threshold-10)){
            //passing threshhold on Falling Edge ready for reinitialisation
           //Serial.println("passing threshhold " + String(threshold) + " on Falling Edge ready for reinitialisation");
           digitalWrite(THRESH_PIN, HIGH);
           if (i_fAvg<samples-1) {
              i_fAvg++;
            }else
            {
              i_fAvg=0;
            }
            tapped = false;
        }
      }
      lastread = micros();

    }
    int StopSample = lastread;

    float SampleDuration = (StopSample-StartSample)/1000;

    if (tap_out<signiture) {
      if(elapsed > tdelay) {
        //Tap Out for external Metronome sync
        digitalWrite(TAP_OUT_PIN, LOW);//
        Serial.print(String(elapsed));
        Serial.print("external Tap ");
        
      }else
      if((elapsed > Tap_out_duration_ms) and (digitalRead(TAP_OUT_PIN)==LOW) ) {
        //Tap Out for external Metronome sync
        digitalWrite(TAP_OUT_PIN, HIGH);//
        tap_out = tap_out+1;
        
        Serial.println(String(tap_out));
      } 
    }
  
    if (tapped == true) {

     if (fMaxSum>(fMinSum+30)){
       //lower threshold does not make sense
       
       int avgDiv = fMaxSum-fMinSum;
       threshold = ((0.8*avgDiv)+fMinSum)/avgCount;
       mqtt.publish("/Proberaum/Metronome/threshold", String(threshold));
       mqtt.publish("/Proberaum/Metronome/Max", String(fMaxSum/avgCount));
       mqtt.publish("/Proberaum/Metronome/Min", String(fMinSum/avgCount));
     //  Serial.println(String(SampleDuration) + "new threshold: " + String(threshold) + ", reading: "+String(reading));
     } else
     if (gain <3){
        gain++;
        gain %= 3;
        ch0.setGain(static_cast<MAX9814Gain>(gain));
        Serial.println("increase Gain: " + String(gain));
        mqtt.publish("/Proberaum/Metronome/gain", String(gain));
     }



      //just to prevent problem due to long constand level phases
      /*
      int floatingAVG = fReadSum/(samples);
      
      if (maxFound == false) {
        TimeMax[i_fAvg]=millis();      
       // Serial.print(String(SampleDuration) + "replace FloatingMax: " + String(FloatingMax[i_fAvg]) + " by : "+String(floatingAVG));
        if (FloatingMax[i_fAvg]){
          fMaxSum = fMaxSum - FloatingMax[i_fAvg];
        }
        FloatingMax[i_fAvg] = floatingAVG;
        fMaxSum =fMaxSum+FloatingMax[i_fAvg];

      }//minFound
      if (minFound == false) {
        TimeMin[i_fAvg]=millis();
        Serial.print(String(SampleDuration) + "replace FloatingMin: " + String(FloatingMin[i_fAvg]) + " by : "+String(floatingAVG));
        if (FloatingMax[i_fAvg]){
          fMinSum = fMinSum - FloatingMin[i_fAvg];
        }
        FloatingMin[i_fAvg] = floatingAVG;
        fMinSum =fMinSum + FloatingMin[i_fAvg];
      }*/
    }
//  }//if (pulsing>pulse)
  //ESP.wdtFeed();
  //if ((millis() - lastDebounceTime) > debounceDelay) {
  if (tapped == true) {
      // only toggle the LED if the new button state is HIGH
    //if (reading >= threshold) {
      if((millis() - lastDebounceTime) > 2000) {
        ;
      }
        #ifndef NLCD
        lcd.setCursor(15, 0);
        lcd.write(-1);
        #endif

       // taps[next_tap] = TimeTrig[i_fAvg];

        //lcd.setCursor(0, 1);
        //lcd.print(taps[next_tap]);
        

        
       
        
        
      lastDebounceTime = millis();
    /*} else {
        #ifndef NLCD
        lcd.setCursor(15, 0);
        lcd.print(" ");
        #endif
    }*/
  }
  
  if(BPM == 0) { 
    return;
  }
  
  if(elapsed > noteDuration) { 
    digitalWrite(LED_PIN, HIGH);//
  }



  
//calculate seconds per beat
  tdelay = 60000/BPM;
  int tap_tolerance_bpm = 5*BPM/100;
  tap_fail_limit = taps_per_bar+1;
  tap_tolMin_ms = 60000/(BPM + tap_tolerance_bpm);
  tap_tolMax_ms = 60000/(BPM + tap_tolerance_bpm);
  
  if(elapsed < tdelay) { 
    return;
  }
  
  #ifndef NLCD
  lcd.clear();
  lcd.print("BPM: " + String(BPM));
  #endif

  int play_note = NOTE_C4;
 
  beat = beat % signiture;
  #ifndef NLCD
  lcd.setCursor(0, 1);
  lcd.print(String(beat+1));
  #endif


  if(ACCENT && beat == 0) {
    play_note = NOTE_C6;
  }

  //tone(SPEAKER_PIN, play_note, noteDuration);
  digitalWrite(LED_PIN, LOW);//




  last = millis();
  beat++;
}

int bpms() {
 // Serial.print("bpms");
  if(total_taps < signiture) {
    return 0;
  }
  
  unsigned long total = 0;
  unsigned long t_intervals[signiture];
  int cnt = 0;
  bool tapsValid = true;
  
  for(int i=1; i<=signiture; i++) {
    int tap = next_tap - i;
    
    if(tap < 0) {
      tap = taps_len + tap;
    }
    
      
    if(i>1) {

      tap_intervals[tap] = (taps[tap+1%taps_len] - taps[tap]);
      total += (taps[tap+1%taps_len] - taps[tap]);
      cnt++;

      //Check if previous Taps have consistent Timing
      float avg_interval = (total / (cnt));//cnt
      float t_diff = abs(float(avg_interval) - float(tap_intervals[tap]));
      float dub_half_interval;
      bool halfint;
      
      if (avg_interval > tap_intervals[tap]) {
        t_diff = abs(float(avg_interval) - float(tap_intervals[tap]));
        dub_half_interval = tap_intervals[tap] * 2;
        halfint = false;
      }
      if (avg_interval < tap_intervals[tap]){
         t_diff = abs(float(tap_intervals[tap]) - float(avg_interval));
         dub_half_interval = tap_intervals[tap] / 2;
         halfint = true;
      }
      float t_deviation;
      //Serial.println(String(i+2) + ": t_diff " + String(t_diff) + "[ms]");
      t_deviation= t_diff / float(avg_interval);
      //Serial.print(String(i+2) + ": t_deviation " + String(t_deviation) + "[-]");
      t_deviation = t_deviation *100;
      
      if (tap_tolerance_perc < t_deviation) {
        //Serial.println(String(i+2) + ": deviation " + String(t_deviation) + "[%]");

        if (avg_interval > dub_half_interval) {
          t_diff = abs(float(avg_interval) - float(dub_half_interval));
        }
        if (avg_interval < dub_half_interval){
           t_diff = abs(float(dub_half_interval) - float(avg_interval));
        }

        //Serial.println(String(i+2) + ": dub_half t_diff " + String(t_diff) + "[ms]");
        t_deviation= t_diff / float(avg_interval);
        //Serial.print(String(i+2) + ": dub_half t_deviation " + String(t_deviation) + "[-]");
        t_deviation = t_deviation *100;
        
        if (tap_tolerance_perc < t_deviation) {
          //Serial.println(String(i+2) + ": dub_half deviation " + String(t_deviation) + "[%]");
          tapsValid = false ;
        }else
        {
          //int match_half =0
          //int match_double =0
          //int match_exact =0
          total -= (taps[tap+1%taps_len] - taps[tap]);
          total += dub_half_interval;
          if (halfint = false) {
            match_double = match_double+1;
          }else
          {
            match_half = match_half+1;
          }
        }
      }else
      {
        match_exact = match_exact+1;
      }
    }
  }

  if ((tapsValid == false) and (total_taps > signiture)) {
    int min_dev = tap_tolerance_perc;
    int min_taps = 0;
    for(int i=2; i<(total_taps/2); i++) {
      float t_deviation = AnalyzeTiming(i);
      if (min_dev > t_deviation) {
          //Serial.println(String(i) + ":passed " + String(t_deviation) + "[%]");
          int min_dev = t_deviation;
          int min_taps = i;
          taps_per_bar = i;
      }
    }
    if (tap_tolerance_perc > min_dev) {
      //Serial.println("Call GetBeatsPerBar " + String(min_taps) + "::::");
      GetBeatsPerBar(min_taps);
    }
  }else
  {
      if (tapsValid == false) {
        return 0;
      }else
      {
        if ((total>0) and (signiture>0)) {
          return 60000/(total / (signiture - 1));
        }else
        {
          return BPM;
        }
      }
  }

//just to be sure ...
return 0;


}

void reset_taps() {
  Serial.println("reset_taps");
  beat = 0;
  for(int i=0;i<taps_len;i++) {
    taps[i] = 0;
  }
  next_tap = 0;
  total_taps = 0;
  signiture = 4;
  taps_per_bar=signiture;

match_half =0;
match_double =0;
match_exact =0;


  //ESP.wdtFeed();
}

float AnalyzeTiming( int Count) {
Serial.println("AnalyzeTiming(" + String(Count) + ")");
    /*
    const int taps_len = 64;
    float taps[taps_len];
    */
    float ext_intervals[total_taps/Count];
    float ext_deviation[total_taps/Count];
    float ext_divisor_2[total_taps];
    float ext_divisor_3[total_taps];
    float ext_divisor_5[total_taps];
    float ext_divisor_7[total_taps];
    
    float total = 0;
    int loopcnt = 0;
    for (int i=0; i<(total_taps/Count);i++){
      ext_intervals[i] = (taps[i+8%taps_len] - taps[i]);
      total = total + ext_intervals[i];
      loopcnt = loopcnt + 1;
    }

    if (loopcnt<Count) {
      return 100; //average out of 2 values is not precise enough
    }
    
    float avg_interval = (total / (loopcnt));//cnt
    float t_diff;
    total = 0;
    loopcnt = 0;
    //Serial.println("AnalyzeTiming(" + String(Count) + ") avg_interval "+ String(avg_interval) + "[ms]");
    for (int tap=0; tap<(total_taps/Count);tap++){
      if (avg_interval > ext_intervals[tap]) {
        t_diff = abs(float(avg_interval) - float(ext_intervals[tap]));
      }
      if (avg_interval < ext_intervals[tap]){
         t_diff = abs(float(ext_intervals[tap]) - float(avg_interval));
      }

      //Serial.println("AnalyzeTiming(" + String(Count) + ")"+ String(tap) + ": t_diff " + String(t_diff) + "[ms]");
      ext_deviation[tap]= t_diff / float(avg_interval);
      //Serial.println("AnalyzeTiming(" + String(Count) + ")"+ String(tap) + ": ext_deviation[i] " + String(ext_deviation[tap]) + "[-]");
      ext_deviation[tap] = ext_deviation[tap] *100;
      //Serial.println("AnalyzeTiming(" + String(Count) + ")"+ String(tap) + ": ext_deviation[i] " + String(ext_deviation[tap]) + "[%]");
      total = total + (ext_deviation[tap]);
      loopcnt = loopcnt + 1;
    }
    Serial.print(" AvgDeviation "+ String(total/loopcnt) + "[%] (out of "+ String(loopcnt) +")");
    Serial.println();
    return (100*total/loopcnt);
}

int GetBeatsPerBar(int TapsPerBeat){
Serial.println("GetBeatsPerBar(" + String(TapsPerBeat) + ")");
  return 0;
}
