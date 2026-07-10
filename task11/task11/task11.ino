#include <Arduino_HS300x.h>
#include <Arduino_BMI270_BMM150.h>
#include <Arduino_APDS9960.h>

// Rates of change to trigger state change.
const float HUMID_JUMP_RATE = 4;
const float TEMP_RISE_RATE = .5; 
const float MAG_SHIFT_RATE = 10;
const int LIGHT_CHANGE_RATE = 100;

// Timing variables
unsigned long lastEventTime = 0;
const unsigned long COOLDOWN_MS = 3000; 

void setup() {
  Serial.begin(115200);
  delay(1500);

  if (!HS300x.begin()) {
    Serial.println("Failed to initialize HS300x sensor.");
    while (1);
  }

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU.");
    while (1);
  }

  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor.");
    while (1);
  }
}

void loop() {
  // Variables to hold incoming data. 
  static float rh;
  static float temp;
  static float mag;
  static int r, g, b, clear;

  // Get the tempature data
  float temp_new = HS300x.readTemperature();
  float rh_new = HS300x.readHumidity();

  float mag_new = mag;
  // Get magnetic data
  if (IMU.magneticFieldAvailable()) {
    float x, y, z;
    IMU.readMagneticField(x, y, z);

    mag_new = sqrt(x*x + y*y + z*z); 
  }

  int clear_new = clear;
  // Read rbg and light
  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b, clear_new);
  }


  int flag_humid_jump = ((rh_new - rh) > HUMID_JUMP_RATE) ? 1 : 0;
  int flag_temp_rise = ((temp_new - temp) > TEMP_RISE_RATE) ? 1 : 0;
  int flag_mag_shift = ((mag_new - mag) > MAG_SHIFT_RATE) ? 1 : 0;
  int flag_light_color = (abs(clear_new - clear) > LIGHT_CHANGE_RATE) ? 1 : 0; 

  
  String final_label = "BASELINE_NORMAL";
  unsigned long currentTime = millis();

  // Set loop to only trigger after cooldown timer.
  if (currentTime - lastEventTime > COOLDOWN_MS) {
    
    if (flag_humid_jump == 1 || flag_temp_rise == 1) {
      final_label = "BREATH_OR_WARM_AIR_EVENT";
      lastEventTime = currentTime;
    } 

    else if (flag_mag_shift == 1) {
      final_label = "MAGNETIC_DISTURBANCE_EVENT";
      lastEventTime = currentTime;
    } 

    else if (flag_light_color == 1) {
      final_label = "LIGHT_OR_COLOR_CHANGE_EVENT";
      lastEventTime = currentTime;
    }

  } else {
    final_label = "BASELINE_NORMAL"; 
  }



  ///////////// MONITOR //////////////
  // Data
  Serial.print("raw,rh="); Serial.print(rh);
  Serial.print(",temp="); Serial.print(temp);
  Serial.print(",mag="); Serial.print(mag);
  Serial.print(",r="); Serial.print(r);
  Serial.print(",g="); Serial.print(g);
  Serial.print(",b="); Serial.print(b);
  Serial.print(",clear="); Serial.println(clear);

  // Flags
  Serial.print("flags,humid_jump="); Serial.print(flag_humid_jump);
  Serial.print(",temp_rise="); Serial.print(flag_temp_rise);
  Serial.print(",mag_shift="); Serial.print(flag_mag_shift);
  Serial.print(",light_or_color_change="); Serial.println(flag_light_color);

  // Event
  Serial.print("event,"); Serial.println(final_label);
  Serial.println(); 


  // Update varibles
  rh = rh_new;
  temp = temp_new;
  mag = mag_new;
  clear = clear_new;

  delay(1500);
}