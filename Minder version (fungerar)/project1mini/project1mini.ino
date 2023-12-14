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

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

struct vector2I
{
    int x;
    int y;
} joystick;
int joystickXRange[] = {0, 1023, -100, 100}, joystickYRange[] = {0, 1023, -100, 100}; // Ville möjligtvis göra någon typ av kalibrition om jag hade tid
#define joystickXInput A1
#define joystickYInput A0

uint16_t pixels[16][16];
uint16_t currentColor = 0;
uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 0;

uint8_t drawSize[] = {1, 1}; // {w, h}

vector2I cursor;

#define stickButton !digitalRead(7) // input-pullup flippar outputen true om man trycker false om man inte trycker
#define greenButton !digitalRead(2) // Input pullup gör så att man bara behöver kopla till ground och arduinon. ingen separat ström eller resistor behövs
#define blueButton !digitalRead(3)
#define redButton !digitalRead(4)

void setup()
{
    // sätter alla pixlar till vit
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            pixels[i][j] = 0xFFFFF;
        }
    }

    pinMode(2, INPUT_PULLUP);       // greenButton
    pinMode(3, INPUT_PULLUP);       // blueButton
    pinMode(4, INPUT_PULLUP);       // redButton
    pinMode(7, INPUT_PULLUP);       // joystick button
    pinMode(joystickXInput, INPUT); // joystick X-axis
    pinMode(joystickYInput, INPUT); // joystick Y-axis

    tft.initR(INITR_144GREENTAB); // Init ST7735R chip, green tab
    tft.fillScreen(0xFFFFF);
}

void loop()
{
    calculateJoystickAxis(&joystick, analogRead(joystickXInput), analogRead(joystickYInput), joystickXRange, joystickYRange);

    /*
        Flyttar markören

        Den kan bara flyttas om man inte ändrar färg eller storlek

        Det hade varit bättre om det hanterades på samma sätt som färgändringen eller storleksöndringen
    */
    if (!redButton && !blueButton)
    {
        if (joystick.x <= -40 && cursor.x != 15)
        {
            resetCursor(&cursor, drawSize, 8, 8);
            cursor.x++;
            delay(100);
        }
        else if (joystick.x >= 40 && cursor.x != 0)
        {
            resetCursor(&cursor, drawSize, 8, 8);
            cursor.x--;
            delay(100);
        }
        if (joystick.y <= -40 && cursor.y != 15)
        {
            resetCursor(&cursor, drawSize, 8, 8);
            cursor.y++;
            delay(100);
        }
        else if (joystick.y >= 40 && cursor.y != 0)
     
        {
            resetCursor(&cursor, drawSize, 8, 8);
            cursor.y--;
            delay(100);
        }
    }

    if (greenButton)
        draw(cursor.x, cursor.y, 8, 8, drawSize, currentColor);

    // Ändra färg
    if (blueButton)
    {
        resetCursor(&cursor, drawSize, 8, 8); // Tror inte att markören behöver tas bort här
        if (redButton)
            red += -joystick.x / 20; // -100 <= x, y <= 100.
                                     // x, y / 20 blir ca mellan 0 och +-4
        else
            blue += -joystick.x / 20; // Overflow är intentional

        green += joystick.y / 20;
    }
    // Ändra storleken
    else if (redButton) // Man ska inte kunna öndra förg och storlek på samma gång
    {
        resetCursor(&cursor, drawSize, 8, 8);
        drawSize[0] += -joystick.x / 90; // -100 <= x, y <= 100.
        drawSize[1] += joystick.y / 90; // x, y / 90 avrundas till +-1
        // Om värdet delas med 100 finns det intet marginal för joysticken som kanske inte når upp hela vägen

        // Gör så att storleken inte kan bli större än skärmen
        if (drawSize[0] > 16)
            drawSize[0] = 16;
        else if (drawSize[0] == 0)
            drawSize[0] = 1;
        if (drawSize[1] > 16)
            drawSize[1] = 16;
        else if (drawSize[1] == 0)
            drawSize[1] = 1;

        delay(100);
    }

    // ändrar färgen. Hade kunnat flyttas in till färgändrings sectionen så att den inte körs även om färgen inte har ändrats
    currentColor = tft.color565(red, green, blue);
    drawCursor(&cursor, drawSize, 8, 8, currentColor);

    // Visar joystick värdena
    // tft.setCursor(0, 110);
    // tft.setTextColor(0, 0xFFFFF);
    // tft.print("x: " + (String)(joystick.x / 90) + " y: " + (String)(joystick.y / 90) + "                      ");
}

/*
    joystickAxis: pekare till variablen som ska ändras
    xInput: analogRead(x-pin)
    yInput: analogRead(y-pin)
    xRange, yRange: Hade kunnat hårdkodas för det här projectet. Jag ville från början göra så att man skulle kunna kalibrera styrspaken på något sätt
*/
void calculateJoystickAxis(vector2I *joystickAxis, int xInput, int yInput, int xRange[], int yRange[])
{
    if (xInput > 600 || xInput < 400) // gör att den inte tar in värden när den är stilla eller knapt rör sig
        joystickAxis->x = map(xInput, xRange[0], xRange[1], xRange[2], xRange[3]);
    else
        joystickAxis->x = 0;
    if (yInput > 600 || yInput < 400)
        joystickAxis->y = map(yInput, yRange[0], yRange[1], yRange[2], yRange[3]);
    else
        joystickAxis->y = 0;
}

/* 
    x, y: position där det ska ritas
    xScale, yScale: skalan av (storleken på skörmen):(storleken av bilden) (storlek i pixlar)
    size: storleken av det som ska ritas
    color: färg som ska ritas
*/
void draw(uint8_t x, uint8_t y, uint8_t xScale, uint8_t yScale, uint8_t size[2], uint16_t color)
{
    tft.fillRect((x * xScale), (y * yScale), xScale * size[0], yScale * size[1], color);

    // sparar in det som har ritats
    for (int i = 0; i < size[0]; i++)
    {
        for (int j = 0; j < size[1]; j++)
        {
            pixels[i + x][j + y] = color;
        }
    }
}

/*
    position: markörens position
    size: markörens storlek
    scaleX, scaleY: skalan av (storleken på skörmen):(storleken av bilden) (storlek i pixlar) (borde nog ha kallat argumenten, som betyder/gör samma sak, samma sak på alla ställen)
    color: markörens färg
*/
void drawCursor(vector2I *position, uint8_t size[2], uint8_t scaleX, uint8_t scaleY, uint16_t color)
{
    tft.fillRect(position->x * scaleX, position->y * scaleY, size[0] * scaleX, size[1] * scaleY, color);
}

/*
    Tar bort markören genom att rita det på bilden som ska vara där markören är

    pos: markörens position
    size: markörens storlek
    scaleX, scaleY: skalan av (storleken på skörmen):(storleken av bilden) (storlek i pixlar)
*/
void resetCursor(vector2I *pos, uint8_t size[2], uint8_t scaleX, uint8_t scaleY)
{
    for (int i = 0; i < size[0]; i++)
    {
        for (int j = 0; j < size[1]; j++)
        {
            tft.fillRect((pos->x + i) * scaleX, (pos->y + j) * scaleY, scaleX, scaleY, pixels[pos->x + i][pos->y + j]);
        }
    }
}