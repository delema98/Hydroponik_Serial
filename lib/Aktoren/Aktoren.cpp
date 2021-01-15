#include "Aktoren.h"

Aktor::Aktor(int Pin)
{
    _Pin = Pin;
}

Aktor::Aktor()
{
}

bool Aktor::getStatus()
{
    return _Status;
}
void Aktor::SetPin(int Pin)
{
    _Pin = Pin;
}

void Aktor::SchalteAus()
{
    if (_Status) //Nur ausführen wenn Aktor noch an war
    {
        
        _LaufzeitinS += (millis() - _StartZeit) / 1000;
        digitalWrite(_Pin, 0);
        _Status = false;
    }
}

void Aktor::SchalteEin()
{
    if (!_Sperre)
    {
        if (!_Status)
            _StartZeit = millis(); //Startzeit wird nur gesetzt, wenn Aktor davor aus war
        digitalWrite(_Pin, 1);
        
        _Status = true;
    }
}

void Aktor::SchalteFuerDauerEin(unsigned long DauerinMs)
{
    _StoppZeit = millis() + DauerinMs;
    
    SchalteEin();
    _flag_DauerModusaktiv = true;
}
void Aktor::UpdateAktor()
{
    if ((millis() >= _StoppZeit) && _flag_DauerModusaktiv)
      {  SchalteAus();
        _flag_DauerModusaktiv = false;
      }
    if (_Sperre)
        SchalteAus();
}

void Aktor::resetLaufzeit()
{
    _LaufzeitinS = 0;
}
float Aktor::getLaufzeitInS()
{
    return _LaufzeitinS;
}

void Aktor::setSperre(bool wert)
{
    _Sperre = wert;
}
bool Aktor::getSperre()
{
    return _Sperre;
}