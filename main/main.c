#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <esp_http_server.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "driver/ledc.h"
#define LEDC_CHANNEL_0     0
#define LEDC_CHANNEL_1     1
#define LEDC_TIMER_0       LEDC_TIMER_0
#define LEDC_TIMER_1       LEDC_TIMER_1
#define LED_GPIO_PIN       4  
#define LED_GPIO_PIN2      23

#define SSID "p"
#define PASSWORD "paulo123"
#define ESP_MAXIMUM_RETRY 5

#define TAG "Aula 11"
#define RETRY_NUM 5

int wifi_connect_status = 0;
int retry_num = 0;
int brightness = 0;
SemaphoreHandle_t semphr_bin_handler;
SemaphoreHandle_t semphr_bin_handler2;
   // Configure PWM parameters
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_10_BIT, // 10-bit resolution for PWM
        .freq_hz = 5000,                      // Frequency in Hz (adjust as needed)
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0
    };
// Configure PWM parameters
    ledc_timer_config_t ledc_timer2 = {
        .duty_resolution = LEDC_TIMER_10_BIT, // 10-bit resolution for PWM
        .freq_hz = 5000,                      // Frequency in Hz (adjust as needed)
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_1
    };
    // Attach the LED GPIO pin to the PWM channel
    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = LED_GPIO_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0
    };
    // Attach the LED GPIO pin to the PWM channel
    ledc_channel_config_t ledc_channel2 = {
        .channel = LEDC_CHANNEL_1,
        .duty = 0,
        .gpio_num = LED_GPIO_PIN2,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0
    };

char html_page[] =  "<!DOCTYPE html><html>"
            "<p>Set LED Brightness: </p>"
            "<input type=\"range\" min=\"0\" max=\"255\" value=\"\" class=\"slider\" id=\"pwmSlider\">"
            "<button onclick=\"sendPWM()\" class=\"button\">Set PWM</button>"
            "<script>"
            "function sendPWM() {"
            "  var pwmValue = document.getElementById(\"pwmSlider\").value;"
            "  var xhr = new XMLHttpRequest();"
            "  xhr.open(\"GET\", \"/pwm?value=\" + pwmValue, true);"
            "  xhr.send();"
            "}"
            "</script>"
            "</body></html>";

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    if (event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "Conectando ...");
        esp_wifi_connect();
    }

    else if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG, "Conectado a rede local");
        wifi_connect_status = 1;
    }

    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Desconectado a rede local");
        wifi_connect_status = 0;

        if (retry_num < RETRY_NUM)
        {
            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(TAG, "Reconectando ...");
        }
    }
    
    else if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "IP: " IPSTR,IP2STR(&event->ip_info.ip));
        retry_num = 0;
    }
}

void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    wifi_config_t wifi_config = 
    {
        .sta = 
        {
            .ssid = SSID,
            .password = PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SSID:%s  password:%s",SSID,PASSWORD);
}

void vtask_flash_led (void *pvParameters)
{

  while(1)
  {
  // pdTRUE = 1
  // pdFALSE = 0
    if(xSemaphoreTake(semphr_bin_handler, portMAX_DELAY) == pdTRUE)
    {
        ledc_set_duty(ledc_channel2.speed_mode, ledc_channel2.channel, 0);
        ledc_update_duty(ledc_channel2.speed_mode, ledc_channel2.channel);
        ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, brightness);
        ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
        printf("Mudou de estado 1\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

}

void vtask_flash_led2 (void *pvParameters)
{

  while(1)
  {
  // pdTRUE = 1
  // pdFALSE = 0
    if(xSemaphoreTake(semphr_bin_handler2, portMAX_DELAY) == pdTRUE)
    {
        ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
        ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
        ledc_set_duty(ledc_channel2.speed_mode, ledc_channel2.channel, brightness);
        ledc_update_duty(ledc_channel2.speed_mode, ledc_channel2.channel);
        printf("Mudou de estado 2\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

}
void pwm_handler(httpd_req_t *req)
{
   char* uri = req->uri;
   int i = 11;
   char pwmValue[100] = "";
   while (uri[i] != '\0') {
        char currentChar = uri[i];
        strncat(pwmValue, &currentChar, 1);
        i++;
    }
    brightness = atoi(pwmValue);
    brightness < 150? xSemaphoreGive(semphr_bin_handler):xSemaphoreGive(semphr_bin_handler2);
    
}

esp_err_t req_handler(httpd_req_t *req)
{
    return httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
}

httpd_uri_t uri_get = 
{
    .uri = "/",
    .method = HTTP_GET,
    .handler = req_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_pwm = 
{
    .uri = "/pwm",
    .method = HTTP_GET,
    .handler = pwm_handler,
    .user_ctx = NULL
};

httpd_handle_t setup_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_pwm); // Register the '/pwm' handler
    }

    return server;
}

void pwmSet(){
    gpio_pad_select_gpio(LED_GPIO_PIN);
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);

    ledc_timer_config(&ledc_timer);
    ledc_channel_config(&ledc_channel);
    
    gpio_pad_select_gpio(LED_GPIO_PIN2);
    gpio_set_direction(LED_GPIO_PIN2, GPIO_MODE_OUTPUT);
    ledc_timer_config(&ledc_timer2);
    ledc_channel_config(&ledc_channel2);
   /*gpio_pad_select_gpio(LED_BUILTIN);
   gpio_set_direction(LED_BUILTIN, GPIO_MODE_OUTPUT);*/
}


void app_main()
{  
    pwmSet();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init ();
    
    vTaskDelay(pdMS_TO_TICKS(10000));

    if (wifi_connect_status)
    {
        setup_server();
        ESP_LOGI(TAG, "Web Server inicializado");
    }
    else
    {
        ESP_LOGI(TAG, "Falha ao conectar à rede local");
    }

    semphr_bin_handler = xSemaphoreCreateBinary();
    if(semphr_bin_handler == NULL)
    {
        printf ("Erro, falha ao criar o semaforo\n");
        return;
    }
    semphr_bin_handler2 = xSemaphoreCreateBinary();
    if(semphr_bin_handler2 == NULL)
    {
        printf ("Erro, falha ao criar o semaforo\n");
        return;
    }
    xTaskCreate (vtask_flash_led, "flash LED", 4098, NULL, 2, NULL);
    xTaskCreate (vtask_flash_led2, "flash LED", 4098, NULL, 2, NULL);
}
