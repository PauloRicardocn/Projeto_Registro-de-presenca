cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

# Defina o caminho para o SDK do Pico
set(PICO_SDK_PATH "C:/Users/Paulo Ricardo/.pico-sdk/sdk/2.1.0")

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# Projeto RFID
project(RFID C CXX ASM)

# Inicializar o Raspberry Pi Pico SDK
pico_sdk_init()

# Adicionar executável. O nome padrão é o nome do projeto
add_executable(RFID RFID.c inc/ssd1306_i2c.c MFRC522.c )  

# Configurar o nome e a versão do programa
pico_set_program_name(RFID "RFID")
pico_set_program_version(RFID "0.1")

# Habilitar saída para UART e USB
pico_enable_stdio_uart(RFID 1)
pico_enable_stdio_usb(RFID 1)

# Gerar o arquivo de cabeçalho PIO a partir do código PIO
pico_generate_pio_header(RFID ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Adicionar bibliotecas necessárias
target_link_libraries(RFID
    pico_stdlib
    hardware_spi
    hardware_pio
    hardware_i2c
    pico_lwip
    pico_cyw43_arch_lwip_threadsafe_background
)

# Incluir diretórios para cabeçalhos
target_include_directories(RFID PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${PICO_SDK_PATH}/lib/lwip/src/include/apps
    ${PICO_SDK_PATH}/lib/lwip/include/apps
)

# Adicionar arquivos extras de saída (como UF2)
pico_add_extra_outputs(RFID)