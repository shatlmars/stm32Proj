#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_cortex.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_utils.h"
#include "stm32f4xx_ll_spi.h"

#include <cstdint>
#include <string>

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
    for(volatile int i = 0; i < 100000; ++i);
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



int main(){
    clock_init();
    SysTick_Config(kSysClkHz / 1000);
    usart_init();
    led_init();

    spi2_init();
    spi_lora_reset();
    spi_lora_wait_not_busy();

    uint8_t status = spi_lora_get_status();

    uart_write_str("LoRa GetStatus: 0x");
    const char hex[] = "0123456789ABCDEF";
    uart_write(hex[(status >> 4) & 0x0F]);
    uart_write(hex[status & 0x0F]);
    uart_write_str("\r\n");

    if (status != 0x00 && status != 0xFF) {
        uart_write_str("SX126x responds OK\r\n");
    } else {
        uart_write_str("SX126x no response\r\n");
    }

    uint32_t next_blink = 0;
    std::string line;
    while(true){
        uint8_t byte = 0;
        if(ring_pop(byte)){
            if(byte == '\n'){
                if(!line.empty()){
                    line += "\r\n";
                    uart_write_str(line.c_str());
                    line.clear();
                    line = "from stm: " ;
                    LL_GPIO_TogglePin(GPIOA, LL_GPIO_PIN_6);
                }
            } else if(byte != '\r'){
                line += static_cast<char>(byte);
            }
        }
    }
    return 0;
}