#include "OV2640.h"

#define TAG "OV2640"

// definitions appropriate for the ESP32-CAM devboard (and most clones)
camera_config_t esp32cam_config {

    .pin_pwdn = -1,
    .pin_reset = 15,

    .pin_xclk = 27,

    .pin_sscb_sda = 25,
    .pin_sscb_scl = 23,

    .pin_d7 = 19,
    .pin_d6 = 36,
    .pin_d5 = 18,
    .pin_d4 = 39,
    .pin_d3 = 5,
    .pin_d2 = 34,
    .pin_d1 = 35,
    .pin_d0 = 17,
    .pin_vsync = 22,
    .pin_href = 26,
    .pin_pclk = 21,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    // .frame_size = FRAMESIZE_UXGA, // needs 234K of framebuffer space
    // .frame_size = FRAMESIZE_SXGA, // needs 160K for framebuffer
    .frame_size = FRAMESIZE_XGA, // needs 96K or even smaller FRAMESIZE_SVGA - can work if using only 1 fb
    // .frame_size = FRAMESIZE_SVGA,
    .jpeg_quality = 10,               //0-63 lower numbers are higher quality
    .fb_count = 1 // if more than one i2s runs in continous mode.  Use only with jpeg
};

camera_config_t esp32cam_ttgo_t_config {

    .pin_pwdn = 26,
    .pin_reset = -1,

    .pin_xclk = 32,

    .pin_sscb_sda = 13,
    .pin_sscb_scl = 12,

    .pin_d7 = 39,
    .pin_d6 = 36,
    .pin_d5 = 23,
    .pin_d4 = 18,
    .pin_d3 = 15,
    .pin_d2 = 4,
    .pin_d1 = 14,
    .pin_d0 = 5,
    .pin_vsync = 27,
    .pin_href = 25,
    .pin_pclk = 19,
    .xclk_freq_hz = 16500000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_UXGA,
    .jpeg_quality = 10, //0-63 lower numbers are higher quality
    .fb_count = 2 // if more than one i2s runs in continous mode.  Use only with jpeg
};




void OV2640::run(void)
{
    if(fb)
        //return the frame buffer back to the driver for reuse
        esp_camera_fb_return(fb);

    fb = esp_camera_fb_get();
}

void OV2640::runIfNeeded(void)
{
    if(!fb)
        run();
}

int OV2640::getWidth(void)
{
    runIfNeeded();
    return fb->width;
}

int OV2640::getHeight(void)
{
    runIfNeeded();
    return fb->height;
}

size_t OV2640::getSize(void)
{
    runIfNeeded();
    return fb->len;
}

uint8_t *OV2640::getfb(void)
{
    runIfNeeded();
    return fb->buf;
}

framesize_t OV2640::getFrameSize(void)
{
    return _cam_config.frame_size;
}

void OV2640::setFrameSize(framesize_t size)
{
    _cam_config.frame_size = size;
}

pixformat_t OV2640::getPixelFormat(void)
{
    return _cam_config.pixel_format;
}

void OV2640::setPixelFormat(pixformat_t format)
{
    switch (format)
    {
    case PIXFORMAT_RGB565:
    case PIXFORMAT_YUV422:
    case PIXFORMAT_GRAYSCALE:
    case PIXFORMAT_JPEG:
        _cam_config.pixel_format = format;
        break;
    default:
        _cam_config.pixel_format = PIXFORMAT_GRAYSCALE;
        break;
    }
}


esp_err_t OV2640::init(camera_config_t config)
{
    memset(&_cam_config, 0, sizeof(_cam_config));
    memcpy(&_cam_config, &config, sizeof(config));

    //power up the camera if PWDN pin is defined
    if(config.pin_pwdn != -1) {
        pinMode(config.pin_pwdn, OUTPUT);
        digitalWrite(config.pin_pwdn, LOW);
    }

    esp_err_t err = esp_camera_init(&_cam_config);
    if (err != ESP_OK)
    {
        ESP_LOGD(TAG, "Camera probe failed with error 0x%x", err);
        return err;
    }
    // ESP_ERROR_CHECK(gpio_install_isr_service(0));

    return ESP_OK;
}
