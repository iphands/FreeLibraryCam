#include <Arduino.h>
#include <WiFi.h>
#include <TimeLib.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include "secrets.h"
#include "esp32-hal-cpu.h"
#include "esp_wifi.h"

#define uS_TO_S_FACTOR 1000000ULL

// const int LED = 4;
const int BUTTON  = 12;
const int SPEAKER = 2;
const int LEDC_CHAN = 15;
const int HTTP_BUFF = 1460;
const int CHIRP_DELAY = 32;
const int SLEEP_SECS  = 300;

const String server_name = "camupload.lan";
const char* server_name_c = server_name.c_str();
const String server_path = "/";
const int server_port = 8000;

WiFiClient client;

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void beep(uint freq, uint d) {
  ledcWriteTone(LEDC_CHAN, freq);
  delay(d);
  ledcWrite(LEDC_CHAN, 0);
}

void beep_sweep() {
  for (int i = 100; i < 3000; i += 100) {
    beep(i, 32);
  }
}

void beep_one_two_three_go() {
  beep(523,  512);
  delay(128);
  beep(523,  512);
  delay(128);
  beep(523,  512);
  delay(128);
  beep(1046, 750);
}

void beep_success() {
  beep(523, 128);
  delay(16);

  beep(659, 128);
  delay(16);

  beep(784, 256);
  ledcWrite(LEDC_CHAN, 0);
}

void beep_chirp() {
  beep(1500, CHIRP_DELAY);
  delay(CHIRP_DELAY);

  beep(1000, CHIRP_DELAY);
  delay(CHIRP_DELAY);

  beep(1500, CHIRP_DELAY);
}

void beep_error() {
  beep(300/2, 256);
  delay(10);
  beep(150/2, 256);
}

void do_wifi(bool output) {
  delay(250);
  if (WiFi.status() != WL_CONNECTED) {
    if (output) {
      Serial.print("Connecting to ");
      Serial.println(ssid);
    }

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      beep_chirp();
      Serial.print(".");
      delay(250);
    }

    if (output) {
      Serial.print("ESP32-CAM IP Address: ");
      Serial.println(WiFi.localIP());
    }
  }
}

void setup() {
  pinMode(SPEAKER, OUTPUT);
  pinMode(BUTTON, INPUT);

  ledcSetup(LEDC_CHAN, 2000, 8);
  ledcAttachPin(SPEAKER, LEDC_CHAN);

  beep_chirp();

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_XGA;
  config.jpeg_quality = 15;
  config.fb_count = 1;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  WiFi.mode(WIFI_STA);
  do_wifi(true);

  // Serial.printf("freq: %d\n", getCpuFrequencyMhz());
  // setCpuFrequencyMhz(80);
  // Serial.printf("freq: %d\n", getCpuFrequencyMhz());

  beep_success();
  esp_sleep_enable_timer_wakeup(SLEEP_SECS * uS_TO_S_FACTOR);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON, 0);
}

void sleep() {
  esp_light_sleep_start();
}

int state = 0;

bool get_connection() {
  client.stop();
  delay(100);
  Serial.print("Connecting to " + server_name + ": ");
  for (int i = 0; i < 10; i++) {
    delay(100);

    if (client.connect(server_name_c, server_port)) {
      beep_chirp();
      Serial.println(" success");
      return true;
    }

    Serial.print("-");
    delay(250);
    do_wifi(false);
  }

  Serial.println(" failed");
  return false;
}

void ping_server() {
  Serial.println("pinging server: " + server_name);
  if (get_connection()) {
    client.println("GET /ping HTTP/1.1");
    client.println("");
    Serial.println("ping successful!");
  }
}

void loop() {
  state = digitalRead(BUTTON);
  if (!state) {
    Serial.println("button pressed");
    sendPhoto();
  } else {
    ping_server();
    delay(100);
    sleep();
  }
}

void sendPhoto() {
  if (get_connection()) {
    camera_fb_t * fb = NULL;
    beep_one_two_three_go();
    fb = esp_camera_fb_get();

    beep_chirp();

    if (!fb) {
      Serial.println("Camera capture failed");
      ESP.restart();
    }

    String tail = "\r\n#DEADBEEF#\r\n";

    uint32_t imageLen = fb->len;
    // uint32_t extraLen = head.length() + tail.length();
    uint32_t extraLen = tail.length();
    uint32_t totalLen = imageLen + extraLen;

    client.println("POST " + server_path + " HTTP/1.1");
    client.println("Host: " + server_name);

    client.print("Content-Disposition: form-data; name=\"imageFile\"; filename=\"");
    client.print(year());
    client.print(month());
    client.print(day());
    client.print(hour());
    client.print(minute());
    client.print(second());
    client.println(".jpg\"");

    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=#DEADBEEF#");
    client.println("");

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;

    for (size_t n = 0; n < fbLen; n = n + HTTP_BUFF) {
      beep_chirp();
      if (n + HTTP_BUFF < fbLen) {
        client.write(fbBuf, HTTP_BUFF);
        fbBuf += HTTP_BUFF;
      } else if (fbLen % HTTP_BUFF > 0) {
        size_t remainder = fbLen % HTTP_BUFF;
        client.write(fbBuf, remainder);
      }
    }

    client.print(tail);

    delay(100);
    beep_sweep();
    esp_camera_fb_return(fb);
  } else {
    Serial.println("Connection to " + server_name +  " failed.");
    beep_error();
  }

  sleep();
}
