class pixelSheet
{
public:
    pixelSheet(uint8_t sizeX, uint8_t sizeY, uint16_t col) : sizeX(sizeX),
                                                             sizeY(sizeY),
                                                             color(col)
    {
        initializePixels();
    }

    void setPixel(int x, int y)
    {
        int section, column = 0;
        calculateBitPosition(x, y, &section, &column);

        bitWrite(pixels[y][section], column, readPixel(x, y));
    }

    int readPixel(int x, int y)
    {
        int section, column = 0;
        calculateBitPosition(x, y, &section, &column);

        return bitRead(pixels[y][section], column);
    }

    void Color(uint16_t col)
    {
        color = col;
    }

    uint16_t Color()
    {
        return color;
    }

    void initializePixels()
    {
        if (sizeX <= 120 && sizeY <= 100)
            pixels = (uint8_t **)malloc(sizeof(uint8_t[sizeY][sizeX / 8]));
            
        calculateAspectRatio();
    }

private:
    uint8_t **pixels;
    uint8_t sizeX, sizeY;
    uint16_t color;

    int aspectRatio[2];

    void calculateBitPosition(int x, int y, int *sectionOut, int *columnOut)
    {
        int section = x, column = 0;
        while (section % 8 != 0)
        {
            section--;
            column++;
        }
        section = section / 8;

        sectionOut = section;
        columnOut = column;
    }    
};