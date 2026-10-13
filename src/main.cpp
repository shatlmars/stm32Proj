#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_cortex.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_utils.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_tim.h"

#include <cstdint>
#include <string>
#include <cstring>

#define kSysClkHz 168000000
#define kPclk2Hz 84000000
#define kBaud 115200

uint8_t ring[128];
volatile uint8_t head = 0;
volatile uint8_t tail = 0;
volatile uint32_t ms = 0;

void clock_init() {
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    LL_RCC_HSE_Enable();

    while(!LL_RCC_HSE_IsReady()) {

    }
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_8, 336, LL_RCC_PLLP_DIV_2);
    LL_RCC_PLL_Enable();
    while(!LL_RCC_PLL_IsReady()) {

    }

    LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);
    while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_5){

    }

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL){

    }

    SystemCoreClock = kSysClkHz;
}

void usart_init() {
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);

    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_10, LL_GPIO_MODE_ALTERNATE);

    LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_9, LL_GPIO_AF_7);
    LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_10, LL_GPIO_AF_7);

    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_9, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_9, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_9, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_10, LL_GPIO_PULL_UP);

    LL_USART_SetDataWidth(USART1, LL_USART_DATAWIDTH_8B);
    LL_USART_SetStopBitsLength(USART1, LL_USART_STOPBITS_1);
    LL_USART_SetParity(USART1, LL_USART_PARITY_NONE);
    LL_USART_SetHWFlowCtrl(USART1, LL_USART_HWCONTROL_NONE);
    LL_USART_SetTransferDirection(USART1, LL_USART_DIRECTION_TX_RX);
    LL_USART_SetOverSampling(USART1, LL_USART_OVERSAMPLING_16);
    LL_USART_SetBaudRate(USART1, kPclk2Hz, LL_USART_OVERSAMPLING_16, kBaud);

    LL_USART_Enable(USART1);

    LL_USART_EnableIT_RXNE(USART1);
    NVIC_SetPriority(USART1_IRQn, 0);
    NVIC_EnableIRQ(USART1_IRQn);
}

void usart_second_init() {
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_2, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_3, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_2, LL_GPIO_AF_7);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_3, LL_GPIO_AF_7);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_2, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_2, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_3, LL_GPIO_PULL_UP);

    LL_USART_SetDataWidth(USART2, LL_USART_DATAWIDTH_8B);
    LL_USART_SetStopBitsLength(USART2, LL_USART_STOPBITS_1);
    LL_USART_SetParity(USART2, LL_USART_PARITY_NONE);
    LL_USART_SetHWFlowCtrl(USART2, LL_USART_HWCONTROL_NONE);
    LL_USART_SetTransferDirection(USART2, LL_USART_DIRECTION_TX_RX);
    LL_USART_SetOverSampling(USART2, LL_USART_OVERSAMPLING_16);
    LL_USART_SetBaudRate(USART2, 42000000, LL_USART_OVERSAMPLING_16, 115200);

    LL_USART_Enable(USART2);
}

void uart_write(uint8_t byte) {
    while(!LL_USART_IsActiveFlag_TXE(USART1)) {

    }

    LL_USART_TransmitData8(USART1, byte);
}

void uart_write_str(const char* s) {
    while(*s) {
        uart_write(static_cast<uint8_t>(*s++));
    }
}

bool ring_pop(uint8_t& out) {
    if(head == tail) {
        return false;
    }
    out = ring[tail];
    tail = (tail + 1) % sizeof(ring);
    return true;
}

void led_init(){
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_6, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_6, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_6);
}

void spi2_init() {
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI2);

    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_13 | LL_GPIO_PIN_14 | LL_GPIO_PIN_15, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_13, LL_GPIO_AF_5);
    LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_14, LL_GPIO_AF_5);
    LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_15, LL_GPIO_AF_5);
    LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_13 | LL_GPIO_PIN_14 | LL_GPIO_PIN_15, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_14, LL_GPIO_PULL_UP);

    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_12, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_12, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_12);

    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_10, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_10, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_10);

    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_11, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_11, LL_GPIO_PULL_NO);

    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_9, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_9, LL_GPIO_PULL_NO);

    LL_SPI_SetBaudRatePrescaler(SPI2, LL_SPI_BAUDRATEPRESCALER_DIV4);
    LL_SPI_SetTransferDirection(SPI2, LL_SPI_FULL_DUPLEX);
    LL_SPI_SetClockPhase(SPI2, LL_SPI_PHASE_1EDGE);
    LL_SPI_SetClockPolarity(SPI2, LL_SPI_POLARITY_LOW);
    LL_SPI_SetDataWidth(SPI2, LL_SPI_DATAWIDTH_8BIT);
    LL_SPI_SetNSSMode(SPI2, LL_SPI_NSS_SOFT);
    LL_SPI_SetMode(SPI2, LL_SPI_MODE_MASTER);
    LL_SPI_Enable(SPI2);
}

void cs_low() {LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_12);}
void cs_high() {LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_12);}

uint8_t spi_transfer(uint8_t data){
    while(!LL_SPI_IsActiveFlag_TXE(SPI2));
    LL_SPI_TransmitData8(SPI2, data);
    while(!LL_SPI_IsActiveFlag_RXNE(SPI2));
    return LL_SPI_ReceiveData8(SPI2);
}

void spi_lora_reset(){
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_10);
    for(volatile int i = 0; i < 200000; ++i);
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_10);
}

bool spi_lora_is_busy() {
    return LL_GPIO_IsInputPinSet(GPIOB, LL_GPIO_PIN_11);
}

void spi_lora_wait_not_busy() {
    while(spi_lora_is_busy());
}

uint8_t spi_lora_get_status() {
    spi_lora_wait_not_busy();
    cs_low();
    spi_transfer(0xC0);
    uint8_t status = spi_transfer(0x00);
    cs_high();
    return status;  
}

extern "C" void SysTick_Handler(void){
    ++ms;
}

extern "C" void USART1_IRQHandler(void) {
    if(LL_USART_IsActiveFlag_RXNE(USART1)) {
        uint8_t byte = LL_USART_ReceiveData8(USART1);
        uint8_t next = static_cast<uint8_t>((head + 1) % sizeof(ring));
        if(next != tail) {
            ring[head] = byte;
            head = next;
        }
    }

    if(LL_USART_IsActiveFlag_ORE(USART1)) {
        (void)USART1->SR;
        (void)USART1->DR;
    }
}


void uart_second_write(uint8_t byte){
    while(!LL_USART_IsActiveFlag_TXE(USART2)){

    }
    LL_USART_TransmitData8(USART2, byte);
}

void uart_second_write_str(const char* str){
    while(*str){
        uart_second_write(static_cast<uint8_t>(*str++));
    }
}


void send_and_wait(const char* cmd, uint32_t wait_ms){
    uart_second_write_str(cmd);
        uart_second_write_str("\r\n");
        uint32_t start = ms;
        while (ms - start < wait_ms) {
            if (LL_USART_IsActiveFlag_RXNE(USART2)) {
                uint8_t b = LL_USART_ReceiveData8(USART2);
                uart_write(b);   // сразу видим ответ в терминале
            }
        }
}

void init_esp32() {
    uart_write_str("Step 1: AT\r\n");
    for (int i = 0; i < 3; i++) {
        send_and_wait("AT", 300);
    }

    uart_write_str("Step 2: CWMODE\r\n");
    send_and_wait("AT+CWMODE=1", 500);

    uart_write_str("Step 3: CWJAP (waiting up to 10s)\r\n");
    send_and_wait("AT+CWJAP=\"ASUS\",\"27031992\"", 10000);

    uart_write_str("Step 4: CIPMUX\r\n");
    send_and_wait("AT+CIPMUX=1", 500);

    uart_write_str("Step 5: CIPSTART\r\n");
    send_and_wait("AT+CIPSTART=0,\"UDP\",\"0.0.0.0\",5000,5000,2", 2000);

    uart_write_str("Init finished\r\n");
}

void servo_init() {
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);

    // PA0 = TIM2_CH1
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_0, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_0, LL_GPIO_AF_1);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_0, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_0, LL_GPIO_OUTPUT_PUSHPULL);

    // TIM2: 50 Гц
    // APB1 = 42 МГц, таймер получает 84 МГц (x2)
    // 84 МГц / 84 = 1 МГц (1 мкс тик)
    // 1 МГц / 20000 = 50 Гц
    LL_TIM_SetPrescaler(TIM2, 84 - 1);
    LL_TIM_SetAutoReload(TIM2, 20000 - 1);
    LL_TIM_SetCounterMode(TIM2, LL_TIM_COUNTERMODE_UP);

    LL_TIM_OC_SetMode(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
    LL_TIM_OC_SetPolarity(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_OCPOLARITY_HIGH);
    LL_TIM_OC_SetCompareCH1(TIM2, 1500);  // 1.5 мс — середина
    LL_TIM_OC_EnablePreload(TIM2, LL_TIM_CHANNEL_CH1);
    LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH1);

    LL_TIM_EnableCounter(TIM2);
    LL_TIM_GenerateEvent_UPDATE(TIM2);
}

// Установка угла (примерно -90…+90)
// pulse_us: 1000 = -90°, 1500 = 0°, 2000 = +90°
void servo_set_pulse(uint16_t pulse_us) {
    if (pulse_us < 800)  pulse_us = 800;
    if (pulse_us > 2200) pulse_us = 2200;
    LL_TIM_OC_SetCompareCH1(TIM2, pulse_us);
}

int main() {
    clock_init();
    SysTick_Config(kSysClkHz / 1000);
    usart_init();          // UART1
    usart_second_init();
    servo_init();

    for (volatile int i = 0; i < 100000; i++);
    uart_write_str("\rBOOT\r\n");

    led_init();
    // ждём загрузки ESP32
    uint32_t t = ms;
    while (ms - t < 4000);
    std::string line;
    bool flag_esp32_init{false};
    while (true) {
        servo_set_pulse(1000);   // в одну сторону
        uint32_t t = ms;
        while (ms - t < 1000);

        servo_set_pulse(2000);   // в другую
        t = ms;
        while (ms - t < 1000);

        servo_set_pulse(1500);   // центр
        t = ms;
        while (ms - t < 1000);
        if (LL_USART_IsActiveFlag_RXNE(USART2)) {
            uint8_t b = LL_USART_ReceiveData8(USART2);
            uart_write(b);
        }

        uint8_t byte = 0;
        if (ring_pop(byte)) {
            if (byte == '\n') {
                if (!line.empty()) {
                    uart_write_str("\r\n>> ");
                    uart_write_str(line.c_str());
                    uart_write_str("\r\n");
                    if(strcmp("init", line.c_str()) == 0){
                        if(!flag_esp32_init){
                            uart_write_str("Initializing ESP32...\r\n");
                            init_esp32();
                            flag_esp32_init = true;
                        }else{
                            uart_write_str("ESP32 already initialized\r\n");
                        }
                    }else{
                        // uart_second_write_str(line.c_str());
                    }
                    line.clear();
                    LL_GPIO_TogglePin(GPIOA, LL_GPIO_PIN_6);
                }
            } else if (byte != '\r') {
                line += static_cast<char>(byte);
            }
        }
    }
    return 0;
}