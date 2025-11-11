#pragma once
#include <QString>
#include <QStringList>
#include <memory>

#pragma once
#include <QString>
#include <QStringList>
#include <QColor>

enum ButtonState {
    Normal,
    Hovered,
    Pressed,
    Disabled
};

struct ControlData
{
    // --- Basisdaten ---
    QString type;       // WTYPE_BUTTON, WTYPE_STATIC, ...
    QString id;         // WIDC_xxx
    QString texture;    // "WndEditTile00.tga"

    // --- Layoutkoordinaten ---
    int mod0 = 0;       // Mode / Unk1 (nach der Textur)
    int x1 = 0;         // left
    int y1 = 0;         // top
    int x2 = 0;         // right
    int y2 = 0;         // bottom

    // --- Flags ---
    QString flagsHex;   // 0x220000 etc.

    // --- Reservierte Felder / Padding ---
    int mod1 = 0;
    int mod2 = 0;
    int mod3 = 0;
    int mod4 = 0;

    // --- Farbe ---
    QColor color = QColor(255, 255, 255);

    // --- Strings / Unterblöcke ---
    QString titleId;    // // Title String – IDS_RESDATA_INC_xxx
    QString tooltipId;  // // ToolTip – IDS_RESDATA_INC_xxx

    // --- Debug / Metadaten ---
    int sourceLine = 0;      // Zeilennummer in der Layoutdatei
    QString rawHeader;       // ursprüngliche Textzeile
    QStringList tokens;      // Tokenliste zur Analyse
    bool valid = false;

    quint32 flagsMask = 0;             // Effektive Bitmaske
    QVector<QString> resolvedMask;     // Einzel gesetzte Flags

    bool disabled = false;
    bool isPressed = false;
    bool isHovered = false;
};
