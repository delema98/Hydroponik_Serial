#include <Arduino.h>
#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DFRobot_EC10.h>
#include <CO2Sensor.h>
#include <Aktoren.h>

//Sensor Pins
#define PIN_MG811 A0
#define PIN_LDR A1
#define PIN_PH A2 //grün
#define PIN_EC A3 //braun
#define PIN_TRIG 4 //gelb
#define PIN_ECHO 5 //grün
#define PIN_DS18B20 2
#define PIN_DHT21 12
#define PIN_LDR_SCHALTER 6

//Aktor Pins
#define PIN_WASSERPUMPE 7 //K1
#define PIN_LUFTPUMPE 8 //K2
#define PIN_DOSIERPUMPE 9 //K3
#define PIN_LEDVORNE 10 //K4
#define PIN_LEDHINTEN 11 //K5

//Helligkeitsschwelle
#define LDRSCHWELLE 0.7 //1: Dunkel  0: Hell

//Dimensionen Wasserkiste

#define WASSERKISTE_BREITE 32.8 //in cm
#define WASSERKISTE_LAENGE 37.7 //in cm

//PH Regelung

#define REGELUNGINTERVALL 30*60 //ca 30min   //in Sekunden, wie oft PH Wert eingelesen werden soll
#define ML_PHDOWN_PRO_LITER_WASSER 0.2  
#define FAKTORMLPROSEK 1.11
#define LiterInSystemWennWasserpumpeAn 1.8
#define DauerLuftpumpeAninS 300
#define REGELUNGZEITSPERREinS 60*60; // 1 Std warten bevor erneut gemessen wird
float getBufferElementPos(int pos);

extern float PH_UNTERESCHWELLE;
extern float PH_OBERESCHWELLE;

//Serielle Kommunikation
void SendSerialInteger(int wert);
void recvWithStartEndMarkers();
void bearbeiteStringBefehl();

//Ultraschall
void init_Ultraschall();
float distance(int pinecho, int pintrigger);
float Wasserhoehe();
float Kiste_Liter();

//Relaispin
void init_Relais_Pins();

//LDR Schalter Modul
void init_LdrSchalter();
bool UmgebungslichtZuDunkel();
bool UmgebungslichtZuDunkelMittelwert();
void BufferFuellenLDR(bool akt_LDR_Wert);
float getMittelwertBufferLDR();



//PH-Sensor
float getPHWert();

//Regelung PH
bool Regelung_PH(Aktor& Dosierpumpe, Aktor& Luftpumpe, bool StatusWasserpumpe); //ist true wenn die Dosierpumpe angesteuert wurde
void BufferFuellen(float _akt_PH_Wert);
float getMittelwertBuffer();
void clearBuffer();
int getBufferAnzahlElemente();

//Ansteuerung Lichter
void alleLichter(bool status);




//Hilfsfunktionen für Aktor Klasse
void UpdateAlleAktoren(Aktor feld[], int Feldgroesse);
void AlleAktorenAus(Aktor feld[], int Feldgroesse);