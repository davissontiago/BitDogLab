/*
 * Cliente HTTP para Raspberry Pi Pico W - Versão Multiplayer
 */
#include <stdio.h>
#include <string.h>
#include "pico/stdio.h"
#include "pico/cyw43_arch.h"
#include "pico/async_context.h"
#include "lwip/altcp_tls.h"
#include "example_http_client_util.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

// Configurações de rede
#define HOST "192.168.XX.XXX"
#define PORT 5000
#define INTERVALO_MS 50
#define TIMEOUT_CONEXAO 30000
#define PLAYER_ID 1 // Mudar para 2 no segundo RP2040

// Definições de pinos atualizadas
#define LED_PLAYER1 13 // Vermelho para Player 1
#define LED_PLAYER2 12 // Azul para Player 2
#define LED_CALIBRANDO 11

// Pinos do joystick
const int JOYSTICK_X = 27;
const int JOYSTICK_Y = 26;
const int BOTAO_JOYSTICK = 22;
const int ADC_CANAL_X = 1;
const int ADC_CANAL_Y = 0;

void configurar_joystick()
{
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
    gpio_init(BOTAO_JOYSTICK);
    gpio_set_dir(BOTAO_JOYSTICK, GPIO_IN);
    gpio_pull_up(BOTAO_JOYSTICK);
}

void configurar_leds()
{
    // Configurar os LEDs como PWM
    gpio_set_function(LED_PLAYER1, GPIO_FUNC_PWM);
    gpio_set_function(LED_PLAYER2, GPIO_FUNC_PWM);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.f);
    pwm_config_set_wrap(&config, 255);

    pwm_init(pwm_gpio_to_slice_num(LED_PLAYER1), &config, true);
    pwm_init(pwm_gpio_to_slice_num(LED_PLAYER2), &config, true);

    gpio_init(LED_CALIBRANDO);
    gpio_set_dir(LED_CALIBRANDO, GPIO_OUT);
}

void atualizar_leds()
{
    if (PLAYER_ID == 1)
    {
        // Player 1 - Vermelho fraco
        pwm_set_gpio_level(LED_PLAYER1, 50);
        pwm_set_gpio_level(LED_PLAYER2, 0);
    }
    else if (PLAYER_ID == 2)
    {
        // Player 2 - Azul fraco
        pwm_set_gpio_level(LED_PLAYER1, 0);
        pwm_set_gpio_level(LED_PLAYER2, 50);
    }
}

void ler_eixos_joystick(uint16_t *x, uint16_t *y)
{
    adc_select_input(ADC_CANAL_X);
    *x = adc_read();
    adc_select_input(ADC_CANAL_Y);
    *y = adc_read();
}

void calibrar_joystick(uint16_t *min_x, uint16_t *max_x, uint16_t *min_y, uint16_t *max_y) {
    gpio_put(LED_CALIBRANDO, 1); // Liga LED de calibração
    printf("Calibrando joystick... Mova em todas as direcoes\n");
    uint32_t inicio = to_ms_since_boot(get_absolute_time());
    while (to_ms_since_boot(get_absolute_time()) - inicio < 4000)
    {
        uint16_t x, y;
        ler_eixos_joystick(&x, &y);

        *min_x = x < *min_x ? x : *min_x;
        *max_x = x > *max_x ? x : *max_x;
        *min_y = y < *min_y ? y : *min_y;
        *max_y = y > *max_y ? y : *max_y;

        sleep_ms(10);
    }
    gpio_put(LED_CALIBRANDO, 0); // Desliga LED de calibração
    printf("Calibracao finalizada:\nX: min=%d, max=%d\nY: min=%d, max=%d\n", *min_x, *max_x, *min_y, *max_y);
}

int conectar_wifi()
{
    printf("Conectando a %s...\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, TIMEOUT_CONEXAO))
    {
        printf("Falha na conexao Wi-Fi\n");
        return -1;
    }
    printf("Conectado! IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    return 0;
}

int enviar_comando(int player, int valor)
{
    char url[128];
    sprintf(url, "/control?player=%d&action=move&value=%d", player, valor);

    EXAMPLE_HTTP_REQUEST_T req = {
        .hostname = HOST,
        .url = url,
        .port = PORT,
        .headers_fn = http_client_header_print_fn,
        .recv_fn = http_client_receive_print_fn};

    return http_client_request_sync(cyw43_arch_async_context(), &req);
}

void processar_movimento(int eixo_y_norm)
{
    int valor = 0;
    const int centro = 50;  // Valor central normalizado (0-100)
    const int deadzone = 5; // Zona morta

    if (eixo_y_norm < centro - deadzone)
    {
        // Movimento para BAIXO (valor positivo)
        valor = (centro - deadzone - eixo_y_norm) * 2;
    }
    else if (eixo_y_norm > centro + deadzone)
    {
        // Movimento para CIMA (valor negativo)
        valor = -((eixo_y_norm - (centro + deadzone)) * 2);
    }

    if (enviar_comando(PLAYER_ID, valor) != 0)
    {
        atualizar_leds(false);
        cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000);
        atualizar_leds(true);
    }
}

int main() {
    stdio_init_all();

    printf("Iniciando as entradas e saídas...\n");

    printf("\nIniciando cliente multiplayer (Player %d)...\n", PLAYER_ID);

    if (cyw43_arch_init()){
        printf("[ERRO] Falha ao inicializar Wi-Fi\n");
        return 1;
    } else{
        printf("[OK] Hardware Wi-Fi inicializado!\n");
    } 

    cyw43_arch_enable_sta_mode();

    if (conectar_wifi() != 0) {
        printf("[ERRO] Falha ao conectar. Verifique SSID/senha.\n");
        return 1;
    }

    configurar_joystick();
    configurar_leds();

    uint16_t min_x = 4095, max_x = 0;
    uint16_t min_y = 4095, max_y = 0;
    calibrar_joystick(&min_x, &max_x, &min_y, &max_y);

    atualizar_leds(false);

    uint16_t x, y;
    while (true)
    {
        ler_eixos_joystick(&x, &y);
        int eixo_y_norm = (y - min_y) * 100 / (max_y - min_y);

        processar_movimento(eixo_y_norm);
        printf("Player %d - Y: %d (%d%%)\n", PLAYER_ID, y, eixo_y_norm);

        sleep_ms(INTERVALO_MS);
    }
}