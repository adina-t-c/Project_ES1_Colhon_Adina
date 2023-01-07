#include "mbed.h"
#include "rtos.h"
#include "math.h"

#include "mma.h"

DigitalOut ledR(LED1);
DigitalOut ledG(LED2);
DigitalOut ledB(LED3);

PinName const SDA = PTE25;
PinName const SCL = PTE24;

I2C i2c(SDA, SCL);

MMAUnit mma(&i2c);

/*4 point moving average buffers*/
int32_t x_buffer[4];
int32_t y_buffer[4];

/*values extracted from that average*/
int32_t x;
int32_t y;

/*index value used in the population of the buffers*/
int8_t  index = 0;

/*initial position of the board*/
int16_t initial_x = 0;
int16_t initial_y = 0;

/*blink speeds for the different axis positions*/
int16_t b_spd_x_poz = 0;
int16_t b_spd_x_neg = 0;
int16_t b_spd_y_poz = 0;
int16_t b_spd_y_neg = 0;

Mutex mma_access;

/*This function initializes the LED values*/
void init_leds()
{
    ledR = 1;
    ledG = 1;
    ledB = 1;
}

/*This function calculates the blink speed based on the difference from the initial axis position
There are three different possible blink speeds*/
uint8_t calculate_blink(int16_t curr_val, int16_t init_val)
{
    uint16_t diff= abs(curr_val - init_val);
    uint8_t blink_speed;
    
    if(UINT14_MAX/8 > diff)
    {
        blink_speed = 1;
    }
    else if(UINT14_MAX/6 > diff)
    {
        blink_speed = 2;
    }
    else
    {
        blink_speed = 3;
    }
    
    return blink_speed;
}       

/*This function checks where on the X axis the board currently is and triggers the LED blink speed calculation based on the board possition
Some buffer values between the different zones were added to make the algorithm easier to predict while manually turning the board, since the human hand isn't precise enough for exact values*/
void check_x_axis(int16_t x)
{
    if((initial_x + 50)>x && (initial_x - 50)<x)
    {
        b_spd_x_poz = 0;
        b_spd_x_neg = 0;
    }
    else if((initial_x - 500)>=x)
    {
        b_spd_x_neg = calculate_blink(x,initial_x);
    }
    else if((initial_x + 500)<=x)
    {
        b_spd_x_poz = calculate_blink(x,initial_x);
    }
}

/*This function checks where on the Y axis the board currently is and triggers the LED blink speed calculation based on the board possition
Some buffer values between the different zones were added to make the algorithm easier to predict while manually turning the board, since the human hand isn't precise enough for exact values*/
void check_y_axis(int16_t y)
{
    if((initial_y + 50)>y && (initial_y - 50)<y)
    {
        b_spd_y_poz = 0;
        b_spd_y_neg = 0;
    }
    else if((initial_y - 500)>=y)
    {
        b_spd_y_neg = calculate_blink(y,initial_y);
    }
    else if((initial_y + 500)<=y)
    {
        b_spd_y_poz = calculate_blink(y,initial_y);
    }
}

/*This thread triggers the board position and blink speed calculations*/
void led_thread(void const *argument)
{
    while (true) {     
            mma_access.lock();
            check_x_axis(x);
            check_y_axis(y);
            mma_access.unlock();
    }
}

/*This function populates the buffers before the actual reading begins*/
void init_buffers()
{
   for(int8_t i=0;i<4;i++)
   {
       mma.mmaReadAcc();
       x_buffer[i] = mma.getAccX();
       y_buffer[i] = mma.getAccY();
    }
}

/*This thread handles the reading of the x and y axis
It applies a four point moving average on the values before providing the values to the rest of the program*/
void read_thread()
{
    while(true)
    {
       int32_t sum_x = 0;
       int32_t sum_y = 0;
       
       mma.mmaReadAcc();
       
       x_buffer[index] = mma.getAccX();
       y_buffer[index] = mma.getAccY();
       
       index = (index + 1) % 4;
       
       for(int8_t i=0;i<4;i++)
       {
           sum_x += x_buffer[i];
           sum_y += y_buffer[i];
        }
        
        x = sum_x/4;
        y = sum_y/4;  
    }    
}

/*This function calculates the initial position of the board on the axis and blinks the LED when it's done
It read a total of the different values and takes their average as the initial position*/
void mma_calib_gyro()
{
    uint8_t i=0;
    int32_t total_x = 0;
    int32_t total_y = 0;
    
    while(i!=10)
    {
        mma.mmaReadAcc();
    
        total_x += mma.getAccX();
        total_y += mma.getAccY();
    
        i++;
    }
    
    initial_x = total_x/10;
    initial_y = total_y/10;
    
    ledG = ledB = ledR = 0;
    wait(0.2);
    ledG = ledB = ledR = 1;
    wait(0.2);
    ledG = ledB = ledR = 0;
    wait(0.2);
    ledG = ledB = ledR = 1;
    wait(0.2);
}

/*This thread handles the blinking of the LEDs based on the speeds calculated by the LED thread
It has a varying amount of blinks and wait time per loop, which is determined based on the blink speed values*/
void x_task() {
   while(true) 
   {
        int16_t neg_x, poz_x, poz_y, neg_y, wait_time=0;
        
        mma_access.lock();
        
        poz_x = b_spd_x_poz;
        neg_x = b_spd_x_neg;
        poz_y = b_spd_y_poz;
        neg_y = b_spd_y_neg;
        
        mma_access.unlock();
        
        if(poz_x == 0 && neg_x ==0 && poz_y == 0 && neg_y == 0)
        {
            ledR = 0;
            ledG = 0;
            ledB = 0;
        }
        else
        {
            wait_time = 400;
            
            if(poz_x + neg_x >= 1)
            {
                ledR = 0;
                
                if(neg_x!=0)
                {
                    ledG = 1;
                }
                else
                {
                    ledG = 0;
                }
                
                ledB = 1;
            
                Thread::wait(100);
            
                ledR = 1;
                ledG = 1;
                ledB = 1;
                
                Thread::wait(100);
                
                wait_time -= 200;
            }
            
            if(poz_y + neg_y >= 1)
            {
                ledR = 1;
                
                if(neg_y!=0)
                {
                    ledG = 1;
                }
                else
                {
                    ledG = 0;
                }
                
                ledB = 0;
            
                Thread::wait(100);
            
                ledR = 1;
                ledG = 1;
                ledB = 1;
                
                Thread::wait(100);
                
                wait_time -= 200;
            }
            
            if(wait_time !=0)
            {
                Thread::wait(wait_time);
            }
            
            wait_time = 400;
        
            if(poz_x + neg_x >= 2)
            {
                ledR = 0;
            
                if(neg_x!=0)
                {
                    ledG = 1;
                }
                else
                {
                    ledG = 0;
                }
            
                ledB = 1;
                
                Thread::wait(100);
                
                ledR = 1;
                ledG = 1;
                ledB = 1;
                
                Thread::wait(100);
                
                wait_time -= 200;
            }
            
            if(poz_y + neg_y >= 2)
            {
                ledR = 1;
            
                if(neg_y!=0)
                {
                    ledG = 1;
                }
                else
                {
                    ledG = 0;
                }
            
                ledB = 0;
                
                Thread::wait(100);
                
                ledR = 1;
                ledG = 1;
                ledB = 1;
                
                Thread::wait(100);
                
                wait_time -= 200;
            }

            if(wait_time !=0)
            {
                Thread::wait(wait_time);
            }
            
            wait_time = 400;
            
            if(poz_x + neg_x == 3)
            {
                ledR = 0;
            
                if(neg_x!=0)
                {
                    ledG = 1;
                }
                else
                {
                    ledG = 0;
                }
                
                ledB = 1;
                
                Thread::wait(100);
            
                ledR = 1;
                ledG = 1;
                ledB = 1;
            
                Thread::wait(100);
                
                wait_time -= 200;
            } 
            
            if(poz_y + neg_y == 3)
            {
                ledR = 1;
            
                if(neg_y!=0)
                {
                    ledG = 1;
                }
                else
                {
                    ledG = 0;
                }
                
                ledB = 0;
                
                Thread::wait(100);
            
                ledR = 1;
                ledG = 1;
                ledB = 1;
            
                Thread::wait(100);
                
                wait_time -= 200;
            } 
            
            if(wait_time !=0)
            {
                Thread::wait(wait_time);
            }
            
        }
    }
}

int main()
{
    mma.mmaInit();
    init_leds();
    mma_calib_gyro();
    init_buffers();
    Thread thread_led(led_thread);
    Thread thread_x(x_task);
    Thread thread_read(read_thread);
    while (true) {
        Thread::wait(5000);
    }
}
