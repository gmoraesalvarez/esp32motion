#include "esp_camera.h"
#include "WiFi.h"
#include "avi.h"
#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems
#include "esp_http_client.h"
//#include "FS.h"
//#include "SPIFFS.h"
#define JPEG 1
#define BMP 0
#define MOTION_DETECT 1
#define PIR 0
#if JPEG
  #include "JPEGDEC.h"
  JPEGDEC jpeg;
#endif
/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me−no−dev/arduino−esp32fs−plugin */
//#define FORMAT_SPIFFS_IF_FAILED true
#define FRAME_SIZE_COMP FRAMESIZE_QVGA
#define FRAME_SIZE_SEND FRAMESIZE_UXGA
#define FRAME_SIZE_TEST FRAMESIZE_XGA
#define WIDTH 1024
#define HEIGHT 768
#define BLOCK_SIZE 16  // that's X*X pixels
//#define SAMPLES_COUNT 48 // should be at most half the block size squared. should fit accumulated pixel values in the chosen var type
#define W (WIDTH / BLOCK_SIZE)
#define H (HEIGHT / BLOCK_SIZE)
//#define BLOCK_DIFF_THRESHOLD 0.35
//#define IMAGE_DIFF_THRESHOLD 0.15
#define DEBUG 0
#define LAMP 4
#define LAMPB 15
#define PIR_PIN 14

int SAMPLES_COUNT = 48;
float BLOCK_DIFF_THRESHOLD = 0.35;
float IMAGE_DIFF_THRESHOLD = 0.15;
//uint16_t **prev_frame;
//uint16_t **current_frame;
uint16_t prev_frame[H][W];
uint16_t current_frame[H][W];

int bg_limit = 10000;
int status_limit = 90000;
int orders_limit = 10000;

int timer_bg = millis();
int timer_status = millis();
int timer_orders = millis();
String prev_frame_s = "";
String cur_frame_s = "";
int program = 1;

camera_fb_t *frame_buffer = NULL;

//// send_clip variables reused across loops and global to avoid stack smashing error
int megabytes = 1024*1024*8;   // max file size (has to fit psram if sending full clip)
uint8_t* clientBuf[500];
uint8_t* idxBuf[5000];
int frameCnt = 0;
const int maxframeCnt = 200; // this could be as big as the psram allows if full clip in memory
                          // or any value that the backend allows if chunked streaming
word fileSize = 240; // this is the position to add the frame data after the avi header
word idxbufSize = 0;
word jpegSize[maxframeCnt]; // this array will store the size of each frame


//Replace with your network credentials
const char *ssid = "";
const char *password = "";
const char *ssid_alt = "";
const char *password_alt = "";
const char *status_str = "aqua_pir";

const char *post_url = "http://site/in.php?pic=motion_detect&id=cam"; // Location where images are POSTED
const char *mjpeg_url = "http://somesite.com/in.php?pic=mjpeg&id=cam";
const char *status_url = "http://site/in.php?pic=status&id=cam";
const char *orders_url = "http://site/in.php?pic=orders&id=cam";
const char *motion_url = "http://site/in.php?pic=motion_debug&id=cam";
const char *bg_url = "http://site/in.php?pic=motion_debug_bg&id=cam";
const char *led_url = "http://site/in.php?pic=led&id=cam";


#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
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
#else
#error "Camera model not selected"
#endif

bool setup_camera(framesize_t frameSize, pixformat_t pixFormat) {
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
  config.xclk_freq_hz = 20000000;
  //config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.pixel_format = pixFormat;
  config.frame_size = frameSize;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  bool ok = esp_camera_init(&config) == ESP_OK;

  return ok;
}

/*void writeFile(fs::FS &fs, const char * path, const char * message){
   Serial.printf("Writing file: %s\r\n", path);

   File file = fs.open(path, FILE_WRITE);
   if(!file){
      Serial.println("− failed to open file for writing");
      return;
   }
   if(file.print(message)){
      Serial.println("− file written");
   }else {
      Serial.println("− frite failed");
   }
}
void appendFile(fs::FS &fs, const char * path, const char * message){
   Serial.printf("Appending to file: %s\r\n", path);

   File file = fs.open(path, FILE_APPEND);
   if(!file){
      Serial.println("− failed to open file for appending");
      return;
   }
   if(file.print(message)){
      Serial.println("− message appended");
   } else {
      Serial.println("− append failed");
   }
}
*/
void print_frame(uint16_t frame[H][W]) {
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      Serial.print(frame[y][x]);
      Serial.print("\t");
    }
    Serial.println();
  }
  delay(2000);
}

int start_wifi(){
  Serial.printf("connecting to %s\n",ssid);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname);
  int cur_ssid = 0;
  Serial.println(WiFi.macAddress());
  if (cur_ssid == 1) {
        WiFi.begin(ssid_alt, password_alt);
        Serial.printf("connecting to %s\n",ssid_alt);
      } else {
        WiFi.begin(ssid, password);
        Serial.printf("connecting to %s\n",ssid);
      }
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (tries > 15) {
      WiFi.disconnect();
      WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
      WiFi.setHostname(hostname);
      tries = 0;
      if (cur_ssid == 0) {
        cur_ssid = 1;
        WiFi.begin(ssid_alt, password_alt);
        Serial.printf("connecting to %s\n",ssid_alt);
      } else {
        cur_ssid = 0;
        WiFi.begin(ssid, password);
        Serial.printf("connecting to %s\n",ssid);
      }
    }
    Serial.print(".");
    tries++;
  }
}

void setup() {
  //if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
  //    Serial.println("SPIFFS Mount Failed");
  //    return;
  // }
  //uint16_t _prev_frame[H][W];
  //uint16_t _current_frame[H][W];
  //prev_frame[H][W] = _prev_frame[H][W];
  //current_frame[H][W] = _current_frame[H][W];
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  pinMode(PIR_PIN, INPUT); //pir
  pinMode(LAMP, OUTPUT);
  pinMode(LAMPB, OUTPUT);
  digitalWrite(LAMP, LOW);
  digitalWrite(LAMPB, HIGH);
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.printf("DIFF THRESH =  %.2f\n",BLOCK_DIFF_THRESHOLD);
  Serial.println(setup_camera(FRAME_SIZE_TEST, PIXFORMAT_JPEG) ? "CAMERA INITIAL SETUP OK" : "ERR INIT");

  //frame_buffer = esp_camera_fb_get();
  //esp_camera_fb_return(frame_buffer); // release framebuffer immediately
  //frame_buffer = NULL;

  capture_still();
  delay(1000);
  //update_frame();
  //print_frame(prev_frame);

  // Wi-Fi connection
  start_wifi();

  WiFi.setSleep(false);

  Serial.println("");
  Serial.println("WiFi connected");

  Serial.print(WiFi.localIP());
  Serial.println("");
  delay(2000);

  capture_still();
  delay(1000);
  update_frame();
  send_background();

  status_notify();
}

/**
   For serial debugging
   @param frame
*/

void print_motion(uint16_t frame[H][W]) {
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      Serial.print(frame[y][x]);
      Serial.print("\t");
    }
    Serial.println();
  }
  delay(2000);
}

#if JPEG
int JPEGDraw(JPEGDRAW *pDraw){
  int w = pDraw->iWidth; 
  int h = pDraw->iHeight;
  int _x = pDraw->x;
  int _y = pDraw->y;
  int divisor = (BLOCK_SIZE/8);
  //int bpp = pDraw->iBpp;
  //Serial.printf("bit depth %d, MCU_W %d, MCU_H %d, y_off %d, x_off %d\n", bpp,w,h,_y,_x);
  //for (int y = 0; y < h; y++) { // _x, _y are the functions offset for the current block of decoded pixels.
    if ( _y % divisor == 0 )
    for (int x = 0; x < (w/divisor); x++) { // pDraw->pPixels[x*divisor/2];
      uint16_t pixel = pDraw->pPixels[x*divisor/2];
      //uint8_t red = ((pixel & 0xF800)>>11);
      uint8_t green = ((pixel & 0x07E0)>>5);
      //uint8_t blue = (pixel & 0x001F);
      //uint16_t grayscale = (0.2126 * red) + (0.7152 * green / 2.0) + (0.0722 * blue);
      //current_frame[_y/divisor][x] = (grayscale<<11)+(grayscale<<6)+grayscale; // somehow the JPEGDEC library compresses the 200 pixels to 100.
      current_frame[_y/divisor][x] = green;
    }
  //}
  //Serial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);  
  return 1; // continue decode
}

int decode_jpeg(){
  long lTime;
  if (jpeg.openRAM(frame_buffer->buf, frame_buffer->len, JPEGDraw)) {
    //Serial.printf("JPEG opened\n");
    //jpeg.setPixelType(EIGHT_BIT_GRAYSCALE);
    //lTime = micros();
    if (jpeg.decode(0,0,JPEG_SCALE_EIGHTH)) { // 1/8 sized decode
      //lTime = micros() - lTime;
      //Serial.printf("eighth sized decode in %d us\n", (int)lTime);
    }
    jpeg.close();
  }
}

int downsample_jpeg(){
  //int start = millis();
  // set all 0s in current frame and motion frame
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) {
      current_frame[y][x] = 0;
      //motion_frame[y][x] = 0;
    }
    
  decode_jpeg();
  
  cur_frame_s = "";
  cur_frame_s += status_str;
  cur_frame_s += "\n";
  char tmp[8];
  for (int y = 0; y < H; y++){
    for (int x = 0; x < W; x++) {
      sprintf(tmp,"%d\t",current_frame[y][x]);
      cur_frame_s += tmp;
    }
    cur_frame_s += "\n";
  }
  //int finish = millis() - start;  
  //Serial.printf("Downsampling jpeg took %d \n", finish);

}
#endif

#if BMP
int downsample_bmp(){
  cur_frame_s = "";
  cur_frame_s += status_str;
  cur_frame_s += "\n";
  char tmp[8];
  uint8_t * buf = NULL;
  size_t buf_len = 0;
  int start = millis();
  bool converted = frame2bmp(frame_buffer, &buf, &buf_len); // let's convert the JPEG frame buffer to bitmap for easyish manipulation of the pixels.
  int finish = millis() - start;
  Serial.printf("Converting to bmp took %d \n", finish);
  if (!converted) {
    ESP_LOGE(TAG, "BMP conversion failed");
    return ESP_FAIL;
  }
  
  // set all 0s in current frame and motion frame
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) {
      current_frame[y][x] = 0;
      //motion_frame[y][x] = 0;
    }
    
  // down-sample image in blocks
  //start = millis();
  for (int y = 0; y < H; y++) { // run per block
    for (int x = 0; x < W; x++) {
      int real_y = y * BLOCK_SIZE * 3; // Translate the first pixel position Y of the block in the buffer. Each pixel is 3 bytes, so the position must be multiplied by 3.
      int real_x = (x * BLOCK_SIZE * 3) + 54; // Same thing for X, with the addition of the header. The header is added to X because the buffer runs the X axis first.
      uint16_t cumulo_count = 1;
      int first_pixel_pos = (real_y * WIDTH) + real_x;  // top left pixel of the block
      // Put first pixel in, all 3 bytes, divided by 3 to average into grayscale.
      current_frame[y][x] = ((uint16_t)buf[first_pixel_pos] + (uint16_t)buf[first_pixel_pos + 1] + (uint16_t)buf[first_pixel_pos + 2]) / 3;

      for (int oy = 1; oy < (SAMPLES_COUNT / BLOCK_SIZE); oy++) {  // Loop X and Y the number of samples to take.
        for (int ox = 1; ox < (SAMPLES_COUNT / BLOCK_SIZE); ox++) {
          int offset_result = first_pixel_pos + (oy * WIDTH * 3) + (ox * 3);
          uint64_t pixel = buf[offset_result] + buf[offset_result + 1] + buf[offset_result + 2] / 3; // Take all 3 bytes and average them.
          current_frame[y][x] += pixel; // accumulate offset pixel
          cumulo_count++;
        }
      }
      current_frame[y][x] /= cumulo_count; // average the offset pixels together
      sprintf(tmp,"%d\t",current_frame[y][x]);
      cur_frame_s += tmp;
    }
    cur_frame_s += "\n";
  }
  free(buf);
  //finish = millis() - start;
  //Serial.printf("Downsampling took %d \n", finish);
}
#endif
/**
   Capture image and do down-sampling
*/
bool capture_still() {
  esp_camera_fb_return(frame_buffer); // release previous framebuffer
  frame_buffer = NULL;
  //int start = millis();
  frame_buffer = esp_camera_fb_get();
  //camera_fb_t *frame_buffer = esp_camera_fb_get();
  //Serial.println((char *)frame_buffer->buf);
  if (!frame_buffer)
    return false;
  //int finish = millis() - start;
  //Serial.printf("Pic taking took %d \n", finish);  

  return true;
}

/**
   Compute the number of different blocks
   If there are enough, then motion happened
*/
bool motion_detect() {
  uint16_t changes = 0;
  const uint16_t blocks = W * H;
  float delta = 0.01;
  Serial.printf("BLOCK_DIFF_THRESH %.2f\n", BLOCK_DIFF_THRESHOLD);
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      delta = abs(1.0 * (current_frame[y][x] - prev_frame[y][x]) / prev_frame[y][x]);
      if (delta > BLOCK_DIFF_THRESHOLD) {
        changes += 1;
        //Serial.printf("D: %.2f > %.2f)\t",delta,BLOCK_DIFF_THRESHOLD);
#if DEBUG
        Serial.print("diff\t");
        Serial.print(y);
        Serial.print('\t');
        Serial.println(x);
        motion_frame[y][x] = current_frame[y][x];
#endif
      }
    }
    //Serial.println();
  }
  //#if DEBUG
  Serial.printf("Changed %d out of %d\n", changes, blocks);
  //Serial.println("motion frame");
  //print_motion(motion_frame);
  //Serial.println("---------------");
  //#endif
  return (1.0 * changes / blocks) > IMAGE_DIFF_THRESHOLD;
}


/**
   Copy current frame to previous
*/
void update_frame() {
  char tmp[8];
  prev_frame_s = "";
  prev_frame_s += status_str;
  prev_frame_s += "\n";
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      prev_frame[y][x] = current_frame[y][x];
      sprintf(tmp,"%d\t",prev_frame[y][x]);
      prev_frame_s += tmp;
    }
    prev_frame_s += "\n";
  }
}

int process_orders(String orders){
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
        digitalWrite(LAMP, LOW);
        digitalWrite(LAMPB, HIGH);
      }
      if (LED == "YLE") {
        digitalWrite(LAMP, HIGH);
        digitalWrite(LAMPB, LOW);

      }
      if (LED == "NLE") {
        digitalWrite(LAMP, LOW);
        digitalWrite(LAMPB, HIGH);
      }
      if (BGLIM.toInt() > 0) {
        bg_limit = BGLIM.toInt() * 1000;
      }
      if (BLDIF.toInt() > 0) {
        BLOCK_DIFF_THRESHOLD = BLDIF.toInt() * 0.01;
      }
      if (IMDIF.toInt() > 0) {
        IMAGE_DIFF_THRESHOLD = IMDIF.toInt() * 0.01;
      }
    } else { Serial.println("Not an ORDERS string."); }
    Serial.printf("program is now %d \n", program);
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
  String str = "";
  switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
      Serial.println("HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      Serial.printf("HTTP_EVENT_ON_CONNECTED -> ");
      break;
    case HTTP_EVENT_HEADER_SENT:
      Serial.printf("HTTP_EVENT_HEADER_SENT -> ");
      break;
    case HTTP_EVENT_ON_HEADER:
      //Serial.println();
      Serial.printf("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      //Serial.println();
      Serial.printf(" -> HTTP_EVENT_ON_DATA, len=%d \n", evt->data_len);
      printf("%.*s \n", evt->data_len, (char*)evt->data);
      
      str += (char*)evt->data;
      Serial.println(str);

      process_orders(str);

      //if (!esp_http_client_is_chunked_response(evt->client)) {
      // Write out data
      //printf("%.*s", evt->data_len, (char*)evt->data);
      //}
      break;
    case HTTP_EVENT_ON_FINISH:
      //Serial.println("");
      Serial.println(" -> HTTP_EVENT_ON_FINISH");
      break;
    case HTTP_EVENT_DISCONNECTED:
      Serial.println(" -> HTTP_EVENT_DISCONNECTED");
      break;
  }
  return ESP_OK;
}

static esp_err_t send_photo()
{
  esp_http_client_handle_t http_client;

  esp_http_client_config_t config_client = {0};
  config_client.url = post_url;
  config_client.event_handler = _http_event_handler;
  config_client.method = HTTP_METHOD_POST;

  http_client = esp_http_client_init(&config_client);

  esp_http_client_set_post_field(http_client, (char *)frame_buffer->buf, frame_buffer->len);

  esp_http_client_set_header(http_client, "Content-Type", "image/jpg");

  esp_err_t err = esp_http_client_perform(http_client);

  if (err == ESP_OK) {
    Serial.print("esp_http_client_get_status_code: ");
    Serial.println(esp_http_client_get_status_code(http_client));
  }

  esp_http_client_cleanup(http_client);

}


void send_clip()
{

  Serial.println("--");
  Serial.println("SEND THEM CHUNKS");
  esp_http_client_config_t chunk_config = {0};
  chunk_config.event_handler = _http_event_handler;
  chunk_config.url = mjpeg_url;
  chunk_config.method = HTTP_METHOD_POST;

	esp_http_client_handle_t chunk_client = esp_http_client_init(&chunk_config);

  esp_http_client_set_header(chunk_client, "Content-Type", "video/x-motion-jpeg");
  esp_http_client_set_header(chunk_client, "Transfer-Encoding", "chunked");

	esp_err_t err = esp_http_client_open(chunk_client, -1); // write_len=-1 sets header "Transfer-Encoding: chunked" and method to POST
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
		//return 0;
    return;
	}

  Serial.println("connected");

  ////////////////////////////////////////////////////////////////////////////////
  fileSize = 240; // have to reset this every new clip
  memcpy(clientBuf, aviHeader, fileSize); // copy the avi header template (240 bytes), have to do it every new clip

  int headroom = 200000;
  frameCnt = 0; // gotta reset this too
  const char* dwFourCC = "00db";  // this and the frame size in bytes goes in between every frame
  char chunk_size_char[16];
  Serial.println("capturing");
  for (int i = 0;i < maxframeCnt; i++){
    frameCnt++;

    capture_still();

    jpegSize[i] = frame_buffer->len;
    
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
    esp_http_client_write(chunk_client,(char*)frame_buffer->buf, jpegSize[i]); // data   , 
    esp_http_client_write(chunk_client,"\r\n", 2);
    Serial.print("sent frame: ");
    Serial.println(i);
    fileSize += jpegSize[i];

    headroom = jpegSize[i] * 2;
    if ( fileSize > (megabytes - headroom) ){ break; }  // break the photo capture around 2 framesizes before the end of allocated memory
    
  }

  // these commands will replace the relevant data in the header
  word jpgs_width = WIDTH;
	word jpgs_height = HEIGHT;

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
  word biSizeImage = ((jpgs_width*24/8 + 3)&0xFFFFFFFC)*jpgs_height;
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
  char chunk_indexsize_char[8];
  sprintf(chunk_indexsize_char,"%08X", idxbufSize);
  esp_http_client_write(chunk_client,chunk_indexsize_char, 8); // length
  esp_http_client_write(chunk_client,"\r\n", 2);
  esp_http_client_write(chunk_client,(char*)idxBuf, idxbufSize); // data 
  esp_http_client_write(chunk_client,"\r\n", 2);

  /// sending the header chunk to the end of the file. Server side code will have to deal with
  /// putting it on the head
  char chunk_headsize_char[8];
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

  Serial.println("back to the loop");
  //return 1;
}

int send_motion()
{
  esp_http_client_handle_t http_client;

  esp_http_client_config_t config_client = {0};
  config_client.url = motion_url;
  config_client.event_handler = _http_event_handler;
  config_client.method = HTTP_METHOD_POST;

  http_client = esp_http_client_init(&config_client);

  const char* cur_frame_c = cur_frame_s.c_str();
  esp_http_client_set_post_field(http_client, cur_frame_c, strlen(cur_frame_c));

  esp_http_client_set_header(http_client, "Content-Type", "text/plain");

  esp_err_t err = esp_http_client_perform(http_client);

  if (err == ESP_OK) {
    Serial.print("esp_http_client_get_status_code: ");
    Serial.println(esp_http_client_get_status_code(http_client));
  }

  esp_http_client_cleanup(http_client);

}

int send_background() {
  esp_http_client_handle_t http_client;

  esp_http_client_config_t config_client = {0};
  config_client.url = bg_url;
  config_client.event_handler = _http_event_handler;
  config_client.method = HTTP_METHOD_POST;

  http_client = esp_http_client_init(&config_client);

  const char* prev_frame_c = prev_frame_s.c_str();
  esp_http_client_set_post_field(http_client, prev_frame_c, strlen(prev_frame_c));

  esp_http_client_set_header(http_client, "Content-Type", "text/plain");

  esp_err_t err = esp_http_client_perform(http_client);

  if (err == ESP_OK) {
    Serial.print("esp_http_client_get_status_code: ");
    Serial.println(esp_http_client_get_status_code(http_client));
  }

  esp_http_client_cleanup(http_client);
}

int status_notify() {
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
}

int check_orders() {
  esp_http_client_handle_t http_client;

  esp_http_client_config_t config_client = {0};
  config_client.url = orders_url;
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
}

void loop() {
#if MOTION_DETECT
  if (program == 1) {  // MOTION DETECTION ROUTINE
    if (!capture_still()) {
      Serial.println("Failed capture");
      delay(3000);
      return;
    }
    else {
      #if BMP
        downsample_bmp();
      #endif
      #if JPEG
        downsample_jpeg();
      #endif

      #if DEBUG
        Serial.println("Current frame:");
        print_frame(current_frame);
        Serial.println("---------------");
      #endif
    }

    if (motion_detect()) {
      Serial.println("Motion detected");
      send_clip();
      //send_photo();
      //send_motion();
      //send_background();
      //delay(1000);
    }
    if ( (millis() - timer_bg) > bg_limit) {
      Serial.println("Update background frame");
      timer_bg = millis();
      update_frame();
      //print_frame(prev_frame);
    }
  }
  /////////////////////////////////////////////////////
  /////////////////////////////////////////////////////

  if (program == 2) { // SEND PHOTO NOW ROUTINE
    if (!capture_still()) {
      Serial.println("Failed capture");
      delay(3000);
      return;
    }
    send_photo();
    //program = 0;
  }
  /////////////////////////////////////////////////////
  /////////////////////////////////////////////////////

  if (program == 0) { // JUST SLEEP

  }
  /////////////////////////////////////////////////////
  /////////////////////////////////////////////////////

  if ( (millis() - timer_status) > status_limit ) { // SEND STATUS
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
#endif
#if PIR
  //Serial.println(digitalRead(PIR_PIN));
  if (program == 1) {
    if (digitalRead(PIR_PIN)) { // If movement - Take photo
      //Serial.println("PIR Moved");
      digitalWrite(LAMP, HIGH);
      digitalWrite(LAMPB, LOW);
      if (!capture_still()) {
        Serial.println("Failed capture");
        delay(3000);
        return;
      }
      send_photo();
      digitalWrite(LAMP, LOW);
      digitalWrite(LAMPB, HIGH);
    }
  }
  
  if (program == 2) { // SEND PHOTO NOW ROUTINE
    if (!capture_still()) {
      Serial.println("Failed capture");
      delay(3000);
      return;
    }
    send_photo();
  }
  
  if (program == 0) { // JUST SLEEP

  }
  if ( (millis() - timer_status) > status_limit ) {
      if (WiFi.status() != WL_CONNECTED){
        start_wifi();
      }
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
#endif
}
