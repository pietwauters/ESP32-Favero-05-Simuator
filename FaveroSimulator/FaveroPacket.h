#ifndef FAVEROPACKET_H
#define FAVEROPACKET_H
#include  <cstdint>
#include <iostream>
#include <mutex>

#define RIGHT_RED_CARD  0x01 << 0
#define LEFT_RED_CARD  0x01 << 1
#define RIGHT_YELLOW_CARD  0x01 << 2
#define LEFT_YELLOW_CARD  0x01 << 3

#define LEFT_WHITE  0x01 << 0
#define RIGHT_WHITE  0x01 << 1
#define RED  0x01 << 2
#define GREEN  0x01 << 3
#define RIGHT_YELLOW  0x01 << 4
#define LEFT_YELLOW  0x01 << 5




class FaveroPacket
{
    public:
        /** Default constructor */
        FaveroPacket();
        /** Default destructor */
        virtual ~FaveroPacket();
        /** Copy constructor
         *  \param other Object to copy from
         */
        FaveroPacket(const FaveroPacket& other);
        void SetScoreLeft(uint8_t value);
        void SetScoreRight(uint8_t value);
        void SetLights(bool Red, bool WhiteLeft, bool WhiteRight, bool Green, bool OrangeLeft, bool OrangeRight);
        void SetLights(uint8_t value){std::lock_guard<std::mutex> guard(packet_mutex);m_Buffer[5] = value; CalculateCRC();};
        void SetPriority(uint8_t value);
        void SetTimer(uint8_t minutes, uint8_t seconds);
        void SetMatchCount(uint8_t value);
        void SetCards(uint8_t value);
        void SetChronoRunning(bool value);
        void CopyToBuffer(uint8_t * Destination);
        void Print();



    protected:

    private:
        void CalculateCRC();
        uint8_t m_Buffer[10]; //!< Member variable "m_Buffer[10];"
        std::mutex packet_mutex;
};

#endif // FAVEROPACKET_H
