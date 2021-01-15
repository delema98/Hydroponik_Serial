#include "mainHeader.h"
extern int Wasserkiste_Hoehe;

float distance(int _PinTrig, int _PinEcho)
{
    unsigned long duration;
    float distance;

    digitalWrite(_PinTrig, LOW);
    delayMicroseconds(2);

    digitalWrite(_PinTrig, HIGH);
    delayMicroseconds(10);

    duration = pulseIn(_PinEcho, HIGH);
    distance = (float)duration / 58;

    return distance;
}

float Wasserhoehe()
{
    return Wasserkiste_Hoehe - distance(PIN_TRIG, PIN_ECHO);
}

float Kiste_Liter()
{
    float vol = WASSERKISTE_BREITE * WASSERKISTE_LAENGE * Wasserhoehe(); //in cm
    return vol / 1000;
}

float getPHWert()
{
    int _PinPH = PIN_PH;
    unsigned long int avgValue; //Store the average value of the sensor feedback
    int buf[10], temp;

    for (int i = 0; i < 10; i++) //Get 10 sample value from the sensor for smooth the value
    {
        buf[i] = analogRead(_PinPH);
        delay(10);
    }
    for (int i = 0; i < 9; i++) //sort the analog from small to large
    {
        for (int j = i + 1; j < 10; j++)
        {
            if (buf[i] > buf[j])
            {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }

    avgValue = 0;
    for (int i = 2; i < 8; i++)
        avgValue += buf[i]; //take the average value of 6 center sample

    float phValue = (float)avgValue * 5.0 / 1024 / 6; //convert the analog into millivolt
    phValue = 3.5 * phValue;                          //convert the millivolt into pH value

    return phValue;
}