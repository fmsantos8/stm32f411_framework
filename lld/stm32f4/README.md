# Dependências STM32F4

Subconjunto de CMSIS e STM32Cube HAL necessário à integração do framework.
Obtido do SDK disponível localmente em `FW.0075.000/src/lld/stm32f4`
(HAL ST V1.8.5; CMSIS Cortex-M4 V5.1.2). Não há dependência de caminhos desse
outro projeto para compilar.

Inclui headers de F411/F407, headers do core para GCC, system CMSIS e os módulos
HAL selecionados em `config/stm32f4xx_hal_conf.h`, com dependências de headers.
DMA e extensões de TIM são dependências dos drivers ST; as operações expostas
pelo port são síncronas e não usam DMA.

Arquivos preservados, exceto pela restauração do `HAL_MspInit` fraco vazio em
`stm32f4xx_hal.c`: a cópia local o havia comentado. A restauração corresponde ao
[arquivo oficial V1.8.5](https://github.com/STMicroelectronics/stm32f4xx-hal-driver/blob/v1.8.5/Src/stm32f4xx_hal.c).
Clock, pinos, interrupções e política do framework ficam no port.

O aviso ST original está em `STM32F4xx_HAL_Driver/LICENSE.txt`. Os arquivos ST
seguem BSD-3-Clause; Arm CMSIS usa Apache-2.0, conforme os cabeçalhos. Cópias
dos textos de licença acompanham os componentes.

Fontes upstream:

- [STM32 HAL](https://github.com/STMicroelectronics/stm32f4xx-hal-driver/tree/v1.8.5)
- [CMSIS STM32F4](https://github.com/STMicroelectronics/cmsis-device-f4)
- [CMSIS Core](https://github.com/ARM-software/CMSIS_5)
