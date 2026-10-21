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

## Ambiente de desenvolvimento

O ambiente padrão é o [devcontainer](.devcontainer/devcontainer.json), como no
`esp32_framework`. Abra o projeto no VS Code com a extensão Dev Containers e
execute **Dev Containers: Reopen in Container**. Requer Docker e host Linux
x86_64 (ou emulação dessa arquitetura).

A imagem usa Ubuntu 24.04 e instala as seguintes versões:

| Dependência | Versão |
| --- | --- |
| STM32CubeF4 | v1.28.3, commit `94cae6e83f00e276a11957e7833c01ac3d0bd7af` |
| Arm GNU Toolchain | 14.3.Rel1 |
| CMake | 3.31.8 |
| Ninja | 1.13.1 |

Os arquivos das ferramentas são verificados por SHA-256 no
[Dockerfile](.devcontainer/Dockerfile). O SDK é obtido do
[repositório oficial da ST](https://github.com/STMicroelectronics/STM32CubeF4/tree/v1.28.3),
com CMSIS Core e os submódulos CMSIS Device e HAL nas revisões vinculadas ao
commit do pacote. Somente esses componentes são baixados; exemplos,
middlewares e BSPs da ST não são necessários. O SDK fica em `/opt/STM32CubeF4`,
fora do repositório, e `STM32F4_SDK_DIR` já vem definido na imagem.

O download ocorre ao construir a imagem. Depois disso, builds e testes não
precisam de rede. Para atualizar o SDK, altere a versão e o commit em
[tools/install_stm32f4_sdk.py](tools/install_stm32f4_sdk.py), reconstrua a imagem
e execute os testes. As versões das ferramentas ficam no Dockerfile.

O container inclui o ambiente de compilação e testes. Gravação e depuração
podem ser feitas com as ferramentas STM32Cube do host; o acesso USB ao ST-Link
dentro do container exige configurar a passagem do dispositivo no Docker.

## Compilar

Inicialize os submódulos do framework, caso ainda não estejam disponíveis:

```sh
git submodule update --init --recursive
```

Dentro do devcontainer:

```sh
cmake --preset Debug
cmake --build --preset Debug
cmake --preset Release
cmake --build --preset Release
```

ELF: `build/<configuração>/stm32f411_framework.elf`.
A integração fica em `cmake/stm32f4.cmake`, fora dos arquivos gerenciados pela
extensão STM32Cube. A seleção do BSP fica em `cmake/boards.cmake`. O arquivo
`config/stm32f4xx_hal_conf.h` continua no projeto, pois configura os módulos HAL
utilizados pelo firmware.

### Compilar sem container

Instale CMake >= 3.21 (para os presets), Ninja, Python 3 e Git, e coloque
`arm-none-eabi-gcc`/`g++` no `PATH`. Pode usar a toolchain do STM32Cube já
instalada no host. Instale o mesmo SDK em um diretório externo:

```sh
python3 tools/install_stm32f4_sdk.py "$HOME/.cache/stm32/STM32CubeF4-v1.28.3"
export STM32F4_SDK_DIR="$HOME/.cache/stm32/STM32CubeF4-v1.28.3"
cmake --preset Debug
cmake --build --preset Debug
```

O instalador requer rede na primeira execução, verifica as revisões e reutiliza
uma instalação existente se estiver íntegra e sem modificações locais. Também
é possível usar um pacote STM32CubeF4 já instalado: a variável deve apontar para
a raiz que contém `Drivers/`, com CMSIS e HAL completos. No CMake, a opção
`-DSTM32F4_SDK_DIR=/caminho/STM32CubeF4` tem prioridade sobre a variável de
ambiente. Os testes Python usam a variável de ambiente.

Ao migrar um diretório de build existente ou alternar entre host e container,
limpe o cache do CMake, que guarda os caminhos do SDK e das ferramentas:

```sh
cmake --fresh --preset Debug
cmake --fresh --preset Release
```

`--fresh` requer CMake >= 3.24; em versões anteriores, remova os diretórios
`build/Debug` e `build/Release` antes de configurar novamente.

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
