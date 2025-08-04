#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP  59

RTC_DATA_ATTR int bootCount = 0;

void setup(){
  Serial.begin(115200);
  delay(1000);

  // Increment and show boot count
  bootCount++;
  Serial.println("Boot Count: " + String(bootCount));

  Serial.println("Dhor Dhor");

  // Configure timer wakeup after 59 seconds
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // wake_up
  delay(1000);

 
  Serial.println("59S er jonno ghumailam");
  Serial.flush();
  esp_deep_sleep_start();
}

void loop(){
  // Never called after deep sleep
}