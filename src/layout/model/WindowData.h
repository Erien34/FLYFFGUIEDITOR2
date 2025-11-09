#pragma once
#include <QString>
#include <QStringList>
#include <vector>
#include <memory>
#include "ControlData.h"

struct WindowData {
    // Header
    QString name;       // z.B. APP_CONFIRM_BUY
    QString texture;       // z.B. "WndTile03.tga"
    QString titletext;       // z.B. ""
    QStringList headerTokens; // komplette Header-Zeile (unverändert, inkl. Hex)

    // Komfortfelder (nur falls parsebar):
    int modus = 0;       // z.B. 1
    int width = 0;      // z.B. 256
    int height = 0;     // z.B. 160
    QString flagsHex;   // z.B. "0x2410000" (NICHT konvertieren)
    int mod = 0;    // z.B. 26

    // Fenster-Unterblöcke
    QString titleId;    // aus // Title String - Block
    QString helpId;     // aus // Help Key - Block

    //---------------------------------------- ^Window Data

    // Controls
    std::vector<std::shared_ptr<ControlData>> controls;

    quint32 flagsMask = 0;             // Effektive Bitmaske
    QVector<QString> resolvedMask;     // Einzel gesetzte Flags

    bool valid = true;

    // Parser-Metadaten (nicht Teil der originalen FlyFF-Struktur)
    int sourceLine = -1;          // Zeilennummer in resdata.inc, an der das Fenster beginnt
    QString rawHeader;            // Originaltextzeile des Fensters (z. B. "APP_CONFIRM_BUY ...")
    bool isCorrupted = false;
};
