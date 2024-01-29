#include "FaveroPacket.h"
#include <bitset>
#include <cstring>

// Favero transmits 10 bytes bursts containing the entire machine status, repeated every about 42msec.
//  Serial type: 2400-N-8-1
/*
 1° byte: FFh  = Start string
          The FFh value identifies the beginning of the string.

 2° byte: XXh  = Right score
          Ex.: if =06h , the right score is 6

 3° byte: XXh  = Left score
          Ex.: if =12h, the left score is 12

 4° byte: XXh  = Seconds of the time (units and tens)
          Ex.: if =56h , the seconds of the time = 56.

 5° byte: 0Xh  = Minutes of the time (only units)
          Ex.: if =02h , the minutes of the time = 2.

 6° byte: XXh  = Define the state of the lamps (red, green, whites and
yellows).  Every bit defines the state of a lamp (zero=OFF, 1=ON).
 Following is the correspondence of the 8 bits:
          Bit D0 = Left white lamp
          Bit D1 = Right white lamp
          Bit D2 = RED lamp (left)
          Bit D3 = GREEN lamp (right)
          Bit D4 = Right yellow lamp
          Bit D5 = Left yellow lamp
          Bit D6 = 0   not used
          Bit D7 = 0   not used
       Example: if byte 6° = 14h , we have D2=1 (red light=on) and D4=1
(right yellow light=on)

 7° byte: 0Xh  = Number of matches and Priorite signals.
       The D0 e D1 bits define the number of matches (from 0 to 3):
          D1=0 D0=0  Num.Matchs = 0
          D1=0 D0=1  Num.Matchs = 1
          D1=1 D0=0  Num.Matchs = 2
          D1=1 D0=1  Num.Matchs = 3
       The D2 e D3 bits define the signals of Priorite:
          D2 = Right Priorite (if=1 is ON)
          D3 = Left Priorite (if=1 is ON)
     Example: if byte 7° =0Ah (D0=0, D1=1, D2=0 D3=1) , the number of
Matchs is =2 and the Left Priorite lamp is ON.

 8° byte: XXh  This byte is only for our use. Do not consider this byte.
Its value is always different from FFh.
The bit D0 of the Byte8 defines the running of the chrono:
D0=0 = Stop
D0=1 = Running.

 9° byte:  Red and Yellow penalty cards.
       The 4 bits D0, D1, D2, e D3 are used on the following way:
          D0 = Right RED penalty card
          D1 = Left RED penalty card
          D2 = Right YELLOW penalty card
          D3 = Left YELLOW penalty card
       Do not consider the bit D4 and D5 which can be at zero or 1, instead
the bit D6 and D7 are always =0.
      Example: if byte 8° = 38h , we have D3=1 and so the left yellow
penalty card is ON.

 10° byte:  CRC , it is the sum without carry of the 9 previous bytes.

 As example, the string could be:
 FFh, 06h, 12h, 56h, 02h, 14h, 0Ah, 00h, 38h, 56h
 which will display:
 Right score = 6
 Left score = 12
 Time = 2:56
 The Lamps ON are: Red, Yellow right, Left priorite.
 Number of Matchs = 2
 Left yellow penalty lamp = ON.

 */

FaveroPacket::FaveroPacket()
{
    //ctor
    std::lock_guard<std::mutex> guard(packet_mutex);
    m_Buffer[0]=0xFF;
    m_Buffer[1]=0x00;
    m_Buffer[2]=0x00;
    m_Buffer[3]=0x00;;
    m_Buffer[4]=0x00;
    m_Buffer[5]=0x00;
    m_Buffer[6]=0x00;;
    m_Buffer[7]=0x00;
    m_Buffer[8]=0x00;
    CalculateCRC();

}

FaveroPacket::~FaveroPacket()
{
    //dtor
}

FaveroPacket::FaveroPacket(const FaveroPacket& other)
{
    //copy ctor
    std::lock_guard<std::mutex> guard(packet_mutex);
    for(int i=0; i<10; i++)
    {
         m_Buffer[i] = other.m_Buffer[i];
    }
}

void FaveroPacket::CalculateCRC()
{
    //std::lock_guard<std::mutex> guard(packet_mutex);
    uint8_t myCRC = 0;
    for(int i=0; i<9; i++)
    {
        myCRC += m_Buffer[i];
    }
    m_Buffer[9] = myCRC;
}

void FaveroPacket::SetScoreLeft(uint8_t value)
{
    std::lock_guard<std::mutex> guard(packet_mutex);
    uint8_t upper = (value / 10)*16;
    uint8_t lower = value - (value / 10)*10;

    m_Buffer[2] = upper + lower;
    CalculateCRC();
}

void FaveroPacket::SetScoreRight(uint8_t value)
{
    std::lock_guard<std::mutex> guard(packet_mutex);
    uint8_t upper = (value / 10)*16;
    uint8_t lower = value - (value / 10)*10;
    m_Buffer[1] = upper + lower;
    CalculateCRC();
}

void FaveroPacket::SetTimer(uint8_t minutes, uint8_t seconds)
{
    std::lock_guard<std::mutex> guard(packet_mutex);
    uint8_t upper = (minutes / 10)*16;
    uint8_t lower = minutes - (minutes / 10)*10;
    m_Buffer[4] = upper + lower;
    upper = (seconds / 10)*16;
    lower = seconds - (seconds / 10)*10;
    m_Buffer[3] = upper + lower;

    CalculateCRC();
}

void FaveroPacket::SetPriority(uint8_t value)
{
    std::lock_guard<std::mutex> guard(packet_mutex);
    //0 -> No Prio; 1 -> PrioLeft; 2 -> PrioRight
    // Step1 clear the Prio bits
    m_Buffer[6] &= ~(1 << 2);
    m_Buffer[6] &= ~(1 << 3);
    if(1 == value)
        m_Buffer[6] |= 0x01 << 3;
    if(2 == value)
        m_Buffer[6] |= 0x01 << 2;

    CalculateCRC();
}
void FaveroPacket::SetMatchCount(uint8_t value)
{
    std::lock_guard<std::mutex> guard(packet_mutex);
    //0 -> No Prio; 1 -> PrioLeft; 2 -> PrioRight
    // Step1 clear the Prio bits
    uint8_t temp = value & 0x03;
    m_Buffer[6] &= ~(0x03);
    m_Buffer[6] |= temp;

    CalculateCRC();
}

void FaveroPacket::SetChronoRunning(bool value)
{
    std::lock_guard<std::mutex> guard(packet_mutex);
    m_Buffer[7] = 0;
    if(value)
         m_Buffer[7] = 1;
    CalculateCRC();
}

void FaveroPacket::SetCards(uint8_t value)
{
    std::lock_guard<std::mutex> guard(packet_mutex);
    m_Buffer[8] = value + 0x30;
    CalculateCRC();
}

void FaveroPacket::SetLights(bool Red, bool WhiteLeft, bool WhiteRight, bool Green, bool OrangeLeft, bool OrangeRight)
{
    std::lock_guard<std::mutex> guard(packet_mutex);
    uint8_t value = 0;
    if(Red)
        value |= 0x01 << 2;
    if(WhiteLeft)
        value |= 0x01 << 0;
    if(WhiteRight)
        value |= 0x01 << 1;
    if(Green)
        value |= 0x01 << 3;
    if(OrangeLeft)
        value |= 0x01 << 5;
    if(OrangeRight)
        value |= 0x01 << 4;

    m_Buffer[5] = value;
    CalculateCRC();
}

void FaveroPacket::Print()
{
    std::lock_guard<std::mutex> guard(packet_mutex);
    for(int i=0; i<10; i++)
    {
        std::cout << std::dec << i+1 << "° byte: 0x" << std::hex  << (int) m_Buffer[i] << std::endl;
    }
}

void FaveroPacket::CopyToBuffer(uint8_t * Destination)
{
    std::lock_guard<std::mutex> guard(packet_mutex);
    memcpy((void *)Destination,(void *)m_Buffer,10);
}
