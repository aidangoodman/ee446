#include <PDM.h>
#include <Arduino_APDS9960.h>
#include <Arduino_BMI270_BMM150.h>

// Threshold values to set the state change 
const int THRESHOLD_MIC = 100;       
const int THRESHOLD_DARK = 30;      
const float THRESHOLD_MOTION = 2; 
const int THRESHOLD_NEAR = 50;     

// Task 5
short sampleBuffer[256];
volatile int samplesRead = 0;

void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read(sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2;
}


void setup() {

  // Setup the serial connection
  Serial.begin(115200);
  
  // IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU.");
    while (1);
  }

  // LIGHT & PROXIMITY
  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor.");
    while (1);
  }

  // MIC
  PDM.onReceive(onPDMdata);
  if (!PDM.begin(1, 16000)) {
    Serial.println("Failed to start PDM microphone.");
    while (1);
  }
}

void loop() {

  // Vars to hold incoming data 
  static int mic_data;
  static int clear_data;
  static float motion_data;
  static int prox_data;

  // Task 5 reading from microgpone
  if (samplesRead) {
    long sum = 0;
    for (int i = 0; i < samplesRead; i++) {
      sum += abs(sampleBuffer[i]);
    }
    mic_data = sum / samplesRead;
    samplesRead = 0;
  }

  // Task 9C light reading 
  if (APDS.colorAvailable()) {
    int r, g, b;
    APDS.readColor(r, g, b, clear_data);
  }

  // Task 6A motion reading
  if (IMU.accelerationAvailable()) {
    float x, y, z;
    IMU.readAcceleration(x, y, z);
    motion_data = abs(x) + abs(y) + abs(z); 
  }

  // Task 9A prox reading
  if (APDS.proximityAvailable()) {
    prox_data = APDS.readProximity();
  }

  // Set flags to determine certain states, once they reach the threshold values. 
  int flag_sound = (mic_data > THRESHOLD_MIC) ? 1 : 0;
  int flag_dark = (clear_data < THRESHOLD_DARK) ? 1 : 0;
  int flag_moving = (motion_data > THRESHOLD_MOTION) ? 1 : 0;
  int flag_near = (prox_data < THRESHOLD_NEAR) ? 1 : 0; 

  // --- COMBINE DECISIONS (RULE-BASED LOGIC) ---
  String final_label = "UNKNOWN";

  // Main logic for determining states, defaults to UNKNOWN. 
  if (flag_sound == 0 && flag_dark == 0 && flag_moving == 0 && flag_near == 0) {
    final_label = "QUIET_BRIGHT_STEADY_FAR";
  } else if (flag_sound == 1 && flag_dark == 0 && flag_moving == 0 && flag_near == 0){
    final_label = "NOISY_BRIGHT_STEADY_FAR";
  } else if(flag_sound == 0 && flag_dark == 1 && flag_moving == 0 && flag_near == 1){
    final_label = "QUIET_DARK_STEADY_NEAR";
  } else if(flag_sound == 1 && flag_dark == 0 && flag_moving == 1 && flag_near == 1){
    final_label = "NOISY_BRIGHT_MOVING_NEAR ";
  } else {
     final_label = "UNKNOWN";
  }

  
  // Directed output per insturctions
  Serial.print("raw,mic="); Serial.print(mic_data);
  Serial.print(",clear="); Serial.print(clear_data);
  Serial.print(",motion="); Serial.print(motion_data);
  Serial.print(",prox="); Serial.println(prox_data);

  Serial.print("flags,sound="); Serial.print(flag_sound);
  Serial.print(",dark="); Serial.print(flag_dark);
  Serial.print(",moving="); Serial.print(flag_moving);
  Serial.print(",near="); Serial.println(flag_near);

  Serial.print("state,"); Serial.println(final_label);
  Serial.println(); 

  delay(500);
}