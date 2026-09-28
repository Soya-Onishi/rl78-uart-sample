#ifdef __RL78__
    #define EI()    __EI()
    #define DI()    __DI()
    #define far     __far
#else
    #define EI()
    #define DI()
    #define far 
#endif


#define INTST0                          0x001E
#define INTSR0                          0x002C
#define INTTM00                         0x0020

#define SPS0    (*(volatile unsigned short *)0x0126)
#define SMR00   (*(volatile unsigned short *)0x0110)
#define SCR00   (*(volatile unsigned short *)0x0118)
#define SDR00   (*(volatile unsigned short *)0xFF10)

#define SMR01   (*(volatile unsigned short *)0x0112)
#define SCR01   (*(volatile unsigned short *)0x011A)
#define SDR01   (*(volatile unsigned short *)0xFF12)

#define SOE0    (*(volatile unsigned short *)0x012A)
#define SS0     (*(volatile unsigned short *)0x0122)

#define IF0H    (*(volatile unsigned char *)0xFFE1)
#define MK0     (*(volatile unsigned short *)0xFFE4)
#define MK1     (*(volatile unsigned short *)0xFFE6)

#define SH_ADDRL (*(volatile unsigned short far *)0xE0000)
#define SH_ADDRH (*(volatile unsigned short far *)0xE0002)
#define SH_WRITE (*(volatile unsigned short far *)0xE0004)

#define RECV_BUFFER_SIZE  (128)
#define RECV_BUFFER_COUNT (8)

static volatile unsigned short timer_count;

static volatile unsigned char send_index;
static volatile unsigned char send_data[RECV_BUFFER_SIZE];
static volatile unsigned char send_enabled;

static volatile unsigned short recv_data_length[RECV_BUFFER_COUNT];
static volatile unsigned char recv_data_index;
static volatile unsigned char recv_data[RECV_BUFFER_COUNT][RECV_BUFFER_SIZE];
static volatile unsigned char has_recv_data;

#pragma interrupt sau_rx_int(vect=INTSR0)
void sau_rx_int(void) {
    const unsigned char recvbyte = SDR01;
    unsigned char is_eol = recvbyte == '\0'; 

    const unsigned short buf_index = recv_data_length[recv_data_index];
    recv_data[recv_data_index][buf_index] = recvbyte;
    recv_data_length[recv_data_index]++;

    if(is_eol) {
        has_recv_data |= 1 << recv_data_index;
        recv_data_index = (recv_data_index + 1) % RECV_BUFFER_COUNT;
        recv_data_length[recv_data_index] = 0;
    }
}

#pragma interrupt sau_tx_int(vect=INTST0)
void sau_tx_int(void) {
    if(!send_enabled) {
        return;
    }

    SDR00 = send_data[send_index];

    if(send_data[send_index] == '\0') {
        send_enabled = 0;
    } 
    send_index++;
}

static void init_sau(void) {
    SPS0 = 0x0003;

    SMR00 = 0x0022;
    SCR00 = 0x8093;
    SDR00 = 103U << 9;
    SOE0 |= 0x0001;
    SS0 |= 0x0001;

    SMR01 = 0x0122;
    SCR01 = 0x4093;
    SDR01 = 103U << 9;
    SS0 |= 0x0002;

    MK0 &= ~0x6000;
    MK1 &= ~0x0010;
}

int main(void) {
    init_sau();
    SH_ADDRL = (unsigned short)(((unsigned long)send_data) >> 0);
    SH_ADDRH = (unsigned short)(((unsigned long)send_data) >> 16);
    EI();

    while(1) {
        unsigned char recv_buffer_index = 0;
        unsigned char temp_buffer[RECV_BUFFER_SIZE];
        unsigned char temp_buffer_busy = 0;

        DI();
        if(has_recv_data && !send_enabled) {
            for(int i = 0; i < RECV_BUFFER_COUNT; i++) {
                if(has_recv_data & (1 << i)) {
                    recv_buffer_index = i;
                    break;
                }
            }

            for(int i = 0; i < recv_data_length[recv_buffer_index]; i++) {
                temp_buffer[i] = recv_data[recv_buffer_index][i];
            }

            has_recv_data &= ~(1 << recv_buffer_index);
            temp_buffer_busy = 1;
        }
        EI();

        if(!temp_buffer_busy) {
            continue;
        }

        for(int i = 0; temp_buffer[i] != '\0'; i++) {
            if('a' <= temp_buffer[i] && temp_buffer[i] <= 'z') {
                temp_buffer[i] = temp_buffer[i] - 'a' + 'A';
            } else if('A' <= temp_buffer[i] && temp_buffer[i] <= 'Z') {
                temp_buffer[i] = temp_buffer[i] - 'A' + 'a';
            }
        }

        DI();
        for(int i = 0;; i++) {
            send_data[i] = temp_buffer[i];
            if(!temp_buffer[i]) {
                break;
            }
        }
        SH_WRITE = 1;
        EI();
    }
}