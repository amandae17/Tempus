#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include <stdio.h>
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi.h"

#define WIFI_SSID CONFIG_ESP_WIFI_SSID
#define WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define WIFI_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1


#define TAG "Wifi"

static EventGroupHandle_t s_wifi_event_group;

static int s_retry_num = 8;
extern SemaphoreHandle_t conexaoWifiSemaphore;

static void event_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data){

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect(); // Se for um evento de wifi e estiver inticializando, tenta conectar
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) { // Se o wifi está desconectado
        if (s_retry_num < WIFI_MAXIMUM_RETRY) { // Tenta reconectar
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT); // Se falhou seta bits no event group para reportar a falha de conexão.
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) { // Se for um evento do tipo IP 
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data; // e conseguiu um IP, imprime o IP
        ESP_LOGI(TAG, "Endereço ip recebido:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0; // Seta a quantidade de retry para 0
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT); // Seta o bit WIFI_CONNECTED_BIT no wifi event group   
        xSemaphoreGive(conexaoWifiSemaphore);
    }
}

void wifi_start() {
    s_wifi_event_group = xEventGroupCreate(); // Criação do event group

    ESP_ERROR_CHECK(esp_netif_init()); // Inicialização network interface

    ESP_ERROR_CHECK(esp_event_loop_create_default()); // Inicialização do event loop
    esp_netif_create_default_wifi_sta();//Inicializar as configurações padrões do modo station

    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT(); // Struct de configuração do wifi inicializado com parâmetros padrão.
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config)); // Carregar as configurações e inicializa o wifi.

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID, &event_handler, NULL)); // Define quem trata os eventos de wifi
    // Parâmetro 1: Tipos de eventos base
    // Parâmetro 2: ID do evento
    // Parâmetro 3: Função para rodar no evento
    // Parâmetro 4: Argumento para o handler
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP, &event_handler, NULL)); // Define quem trata os eventos de newtwork interface
    
    wifi_config_t config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &config));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    //Decadastra os event handlers no modo de teste(opcional) 
    //ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    //ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);

}