#include "mainHeader.h"

/********************************************************************************
TODO
- Regelung aktivieren/ deaktivieren noch als Kommandos hinzufügen
- Dosierpumpe mit Zeit ansteuern
- Dosierpumpe maximale Laufzeit reinprogrammieren




- Kalibrierung EC Sensor
  - variabler Wert vorgeben / Schwellen entsprechend anpassen
- LDR Mittelwert noch aktivieren, bis jetzt noch anderer Modus aktiv
- nachfüllmodus checken (Sperre Dosierpumpe)
- beeinflussen sich ph und ec sensor? 
- Was wenn Messwerte Quatsch sind? (z.B. ph Wert für PH Down)
- Düngen Alarm 
- Überschriebene Sensorwerte löschen (Wassertemperatur (auch bei EC Messung), PH Wert, EC Wert)
- Detektieren wenn lange kein Signal mehr eintrifft (-> Notlauf??)
- AlleAktor - Funktionen --> Feld oder Referenz auf Feld übergeben?
- neue Kommandos: Pumpen nur für bestimmte Zeit an  

- defines als globale variablen ändern (z.b. Schwellen)

- von Node Red zu sendende Werte
  - Regelungsintervall
  - LDRSCHWELLE
  - DauerLuftpumpeAn
  - evtl Faktor wenn ph Downer eine andere Konzentration hat?
  
Erledigt
- Ultraschallsensor abstand setzen wenn Kiste leer 
- Notfallmodus einrichten
- Kalibrierung für EC Sensor integrieren (versenden von 3 Strings, in der Zeit nichts anderes)
  - evtl Probleme, da der Sensor auf Serielle Kommandos wartet & tlw auch schreibt 
- Mittelwert bei LDR Schwelle berechnen
- PH Schwellen festlegen --> Kann Dani mir auch Werte schicken?  
- Regelungsaufruf Intervall --> 30min 
- 1h warten nachdem die Dosierpumpe an war, ph reagiert nur langsam
- PH Schwellen noch passend zuordnen
- für Wartung, Regelparameter (z.B. aktueller Mittelwertbuffer ausgeben)
  - getPhAnzahlBufferElemente //gibt die Anzahl der im Buffer gespeicherten werte zurück (0-9)
  - getPhBufferElementNr:9 //gibt Elementwert*100 an stelle 9 zurück
  - getPhBufferAvg //gibt Mittelwert des Buffers zurück *100

********************************************************************************/

//Notlauf
bool Notlauf_flag = false; //Zentral oder über Befehl ansteuern?

//Regelung PH Down
bool Nachfuellen = 0; //wenn 1 dann muss die Regelung gesperrt werden
unsigned long lastTimeRegelung = 0;
unsigned long lastTimeAktorUpdate = 0;
int RegelungZeitSperre = 0;
float PH_UNTERESCHWELLE = 4.5;
float PH_OBERESCHWELLE = 7.5;
bool Regelungssperre = 0;
//Wasserspiegel
int Wasserkiste_Hoehe = 35; //in cm //da wo der Ultraschallsensor sitzt

//Licht
bool Lichtsperre = false;
unsigned long lastTimeLichtUpdate = 0;
//Abfrage Dosierpumpe
unsigned long LetzteAbfrageDosierpumpe = 0;

//serielle Kommunikation mit RPi
byte command;
const byte numChars = 32;
char receivedChars[numChars];
boolean newData = false;

//DHT nur alle 2 Sek lesen
unsigned long lastReadTemp = 0;
unsigned long lastReadHum = 0;
float lastDhtTemp = 0;
float lastDhtHum = 0;

//Klassenobjekte
CO2Sensor co2sensor(PIN_MG811, 0.99, 3);
DHT dht(PIN_DHT21, DHT21);

OneWire oneWire(PIN_DS18B20);
DallasTemperature OneWireSensors(&oneWire);

DFRobot_EC10 ec;

Aktor Dosierpumpe(PIN_DOSIERPUMPE);
Aktor Wasserpumpe(PIN_WASSERPUMPE);
Aktor Luftpumpe(PIN_LUFTPUMPE);

void setup()
{
  Serial.begin(9600);
  init_Relais_Pins();
  dht.begin();
  co2sensor.calibrate();
  OneWireSensors.begin();
  ec.begin();
  init_Ultraschall();
}

void loop()
{
  //Serielle Kommunikation
  recvWithStartEndMarkers();
  if (newData)
    bearbeiteStringBefehl();

  //Funktionsaufruf Regelung, z.B. alle 30min
  if (millis() - lastTimeRegelung >= (REGELUNGINTERVALL * 1000) && !Regelungssperre)
  {
    if (RegelungZeitSperre > 0)
      RegelungZeitSperre--;
    else
    {
      if (Regelung_PH(Dosierpumpe, Luftpumpe, Wasserpumpe.getStatus())) //gibt true zurück, wenn Dosierung ausgeführt wurde
        RegelungZeitSperre = 2;
    }
    lastTimeRegelung = millis();
  }

  //updateAktoren alle 200ms
  if (millis() - lastTimeAktorUpdate >= 200)
  {
    Wasserpumpe.UpdateAktor();
    Luftpumpe.UpdateAktor();
    Dosierpumpe.UpdateAktor();

    lastTimeAktorUpdate = millis();
  }

  //update Licht alle 10sek
  if (millis() - lastTimeLichtUpdate >= 1000)
  {
    //Serial.println("Lichtcheck");
    if (UmgebungslichtZuDunkel() && !Lichtsperre)
      alleLichter(true);
    else
      alleLichter(false);

    lastTimeLichtUpdate = millis();
  }
}

void recvWithStartEndMarkers()
{
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;

  while (Serial.available() > 0 && newData == false)
  {
    rc = Serial.read();

    if (recvInProgress == true)
    {
      if (rc != endMarker)
      {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars)
        {
          ndx = numChars - 1;
        }
      }
      else
      {
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }
    else if (rc == startMarker)
    {
      recvInProgress = true;
    }
  }
}

void bearbeiteStringBefehl()
{
  //Serial.print(receivedChars);
  //Serial.print(": ");
  if (strcmp(receivedChars, "Lufttemperatur") == 0)
  {
    if (millis() - lastReadTemp > 2000U) // nur neuer Wert auslesen wenn letztes lesen >2s
    {
      lastDhtTemp = dht.readTemperature();
      lastReadTemp = millis();
    }
    SendSerialInteger(lastDhtTemp * 10);
    //Serial.println(lastDhtTemp * 10);
  }
  else if (strcmp(receivedChars, "Luftfeuchtigkeit") == 0)
  {
    if (millis() - lastReadHum > 2000U) // nur neuer Wert auslesen wenn letztes lesen >2s
    {
      lastDhtHum = dht.readHumidity();
      lastReadHum = millis();
    }

    SendSerialInteger(lastDhtHum * 10);
    //Serial.println(lastDhtHum * 10);
  }
  else if (strcmp(receivedChars, "Wassertemperatur") == 0)
  {
    OneWireSensors.requestTemperatures();
    float temp = OneWireSensors.getTempCByIndex(0);
    // temp = 30.2;
    SendSerialInteger(temp * 100);
    //Serial.println(temp*100);
  }
  else if (strcmp(receivedChars, "CO2Wert") == 0)
  {
    SendSerialInteger(co2sensor.read());
    //Serial.println(co2sensor.read());
  }
  else if (strcmp(receivedChars, "ECWert") == 0)
  {
    OneWireSensors.requestTemperatures();
    float Temp_Wasser = OneWireSensors.getTempCByIndex(0);
    if (Temp_Wasser < 15 || Temp_Wasser > 25)
      Temp_Wasser = 20; //wenn Sensor fehlerhafte Werte ausgibt

    float voltage = analogRead(PIN_EC) / 1024.0 * 5000;
    float ecValue = ec.calcEC(voltage, Temp_Wasser);

    //Werte werden aktuell überschrieben

    // ecValue = 0.3;

    SendSerialInteger(ecValue * 100); //in ms/cm
  }
  else if (strcmp(receivedChars, "KalibrierungEC") == 0)
  {
    OneWireSensors.requestTemperatures();
    float Temp_Wasser = OneWireSensors.getTempCByIndex(0);
    float voltage = analogRead(PIN_EC) / 1024.0 * 5000;
    Temp_Wasser = 20;
    if (Temp_Wasser < 15 || Temp_Wasser > 25)
      Temp_Wasser = 20;                                            //wenn Sensor fehlerhafte Werte ausgibt
    SendSerialInteger(ec.SerialCalibration(voltage, Temp_Wasser)); //Umwandlung des Fehlerbytes in Integer??
  }
  else if (strcmp(receivedChars, "getKValueEC") == 0)
  {
    SendSerialInteger(ec.getKValue() * 100);
  }
  else if (strcmp(receivedChars, "PHWert") == 0)
  {
    float phWert = getPHWert();
    //phWert = 6.78;
    SendSerialInteger(phWert * 100);
    //Serial.println(phWert * 100);
  }
  // else if (strcmp(receivedChars, "HelligkeitSchwelle") == 0)
  // {
  //   SendSerialInteger(UmgebungslichtZuDunkel());
  //   //Serial.println(800);
  // }
  else if (strcmp(receivedChars, "Wasserspiegel") == 0)
  {
    float value = Kiste_Liter() * 100;
    SendSerialInteger(value);
    //Serial.println(value);
  }
  else if (strcmp(receivedChars, "LichtAn") == 0)
  {
    Lichtsperre = false;
    SendSerialInteger(0x0101);
  }
  else if (strcmp(receivedChars, "LichtAus") == 0)
  {
    Lichtsperre = true;
    SendSerialInteger(0x0F0F);
  }
  else if (strcmp(receivedChars, "WasserpumpeAn") == 0)
  {
    Wasserpumpe.SchalteEin();
    SendSerialInteger(0x0101);
  }
  else if (strcmp(receivedChars, "WasserpumpeAus") == 0)
  {
    Wasserpumpe.SchalteAus();
    SendSerialInteger(0x0F0F);
  }
  else if (strcmp(receivedChars, "LuftpumpeAn") == 0)
  {
    Luftpumpe.SchalteEin();
    SendSerialInteger(0x0101);
  }
  else if (strcmp(receivedChars, "LuftpumpeAus") == 0)
  {
    Luftpumpe.SchalteAus();
    SendSerialInteger(0x0F0F);
  }
  else if (strcmp(receivedChars, "DosierpumpeAn") == 0)
  {
    Dosierpumpe.SchalteEin();
    SendSerialInteger(0x0101);
  }
  else if (strcmp(receivedChars, "DosierpumpeAus") == 0)
  {
    Dosierpumpe.SchalteAus();
    SendSerialInteger(0x0F0F);
  }
  else if (strcmp(receivedChars, "NachfuellModusAktiv") == 0)
  {
    Nachfuellen = true;
    Dosierpumpe.setSperre(true);
    SendSerialInteger(0x0101);
  }
  else if (strcmp(receivedChars, "RegelungsperreAktiv") == 0)
  {
    Regelungssperre = true;
    SendSerialInteger(0x0101);
  }
  else if (strcmp(receivedChars, "RegelungsperreInaktiv") == 0)
  {
    Regelungssperre = false;
    SendSerialInteger(0x0F0F);
  }
  else if (strcmp(receivedChars, "NachfuellModusInaktiv") == 0)
  {
    Nachfuellen = false;
    Dosierpumpe.setSperre(false);
    SendSerialInteger(0x0F0F);
  }
  else if (strcmp(receivedChars, "DosierLaufzeitinS") == 0)
  {
    SendSerialInteger(Dosierpumpe.getLaufzeitInS() * 100); //Laufzeit in S *100
  }
  else if (strcmp(receivedChars, "gefoerderteMengeDosier") == 0)
  {
    float LaufzeitinS = Dosierpumpe.getLaufzeitInS();
    float gefoerderteMengeInMl = LaufzeitinS * FAKTORMLPROSEK;
    SendSerialInteger(gefoerderteMengeInMl * 100); //gefoerderte Menge in ml  *100
  }
  else if (strcmp(receivedChars, "DosierlaufzeitReset") == 0)
  {
    Dosierpumpe.resetLaufzeit();
    SendSerialInteger(Dosierpumpe.getLaufzeitInS() * 100);
  }
  else if (strcmp(receivedChars, "ResetWasserspiegel") == 0) //Wenn Kiste leer
  {
    Wasserkiste_Hoehe = distance(PIN_TRIG, PIN_ECHO);
    SendSerialInteger(Kiste_Liter());
  }
  else if (strcmp(receivedChars, "NotlaufAktiv") == 0)
  {
    Notlauf_flag = true;
    Lichtsperre = true;
    Wasserpumpe.setSperre(true);
    Luftpumpe.setSperre(true);
    Dosierpumpe.setSperre(true);
    SendSerialInteger(0x0101);
  }
  else if (strcmp(receivedChars, "NotlaufInaktiv") == 0)
  {
    Notlauf_flag = false;
    Lichtsperre = false;
    Wasserpumpe.setSperre(false);
    Luftpumpe.setSperre(false);
    Dosierpumpe.setSperre(false);
    SendSerialInteger(0x0F0F);
  }
  else if (strcmp(receivedChars, "RegelungSperreAktiv") == 0)
  {
    Dosierpumpe.setSperre(false);
    SendSerialInteger(0x0F0F);
  }
  else if (strcmp(receivedChars, "RegelungSperreInaktiv") == 0)
  {
    Dosierpumpe.setSperre(false);
    SendSerialInteger(0x0F0F);
  }
  else if (strcmp(receivedChars, "getPhBufferAvg") == 0)
  {
    float avg = getMittelwertBuffer();
    SendSerialInteger(avg * 100);
  }
  else if (strcmp(receivedChars, "getPhAnzahlBufferElemente") == 0)
  {
    SendSerialInteger(getBufferAnzahlElemente());
  }
  else if (strstr(receivedChars, "getPhBufferElementNr:") != 0)
  {
    int zehner = receivedChars[21] - '0';
    int einer = receivedChars[22] - '0';
    int ElementPos = zehner*10 + einer;
    if (ElementPos<0 || ElementPos>10)
      ElementPos = 0;
    float Element = getBufferElementPos(ElementPos);
    SendSerialInteger(Element * 100);
  }
  else if (strstr(receivedChars, "PhNeueObereSchwelle:") != 0)
  {
    int zehner = receivedChars[20] - '0';
    int einer = receivedChars[21] - '0';
    float neueSchwelle = ((float)(zehner * 10 + einer)) / 10;
    PH_OBERESCHWELLE = neueSchwelle;
    SendSerialInteger(neueSchwelle * 10);
  }
  else if (strstr(receivedChars, "PhNeueUntereSchwelle:") != 0)
  {
    int zehner = receivedChars[21] - '0';
    int einer = receivedChars[22] - '0';
    float neueSchwelle = ((float)(zehner * 10 + einer)) / 10;
    PH_UNTERESCHWELLE = neueSchwelle;
    SendSerialInteger(neueSchwelle * 10);
  }
  else
  {
    SendSerialInteger(0xFFFF);
  }
  newData = false;
  //Serial.println();
}
