# stm32f411_framework

Integração do framework com STM32F411CEU6, STM32Cube HAL e CMSIS, em C11.
O [port STM32F4](hal/ports/stm32f4/README.md) implementa sistema, GPIO, UART,
I2C e temporizadores e pode ser integrado em uma placa STM32F407.

`hal_system_init()` usa HSI e PLL para operar a 100 MHz no F411, sem cristal
externo. Cada `config` inicializa clocks, pinos e o controlador necessário.
O [BSP em boards/](boards/README.md) seleciona os recursos da Black Pill conforme
[pinout.md](pinout.md). `Src/main.c` inicializa o sistema, constrói os recursos,
sela a arena e inicia o timer de 1 ms. A integração mantém a contagem de
notificações do timer; a lógica dos dispositivos fica a cargo da aplicação.

## Compilar

Requer CMake >= 3.20, Ninja e `arm-none-eabi-gcc`/`g++` no `PATH`. Os submódulos
`hal/interface`, `hal/memory` e `hal/ports/stm32f4` devem estar inicializados.
O subconjunto do SDK em `lld/stm32f4` tem procedência descrita em seu
[README](lld/stm32f4/README.md).

```sh
cmake --preset Debug
cmake --build --preset Debug
cmake --preset Release
cmake --build --preset Release
```

ELF: `build/<configuração>/stm32f411_framework.elf`.
`STM32F4_SDK_DIR` pode selecionar outro diretório com a mesma organização de
CMSIS e `STM32F4xx_HAL_Driver`. A integração fica em `cmake/stm32f4.cmake`, fora
dos arquivos gerenciados pela extensão STM32Cube. A seleção do BSP fica em
`cmake/boards.cmake`.

## Verificar o port

```sh
python3 tests/stm32f4/run.py
python3 tests/stm32f4/cross_build.py
python3 tests/boards/run.py
```

O primeiro requer Linux, compilador C nativo e UBSan. Executa seis cenários
do port para cada MCU e um cenário de integração do BSP F411, com registradores
mapeados em memória e drivers ST reais. Somente
CPU, clock e NVIC são simulados; wrappers injetam falhas e transmissão parcial.
Cobre clocks/GPIOs, rotas/formatos, recursos, falhas, timeouts, selagem, períodos
e callbacks.

O segundo usa ARM GCC (ou `ARM_CC`) para compilar e linkar todas as funções
públicas nos dois MCUs com warnings tratados como erros. Esses ELFs verificam
o link e não são imagens para gravação. Teste elétrico, precisão de frequência
e comunicação real ainda exigem placa e periféricos.

O terceiro verifica o contrato do BSP com falhas em cada etapa de construção:
LED apagado primeiro, configurações da bancada, preservação da saída e rejeição
de uma segunda tentativa após sucesso ou falha.
