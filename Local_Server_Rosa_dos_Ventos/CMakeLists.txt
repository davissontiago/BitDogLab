# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 1.5.1)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# CMake mínimo requerido
cmake_minimum_required(VERSION 3.13)

# Configurações padrão
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configuração do ambiente Pico SDK (não modificar esta seção)
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 1.5.1)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

# Configuração da placa (usando Pico W)
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Incluir SDK do Raspberry Pi Pico (deve estar antes do projeto)
include(pico_sdk_import.cmake)

# Nome do projeto
project(picow_http_client C CXX ASM)

# Inicializar o SDK
pico_sdk_init()

# Configurar biblioteca HTTP utilitária
pico_add_library(example_lwip_http_util NOFLAG)
target_sources(example_lwip_http_util INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/example_http_client_util.c
)
pico_mirrored_target_link_libraries(example_lwip_http_util INTERFACE
    pico_lwip_http
    pico_lwip_mbedtls
    pico_mbedtls
)
target_include_directories(example_lwip_http_util INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Executável principal
add_executable(picow_http_client picow_http_client.c)

# Credenciais Wi-Fi (substitua com suas credenciais)
set(WIFI_SSID "PM-TOTUSTUUS")
set(WIFI_PASSWORD "MR15saTOTUS06")
target_compile_definitions(picow_http_client PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
)

# Configurações do programa
pico_set_program_name(picow_http_client "picow_http_client")
pico_set_program_version(picow_http_client "0.1")

# Configuração de saída (USB para debug)
pico_enable_stdio_uart(picow_http_client 0)
pico_enable_stdio_usb(picow_http_client 1)

# Bibliotecas necessárias
target_link_libraries(picow_http_client
    pico_stdlib
    hardware_pwm
    hardware_adc
    hardware_gpio
    hardware_irq
    pico_cyw43_arch_lwip_threadsafe_background
    example_lwip_http_util
)

# Diretórios de include adicionais
target_include_directories(picow_http_client PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${PICO_SDK_PATH}/src/rp2_common/hardware_pwm/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_adc/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_gpio/include
)

# Gerar arquivos de saída adicionais (UF2, HEX, etc.)
pico_add_extra_outputs(picow_http_client)

# Executável de verificação (opcional)
add_executable(picow_http_client_verify
    picow_http_verify.c
)
target_compile_definitions(picow_http_client_verify PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    ALTCP_MBEDTLS_AUTHMODE=MBEDTLS_SSL_VERIFY_REQUIRED
)
target_include_directories(picow_http_client_verify PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
)
target_link_libraries(picow_http_client_verify
    pico_cyw43_arch_lwip_threadsafe_background
    example_lwip_http_util
    pico_stdlib
)
pico_add_extra_outputs(picow_http_client_verify)

# Ignorar avisos específicos do lwIP
set_source_files_properties(
    ${PICO_LWIP_PATH}/src/apps/altcp_tls/altcp_tls_mbedtls.c
    PROPERTIES
    COMPILE_OPTIONS "-Wno-unused-result"
)