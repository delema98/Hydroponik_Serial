#include "mainHeader.h"

//Buffer bekommt alle 10min einen neuen Wert
//Dann wird der Durchschnitt des Buffers gebildet
//Wenn der Durchschnitt unter der unteren PH Grenze liegt, wird die Dosierpumpe + Luftpumpe entsprechend angesteuert

float avgbufPH[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //Buffer bekommt alle 10min einen neuen Wert
int anzahlAuslesen = 0;

bool Regelung_PH(Aktor &Dosierpumpe, Aktor &Luftpumpe, bool StatusWasserpumpe)
{
    //neuen Wert in Buffer schreiben
    //BufferFuellen(getPHWert());

    BufferFuellen(6);

    //best. Menge an PH downer hinzugeben
    //wenn Buffer voll (kein falscher Mittelwert)
    //und obere Schwelle überschritten wurde
    if (anzahlAuslesen >= 10 && getMittelwertBuffer() > PH_OBERESCHWELLE)
    {
        //Dosierpumpe für bestimmte Zeit an

        float aktuelleLiterInSystem = Kiste_Liter();
        if (StatusWasserpumpe)
            aktuelleLiterInSystem += LiterInSystemWennWasserpumpeAn;

        float gesamteMengePhDown = aktuelleLiterInSystem * ML_PHDOWN_PRO_LITER_WASSER;
        float ZeitAn = gesamteMengePhDown * FAKTORMLPROSEK;
        Dosierpumpe.SchalteFuerDauerEin(ZeitAn * 1000);
        Luftpumpe.SchalteFuerDauerEin(DauerLuftpumpeAninS * 1000);

        //clear Buffer
        clearBuffer();

        return true;
    }
    else
        return false;
}

void BufferFuellen(float _akt_PH_Wert) //return 1 wenn mind 10 Werte in Buffer,
{
    if (anzahlAuslesen < 10)
    {
        avgbufPH[anzahlAuslesen] = _akt_PH_Wert;
    }
    else //es wurden schon mehr als 10 Werte gelesen, also alter Wert rausschmeißen
    {
        for (int i = 0; i < 9; i++)
        {
            avgbufPH[i] = avgbufPH[i + 1];
        }
        avgbufPH[9] = _akt_PH_Wert;
    }

    anzahlAuslesen++;
}

float getMittelwertBuffer()
{
    float MittelwertBuffer = 0;
    for (int i = 0; i < 10; i++)
    {
        MittelwertBuffer += avgbufPH[i];
    }
    return MittelwertBuffer / 10;
}

void clearBuffer()
{
    anzahlAuslesen = 0;
    for (int i = 0; i < 10; i++)
    {
        avgbufPH[i] = 0;
    }
}

int getBufferAnzahlElemente()
{
    if (anzahlAuslesen > 9)
        return 10;
    else
        return anzahlAuslesen;
}

float getBufferElementPos(int pos)
{
    return avgbufPH[pos];
}