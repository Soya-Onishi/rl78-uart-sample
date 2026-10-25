#define INTST0                          0x001E
#define INTTM00                         0x0020

#define P0      (*(volatile unsigned char *)0xFF00)
#define PM0     (*(volatile unsigned char *)0xFF20)

#define SPS0    (*(volatile unsigned short *)0x0126)
#define SMR00   (*(volatile unsigned short *)0x0110)
#define SCR00   (*(volatile unsigned short *)0x0118)
#define SDR00   (*(volatile unsigned short *)0xFF10)
#define SOE0    (*(volatile unsigned short *)0x012A)
#define SS0     (*(volatile unsigned short *)0x0122)

#define TPS0    (*(volatile unsigned short *)0x01B6)
#define TMR00   (*(volatile unsigned short *)0x0190)
#define TDR00   (*(volatile unsigned short *)0xFF18)
#define TS0     (*(volatile unsigned short *)0x01B2)

#define IF0H    (*(volatile unsigned char *)0xFFE1)
#define MK0     (*(volatile unsigned short *)0xFFE4)

#ifdef __RL78__
    #define EI()    __EI()
    #define DI()    __DI()
#else
    #define EI()
    #define DI()
#endif

static volatile unsigned short timer_count;
static volatile unsigned short uart_index;

static char send_data[256];
static unsigned char uart_enabled;

#pragma interrupt sau_tx_int(vect=INTST0)
void sau_tx_int(void) {
    if(!uart_enabled) {
        return;
    }

    SDR00 = send_data[uart_index];

    if(send_data[uart_index] == '\0') {
        uart_enabled = 0;
    } 
    uart_index++;
}

#pragma interrupt tau_int(vect=INTTM00)
void tau_int(void) {
    timer_count++;
}

static void init_sau(void) {
    SPS0 = 0x0003;

    SMR00 = 0x0022;
    SCR00 = 0x8093;
    SDR00 = 103U << 9;
    SOE0 |= 0x0001;
    SS0 |= 0x0001;
}

static void init_tau(void) {
    TPS0 = 0x0067;
    TMR00 = 0x0000;
    TDR00 = 2500;
    TS0 = 0x0001; 
}

static void print_loop_count(char* buffer, unsigned long long count) {
    int i = 0;
    static const char digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    for(i = 0; i < sizeof(count) * 2; i++) {
        unsigned char c = (count >> (i * 4)) & 0x0F;
        buffer[sizeof(count) * 2 - i - 1] = digits[c];
    }
    buffer[i] = '\n';
    buffer[i + 1] = '\0';
}


int main(void) {
    const char message[] = { "Hello, QEMU World! 0x" };
    int i;
    unsigned long long loop_count = 0;

    DI();
    timer_count = 0;
    uart_index = 0;
    PM0 |= 0x01;
    EI();

    for(i = 0; message[i] != '\0'; i++) {
        send_data[i] = message[i];
    }

    MK0 &= ~0x6000;
    init_sau();
    init_tau(); 

    while(1) {
        static unsigned int sw_old_state = 0;
        unsigned int sw_state;
        unsigned int sw_on_edge;

        DI();
        sw_state = ((P0 & 0x01) == 1);
        EI();

        sw_old_state = sw_state;
        loop_count++;

        // on edge
        if(sw_state && !sw_old_state) {
            DI();
            uart_enabled = 1;
            timer_count = 0;

            print_loop_count(&send_data[sizeof(message) - 1], loop_count);

            uart_index = 0;
            loop_count = 0;
            IF0H |= 0x20;
            EI();
        }
    }
}