/**
 * Cliente HTTP para Raspberry Pi Pico W
 */
#include <stdio.h>
#include "pico/stdio.h"
#include "pico/cyw43_arch.h"
#include "pico/async_context.h"
#include "lwip/altcp_tls.h"
#include "example_http_client_util.h"

// ======= CONFIGURAÇÕES ======= //
#define HOST "192.168.XX.XXX" // Substitua pelo IP do servidor
#define PORT 5000
#define INTERVALO_MS 500
#define BUTTON_A 5
#define LED_RED 13
// ============================= //

int main()
{
    // Inicializando entradas padrão
    stdio_init_all();

    // Configuração do Pino do Botão
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    // Conifguração do Led
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);

    printf("\nIniciando cliente HTTP...\n");

    if (cyw43_arch_init())
    {
        printf("Erro ao inicializar Wi-Fi\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    printf("Conectando a %s...\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
                                           CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("Falha na conexão Wi-Fi\n");
        return 1;
    }

    printf("Conectado! IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));

    // Loop Principal
    int button_state = 1; // Variável para controlar o estado do botão (1 = solto, 0 = pressionado)

    while (1) {
        const char *path = NULL;

        // Se o botão A for apertado
        if (gpio_get(BUTTON_A) == 0)
        {
            if (button_state == 1)
            {                    // Se o botão estava solto e agora foi pressionado
                path = "/CLICK"; // Envia a mensagem "CLICK"
                gpio_put(LED_RED, 1);
                button_state = 0; // Atualiza o estado para pressionado
            }
        }
        else
        {
            if (button_state == 0)
            {                    // Se o botão estava pressionado e agora foi solto
                path = "/SOLTO"; // Envia a mensagem "SOLTO"
                gpio_put(LED_RED, 0);
                button_state = 1; // Atualiza o estado para solto
            }
        }

        if (path != NULL)
        {
            EXAMPLE_HTTP_REQUEST_T req = {0};
            req.hostname = HOST;
            req.url = path;
            req.port = PORT;
            req.headers_fn = http_client_header_print_fn;
            req.recv_fn = http_client_receive_print_fn;

            printf("Enviando comando: %s\n", path);
            int result = http_client_request_sync(cyw43_arch_async_context(), &req);

            if (result == 0)
            {
                printf("Comando enviado com sucesso!\n");
            }
            else
            {
                printf("Erro ao enviar comando: %d\n", result);
            }

            sleep_ms(20); // Evita múltiplos envios rápidos
        }

        sleep_ms(INTERVALO_MS);
    }
}
