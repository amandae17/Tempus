#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "rom/ets_sys.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/semphr.h"

#include "wifi.h"
#include "mqtt.h"

#include "DHT22.h" // Importa a biblioteca do dht22

SemaphoreHandle_t conexaoWifiSemaphore;
SemaphoreHandle_t conexaoMQTTSemaphore;

void DHT_reader_task(void *pvParameter)
{
	setDHTgpio(GPIO_NUM_17); // Seta a porta utilizada pelo ESP32 para leitura do sensor.
	if(xSemaphoreTake(conexaoMQTTSemaphore,portMAX_DELAY)) { // Se o semáforo 
		while(true) {
		
			printf("DHT Sensor Readings\n" ); //Escreve a string na porta serial
			int ret = readDHT(); // Realiza a leitura dos dados e retorna um código de erro 
				
			errorHandler(ret); // Função que trata os erros

			float humidityRead = getHumidity();// Faz a leitura do valor da humidade.
			float temperatureRead = getTemperature();// Faz a leitura do valor da temperatura.        
			printf("Humidity %.2f %%\n", humidityRead);// Escreve o valor lido na porta serial.
			printf("Temperature %.2f degC\n\n", temperatureRead);// Escreve o valor lido na porta serial. 	
				
			char temperatura[10];
			char humidade[10];
			sprintf(temperatura, "%.2f", temperatureRead); // Converte o valor em float para String com formatação.
			sprintf(humidade, "%.2f", humidityRead); // Converte o valor em float para String com formatação.
			mqqt_envia_mensagem("univap/projeto4/tempus/temperatura", temperatura); // Envia a mensagem no tópico especificado	
			mqqt_envia_mensagem("univap/projeto4/tempus/humidade", humidade); // Envia a mensagem no tópico especificado	
			
			vTaskDelay(10000 / portTICK_PERIOD_MS); // Função que o faz delay de 10s para a leitura do sensor	  
		}
	}
}

void connectMqtt(void * params) {
	while(true) {
		if(xSemaphoreTake(conexaoWifiSemaphore,portMAX_DELAY)){
			mqtt_start(); // Inicializa as comunicações MQTT			
		}
	}
}

void app_main()
{   // Inicialização do NVS
	esp_err_t ret = nvs_flash_init(); // Utiliza a unidade de memória volátil para armazenar os dados de leitura.
        
	if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) { // Tratamento de erros
		ESP_ERROR_CHECK(nvs_flash_erase()); // Limpa a memória no caso de erro
		ret = nvs_flash_init(); // Tenta inicializar de novo a memória no caso de erro
	}
	ESP_ERROR_CHECK(ret); // Checa se ocorreu algum erro na inicialização da memória
	
    conexaoWifiSemaphore = xSemaphoreCreateBinary(); // Cria um semáforo do tipo binário
    conexaoMQTTSemaphore = xSemaphoreCreateBinary(); // Cria um semáforo do tipo binário
    wifi_start(); // Inicializa o wifi

	xTaskCreate(&connectMqtt, "connectMqtt", 4096, NULL, 1, NULL);

	vTaskDelay(2000 / portTICK_PERIOD_MS); // Função que o faz delay de 2s para o início da leitura do sensor

	xTaskCreate(&DHT_reader_task, "DHT_reader_task", 8192, NULL, 5, NULL ); // Cria uma task para fazer a leitura do sensor
	// O primeiro parâmetro é um ponteiro para a função de leitura, o segundo é um nome para debug
	// O terceiro é o tamanho da memória utilizada para rodar o código, 
	// O quarto é algum parâmetro de entrada
	// O quinto é nível de prioridade no sistema
	// O sexto é uma função que lida com erros do sistema
	
}
