#include "mainHeader.h"

/**************************************************************************************************
TODO
- Detektieren wenn lange kein Signal mehr eintrifft (-> Notlauf??)
- Dosierpumpe maximale Laufzeit reinprogrammieren
- nachfüllmodus checken (Sperre Dosierpumpe)

- beeinflussen sich ph und ec sensor? 
- Was wenn Messwerte Quatsch sind? (z.B. ph Wert für PH Down)
- defines als globale variablen ändern (z.b. Schwellen)

- EC Kalibrierschwellen aktuell noch deaktiviert
  
Erledigt
- Regelung aktivieren/ deaktivieren noch als Kommandos hinzufügen
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
- Dosierpumpe mit Zeit ansteuern
- Lichtsteuerung Mittelwert implementieren 
- LDR Mittelwert noch aktivieren, bis jetzt noch anderer Modus aktiv
- Kalibrierung EC Sensor
  - variabler Wert vorgeben
***************************************************************************************************/

//Notlauf
bool Notlauf_flag = false; //Zentral oder über Befehl ansteuern?

//Regelung PH Down
bool Nachfuellen = 0; //wenn 1 dann muss die Regelung gesperrt werden
bool Regelungssperre = 0;
int RegelungsintervallInSek = 30 * 60;
unsigned long lastTimeRegelung = 0;
unsigned long lastTimeAktorUpdate = 0;
int RegelungZeitSperre = 0;
float PH_UNTERESCHWELLE = 4.5;
float PH_OBERESCHWELLE = 7.5;

//Wasserspiegel
int Wasserkiste_Hoehe = 35; //in cm //da wo der Ultraschallsensor sitzt

//Licht
bool Lichtsperre = false;
unsigned long lastTimeLichtUpdate = 0;
float LDRSCHWELLE = 0.3;      // 0: Dunkel 1: Hell
int LichtRegelIntervall = 10; //in s

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
  if (millis() - lastTimeRegelung >= (RegelungsintervallInSek * 1000) && !Regelungssperre)
  {
    if (RegelungZeitSperre > 0)
      RegelungZeitSperre--;
    else
    {
      if (Regelung_PH(Dosierpumpe, Luftpumpe, Wasserpumpe.getStatus())) //gibt true zurück, wenn Dosierung ausgeführt wurde
        RegelungZeitSperre = 2;                                         //nachdem Dosierpumpe an war, 2 Regelvorgänge abwarten bevor was neues passiert
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

  //update Licht im Intervall "LichtRegelIntervall"
  if (millis() - lastTimeLichtUpdate >= (LichtRegelIntervall * 1000))
  {
    if (UmgebungslichtZuDunkelMittelwert() && !Lichtsperre)
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
    SendSerialInteger(temp * 100);
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
    SendSerialInteger(phWert * 100);
  }
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
  else if (strcmp(receivedChars, "NachfuellModusInaktiv") == 0)
  {
    Nachfuellen = false;
    Dosierpumpe.setSperre(false);
    SendSerialInteger(0x0F0F);
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
  else if (strcmp(receivedChars, "getLichtBufferAvg") == 0)
  {
    float avg = getMittelwertBufferLDR();
    SendSerialInteger(avg * 100); //0...100 --> 0 = dunkel 100 = Hell
  }
  else if (strcmp(receivedChars, "getLichtSchwelle") == 0)
  {
    float avg = LDRSCHWELLE;
    SendSerialInteger(avg * 100); //0...100 --> 0 = dunkel 100 = Hell
  }
  else if (strcmp(receivedChars, "getLichtRegelIntervall") == 0)
  {
    SendSerialInteger(LichtRegelIntervall);
  }
  else if (strstr(receivedChars, "NeuesLichtIntervall:") != 0)
  {
    int zehner = receivedChars[20] - '0';
    int einer = receivedChars[21] - '0';
    LichtRegelIntervall = (zehner * 10 + einer);
    SendSerialInteger(LichtRegelIntervall);
  }
  else if (strstr(receivedChars, "NeueLichtschwelle:") != 0)
  {
    int zehner = receivedChars[18] - '0';
    int einer = receivedChars[19] - '0';
    float neueSchwelle = ((float)(zehner * 10 + einer)) / 100;
    LDRSCHWELLE = neueSchwelle;
    SendSerialInteger(neueSchwelle * 100);
  }
  else if (strcmp(receivedChars, "getLichtStatus") == 0)
  {
    SendSerialInteger(digitalRead(PIN_LEDVORNE));
  }
  else if (strcmp(receivedChars, "getWasserpumpeStatus") == 0)
  {
    SendSerialInteger(Wasserpumpe.getStatus());
  }
  else if (strcmp(receivedChars, "getLuftpumpeStatus") == 0)
  {
    SendSerialInteger(Luftpumpe.getStatus());
  }
  else if (strstr(receivedChars, "DosierpumpeIntervallSek:") != 0)
  {
    int zehner = receivedChars[24] - '0';
    int einer = receivedChars[25] - '0';
    int Intervall = zehner * 10 + einer;
    Dosierpumpe.SchalteFuerDauerEin(Intervall * 1000);
    SendSerialInteger(Intervall);
  }
  else if (strstr(receivedChars, "PhRegelungsintervallInMin:") != 0)
  {
    int zehner = receivedChars[26] - '0';
    int einer = receivedChars[27] - '0';
    int Intervall = zehner * 10 + einer;
    RegelungsintervallInSek = Intervall * 60;
    SendSerialInteger(Intervall * 100);
  }
  else if (strcmp(receivedChars, "getPhIntervallInMin") == 0)
  {
    SendSerialInteger(RegelungsintervallInSek / 60);
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
    int ElementPos = zehner * 10 + einer;
    if (ElementPos < 0 || ElementPos > 10)
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
   else if (strstr(receivedChars, "PhSolution:") != 0)
  {
    int tausend= receivedChars[11] - '0';
    int hundert = receivedChars[12] - '0';
    int zehner = receivedChars[13] - '0';
    int einer = receivedChars[14] - '0';
    float solution = ((float)(tausend*1000 + hundert * 100 + zehner * 10 + einer)) / 100;
    ec.setSolution(solution);
    SendSerialInteger(ec.getSolution() * 100);
  }
  else
  {
    SendSerialInteger(0xFFFF);
  }
  newData = false;
  //Serial.println();
}
