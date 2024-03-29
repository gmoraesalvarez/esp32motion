#include "src/OV2640.h"
#include <WiFi.h>
#include "esp_http_client.h"
#include "src/U8x8lib.h"
#include "avi.h"

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 22, /* data=*/ 21);

#define PIR_PIN 33

int status_limit = 90000;
int orders_limit = 10000;
int timer_bg = millis();
int timer_status = millis();
int timer_orders = millis();
int program = 1;
//// send_clip variables reused across loops and global to avoid stack smashing error
int megabytes = 1024*1024*16;   // max file size (has to fit psram if sending full clip)
uint8_t* clientBuf[500];
uint8_t* idxBuf[5000];
int frameCnt = 0;
const int maxframeCnt = 200; // this could be as big as the psram allows if full clip in memory
                          // or any value that the backend allows if chunked streaming
word fileSize = 240; // this is the position to add the frame data after the avi header
word idxbufSize = 0;
word jpegSize[maxframeCnt]; // this array will store the size of each frame


#if defined(ESP32)
  #define USE_SPIFFS            true
  #define ESP_DRD_USE_EEPROM    true
#else  
  #error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.  
#endif
#include <ESP_WiFiManager.h>                    //https://github.com/khoih-prog/ESP_WiFiManager
#define DRD_TIMEOUT             2
#define DRD_ADDRESS             0
#include <ESP_DoubleResetDetector.h>            //https://github.com/khoih-prog/ESP_DoubleResetDetector
DoubleResetDetector* drd;
bool      initialConfig = false;

const char *status_str = "cam_ttgo";

const char *post_url = "http://storageserver.com/in.php?pic=motion_detect&id=cam_ttgo"; // Location where images are POSTED
const char *mjpeg_url = "http://storageserver.com/in.php?pic=mjpeg&id=cam_ttgo";
const char *status_url = "http://commandserver.com/in.php?pic=status&id=cam_ttgo";
const char *orders_url = "http://commandserver.com/in.php?pic=orders&id=cam_ttgo";
const char *motion_url = "http://commandserver.com/in.php?pic=motion_debug&id=cam_ttgo";
const char *bg_url = "http://commandserver.com/in.php?pic=motion_debug_bg&id=cam_ttgo";
const char *led_url = "http://commandserver.com/in.php?pic=led&id=cam_ttgo";


OV2640 cam;

void setup()
{
  gpio_install_isr_service(0);
  u8x8.begin();
  u8x8.setFont(u8x8_font_5x7_f);
  //u8x8.setFont(u8x8_font_amstrad_cpc_extended_r);

  pinMode(PIR_PIN, INPUT); //pir

  cam.init(esp32cam_ttgo_t_config);

  u8x8.clear();
  u8x8.setCursor(1, 1);
  u8x8.print("wifi setup");
  Serial.begin(115200); while (!Serial); delay(200);
  Serial.print(F("\nStarting ConfigOnDoubleReset_minimal on ")); Serial.println(ARDUINO_BOARD);
  Serial.println(ESP_WIFIMANAGER_VERSION); 
  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  if (drd->detectDoubleReset()) { Serial.println(F("DRD")); initialConfig = true; }
  ESP_WiFiManager ESP_wifiManager("ConfigOnDoubleReset");
  if (ESP_wifiManager.WiFi_SSID() == "") { Serial.println(F("No AP credentials")); initialConfig = true; }
  if (initialConfig) {
    u8x8.clear();
    u8x8.setCursor(1, 1);
    u8x8.print("CONFIG PORTAL");
    Serial.println(F("Starting Config Portal")); 
    if (!ESP_wifiManager.startConfigPortal()) { Serial.println(F("Not connected to WiFi")); u8x8.clear(); u8x8.setCursor(1, 5); u8x8.print("CONN ERR"); }
    else { Serial.println(F("connected")); u8x8.clear(); u8x8.setCursor(1, 5); u8x8.print("CONN OK"); }
  }
  else { WiFi.mode(WIFI_STA); WiFi.begin(); } 
  unsigned long startedAt = millis();
  Serial.print(F("After waiting "));
  int connRes = WiFi.waitForConnectResult();
  float waited = (millis() - startedAt);
  Serial.print(waited / 1000); Serial.print(F(" secs , Connection result is ")); Serial.println(connRes);
  if (WiFi.status() != WL_CONNECTED) { Serial.println(F("Failed to connect")); }
  else { Serial.print(F("Local IP: ")); Serial.println(WiFi.localIP()); u8x8.clear(); u8x8.setCursor(1, 5); u8x8.print(WiFi.localIP());}


  Serial.println("Notify");
  status_notify(); 
}

void check_orders() {
  esp_http_client_handle_t http_client;

  esp_http_client_config_t config_client = {0};
  config_client.url = orders_url;
  config_client.event_handler = _http_event_handler;
  config_client.method = HTTP_METHOD_POST;

  http_client = esp_http_client_init(&config_client);

  esp_http_client_set_post_field(http_client, status_str, 8);

  esp_http_client_set_header(http_client, "Content-Type", "text/plain");

  esp_err_t err = esp_http_client_perform(http_client);

  if (err == ESP_OK) {
    Serial.print("esp_http_client_get_status_code: ");
    Serial.println(esp_http_client_get_status_code(http_client));
  }

  esp_http_client_cleanup(http_client);
  Serial.println("Checked orders");
}

void status_notify() {
  esp_http_client_handle_t http_client;

  esp_http_client_config_t config_client = {0};
  config_client.url = status_url;
  config_client.event_handler = _http_event_handler;
  config_client.method = HTTP_METHOD_POST;

  http_client = esp_http_client_init(&config_client);

  esp_http_client_set_post_field(http_client, status_str, 16);

  esp_http_client_set_header(http_client, "Content-Type", "text/plain");

  esp_err_t err = esp_http_client_perform(http_client);

  if (err == ESP_OK) {
    Serial.print("esp_http_client_get_status_code: ");
    Serial.println(esp_http_client_get_status_code(http_client));
  }

  esp_http_client_cleanup(http_client);
  Serial.println("Finish notify");
}

void process_orders(String orders){
    Serial.printf("program was %d \n", program);
    String COMMAND = orders.substring(0, 3);
    String LED = orders.substring(4, 7);
    String BGLIM = orders.substring(8, 11);
    String BLDIF = orders.substring(12, 15);
    String IMDIF = orders.substring(16, 19);
    String CTRL = orders.substring(20, 31);
    //Serial.println(CTRL);
    if ( CTRL == "CTRL_STRING"){
      Serial.println("Received control string from server");
      if (COMMAND == "MDT") {
        program = 1;
      }
      if (COMMAND == "PIC") {
        program = 2;
      }
      if (COMMAND == "SLP") {
        program = 0;
        //digitalWrite(LAMP, LOW);
      }
      if (LED == "YLE") {
        //digitalWrite(LAMP, HIGH);
      }
      if (LED == "NLE") {
        //digitalWrite(LAMP, LOW);
      }
      if (BGLIM.toInt() > 0) {
        //bg_limit = BGLIM.toInt() * 1000;
      }
      if (BLDIF.toInt() > 0) {
        //BLOCK_DIFF_THRESHOLD = BLDIF.toInt() * 0.01;
      }
      if (IMDIF.toInt() > 0) {
        //IMAGE_DIFF_THRESHOLD = IMDIF.toInt() * 0.01;
      }
    } else { Serial.println("Not an ORDERS string."); }
    Serial.printf("program is now %d \n", program);
    u8x8.clear(); u8x8.setCursor(1, 5); u8x8.print("PROG"); u8x8.setCursor(7, 5); u8x8.print(program);
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
  String str = "__________________________________";
  switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
      Serial.println("HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      Serial.println("HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      Serial.println("HTTP_EVENT_HEADER_SENT");
      break;
    case HTTP_EVENT_ON_HEADER:
      Serial.println();
      Serial.printf("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      Serial.println();
      Serial.printf("HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      printf("%.*s \n", evt->data_len, (char*)evt->data);
      
      str = (char*)evt->data;
      Serial.println("got data from the server:");
      Serial.println(str);

      process_orders(str);

      break;
    case HTTP_EVENT_ON_FINISH:
      Serial.println("");
      Serial.println("HTTP_EVENT_ON_FINISH");
      break;
    case HTTP_EVENT_DISCONNECTED:
      Serial.println("HTTP_EVENT_DISCONNECTED");
      break;
  }
  return ESP_OK;
}

void send_clip()
{
  u8x8.clear();
  u8x8.setCursor(1, 1);
  u8x8.print(" _______ ");
  u8x8.setCursor(1, 2);
  u8x8.print("|       |");
  u8x8.setCursor(1, 3);
  u8x8.print("|  (O)  |");
  u8x8.setCursor(1, 4);
  u8x8.print("|       |");
  u8x8.setCursor(1, 5);
  u8x8.print("|_____[]|");
  u8x8.setCursor(1, 6);
  u8x8.print(" | |     ");
  u8x8.setCursor(1, 7);
  u8x8.print(" |_|     ");

/*
  u8x8.print("   _____ ");
  u8x8.print("  |  -  |");
  u8x8.print("  | (O) |");
  u8x8.print("  |  -  |");
  u8x8.print("  |_'_[]|");
  u8x8.print("   | |     ");
  u8x8.print("   |_|     ");
*/

  Serial.println("--");
  Serial.println("SEND THEM CHUNKS");
  esp_http_client_config_t chunk_config = {0};
  chunk_config.url = mjpeg_url;

	esp_http_client_handle_t chunk_client = esp_http_client_init(&chunk_config);

	esp_err_t err = esp_http_client_open(chunk_client, -1); // write_len=-1 sets header "Transfer-Encoding: chunked" and method to POST
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
		//return 0;
    return;
	}
  esp_http_client_set_header(chunk_client, "Content-Type", "video/x-motion-jpeg");

  Serial.println("connected");

  ////////////////////////////////////////////////////////////////////////////////
  fileSize = 240; // have to reset this every new clip
  memcpy(clientBuf, aviHeader, fileSize); // copy the avi header template (240 bytes), have to do it every new clip

  int headroom = 200000;
  frameCnt = 0; // gotta reset this too
  const char* dwFourCC = "00db";  // this and the frame size in bytes goes in before every frame
  char chunk_size_char[16];
  //char http_msg[30];
  Serial.println("capturing");
  for (int i = 0;i < maxframeCnt; i++){
    frameCnt++;

    cam.run();
    jpegSize[i] = cam.getSize();
    //fileSize += 8;
    esp_http_client_write(chunk_client,"4", 1); // length
    esp_http_client_write(chunk_client,"\r\n", 2);
    esp_http_client_write(chunk_client, dwFourCC, 4); // data
    esp_http_client_write(chunk_client,"\r\n", 2);
    //Serial.println("sent fourcc");
    fileSize += 4;

    esp_http_client_write(chunk_client,"4", 1); // length
    esp_http_client_write(chunk_client,"\r\n", 2);
    esp_http_client_write(chunk_client, (char*)&jpegSize[i], 4); // data 
    esp_http_client_write(chunk_client,"\r\n", 2);
    //Serial.print("sent length: ");
    //Serial.println((char*)&jpegSize[i]);
    fileSize += 4;
    sprintf(chunk_size_char,"%08X",jpegSize[i]);
    //Serial.println(jpegSize[i]);
    //Serial.print("format size: ");
    //Serial.println(chunk_size_char);     
    esp_http_client_write(chunk_client,chunk_size_char, 8); // length
    esp_http_client_write(chunk_client,"\r\n", 2);


    esp_http_client_write(chunk_client,(char*)cam.getfb(), jpegSize[i]); // data   , 
    esp_http_client_write(chunk_client,"\r\n", 2);
    Serial.print("sent frame: ");
    Serial.println(i);
    fileSize += jpegSize[i];

    headroom = jpegSize[i] * 2;
    if ( fileSize > (megabytes - headroom) ){ break; }  // break the photo capture around 2 framesizes before the end of allocated memory
    
  }

  u8x8.clear();
  u8x8.setCursor(1, 1);
  u8x8.print("     ____");
  u8x8.setCursor(1, 2);
  u8x8.print("    / = /");
  u8x8.setCursor(1, 3);
  u8x8.print("  ,/___/");
  u8x8.setCursor(1, 4);
  u8x8.print("  |:::|");
  u8x8.setCursor(1, 5);
  u8x8.print("  |:::|");
  u8x8.setCursor(1, 6);
  u8x8.print("  |[_]|");
  u8x8.setCursor(1, 7);
  u8x8.print("     l");
  //u8x8.setCursor(12, 1);
  //u8x8.print(frameCnt);
/*
 __i
|---|    
|[_]|    
|:::|    
|:::|    
`\   \   
  \_=_\ 
   ____
  / = / 
,/___/  
|:::|    
|:::|
|[_]|
   l
*/

  // these commands will replace the relevant data in the header
  word jpgs_width = cam.getWidth();
	word jpgs_height = cam.getHeight();

  word dwSize = fileSize - 8;
  memcpy(clientBuf+4, &dwSize, 4);
  dwSize = fileSize - 20;
  memcpy(clientBuf+16, &dwSize, 4);
  word dwTotalFrames = frameCnt;
  memcpy(clientBuf+48, &dwTotalFrames, 4);
  memcpy(clientBuf+64, &jpgs_width, 4);
  memcpy(clientBuf+68, &jpgs_height, 4);
  memcpy(clientBuf+140, &dwTotalFrames, 4);
  memcpy(clientBuf+168, &jpgs_width, 4);
  memcpy(clientBuf+172, &jpgs_height, 4);
  //word biSizeImage = ((jpgs_width*24/8 + 3)&0xFFFFFFFC)*jpgs_height;
  memcpy(clientBuf+184, &jpgs_height, 4);
  memcpy(clientBuf+224, &dwTotalFrames, 4);
  dwSize = fileSize - 232;
  memcpy(clientBuf+236, &dwSize, 4);

  // these commands will create the index in the end of the avi buffer;
  word index_length = 4*4*frameCnt;
  
  idxbufSize = 0;
  dwFourCC = "idx1";
  memcpy(idxBuf+idxbufSize, dwFourCC, 4);
  idxbufSize += 4;
  memcpy(idxBuf+idxbufSize, &index_length, 4);

  unsigned long AVI_KEYFRAME = 16;
	unsigned long offset_count = 4;
  dwFourCC = "00db";
  for (int i=0;i<frameCnt;i++){
    idxbufSize += 4;
    memcpy(idxBuf+idxbufSize, dwFourCC, 4);
    idxbufSize += 4;
    memcpy(idxBuf+idxbufSize, &AVI_KEYFRAME, 4);
    idxbufSize += 4;
    memcpy(idxBuf+idxbufSize, &offset_count, 4);
    idxbufSize += 4;
    memcpy(idxBuf+idxbufSize, &jpegSize[i], 4);
    offset_count += jpegSize[i]+8;
  }
  fileSize += idxbufSize;                         // this is part of that

  /// sending avi index chunk. this should end the file after the backend code moves the header to the head.
  char chunk_indexsize_char[9];
  sprintf(chunk_indexsize_char,"%08X", idxbufSize);
  esp_http_client_write(chunk_client,chunk_indexsize_char, 8); // length
  esp_http_client_write(chunk_client,"\r\n", 2);
  esp_http_client_write(chunk_client,(char*)idxBuf, idxbufSize); // data 
  esp_http_client_write(chunk_client,"\r\n", 2);

  /// sending the header chunk to the end of the file. Server side code will have to deal with
  /// putting it on the head
  char chunk_headsize_char[9];
  sprintf(chunk_headsize_char,"%08X", 240); //header size is 240 bytes
  esp_http_client_write(chunk_client,chunk_headsize_char, 8); // length
  esp_http_client_write(chunk_client,"\r\n", 2);
  esp_http_client_write(chunk_client,(char*)clientBuf, 240); // data 
  esp_http_client_write(chunk_client,"\r\n", 2);

  Serial.println("frames captured");
  Serial.println(frameCnt);
  Serial.println("avi size"); // hopefully everything fits the allocated memory
  Serial.println(fileSize);

  //  finish the chunked stream
	esp_http_client_write(chunk_client,"0", 1);  // end
	esp_http_client_write(chunk_client,"\r\n", 2);
	esp_http_client_write(chunk_client,"\r\n", 2);
  
  Serial.println("sent end of stream");
  Serial.println();
  Serial.print("esp_http_client_fetch_headers: ");
	Serial.println(esp_http_client_fetch_headers(chunk_client)); //for content length
  Serial.print("esp_http_client_get_status_code: ");
	Serial.println(esp_http_client_get_status_code(chunk_client));
  Serial.println();
	//	esp_http_client_read()
	esp_http_client_close(chunk_client);
	esp_http_client_cleanup(chunk_client);


  u8x8.clear();
  u8x8.setCursor(1, 1);
  u8x8.print("  vv        ");
  u8x8.setCursor(1, 2);
  u8x8.print("  vvv       ");
  u8x8.setCursor(1, 3);
  u8x8.print("  vvv       ");
  u8x8.setCursor(1, 4);
  u8x8.print("  v vv      ");
  u8x8.setCursor(1, 5);
  u8x8.print("  v   vv    ");
  u8x8.setCursor(1, 6);
  u8x8.print("  v    vv   ");
  u8x8.setCursor(1, 7);
  u8x8.print(" v       vv ");

  Serial.println("back to the loop");
  //return 1;
}


void loop()
{
  drd->loop();
  if (program == 1) {
    if (digitalRead(PIR_PIN)) { // If movement - Take photo
      //send_photo();
      send_clip();
    }
  }
  
  if (program == 2) { // SEND PHOTO NOW ROUTINE
    //send_photo();
    send_clip();
  }
  
  //if (program == 0) { // JUST SLEEP

  //}
  if ( (millis() - timer_status) > status_limit ) {
      u8x8.clear();
      Serial.println("Notify");
      status_notify();
      timer_status = millis();
  }
  if ( (millis() - timer_orders) > orders_limit ) { // CHECK ORDERS
    Serial.println("Check orders");
    check_orders();
    //check_led();
    timer_orders = millis();
  }
}
