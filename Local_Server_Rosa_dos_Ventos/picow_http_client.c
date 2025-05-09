#include <stdio.h>
#include <string.h>
#include "pico/stdio.h"
#include "pico/cyw43_arch.h"
#include "pico/async_context.h"
#include "lwip/altcp_tls.h"
#include "example_http_client_util.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

// ======== DEFINIÇÕES ===========
#define HOST "192.168.XX.XXX"
#define PORT 5000
#define INTERVALO_MS 50
#define TIMEOUT_CONEXAO 30000

#define BUTTON_A 5
#define SENSOR_INFRA 3
#define LED_AZUL 12
#define LED_VERDE 11
#define LED_VERMELHO 13

const int JOYSTICK_X = 26;
const int JOYSTICK_Y = 27;
const int BOTAO_JOYSTICK = 22;
const int ADC_CANAL_X = 0;
const int ADC_CANAL_Y = 1;

// Estrutura para associar direção com uma cor RGB
typedef struct {
    uint8_t r, g, b;
    const char *nome;
} CorDirecao;

const CorDirecao cores_direcoes[] = {
    {255, 0, 0, "Norte"}, {0, 255, 0, "Sul"}, {0, 0, 255, "Leste"},
    {255, 255, 0, "Oeste"}, {255, 0, 255, "Nordeste"}, {255, 128, 0, "Noroeste"},
    {0, 255, 255, "Sudeste"}, {128, 0, 255, "Sudoeste"}, {0, 0, 0, "Centro"}
};

// Estrutura para mapear botões a mensagens
typedef struct {
    uint gpio;
    const char *mensagem;
} Botao;

const Botao botoes[] = {
    {BUTTON_A, "BUTTON_A"},
    {SENSOR_INFRA, "SENSOR_INFRA"}
};

// ========== CONFIGURAÇÕES INICIAIS ==========

// Inicializa os pinos e canais ADC para o joystick
void configurar_joystick() {
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
    gpio_init(BOTAO_JOYSTICK);
    gpio_set_dir(BOTAO_JOYSTICK, GPIO_IN);
    gpio_pull_up(BOTAO_JOYSTICK);
}

// Inicializa LEDs RGB como GPIO de saída
void configurar_leds() {
    gpio_init(LED_AZUL); gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_init(LED_VERDE); gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_init(LED_VERMELHO); gpio_set_dir(LED_VERMELHO, GPIO_OUT);
}

// Inicializa botões como entrada com pull-up
void configurar_botoes() {
    for (size_t i = 0; i < sizeof(botoes) / sizeof(botoes[0]); i++) {
        gpio_init(botoes[i].gpio);
        gpio_set_dir(botoes[i].gpio, GPIO_IN);
        gpio_pull_up(botoes[i].gpio);
    }
}

// Inicializa o PWM para controle dos LEDs RGB
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

// ========== LEITURA E PROCESSAMENTO =========

// Lê os valores analógicos dos eixos do joystick
void ler_eixos_joystick(uint16_t *x, uint16_t *y) {
    adc_select_input(ADC_CANAL_X);
    *x = adc_read();
    adc_select_input(ADC_CANAL_Y);
    *y = adc_read();
}

// Determina a direção com base nos eixos e monta a URL da mensagem
void processar_direcao(int eixo_x_norm, int eixo_y_norm, char *url) {
    if (eixo_y_norm < 40) {
        if (eixo_x_norm < 40) sprintf(url, "/mensagem?msg=Sudoeste");
        else if (eixo_x_norm > 60) sprintf(url, "/mensagem?msg=Noroeste");
        else sprintf(url, "/mensagem?msg=Oeste");
    } else if (eixo_y_norm > 60) {
        if (eixo_x_norm < 40) sprintf(url, "/mensagem?msg=Sudeste");
        else if (eixo_x_norm > 60) sprintf(url, "/mensagem?msg=Nordeste");
        else sprintf(url, "/mensagem?msg=Leste");
    } else {
        if (eixo_x_norm < 40) sprintf(url, "/mensagem?msg=Sul");
        else if (eixo_x_norm > 60) sprintf(url, "/mensagem?msg=Norte");
        else sprintf(url, "/mensagem?msg=Centro");
    }

    acender_led_por_direcao(url);
}

// Verifica se algum botão foi pressionado e atualiza a URL da mensagem
void verificar_botoes(char *url) {
    for (size_t i = 0; i < sizeof(botoes) / sizeof(botoes[0]); i++) {
        if (gpio_get(botoes[i].gpio) == 0) {
            sprintf(url, "/mensagem?msg=%s", botoes[i].mensagem);
            return;
        }
    }
}

// ========== CONTROLE DE LED ==========

// Acende o LED RGB com uma cor específica (aplicando correção gamma simples)
void acender_led_rgb(uint8_t r, uint8_t g, uint8_t b) {
    pwm_set_gpio_level(LED_VERMELHO, r * r / 255);
    pwm_set_gpio_level(LED_VERDE, g * g / 255);
    pwm_set_gpio_level(LED_AZUL, b * b / 255);
}

// Define a cor do LED RGB com base no nome da direção
void acender_led_por_direcao(const char *direcao) {
    for (size_t i = 0; i < sizeof(cores_direcoes) / sizeof(cores_direcoes[0]); i++) {
        if (strstr(direcao, cores_direcoes[i].nome)) {
            acender_led_rgb(cores_direcoes[i].r, cores_direcoes[i].g, cores_direcoes[i].b);
            return;
        }
    }
    acender_led_rgb(0, 0, 0); // Desliga o LED se direção não reconhecida
}

// ========== CONEXÃO E ENVIO ==========

// Conecta à rede Wi-Fi com timeout
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

// Envia requisição HTTP para o servidor com base na URL fornecida
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

// ========== FUNÇÃO PRINCIPAL ==========

int main() {
    stdio_init_all();
    printf("\nIniciando cliente HTTP aprimorado...\n");

    configurar_leds();
    configurar_joystick();
    configurar_botoes();
    inicializar_pwm();

    const uint16_t min_x = 10, max_x = 4095;
    const uint16_t min_y = 10, max_y = 4095;

    if (cyw43_arch_init()) {
        printf("Erro ao inicializar Wi-Fi\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    if (conectar_wifi() != 0) return 1;

    char url[128];
    uint16_t valor_x, valor_y;

    while (true) {
        ler_eixos_joystick(&valor_x, &valor_y);
        int eixo_x_norm = (valor_x - min_x) * 100 / (max_x - min_x);
        int eixo_y_norm = (valor_y - min_y) * 100 / (max_y - min_y);

        processar_direcao(eixo_x_norm, eixo_y_norm, url);
        verificar_botoes(url);

        if (enviar_mensagem_http(url) != 0) {
            cyw43_arch_wifi_connect_timeout_ms(
                WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000);
        }

        printf("X: %d (%d%%), Y: %d (%d%%)\n", valor_x, eixo_x_norm, valor_y, eixo_y_norm);
        sleep_ms(INTERVALO_MS);
    }
}
