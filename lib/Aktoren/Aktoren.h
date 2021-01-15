#include <Arduino.h>

class Aktor
{
public:
    Aktor();
    Aktor(int Pin);
    bool getStatus();
    void setSperre(bool wert);
    bool getSperre();
    void SetPin(int Pin);
    void SchalteEin();
    void SchalteAus();
    void SchalteFuerDauerEin(unsigned long DauerinMs);
    void UpdateAktor(); //muss regelmäßig aufgerufen werden, damit aktor nach bestimmter Zeit ausgeschaltet wird
    void resetLaufzeit();
    float getLaufzeitInS();

private:
    bool _Status;
    bool _flag_DauerModusaktiv;
    bool _Sperre;
    int _Pin;
    unsigned long _StartZeit;
    unsigned long _StoppZeit;
    float _LaufzeitinS;
};
