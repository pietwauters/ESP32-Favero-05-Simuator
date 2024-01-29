#include "FaveroPacket.h"
#include "BluetoothSerial.h"
#include <thread>
#include <mutex>
#include <chrono>
#include <random>
#include "esp_task_wdt.h"
using namespace std;

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
FaveroPacket * CurrentPacket = NULL;
bool bStopCommThread = false;
bool StopChronoThread = false;
bool ChronoRunning = false;
int ChronoSpeed = 25;
uint8_t ScoreLeft = 0;
uint8_t ScoreRight = 0;
volatile bool ContinueSimulation = true;

TaskHandle_t commPortTask;
TaskHandle_t chronoTask;
TaskHandle_t simulateTask;


void commportthread(void *parameter)
{
uint8_t transmitBuffer[10];

    std::random_device seeder;
    std::mt19937 rng(seeder());
    std::uniform_int_distribution<int> gen(0, 1000); // uniform, unbiased
    std::uniform_int_distribution<int> gen1(0, 9); // uniform, unbiased
    std::uniform_int_distribution<int> gen2(0, 255); // uniform, unbiased

    int i = 0;
    while(!bStopCommThread)
    {
        CurrentPacket->CopyToBuffer(transmitBuffer);
        if(gen(rng)<5)
        {
            // generate error
            transmitBuffer[gen1(rng)] = gen2(rng);

        }
        while(!Serial.availableForWrite())
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
          esp_task_wdt_reset();
        }
        Serial.write(transmitBuffer, 10);
        SerialBT.write(transmitBuffer, 10);
        //CurrentPacket->Print();
        //cout << i++ << endl;
        
        esp_task_wdt_reset();
    }
}


void ChronoThread(void *parameter)
{
    uint8_t minutes = 3;
    uint8_t seconds = 0;
    uint8_t matchCount = 1;
    bool OneMinutePause = false;
    CurrentPacket->SetMatchCount(matchCount);

    while(!StopChronoThread)
    {
        minutes = 3;
        seconds = 0;
        esp_task_wdt_reset();
        for(int i=0; i< 180; i++)
        {
            //cout << "Chronothread: " <<  dec << (int) minutes << ":" << (int) seconds << endl;
            esp_task_wdt_reset();
            for(int i = 0; i < ChronoSpeed; i++)
            {
                if(!ChronoRunning)
                {
                    CurrentPacket->SetChronoRunning(false);
                    while(!ChronoRunning)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        esp_task_wdt_reset();
                        if(StopChronoThread)
                            return;
                    }
                    CurrentPacket->SetChronoRunning(true);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                esp_task_wdt_reset();
                if(StopChronoThread)
                    return;
            }

            if(seconds == 0)
            {
                seconds = 59;
                if(minutes)
                    minutes--;
                else
                {
                    minutes = 1;

                }
            }
            else
                seconds--;
            CurrentPacket->SetTimer(minutes,seconds);
          
        }
        minutes = 1;
        seconds = 0;
        CurrentPacket->SetTimer(minutes,seconds);
        seconds = 60;
        for(int i=0; i< 60; i++)
        {
            esp_task_wdt_reset();
            for(int i = 0; i < ChronoSpeed; i++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                esp_task_wdt_reset();
                if(StopChronoThread)
                    return;
            }
            seconds--;
            minutes = 0;
            CurrentPacket->SetTimer(minutes,seconds);
        }
        matchCount++;
        CurrentPacket->SetMatchCount(matchCount);

    }
}


void DoRandomLights(FaveroPacket * CurrentPacket)
{
    static std::random_device seeder;
     std::mt19937 rng(seeder());
     std::uniform_int_distribution<int> gen(0, 1); // uniform, unbiased
     std::uniform_int_distribution<int> gen2(0, 320); // uniform, unbiased

    bool red = false;
    bool green = false;
    bool wl = false;
    bool wr = false;


    int time_between = gen2(rng);
    ChronoRunning = false;
    CurrentPacket->SetChronoRunning(false);
    
    if(gen(rng))
    {
        red = true;
        ScoreLeft ++;
        if(ScoreLeft > 15)
            ScoreLeft = 0;
    }
    else
        wl = gen(rng);

    if(gen(rng))
    {
        green = true;
        ScoreRight ++;
        if(ScoreRight > 15)
            ScoreRight = 0;
    }
    else
        wr = gen(rng);

    if(gen(rng))
    {
        CurrentPacket->SetLights(red, wl, false, false, false,false);
        std::this_thread::sleep_for(std::chrono::milliseconds(time_between));
        
        CurrentPacket->SetLights(red, wl, wr, green, false,false);
    }
    else
    {
        CurrentPacket->SetLights(false, false, wr, green, false,false);
        std::this_thread::sleep_for(std::chrono::milliseconds(time_between));
        CurrentPacket->SetLights(red, wl, wr, green, false,false);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 - time_between));
    CurrentPacket->SetScoreLeft(ScoreLeft);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    CurrentPacket->SetLights(false, false, false, false, false,false);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    CurrentPacket->SetScoreRight(ScoreRight);

}

void DoBrokenWire(FaveroPacket * CurrentPacket)
{
    ChronoRunning = false;
    CurrentPacket->SetChronoRunning(false);
    for(int j = 0; j< 5; j++)
    {
        CurrentPacket->SetLights(false, true, true, false, false,false);
        for(int i =0; i< 1500; i+= 10)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            esp_task_wdt_reset();
            if(!ContinueSimulation)
                return;
        }
        CurrentPacket->SetLights(false, false, false, false, false,false);
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }

}



void DoSimulate(void *parameter)
{
    int NrYCardsLeft = 0;
    int NrRCardsLeft = 0;
    int NrYCardsRight = 0;
    int NrRCardsRight = 0;
    int Prio = 0;
    uint8_t cards = 0;
    std::random_device seeder;
    std::mt19937 rng(seeder());
    std::uniform_int_distribution<int> gen(1000, 5000); // uniform, unbiased
    std::uniform_int_distribution<int> genchronorestart(0, 5000); // uniform, unbiased

    int r = gen(rng);
    while(ContinueSimulation)
    {
        r = gen(rng);
        esp_task_wdt_reset();
        for(int i =0; i< r; i+= 10)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            esp_task_wdt_reset();
            if(!ContinueSimulation)
                return;
        }


        DoRandomLights(CurrentPacket);
        r = gen(rng);

        for(int i =0; i< r; i+= 10)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            esp_task_wdt_reset();
            if(!ContinueSimulation)
                return;
        }
        if(genchronorestart(rng) >1000)
            ChronoRunning = true;
        if(genchronorestart(rng) >4700)
            DoBrokenWire(CurrentPacket);

        //Play around with cards and prio
        cards = 0;

        if(genchronorestart(rng) >4100)
        {
            if(NrYCardsLeft)
                NrYCardsLeft = 0;
            else
            {
                NrYCardsLeft = 1;
                cards += LEFT_YELLOW_CARD;
            }

        }
        if(genchronorestart(rng) >4100)
        {
            if(NrYCardsRight)
                NrYCardsRight = 0;
            else
            {
                NrYCardsRight = 1;
                cards += RIGHT_YELLOW_CARD;
            }

        }
        if(genchronorestart(rng) >4900)
        {
            if(NrRCardsLeft)
                NrRCardsLeft = 0;
            else
            {
                NrRCardsLeft = 1;
                cards += LEFT_RED_CARD;
            }

        }
        if(genchronorestart(rng) >4900)
        {
            if(NrRCardsRight)
                NrRCardsRight = 0;
            else
            {
                NrRCardsRight = 1;
                cards += RIGHT_RED_CARD;
            }

        }

        CurrentPacket->SetCards(cards);
    }
}





void setup() {
  CurrentPacket = new FaveroPacket();
  Serial.begin(2400);
  SerialBT.begin("FaveroSimulatorESP32",true); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
  CurrentPacket->SetScoreLeft(12);
  CurrentPacket->SetScoreRight(6);
  //packet.SetLights(true,false,false,false,false,true);
  CurrentPacket->SetLights(RED | RIGHT_YELLOW);
  CurrentPacket->SetPriority(1);
  CurrentPacket->SetTimer(2,56);
  CurrentPacket->SetMatchCount(2);
  CurrentPacket->SetCards(LEFT_YELLOW_CARD);
  CurrentPacket->Print();
  esp_task_wdt_init(10, false);

TaskHandle_t chronoTask;
TaskHandle_t simulateTask;

  xTaskCreatePinnedToCore(
            commportthread,        /* Task function. */
            "commportthread",      /* String with name of task. */
            10240,                            /* Stack size in words. */
            NULL,                             /* Parameter passed as input of the task */
            0,                                /* Priority of the task. */
            &commPortTask         ,           /* Task handle. */
            0);
  //esp_task_wdt_add(commPortTask);
  

  xTaskCreatePinnedToCore(
            ChronoThread,        /* Task function. */
            "ChronoThread",      /* String with name of task. */
            16384,                            /* Stack size in words. 65535*/
            NULL,                             /* Parameter passed as input of the task */
            0,                                /* Priority of the task. */
            &chronoTask,           /* Task handle. */
            1);
  //esp_task_wdt_add(chronoTask);

  xTaskCreatePinnedToCore(
            DoSimulate,        /* Task function. */
            "DoSimulate",      /* String with name of task. */
            20240,                            /* Stack size in words. */
            NULL,                            /* Parameter passed as input of the task */
            0,                                /* Priority of the task. */
            &simulateTask,           /* Task handle. */
            0);
  //esp_task_wdt_add(simulateTask);

  ChronoRunning = true;
    cout << "Favero Simulator:" << endl;
    cout << "Chrono runs faster than in reality by a factor of " << 100 / ChronoSpeed << "x" << endl;
    
}

void loop() {
  
  delay(20);
  esp_task_wdt_reset();

}