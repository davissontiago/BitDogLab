/*
 * Cliente HTTP para Raspberry Pi Pico W
 * Versão aprimorada com tratamento de botões e direções
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

// Definições de pinos
#define BUTTON_A 5
#define BUTTON_B 6
#define LED_AZUL 12
#define LED_VERDE 11
#define LED_VERMELHO 13

// Pinos do joystick
const int JOYSTICK_X = 26;
const int JOYSTICK_Y = 27;
const int BOTAO_JOYSTICK = 22;
const int ADC_CANAL_X = 0;
const int ADC_CANAL_Y = 1;

// Estrutura para cores RGB
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    const char* nome;
} CorDirecao;

// Paleta de cores melhorada
const CorDirecao cores_direcoes[] = {
    {255, 0, 0,    "Norte"},      // Vermelho
    {0, 255, 0,    "Sul"},        // Verde
    {0, 0, 255,    "Leste"},      // Azul
    {255, 255, 0,  "Oeste"},      // Amarelo
    {255, 0, 255,  "Nordeste"},   // Magenta
    {255, 128, 0,  "Noroeste"},   // Laranja
    {0, 255, 255,  "Sudeste"},    // Ciano
    {128, 0, 255,  "Sudoeste"},   // Roxo
    {0, 0, 0,      "Centro"}      // Desligado
};

// Estrutura para botões
typedef struct {
    uint gpio;
    const char* mensagem;
} Botao;

const Botao botoes[] = {
    {BUTTON_A, "BUTTON_A"},
    {BUTTON_B, "BUTTON_B"}
};

void configurar_joystick() {
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
    gpio_init(BOTAO_JOYSTICK);
    gpio_set_dir(BOTAO_JOYSTICK, GPIO_IN);
    gpio_pull_up(BOTAO_JOYSTICK);
}

void configurar_leds() {
    gpio_init(LED_AZUL);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
}

void configurar_botoes() {
    for (size_t i = 0; i < sizeof(botoes)/sizeof(botoes[0]); i++) {
        gpio_init(botoes[i].gpio);
        gpio_set_dir(botoes[i].gpio, GPIO_IN);
        gpio_pull_up(botoes[i].gpio);
    }
}

void inicializar_pwm() {
    gpio_set_function(LED_VERMELHO, GPIO_FUNC_PWM);
    gpio_set_function(LED_VERDE, GPIO_FUNC_PWM);
    gpio_set_function(LED_AZUL, GPIO_FUNC_PWM);
    
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.f);
    pwm_config_set_wrap(&config, 255);
    
    pwm_init(pwm_gpio_to_slice_num(LED_VERMELHO), &config, true);
    pwm_init(pwm_gpio_to_slice_num(LED_VERDE), &config, true);
    pwm_init(pwm_gpio_to_slice_num(LED_AZUL), &config, true);
}

void acender_led_rgb(uint8_t r, uint8_t g, uint8_t b) {
    pwm_set_gpio_level(LED_VERMELHO, r * r / 255);
    pwm_set_gpio_level(LED_VERDE, g * g / 255);
    pwm_set_gpio_level(LED_AZUL, b * b / 255);
}

void acender_led_por_direcao(const char* direcao) {
    for (size_t i = 0; i < sizeof(cores_direcoes)/sizeof(cores_direcoes[0]); i++) {
        if (strstr(direcao, cores_direcoes[i].nome)) {
            acender_led_rgb(cores_direcoes[i].r, cores_direcoes[i].g, cores_direcoes[i].b);
            return;
        }
    }
    acender_led_rgb(0, 0, 0);
}

void ler_eixos_joystick(uint16_t *x, uint16_t *y) {
    adc_select_input(ADC_CANAL_X);
    *x = adc_read();
    adc_select_input(ADC_CANAL_Y);
    *y = adc_read();
}

void calibrar_joystick(uint16_t *min_x, uint16_t *max_x, uint16_t *min_y, uint16_t *max_y) {
    printf("Calibrando joystick... Mova o eixo em todas as direções\n");
    
    uint32_t inicio = to_ms_since_boot(get_absolute_time());
    while (to_ms_since_boot(get_absolute_time()) - inicio < 8000) {
        uint16_t x, y;
        ler_eixos_joystick(&x, &y);
        
        *min_x = x < *min_x ? x : *min_x;
        *max_x = x > *max_x ? x : *max_x;
        *min_y = y < *min_y ? y : *min_y;
        *max_y = y > *max_y ? y : *max_y;
        
        sleep_ms(10);
    }
    
    printf("Calibração finalizada:\n");
    printf("X: min=%d, max=%d\n", *min_x, *max_x);
    printf("Y: min=%d, max=%d\n", *min_y, *max_y);
}

int conectar_wifi() {
    printf("Conectando a %s...\n", WIFI_SSID);
    int resultado = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, TIMEOUT_CONEXAO);
    
    if (resultado != 0) {
        printf("Falha na conexão Wi-Fi. Código: %d\n", resultado);
        return -1;
    }
    
    printf("Conectado! IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    return 0;
}

int enviar_mensagem_http(const char *url) {
    EXAMPLE_HTTP_REQUEST_T req = {
        .hostname = HOST,
        .url = url,
        .port = PORT,
        .headers_fn = http_client_header_print_fn,
        .recv_fn = http_client_receive_print_fn
    };
    
    printf("Enviando: %s\n", url);
    int resultado = http_client_request_sync(cyw43_arch_async_context(), &req);
    
    if (resultado != 0) {
        printf("Erro HTTP: %d\n", resultado);
    }
    
    return resultado;
}

void processar_direcao(int eixo_x_norm, int eixo_y_norm, char *url) {
    if (eixo_y_norm < 40) { // Cima
        if (eixo_x_norm < 40) {
            sprintf(url, "/mensagem?msg=Sudoeste");
        } else if (eixo_x_norm > 60) {
            sprintf(url, "/mensagem?msg=Noroeste");
        } else {
            sprintf(url, "/mensagem?msg=Oeste");
        }
    } else if (eixo_y_norm > 60) { // Baixo
        if (eixo_x_norm < 40) {
            sprintf(url, "/mensagem?msg=Sudeste");
        } else if (eixo_x_norm > 60) {
            sprintf(url, "/mensagem?msg=Nordeste");
        } else {
            sprintf(url, "/mensagem?msg=Leste");
        }
    } else { // Centro vertical
        if (eixo_x_norm < 40) {
            sprintf(url, "/mensagem?msg=Sul");
        } else if (eixo_x_norm > 60) {
            sprintf(url, "/mensagem?msg=Norte");
        } else {
            sprintf(url, "/mensagem?msg=Centro");
        }
    }
    
    acender_led_por_direcao(url);
}

void verificar_botoes(char *url) {
    for (size_t i = 0; i < sizeof(botoes)/sizeof(botoes[0]); i++) {
        if (gpio_get(botoes[i].gpio) == 0) {
            sprintf(url, "/mensagem?msg=%s", botoes[i].mensagem);
            return;
        }
    }
}

int main() {
    stdio_init_all();
    printf("\nIniciando cliente HTTP aprimorado...\n");

    // Inicialização do hardware
    configurar_joystick();
    configurar_leds();
    configurar_botoes();
    inicializar_pwm();

    // Inicialização Wi-Fi
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar Wi-Fi\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    if (conectar_wifi() != 0) {
        return 1;
    }

    // Calibração do joystick
    uint16_t min_x = 4095, max_x = 0;
    uint16_t min_y = 4095, max_y = 0;
    calibrar_joystick(&min_x, &max_x, &min_y, &max_y);

    // Loop principal
    char url[128];
    uint16_t valor_x, valor_y;
    
    while (true) {
        // Ler e normalizar valores do joystick
        ler_eixos_joystick(&valor_x, &valor_y);
        int eixo_x_norm = (valor_x - min_x) * 100 / (max_x - min_x);
        int eixo_y_norm = (valor_y - min_y) * 100 / (max_y - min_y);

        // Processar direção (define a URL padrão)
        processar_direcao(eixo_x_norm, eixo_y_norm, url);

        // Verificar botões (sobrescreve a URL se pressionado)
        verificar_botoes(url);

        // Enviar mensagem HTTP
        if (enviar_mensagem_http(url) != 0) {
            // Tentar reconectar em caso de erro
            cyw43_arch_wifi_connect_timeout_ms(
                WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000);
        }

        printf("X: %d (%d%%), Y: %d (%d%%)\n", 
               valor_x, eixo_x_norm, valor_y, eixo_y_norm);

        sleep_ms(INTERVALO_MS);
    }
}