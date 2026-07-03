#include <stdint.h>

// ─── Base Addresses ───────────────────────────────────────────
#define RCC_BASE     0x40021000
#define GPIOA_BASE   0x40010800
#define TIM3_BASE    0x40000400
#define FLASH_BASE   0x40022000
#define USART2_BASE  0x40004400

// ─── RCC Registers ────────────────────────────────────────────
#define RCC_APB1ENR  (*(volatile uint32_t *)(RCC_BASE + 0x1C))
#define RCC_APB2ENR  (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_CR       (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR     (*(volatile uint32_t *)(RCC_BASE + 0x04))

// ─── GPIOA Registers ──────────────────────────────────────────
#define GPIOA_CRL    (*(volatile uint32_t *)(GPIOA_BASE + 0x00))

// ─── Flash Register ───────────────────────────────────────────
#define FLASH_ACR    (*(volatile uint32_t *)(FLASH_BASE + 0x00))

// ─── SysTick Registers ────────────────────────────────────────
#define SYST_CSR     (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR     (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR     (*(volatile uint32_t *)0xE000E018)

// ─── TIM3 Registers ───────────────────────────────────────────
#define TIM3_CR1     (*(volatile uint32_t *)(TIM3_BASE + 0x00))
#define TIM3_EGR     (*(volatile uint32_t *)(TIM3_BASE + 0x14))
#define TIM3_CCMR1   (*(volatile uint32_t *)(TIM3_BASE + 0x18))
#define TIM3_CCER    (*(volatile uint32_t *)(TIM3_BASE + 0x20))
#define TIM3_PSC     (*(volatile uint32_t *)(TIM3_BASE + 0x28))
#define TIM3_ARR     (*(volatile uint32_t *)(TIM3_BASE + 0x2C))
#define TIM3_CCR1    (*(volatile uint32_t *)(TIM3_BASE + 0x34))

// ─── Servo Definitions ────────────────────────────────────────
#define SERVO_MIN    500     // 0.5ms = 0°   (SG90 actual range)
#define SERVO_MID    1500    // 1.5ms = 90°
#define SERVO_MAX    2500    // 2.5ms = 180° (SG90 actual range)

// ─── NVIC ─────────────────────────────────────────────────────
#define NVIC_ISER1   (*(volatile uint32_t *)0xE000E104)

// ─── USART2 Registers ─────────────────────────────────────────
#define USART2_SR    (*(volatile uint32_t *)(USART2_BASE + 0x00))
#define USART2_DR    (*(volatile uint32_t *)(USART2_BASE + 0x04))
#define USART2_BRR   (*(volatile uint32_t *)(USART2_BASE + 0x08))
#define USART2_CR1   (*(volatile uint32_t *)(USART2_BASE + 0x0C))
#define USART2_CR2   (*(volatile uint32_t *)(USART2_BASE + 0x10))

// ─── Global Variables ─────────────────────────────────────────
volatile uint32_t tick_count = 0;
volatile uint8_t  rec_data   = 0;

// ─── Receive Buffer ───────────────────────────────────────────
#define RX_BUFFER_SIZE  8
volatile char    rx_buffer[RX_BUFFER_SIZE];
volatile uint8_t rx_index    = 0;
volatile uint8_t rx_complete = 0;

// ─── SystemInit ───────────────────────────────────────────────
void SystemInit(void) {
}

// ─── SysTick Handler ──────────────────────────────────────────
void SysTick_Handler(void) {
    tick_count++;
}

// ─── USART2 Interrupt ─────────────────────────────────────────
void USART2_IRQHandler(void) {
    if(USART2_SR & (1 << 5)) {
        char c = (char)USART2_DR;

        if(c == '\r' || c == '\n') {
            if(rx_index > 0) {
                rx_buffer[rx_index] = '\0';
                rx_complete = 1;
                rx_index = 0;
            }
        } else if(rx_index < RX_BUFFER_SIZE - 1) {
            rx_buffer[rx_index++] = c;
        }
    }
}

// ─── get_tick ─────────────────────────────────────────────────
uint32_t get_tick(void) {
    return tick_count;
}

// ─── String to Integer ────────────────────────────────────────
int str_to_int(const char *str) {
    int result = 0;
    while(*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

// ─── PLL Init ─────────────────────────────────────────────────
void pll_init(void) {
    RCC_CR      |=  (1 << 16);
    while(!(RCC_CR & (1 << 17)));

    FLASH_ACR   &= ~(0x7);
    FLASH_ACR   |=  (0x2);

    RCC_CFGR    &= ~(0xF << 4);
    RCC_CFGR    |=  (0x4 << 8);
    RCC_CFGR    &= ~(0x7 << 11);

    RCC_CFGR    |=  (1 << 16);
    RCC_CFGR    &= ~(1 << 17);
    RCC_CFGR    &= ~(0xF << 18);
    RCC_CFGR    |=  (0x7 << 18);

    RCC_CR      |=  (1 << 24);
    while(!(RCC_CR & (1 << 25)));

    RCC_CFGR    &= ~(0x3);
    RCC_CFGR    |=  (0x2);
    while((RCC_CFGR & (0x3 << 2)) != (0x2 << 2));
}

// ─── SysTick Init ─────────────────────────────────────────────
void systick_init(void) {
    SYST_RVR = 71999;
    SYST_CVR = 0;
    SYST_CSR = 0x7;
}

// ─── USART2 Init ──────────────────────────────────────────────
void usart2_init(void) {
    // Enable clocks
    RCC_APB1ENR |= (1 << 17);      // USART2EN
    RCC_APB2ENR |= (1 << 2);       // IOPAEN

    // PA2 → AF push-pull 10MHz (TX) bits 11:8
    GPIOA_CRL   &= ~(0xF << 8);
    GPIOA_CRL   |=  (0x9 << 8);

    // PA3 → floating input (RX) bits 15:12
    GPIOA_CRL   &= ~(0xF << 12);
    GPIOA_CRL   |=  (0x4 << 12);

    USART2_CR1  |=  (1 << 13);     // UE
    USART2_CR1  &= ~(1 << 12);     // M = 8 data bits
    USART2_CR2  &= ~(0x3 << 12);   // 1 stop bit
    USART2_BRR   =   0x139;        // 115200 baud at 36MHz PCLK1
    USART2_CR1  |=  (1 << 3);      // TE
    USART2_CR1  |=  (1 << 2);      // RE
    USART2_CR1  |=  (1 << 5);      // RXNEIE
    NVIC_ISER1  |=  (1 << 6);      // IRQ38 → USART2
}

// ─── TIM3 PWM Init ────────────────────────────────────────────
void tim3_pwm_init(void) {

    // 1. Enable TIM3 clock
    RCC_APB1ENR |= (1 << 1);       // TIM3EN

    // GPIOA clock already enabled in usart2_init()
    // but safe to enable again
    RCC_APB2ENR |= (1 << 2);       // IOPAEN

    // 2. Configure PA6 → AF push-pull 50MHz
    //    Pin 6 → bits 27:24 of CRL
    //    CNF=10, MODE=11 → 1011 = 0xB
    GPIOA_CRL   &= ~(0xF << 24);   // clear bits 27:24 only
    GPIOA_CRL   |=  (0xB << 24);   // set PA6, PA2/PA3 untouched

    // 3. Set Prescaler → 72MHz / (71+1) = 1MHz → 1 tick = 1μs
    TIM3_PSC = 71;

    // 4. Set Auto-Reload → 20000 ticks = 20ms = 50Hz
    TIM3_ARR = 19999;

    // 5. Set PWM Mode 1 on Channel 1
    //    OC1M[2:0] = 110 → bits 6:4 of CCMR1
    //    CC1S = 00 → output mode → bits 1:0 of CCMR1
    //    OC1PE = 1 → bit 3 (preload enable)
    TIM3_CCMR1  = 0;               // clear first — fresh start
    TIM3_CCMR1 |= (0x6 << 4);     // OC1M = 110 (PWM mode 1)
    TIM3_CCMR1 |= (0x1 << 3);     // OC1PE = 1

    // 6. Enable Channel 1 output
    //    CC1E = bit 0 of CCER
    TIM3_CCER   = 0;               // clear first
    TIM3_CCER  |= (1 << 0);        // CC1E = 1

    // 7. Set initial CCR1 value BEFORE update event
    TIM3_CCR1   = SERVO_MID;       // 1500 = 1.5ms = 90°

    // 8. Generate update event to load PSC/ARR/CCR1
    //    into active shadow registers
    TIM3_EGR   |= (1 << 0);        // UG bit

    // 9. Enable counter
    TIM3_CR1   |= (1 << 0);        // CEN = 1
}

// ─── Servo Set Angle ──────────────────────────────────────────
void servo_set_angle(uint8_t angle) {
    uint32_t ccr = SERVO_MIN +
                   ((uint32_t)angle * (SERVO_MAX - SERVO_MIN)) / 180;
    TIM3_CCR1 = ccr;
}

// ─── USART2 Transmit ──────────────────────────────────────────
void usart2_transmit(uint8_t data) {
    while(!(USART2_SR & (1 << 7)));
    USART2_DR = data;
}

// ─── USART2 Send String ───────────────────────────────────────
void usart2_send_string(const char *str) {
    while(*str) {
        usart2_transmit((uint8_t)*str++);
    }
}

// ─── USART2 Send Number ───────────────────────────────────────
void usart2_send_number(uint32_t num) {
    if(num == 0) { usart2_transmit('0'); return; }
    char buf[10];
    int i = 0;
    while(num > 0) { buf[i++] = '0' + (num % 10); num /= 10; }
    while(i > 0)   { usart2_transmit(buf[--i]); }
}

// ─── Main ─────────────────────────────────────────────────────
int main(void) {

    pll_init();
    systick_init();
    usart2_init();      // USART first — establishes PA2/PA3
    tim3_pwm_init();    // PWM second — only touches PA6

    // Verify PLL
    uint32_t sws = (RCC_CFGR >> 2) & 0x3;
    if(sws == 0x2)      usart2_send_string("Clock: PLL 72MHz\r\n");
    else if(sws == 0x0) usart2_send_string("Clock: HSI 8MHz\r\n");
    else if(sws == 0x1) usart2_send_string("Clock: HSE 8MHz\r\n");

    usart2_send_string("Servo Control Ready\r\n");
    usart2_send_string("Enter angle (0-180) and press Enter:\r\n");

    while(1) {

        if(rx_complete) {

            // Copy buffer locally before interrupt overwrites
            char local_buf[RX_BUFFER_SIZE];
            for(int i = 0; i < RX_BUFFER_SIZE; i++) {
                local_buf[i] = rx_buffer[i];
            }
            rx_complete = 0;

            // Parse angle
            int received_angle = str_to_int(local_buf);

            if(received_angle >= 0 && received_angle <= 180) {
                servo_set_angle((uint8_t)received_angle);

                usart2_send_string("Angle: ");
                usart2_send_number((uint32_t)received_angle);
                usart2_send_string("° | CCR1: ");
                usart2_send_number(TIM3_CCR1);
                usart2_send_string("\r\n");

            } else {
                usart2_send_string("Invalid. Enter 0-180.\r\n");
            }
        }
    }
}
