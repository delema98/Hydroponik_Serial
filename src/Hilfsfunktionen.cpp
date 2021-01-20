#include "mainHeader.h"

void SendSerialInteger(int wert)
{
  byte DataToSend[2];
  DataToSend[0] = highByte(wert); //0x5E
  DataToSend[1] = lowByte(wert);  //0x01

  Serial.write(DataToSend, 2);
}

//initialieren aller Relais Pins

void init_Relais_Pins()
{
  pinMode(PIN_LUFTPUMPE, OUTPUT);
  pinMode(PIN_WASSERPUMPE, OUTPUT);
  pinMode(PIN_DOSIERPUMPE, OUTPUT);
  pinMode(PIN_LEDHINTEN, OUTPUT);
  pinMode(PIN_LEDVORNE, OUTPUT);

  digitalWrite(PIN_LUFTPUMPE, 0);
  digitalWrite(PIN_WASSERPUMPE, 0);
  digitalWrite(PIN_DOSIERPUMPE, 0);
  digitalWrite(PIN_LEDVORNE, 0);
  digitalWrite(PIN_LEDHINTEN, 0);
}

void init_Ultraschall()
{
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
}

void init_LdrSchalter()
{
  pinMode(PIN_LDR_SCHALTER, INPUT);
}

void alleLichter(bool status)
{
  digitalWrite(PIN_LEDVORNE, status);
  digitalWrite(PIN_LEDHINTEN, status);
}

bool getLDRStatusHell()// 0: Dunkel 1: Hell
{
  return digitalRead(PIN_LDR_SCHALTER);
}
bool UmgebungslichtZuDunkel()
{
  return !getLDRStatusHell();
}
float avgbufLDR[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //Buffer bekommt alle 2min einen neuen Wert
int anzahlAuslesenLDR = 0;

bool UmgebungslichtZuDunkelMittelwert() 
{
  BufferFuellenLDR(getLDRStatusHell()); // 0: Dunkel 1: Hell

  if (getMittelwertBufferLDR() < LDRSCHWELLE)
    return true;
  else
    return false;
}
void BufferFuellenLDR(bool akt_LDR_Wert)
{
  if (anzahlAuslesenLDR < 10)
    avgbufLDR[anzahlAuslesenLDR] = akt_LDR_Wert;
  else //es wurden schon mehr als 10 Werte gelesen, also alter Wert rausschmeißen
  {
    for (int i = 0; i < 9; i++)
      avgbufLDR[i] = avgbufLDR[i + 1];

    avgbufLDR[9] = akt_LDR_Wert;
  }
  anzahlAuslesenLDR++;
}
float getMittelwertBufferLDR()
{
  float MittelwertBuffer = 0;
  for (int i = 0; i < 10; i++)
  {
    MittelwertBuffer += avgbufLDR[i];
  }
  return MittelwertBuffer / 10;
}