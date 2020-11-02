#include "mbed.h"
#include "rtos.h"

#include "mma.h"

DigitalOut ledR(LED1);
DigitalOut ledG(LED2);
DigitalOut ledB(LED3);

static RawSerial pc(USBTX, USBRX);

PinName const SDA = PTE25;
PinName const SCL = PTE24;

I2C i2c(SDA, SCL);

MMAUnit mma(&i2c);

Queue<int, 32> queue;
int *message;

Mutex mma_access;
volatile bool mess_flg = false;

void led_thread(void const *argument)
{
    while (true) {
        ledR = 0;
        ledG = 1;
        ledB = 1;
        Thread::wait(1000);
        ledR = 1;
        ledG = 0;
        ledB = 1;
        Thread::wait(1000);
        ledR = 1;
        ledG = 1;
        ledB = 0;
        Thread::wait(1000);
    }
}

void mma_repeat(void const *argument) {
    while (true) {
        if(mess_flg) {
            mma_access.lock();
            mma.mmaReadAcc();
            pc.printf("x=%d y=%d z=%d\n\r", mma.getAccX(), mma.getAccY(), mma.getAccZ());
            mma_access.unlock();
        }
        Thread::wait(1000);
    }
}

void serial_read() {
            
    char c;
    c = pc.getc();
    if(c == 'r' || c == 'R') {
        ledR = 0;
        ledG = 0;
        ledB = 0;
        queue.put(message);  
    }
    if(c == 'm' || c == 'M') {
        mess_flg = true;
    }
    if(c == 's' || c == 'S') {
        mess_flg = false;
    }
    
}

uint16_t rec_buff[1200];

void recorder_task() {
    while(true) {
        osEvent evt = queue.get();
        if (evt.status == osEventMessage) {
            mma_access.lock();
            for(int i = 0; i < 1200; i+=3) {
                mma.mmaReadAcc();
                rec_buff[i] = mma.getAccX();
                rec_buff[i+1] = mma.getAccY();
                rec_buff[i+2] = mma.getAccZ();
                Thread::wait(10);
            }
            char* b = (char*)rec_buff;
            for(int j = 0; j < 2400; j++) 
                pc.putc(b[j]);
            mma_access.unlock();
        }
    }
}

int main()
{
    pc.attach(serial_read);
    mma.mmaInit();
    Thread thread_mma(mma_repeat);
    Thread thread_led(led_thread);
    Thread thread_rec(recorder_task);
    

    while (true) {
        Thread::wait(5000);
    }
}
