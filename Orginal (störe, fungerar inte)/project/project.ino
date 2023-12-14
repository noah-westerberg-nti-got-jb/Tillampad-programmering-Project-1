#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735

#if defined(ARDUINO_FEATHER_ESP32) // Feather Huzzah32
#define TFT_CS 14
#define TFT_RST 15
#define TFT_DC 32

#elif defined(ESP8266)
#define TFT_CS 4
#define TFT_RST 16
#define TFT_DC 5

#else
// For the breakout board, you can use any 2 or 3 pins.
// These pins will also work for the 1.8" TFT shield.
#define TFT_CS 10
#define TFT_RST 9 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC 8
#endif

/*
    Tar hand om en siffra som har en specifik definitions mängd.

    Argument:
    min: minsta talet som det kan vara
    max: största talet det kan vara
*/
class uiNumberSelect
{
public:
    uiNumberSelect(uint8_t min, uint8_t max) : maxNum(max),
                                               minNum(min),
                                               currentNum(0)
    {
    }

    void shift(int val)
    {
        currentNum += val;

        if (currentNum > maxNum)
        {
            currentNum = minNum;
        }
        else if (currentNum < minNum)
        {
            currentNum = maxNum;
        }
    }

    uint8_t getCurrent()
    {
        return currentNum;
    }

    void changeSize(uint8_t min, uint8_t max)
    {
        minNum = min;
        maxNum = max;
    }

private:
    uint8_t currentNum = 0;
    uint8_t maxNum, minNum;
};

uint8_t selectedUI = 0;
bool uiEnabled = false;
#define red 0
#define green 1
#define blue 2
#define colorSelect 0
#define copy 1
#define paste 2
#define sizeSelect 3
#define widthSelect 0
#define heightSelect 1

uiNumberSelect drawWidthValue(0, 0);
uiNumberSelect drawHeightValue(0, 0);

/*
    Main ui knapp bilder
    (Fungerar inte)

    0: svart
    1: vit
    2: Den nuvarande färgen
*/
uint8_t colorSelectButton[8][8] =
    {
        {2, 2, 2, 2, 2, 2, 2, 2},
        {2, 2, 2, 2, 2, 2, 2, 2},
        {2, 2, 2, 2, 2, 2, 2, 2},
        {2, 2, 2, 2, 2, 2, 2, 2},
        {2, 2, 2, 2, 2, 2, 2, 2},
        {2, 2, 2, 2, 2, 2, 2, 2},
        {2, 2, 2, 2, 2, 2, 2, 2},
        {2, 2, 2, 2, 2, 2, 2, 2},
};
uint8_t copyButton[8][8] =
    {
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
};
uint8_t pasteButton[8][8] =
    {
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
};
uint8_t sizeSelectButton[8][8] =
    {
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
};

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

struct vector2I
{
    int x;
    int y;
} joystick;
int joystickXRange[4] = {0, 1023, -100, 100}, joystickYRange[4] = {0, 1023, -100, 100}; // Ville möjligtvis göra någon typ av kalibrition 
#define joystickXInput A1
#define joystickYInput A0

uint8_t **pixels = nullptr;
uint8_t aspectRatio[2] = {1, 1};
uint8_t imgHorizontalStart;
uint8_t imgHorizontalEnd;
uint8_t imgVerticalStart;
uint8_t imgVerticalEnd;
uint8_t imgWidth = 0, imgHeight = 0;

/*
    lista på färger som finns
    Om colors[n] kan det finnas n-1 färger. 0 < n < 256
    colors[0] är backgrunds färgen

    färger är representerade av ett 2 byte tal.
    Istället för att representera alla pixlar som en array av 2 byte tal så är de representerade av en array av 1 byte tal som fungerar som en pekare till en lista av färger för att spara på minne.
*/
uint16_t colors[10] = {0};
uint8_t currentColorIndex = 1;
uint16_t &backgroundColor = colors[0] = 0xFFFFF;
uiNumberSelect rNum(0, 256), gNum(0, 256), bNum(0, 256);

uint8_t mode = 0;
#define draw 0
#define mainUI 1
#define colorSet 2
#define copySet 3
#define copyPaste 4
#define setSize 5

uint8_t drawSize[2] = {1, 1}; // {width, height}
uint8_t drawMode;
#define square 0
#define rect 1
#define triangle 2
#define circle 3

struct cursorStruct
{
    vector2I position;
    uint16_t baseColor; // Jag tänkte att markören skulle fada mellan dens färg och färgen som den överlappar på bilden. baseColor är färgen som markören ska vara (inte implementerad)
    uint16_t currentColor; // Nuvarande färgen som markören är. Färg mellan baseColor och färgen som är i markörens position på bilden;
    bool enabled;
} cursor;

#define boardButton !digitalRead(2) // input-pullup flippar outputen true om man trycker false om man inte trycker
#define stickButton !digitalRead(7) // Input pullup gör så att man bara behöver kopla till ground och arduinon. ingen separat ström eller resistor behövs

// Värde som används för att se om man går in i ett nytt läge
// loop cyclen går igenom och gör saker flera gånger men om det finns något som bara behöver göras en gång i början så används den här variablen
bool start = 1;

vector2I copyStart = {0}; // position
uint16_t **copyVal; // lagring av det som kopieras

int8_t shiftVal; // värde som ui siffror ska ändras med

bool testing = true;
void setup()
{
    if (testing)
    {
        Serial.begin(9600);
        while (!Serial)
        {
        }
    }

    pinMode(2, INPUT_PULLUP);       // button
    pinMode(7, INPUT_PULLUP);       // joystick button
    pinMode(joystickXInput, INPUT); // joystick X-axis
    pinMode(joystickYInput, INPUT); // joystick Y-axis

    imgSetup();
}

void loop()
{
    calculateJoystickAxis(&joystick, analogRead(joystickXInput), analogRead(joystickYInput), joystickXRange, joystickYRange);

    // Flyttar/ritar/ritar om den förra positionen av markören
    if (cursor.enabled)
    {
        if (joystick.x >= 40 && cursor.position.x != imgWidth - 1)
        {
            resetCursor(&cursor.position, drawSize, aspectRatio);
            cursor.position.x++;
            delay(100);
        }
        else if (joystick.x <= -40 && cursor.position.x != 0)
        {
            resetCursor(&cursor.position, drawSize, aspectRatio);
            cursor.position.x--;
            delay(100);
        }
        if (joystick.y >= 40 && cursor.position.y != imgHeight - 1)
        {
            resetCursor(&cursor.position, drawSize, aspectRatio);
            cursor.position.y++;
            delay(100);
        }
        else if (joystick.y <= -40 && cursor.position.y != 0)
        {
            resetCursor(&cursor.position, drawSize, aspectRatio);
            cursor.position.y--;
            delay(100);
        }

        drawCursor(&cursor, drawSize, aspectRatio);
    }

    // ui markör position
    if (uiEnabled)
    {
        if (joystick.x >= 50)
        {
            selectedUI++;
            delay(100);
        }
        else if (joystick.x <= -50)
        {
            selectedUI--;
            delay(100);
        }
    }

    switch (mode)
    {
    case draw:
        if (!cursor.enabled)
            cursor.enabled = true;

        if (start)
        {
            drawMainUI(0);
            start = 0;
        }
        
        // bara rectangel implementerad
        switch (drawMode)
        {
        // kvadrat och rectangel hade varit samma function men för kvadraten hade storleken varit lika stor i både x- och y-led
        case square:
        case rect:
            if (boardButton)
            {
                drawRect(cursor.position.x + imgHorizontalStart, cursor.position.y + imgVerticalStart, aspectRatio[0], aspectRatio[1], drawSize, colors[currentColorIndex], 1, 1);
            }
            break;
        case triangle:

            break;
        case circle:

            break;
        }

        if (stickButton)
        {
            setMode(mainUI);
        }
        break;
    case mainUI:
        if (cursor.enabled)
            cursor.enabled = false;
        if (!uiEnabled)
            uiEnabled = true;

        if (selectedUI > 3)
            selectedUI = 3;
        else if (selectedUI < 0)
            selectedUI = 0;

        // ritar bara om ui:n om man flytar på styrspaken
        if (joystick.x * joystick.x > 100)
            drawMainUI(1);

        if (stickButton)
        {
            mode = draw;
        }
        if (boardButton)
        {
            switch (selectedUI)
            {
            case colorSelect:
                setMode(colorSet);
                break;
            case copy:
                setMode(copySet);
                break;
            case paste:
                setMode(copyPaste);
                break;
            case sizeSelect:
                setMode(setSize);
                break;
            }
        }
        break;
    case colorSet:
        if (cursor.enabled)
            cursor.enabled = false;
        if (!uiEnabled)
            uiEnabled = true;

        // skapar ett fönster för ui elementen
        if (start)
        {
            tft.fillRect(30, 30, 38, 38, tft.color565(rNum.getCurrent(), gNum.getCurrent(), bNum.getCurrent()));
            tft.setTextColor(~(tft.color565(rNum.getCurrent(), gNum.getCurrent(), bNum.getCurrent())));
            tft.drawRect(29, 29, 49, 40, 0);
            start = 0;
        }
        if (stickButton)
        {
            setMode(mainUI);
            drawImg();
        }

        uint16_t currentColor;
        if (currentColor == NULL) // ger bara currentColor ett värde första gången effer att den har skapats.
            currentColor = tft.color565(rNum.getCurrent(), gNum.getCurrent(), bNum.getCurrent());

        if (boardButton)
        {
            currentColor = tft.color565(rNum.getCurrent(), gNum.getCurrent(), bNum.getCurrent());
            tft.setTextColor(~currentColor);
            setColor(currentColor);
        }

        if (selectedUI > 2)
            selectedUI = 2;
        else if (selectedUI < 0)
            selectedUI = 0;

        shiftVal = 0;
        if (joystick.y <= -40)
            shiftVal = -1;
        else if (joystick.y >= 40)
            shiftVal = 1;

        switch (selectedUI)
        {
        case red:
            // gräns runt de valda elementet
            tft.drawRect(32, 60, 20, 20, ~currentColor);

            rNum.shift(shiftVal);
            break;
        case green:
            tft.drawRect(57, 60, 20, 20, ~currentColor);

            gNum.shift(shiftVal);
            break;
        case blue:
            tft.drawRect(72, 60, 20, 20, ~currentColor);

            bNum.shift(shiftVal);
            break;
        }

        tft.setCursor(35, 56);
        tft.print(rNum.getCurrent());
        tft.setCursor(60, 56);
        tft.print(gNum.getCurrent());
        tft.setCursor(75, 56);
        tft.print(bNum.getCurrent());

        break;
    case copySet:
        if (!cursor.enabled)
            cursor.enabled = true;

        if (boardButton)
        {
            // Inser när jag kommenterar att det inte hade gått att kopiera från topp väster hörn
            if (copyStart.x == 0 && copyStart.y == 0)
                copyStart = cursor.position;
            else
            {
                free(copyVal);
                // allocerar minne till kopieringen. Ser nu när jag kommenterar att den inte kommer fungera om man sätter det andra positions värdet antingen över eller till vänster av det första värdet.
                copyVal = (uint16_t **)malloc(sizeof(uint16_t[cursor.position.x - copyStart.x][cursor.position.y - copyStart.y]));
                if (copyVal == NULL)
                {
                    free(copyVal);
                    tft.setCursor(5, 5);
                    tft.setTextColor(0, 0xFFFFF);
                    tft.print("ERROR. Not enough memory. Select smaller size");
                    delay(1500);
                    drawImg();
                    break;
                }

                for (int i = 0; i < cursor.position.x; i++)
                {
                    for (int j = 0; j < cursor.position.y; j++)
                    {
                        // kopierar värdena
                        copyVal[i][j] = pixels[copyStart.x + i][copyStart.y + j];
                    }
                }

                setMode(copyPaste);
                selectedUI = paste;
                drawMainUI(1);
            }
        }
        break;
    case copyPaste:
        if (!cursor.enabled)
            cursor.enabled = true;

        if (boardButton)
        {
            uint8_t copySizeX = sizeof(copyVal) / sizeof(*copyVal);
            uint8_t copySizeY = sizeof(*copyVal) / sizeof(**copyVal);

            drawPixels(copyStart.x, copyStart.y, copySizeX, copySizeY, aspectRatio[0], aspectRatio[1], copyVal, 1);

            copyStart = {0, 0};
        }

        if (stickButton)
        {
            setMode(draw);
        }
        break;
    // i princip samma som colorSet
    case setSize:
        if (cursor.enabled)
            cursor.enabled = false;
        if (!uiEnabled)
            uiEnabled = true;

        if (start)
        {
            tft.fillRect(30, 30, 38, 38, 0xFFFFF);
            tft.setTextColor(0);
            tft.drawRect(29, 29, 49, 40, 0);
            start = 0;
        }

        if (stickButton || boardButton)
        {
            setMode(mainUI);
            drawImg();
        }

        if (selectedUI > 1)
            selectedUI = 1;
        else if (selectedUI < 0)
            selectedUI = 0;

        shiftVal = 0;
        if (joystick.y <= -40)
            shiftVal = -1;
        else if (joystick.y >= 40)
            shiftVal = 1;

        switch (selectedUI)
        {
        case widthSelect:
            tft.drawRect(42, 60, 20, 20, 0);

            drawWidthValue.shift(shiftVal);
            break;
        case heightSelect:
            tft.drawRect(62, 60, 20, 20, 0);

            drawHeightValue.shift(shiftVal);
            break;
        }

        tft.setCursor(45, 56);
        tft.print(drawWidthValue.getCurrent());
        tft.setCursor(65, 56);
        tft.print(drawHeightValue.getCurrent());

        break;
    }
}

/*
Argument:
    col: färgen som ska sättas
*/
void setColor(uint16_t col)
{
    colors[++currentColorIndex] = col;
    // borde lägga till nåot som kollar om currentColorIndex är över max värdet och nollställer den
}

void setMode(int m)
{
    start = 1;
    mode = m;
}

void calculateJoystickAxis(vector2I *joystickAxis, int xInput, int yInput, int xRange[], int yRange[])
{
    if (xInput > 600 || xInput < 400) // gör att den inte tar in värden när den är stilla eller knapt rör sig
        joystickAxis->x = map(xInput, xRange[0], xRange[1], xRange[3], xRange[2]);
    else
        joystickAxis->x = 0;
    if (yInput > 600 || yInput < 400)
        joystickAxis->y = map(yInput, yRange[0], yRange[1], yRange[3], yRange[2]);
    else
        joystickAxis->y = 0;
}

void drawRect(uint8_t x, uint8_t y, uint8_t xScale, uint8_t yScale, uint8_t size[2], uint16_t color, bool saveToImg, bool drawOnCanvas)
{
    if (drawOnCanvas) // ritar med bildens skala om den är på bilden
        tft.fillRect((x * xScale) + imgHorizontalStart, (y * yScale) + imgVerticalStart, xScale * size[0], yScale * size[1], color);
    else // ritar utan skalan om det ör något annat på skärmen
        tft.fillRect(x, y, xScale * size[0], yScale * size[1], color);

    // för över värden om den ska sparas på bilden
    if (saveToImg)
    {
        for (int i = 0; i < size[0]; i++)
        {
            for (int j = 0; j < size[1]; j++)
            {
                pixels[i + x - 1][j + y - 1] = currentColorIndex;
            }
        }
    }
}


// funktioner som inte gjordes:
// void drawCircle(uint8_t x, uint8_t y, uint8_t xScale, uint8_t yScale, uint8_t size[], uint16_t color){
//     uint8_t shortestSide = xScale * size[0];
//     if(yScale * size[1] > shortestSide){
//         shortestSide = yScale * size[1];
//     }
//
//     tft.fillRoundRect(x, y, size[0] * xScale, size[1] * yScale, shortestSide / 2, color);
// }
//
// void drawTriangle(uint8_t x, uint8_t y, uint8_t xScale, uint8_t yScale, uint8_t size[], uint16_t color){
//
// }

/*
    Välj storlkek på bild

    Argument: variable där bredd sätts, variabel där höjd sätts
*/
void selectImgSize(uint8_t &w, uint8_t &h)
{
    uiNumberSelect width(1, 120);
    uiNumberSelect height(1, 100);

    while (h == 0)
    {
        while (w == 0)
        {
            if (analogRead(joystickYInput) < 300)
            {
                width.shift(-1);
            }
            else if (analogRead(joystickYInput) > 700)
            {
                width.shift(1);
            }
            if (boardButton)
            {
                w = width.getCurrent();

                delay(100);
            }

            tft.setCursor(35, 60);
            tft.print("Width: ");
            tft.print(width.getCurrent());
            if (width.getCurrent() < 100)
                tft.print(" ");
            if (width.getCurrent() < 10)
                tft.print(" ");
            delay(50);
        }

        if (analogRead(joystickYInput) < 300)
        {
            height.shift(-1);
        }
        else if (analogRead(joystickYInput) > 700)
        {
            height.shift(1);
        }
        if (boardButton)
        {
            h = height.getCurrent();
        }

        tft.setCursor(35, 60);
        tft.print("Height:");
        tft.print(height.getCurrent());
        if (height.getCurrent() < 100)
            tft.print(" ");
        if (height.getCurrent() < 10)
            tft.print(" ");
        if (w != 0) // borde inte göra något men vågar inte ändra på det som fungerar.
            delay(50);
    }
}

/*
    Hittar de största talet som höjden/bredden kan skalas till
    Völdigt otidsoptimerad men den behöver bara köras en gång

    Argument: bredd, höjd, arry där scalan ska sättas, max värdet på platsen bilden kan ta upp på skärmen
*/
void setAspectRatio(uint8_t w, uint8_t h, uint8_t aR[2], uint8_t max[2])
{
    uint8_t size[2] = {w, h};
    for (int i = 0; i < 2; i++) // kör igenom en gång för x-axeln och en gång för y-axeln
    {
        for (int j = max[i]; j > 0; j--)
        {
            int ratio = size[i] * j;
            if (ratio <= max[i])
            {
                aR[i] = j;
                break;
            }
        }
    }
}

void imgSetup()
{
    tft.initR(INITR_144GREENTAB); // Init ST7735R chip, green tab
    tft.fillScreen(backgroundColor); // rad 683 - 685 borde nog vara med i selectImgSize funktionen
    tft.drawRect(30, 30, 68, 68, ~backgroundColor);
    tft.setTextColor(~backgroundColor, backgroundColor); // Färg är motsatt till bakgrunds färg så att det altid kommer att vara hög kontrast
    bool error = 0;
    while (pixels == nullptr) // gör så man inte kan välja ett för stort värde
    {
        if (error) // gör att den inte körs första gången man går igenom
        {
            tft.setCursor(5, 5);
            tft.print("ERROR. Not enough memory. Select smaller size");
        }
        imgWidth = 0;
        imgHeight = 0;
        selectImgSize(imgWidth, imgHeight);
        pixels = (uint8_t **)malloc(sizeof(uint8_t[imgWidth][imgHeight]));
        error = 1;
    }
    // max ritstorlek-värdena
    drawWidthValue.changeSize(1, imgWidth);
    drawHeightValue.changeSize(1, imgHeight);
    pixels = {0}; // alla pixlar får färgindexen 0
    uint8_t max[2] = {128, 100};
    setAspectRatio(imgWidth, imgHeight, aspectRatio, max);
    imgHorizontalStart = 64 - ((imgWidth * aspectRatio[0]) / 2); // 64 = mitten av skärmen
    imgHorizontalEnd = 64 + ((imgWidth * aspectRatio[0]) / 2);
    imgVerticalStart = 50 - ((imgHeight * aspectRatio[1]) / 2); // 50 = mitten av där bilden kan vara. Sista 28 px används för ui.
    imgVerticalEnd = 50 + ((imgHeight * aspectRatio[1]) / 2);

    tft.fillScreen(tft.color565(180, 150, 130));
    uint16_t borderColor = tft.color565(20, 20, 20);
    tft.drawRect(imgHorizontalStart - 1, imgVerticalStart - 1, imgWidth * aspectRatio[0] + 2, imgHeight * aspectRatio[1] + 2, borderColor); // Bildram
    tft.drawFastHLine(0, 101, tft.width() - 1, borderColor); // UI avdelare
    tft.fillRect(imgHorizontalStart, imgVerticalStart, imgWidth * aspectRatio[0], imgHeight * aspectRatio[1], backgroundColor); // ritar ut canvasen

    drawMainUI(0);
}

// Ritar om bilden
void drawImg()
{
    tft.fillScreen(tft.color565(180, 150, 130));
    uint16_t borderColor = tft.color565(20, 20, 20);
    tft.drawRect(imgHorizontalStart - 1, imgVerticalStart - 1, imgWidth * aspectRatio[0] + 2, imgHeight * aspectRatio[1] + 2, borderColor); // Bildram
    tft.drawFastHLine(0, 101, tft.width() - 1, borderColor);                                                                                // UI avdelare
    drawPixels(imgHorizontalStart, imgVerticalStart, imgWidth, imgHeight, aspectRatio[0], aspectRatio[1], pixels, 0);
}

/*
    Ritar ut ett set av pixlar

    Overload: 
    uint8_t **img: bilden den får in kommer ha färgindexer på pixlarna
*/
void drawPixels(uint8_t startX, uint8_t startY, uint8_t sizeX, uint8_t sizeY, uint8_t scaleX, uint8_t scaleY, uint8_t **img, bool saveToImg)
{
    for (int i = 0; i < sizeX; i++)
    {
        for (int j = 0; j < sizeY; j++)
        {
            uint8_t s[2] = {1, 1};
            drawRect(startX + i, startY + j, scaleX, scaleY, s, colors[img[i][j]], saveToImg, saveToImg);
        }
    }
}

/*
    Overload:
    uint16_t **img: bilden kommer ha färgvärden på pixlarna
*/
void drawPixels(uint8_t startX, uint8_t startY, uint8_t sizeX, uint8_t sizeY, uint8_t scaleX, uint8_t scaleY, uint16_t **img, bool saveToImg)
{
    for (int i = 0; i < sizeX; i++)
    {
        for (int j = 0; j < sizeY; j++)
        {
            uint8_t s[2] = {1, 1};
            drawRect(startX + i, startY + j, scaleX, scaleY, s, img[i][j], saveToImg, saveToImg);
        }
    }
}

void drawCursor(cursorStruct *cursor, uint8_t size[2], uint8_t scale[2])
{
    // ritar en gräns runt där man kan rita
    // -1 på positionen för att gå bak ett steg så att gränsen inte går över rit området
    // + 2 på storleken så den kompenserar för -1 på positionen och att den ska gå över ett steg extra för samma andledning som åvan
    tft.drawRect((cursor->position.x * scale[0]) + imgHorizontalStart - 1, (cursor->position.y * scale[1]) + imgVerticalStart - 1, size[0] * scale[0] + 2, size[1] * scale[1] + 2, colors[currentColorIndex]);
}

void resetCursor(vector2I *pos, uint8_t size[2], uint8_t scale[2])
{
    for (int i = -1; i < size[0] + 1; i++)
    {
        for (int j = -1; j < size[0] + 1; j++)
        {
            // går till nästa tal om värdet är utaför värdemängden
            if (i + pos->x == -1 || i + pos->x > imgWidth || j + pos->y == -1 || j + pos->y > imgHeight)
                continue;

            if (i != size[0] || j != size[1]) // gör att den bara ritar en pxiel brett för att ta bort ramen
                tft.fillRect(i + (pos->x * scale[0]) + imgHorizontalStart, j + (pos->y * scale[0]) + imgVerticalStart, scale[0], scale[1], colors[pixels[i + pos->x][j + pos->y]]);
            else
                tft.drawPixel(i + (pos->x * scale[0]) + imgHorizontalStart, j + (pos->y * scale[0]) + imgVerticalStart, colors[pixels[i + pos->x][j + pos->y]]);
            delay(10);
        }
    }
}

void drawMainUI(bool drawMainUIBorder)
{
    // gör att området blir blankt
    tft.fillRect(0, 102, tft.width() - 1, 27, tft.color565(180, 150, 130));

    // ritar
    drawPixels(7, 105, 8, 8, 2, 2, convertNubmersToColors(colorSelectButton), 0);
    tft.drawRect(6, 104, 20, 20, 0); // gräns för knapparna så att de står ut mer
    drawPixels(39, 105, 8, 8, 2, 2, convertNubmersToColors(copyButton), 0);
    tft.drawRect(38, 104, 20, 20, 0);
    drawPixels(71, 105, 8, 8, 2, 2, convertNubmersToColors(pasteButton), 0);
    tft.drawRect(70, 104, 20, 20, 0);
    drawPixels(103, 105, 8, 8, 2, 2, convertNubmersToColors(sizeSelectButton), 0);
    tft.drawRect(102, 104, 20, 20, 0);

    if (drawMainUIBorder) // ritar selections gräns om man använder ui:n
    {
        switch (selectedUI)
        {
        case colorSelect:
            tft.drawRect(3, 104, 20, 20, 0); // positionerna är fel
            break;
        case copy:
            tft.drawRect(35, 104, 20, 20, 0);
            break;
        case paste:
            tft.drawRect(67, 104, 20, 20, 0);
            break;
        case sizeSelect:
            tft.drawRect(99, 104, 20, 20, 0);
            break;
        }
    }
}

/*
    tar in en uint8_t[8][8] där värdena är satta till 0, 1 eller 2
    0 blir svart
    1 blir vitt
    2 blir den nuvarande färgen

    (tror inte den fungerar)
*/
uint16_t **convertNubmersToColors(uint8_t pixels[8][8])
{
    uint16_t img[8][8];
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            switch (pixels[i][j])
            {
            case 0:
                img[i][j] = 0;
                break;
            case 1:
                img[i][j] = 0xFFFFF;
                break;
            case 2:
                img[i][j] = colors[currentColorIndex];
                break;
            }
        }
    }

    return (uint16_t **)img;
}

/*
    Får den inbygda lampan på arduinon att växla mellan att vara tänd och otänd.
    Jag använde den här functionen för att troubleshoota för att se om arduinon var frusen eller om det var något fel med min output
*/
// void blink()
// {
//     switch (digitalRead(13))
//     {
//     case LOW:
//         digitalWrite(13, HIGH);
//         break;
//     case HIGH:
//         digitalWrite(13, LOW);
//         break;
//     }
// }