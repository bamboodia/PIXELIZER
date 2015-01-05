#include <FastLED.h>
#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>


#define DATA_PIN  1
#define CLOCK_PIN 2
#define CHIPSET   WS2801
#define COLRORDER RGB
#define BRIGHTNESS 255

const uint8_t kMatrixWidth  = 10;
const uint8_t kMatrixHeight = 10;
const uint8_t kBorderWidth = 1;

#define level 1024
#define sensitivity 0.02
#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight)

static uint16_t x;
static uint16_t y;
static uint16_t z;
uint16_t speed = 60; // speed is set dynamically once we've started up
uint16_t scale = 2000; // scale is set dynamically once we've started up
uint8_t noise[MAX_DIMENSION][MAX_DIMENSION];
CRGBPalette16 currentPalette( LavaColors_p );
uint8_t       colorLoop = 0;

//AUDIO//
const int myInput = AUDIO_INPUT_LINEIN;
AudioInputI2S          audioInput;         // audio shield: mic or line-in
AudioAnalyzeFFT256    myFFT;
AudioConnection patchCord1(audioInput, 0, myFFT, 0);
AudioControlSGTL5000 audioShield;

//#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
const bool    kMatrixSerpentineLayout = true;
CRGB leds[NUM_LEDS];

byte osci[4]; 
byte p[4];
float band[10];

/*int XY(int x, int y) { 
  if(y > HEIGHT) { y = HEIGHT; }
  if(y < 0) { y = 0; }
  if(x > WIDTH) { x = WIDTH;} 
  if(x < 0) { x = 0; }
  if(x % 2 == 1) {  
  return (x * (WIDTH) + (HEIGHT - y -1));
  } else { 
    // use that line only, if you have all rows beginning at the same side
    return (x * (WIDTH) + y);  
  }
}*/
uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;
  
  if( kMatrixSerpentineLayout == false) {
    i = (y * kMatrixWidth) + x;
  }

  if( kMatrixSerpentineLayout == true) {
    if( y & 0x01) {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    } else {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
  }
  
  return i;
}

void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  
  AudioMemory(14);
  Serial.begin(115200);
  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  //audioShield.lineInLevel(lvl);
  audioShield.autoVolumeControl(0, 3, 0, 1, 0.01, 0.4);
  audioShield.autoVolumeEnable();
   
  LEDS.addLeds<CHIPSET, DATA_PIN, CLOCK_PIN, COLRORDER>(leds,NUM_LEDS); /*SPI CHIPSET (4 WIRE) */
  //LEDS.addLeds<CHIPSET, DATA_PIN, COLRORDER>(leds,NUM_LEDS); /* NON-SPI CHIPSET (3 WIRE) */
  LEDS.setBrightness(BRIGHTNESS);
  // x = random16();
 // y = random16();
 // z = random16();

}

void ReadAudio() {
  
 
  if (myFFT.available()) {   
    
      band[0] = myFFT.read(0);      //0  //0
      band[1] = myFFT.read(1);      //0  //1
      band[2] = myFFT.read(2,3);    //1  //2
      band[3] = myFFT.read(3,5);    //2  //3
      band[4] = myFFT.read(4,6);    //3  //4
      band[5] = myFFT.read(6,14);   //6  //6
      band[6] = myFFT.read(14,32);  //12 //10
      band[7] = myFFT.read(32,45);  //23 //18
      band[8] = myFFT.read(45,69);  //46 //34
      band[9] = myFFT.read(69,116); //92 //64
      //band[10] = myFFT.read(0,127); // for reading whole level
     
       
    }
}

void Line(int x0, int y0, int x1, int y1, byte color) {
int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1;
int dy = -abs(y1-y0), sy = y0 < y1 ? 1 : -1;
int err = dx + dy, e2;
for(;;) {
leds[XY(x0, y0)] = CHSV(color, 255, 255);
if (x0 == x1 && y0 == y1) break;
e2 = 2 * err;
if (e2 > dy) { err += dy; x0 += sx; }
if (e2 < dx) { err += dx; y0 += sy; }
}
}

void Pixel(int x, int y, byte color, byte sat, byte value) {
  leds[XY(x, y)] = CHSV(color, sat, value);
}
 
// delete the screenbuffer
void ClearAll()  
{
  for(int i = 0; i < NUM_LEDS; i++) 
  {
    leds[i] = 0;
  }
}
 
/*
Oscillators and Emitters
*/
 
// set the speeds (and by that ratios) of the oscillators here
void MoveOscillators() {
  osci[0] = osci[0] + random(5,8);
  osci[1] = osci[1] + random(2,5);
  osci[2] = osci[2] + random(3,6);
  osci[3] = osci[3] + random(4,7);
  for(int i = 0; i < 4; i++) { 
    p[i] = sin8(osci[i]) / 27;  
  }
}


// scale the brightness of the screenbuffer down
void DimAll(byte value)  
{
  for(int i = 0; i < NUM_LEDS; i++) 
  {
    leds[i].nscale8(value);
  }
}
 
/*
Caleidoscope1 mirrors from source to A, B and C
 
y
 
|       |
|   B   |   C
|_______________
|       |
|source |   A
|_______________ x
 
*/
void Caleidoscope1() {
  for(int x = 0; x < 10 / 2 ; x++) {
    for(int y = 0; y < 10 / 2; y++) {
      leds[XY( 10 - 1 - x, y )] = leds[XY( x, y )];              // copy to A
      leds[XY( x, 10 - 1 - y )] = leds[XY( x, y )];             // copy to B
      leds[XY( 10 - 1 - x, 10 - 1 - y )] = leds[XY( x, y )]; // copy to C
    }
  }
}
 
/*
Caleidoscope2 rotates from source to A, B and C
 
y
 
|       |
|   C   |   B
|_______________
|       |
|source |   A
|_______________ x
 
*/
void Caleidoscope2() {
  for(int x = 0; x < 10 / 2 ; x++) {
    for(int y = 0; y < 10 / 2; y++) {
      leds[XY( 10 - 1 - x, y )] = leds[XY( y, x )];    // rotate to A
      leds[XY( 10 - 1 - x, 10 - 1 - y )] = leds[XY( x, y )];    // rotate to B
      leds[XY( x, 10 - 1 - y )] = leds[XY( y, x )];    // rotate to C
    }
  }
}
 
// adds the color of one quarter to the other 3
void Caleidoscope3() {
  for(int x = 0; x < 10 / 2 ; x++) {
    for(int y = 0; y < 10 / 2; y++) {
      leds[XY( 10 - 1 - x, y )] += leds[XY( y, x )];    // rotate to A
      leds[XY( 10 - 1 - x, 10 - 1 - y )] += leds[XY( x, y )];    // rotate to B
      leds[XY( x, 10 - 1 - y )] += leds[XY( y, x )];    // rotate to C
    }
  }
}
 
// add the complete screenbuffer 3 times while rotating
void Caleidoscope4() {
  for(int x = 0; x < 10 ; x++) {
    for(int y = 0; y < 10 ; y++) {
      leds[XY( 10 - 1 - x, y )] += leds[XY( y, x )];    // rotate to A
      leds[XY( 10 - 1 - x, 10 - 1 - y )] += leds[XY( x, y )];    // rotate to B
      leds[XY( x, 10 - 1 - y )] += leds[XY( y, x )];    // rotate to C
    }
  }
}
 
// create a square twister
// x and y for center, r for radius
void SpiralStream(int x,int y, int r, byte dimm) {  
  for(int d = r; d >= 0; d--) {                // from the outside to the inside
    for(int i = x-d; i <= x+d; i++) {
       leds[XY(i,y-d)] += leds[XY(i+1,y-d)];   // lowest row to the right
       leds[XY(i,y-d)].nscale8( dimm );}
    for(int i = y-d; i <= y+d; i++) {
       leds[XY(x+d,i)] += leds[XY(x+d,i+1)];   // right colum up
       leds[XY(x+d,i)].nscale8( dimm );}
    for(int i = x+d; i >= x-d; i--) {
       leds[XY(i,y+d)] += leds[XY(i-1,y+d)];   // upper row to the left
       leds[XY(i,y+d)].nscale8( dimm );}
    for(int i = y+d; i >= y-d; i--) {
       leds[XY(x-d,i)] += leds[XY(x-d,i-1)];   // left colum down
       leds[XY(x-d,i)].nscale8( dimm );}
  }
}
 
// give it a linear tail to the side
void HorizontalStream(byte siz)  
{
  for(int x = 1; x < 10 ; x++) {
    for(int y = 0; y < 10; y++) {
      leds[XY(x,y)] += leds[XY(x-1,y)];
      leds[XY(x,y)].nscale8( siz );
    }
  }
  for(int y = 0; y < 10; y++) 
    leds[XY(0,y)].nscale8(siz);
}
 
// give it a linear tail downwards
void VerticalStream(byte siz)  
{
  for(int x = 0; x < 10 ; x++) {
    for(int y = 1; y < 10; y++) {
      leds[XY(x,y)] += leds[XY(x,y-1)];
      leds[XY(x,y)].nscale8( siz );
    }
  }
  for(int x = 0; x < 10; x++) 
    leds[XY(x,0)].nscale8(siz);
}

// give it a linear tail downwards
void VerticalStreamDown(byte siz)  
{
  for(int x = 0; x < 10 ; x++) {
    for(int y = 1; y < 10; y++) {
      leds[XY(x,y)] += leds[XY(x,y+1)];
      leds[XY(x,y)].nscale8( siz );
    }
  }
  for(int x = 0; x < 10; x++) 
    leds[XY(x,0)].nscale8(siz);
}

void volcheck(){
 int val = analogRead(21);
  val = map(val, 0, 1023, 0, 15);
  audioShield.lineInLevel(7);
}


void FillNoise16() {
  for(int i = 0; i < kMatrixWidth; i++) {
    int ioffset = scale * i;
    for(int j = 0; j < kMatrixHeight; j++) {
      int joffset = scale * j;
      noise[i][j] = inoise16(x + ioffset, y + joffset, z);
    }
  }
}

void FillNoise8() {
  for(int i = 0; i < kMatrixWidth; i++) {
    int ioffset = scale * i;
    for(int j = 0; j < kMatrixHeight; j++) {
      int joffset = scale * j;
      noise[i][j] = inoise8(x + ioffset, y + joffset, z);
    }
  }
}


void SetupBlackAndWhiteStripedPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid( currentPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;

}

void SetupHotFirePalette()
{
  CRGB one = CHSV( 0, 255, 255);
  CRGB two  = CHSV( 16, 255, 255);
  CRGB three = CHSV( 24, 255, 255);
  CRGB four = CHSV( 32, 255, 255);
  CRGB five = CHSV( 48, 255, 255);
  CRGB six  = CHSV( 64, 255, 255);
  CRGB seven = CHSV( 64, 127, 255);
  CRGB eight = CHSV( 64, 63, 255);
  CRGB black  = CRGB::Black;
  
  currentPalette = CRGBPalette16( 
    black,  black,  black,  black,
    black, black, one,  one,
    one,  two,  three,  four,
    five, seven, eight,  eight );
}

void SetupColdFirePalette()
{
  CRGB one = CHSV( 96, 255, 255);
  CRGB two  = CHSV( 80, 255, 255);
  CRGB three = CHSV( 64, 255, 255);
  CRGB four = CHSV( 48, 255, 255);
  CRGB five = CHSV( 32, 255, 255);
  CRGB six  = CHSV( 16, 255, 255);
  CRGB seven = CHSV( 0, 255, 255);
  CRGB eight = CHSV( 0, 255, 255);
  CRGB black  = CRGB::Black;
  
  currentPalette = CRGBPalette16( 
    black,  black,  black,  black,
    black, black, black,  black,
    one,  two,  three,  four,
    five, six, seven,  eight );
}

void SetupCandyPalette()
{
  CRGB one = CRGB::DarkViolet;
  CRGB two  = CRGB::DarkViolet;
  CRGB three = CRGB::DeepPink;
  CRGB four = CRGB::DeepPink;
  CRGB five = CRGB::DeepSkyBlue;
  CRGB six  = CRGB::DeepSkyBlue;
  CRGB seven = CRGB::DeepSkyBlue;
  CRGB eight = CRGB::DeepSkyBlue;
  CRGB black  = CRGB::Black;
  
  currentPalette = CRGBPalette16( 
    black,  black,  black,  black,
    black, black, black,  black,
    one,  two,  three,  four,
    five, six, seven,  eight );
}

void SetupSkyPalette()
{
  CRGB one = CHSV( 167, 255, 255);
  CRGB two  = CHSV( 164, 219, 255);
  CRGB three = CHSV( 161, 183, 255);
  CRGB four = CHSV( 158, 147, 255);
  CRGB five = CHSV( 155, 111, 255);
  CRGB six  = CHSV( 152, 75, 255);
  CRGB seven = CHSV( 149, 39, 255);
  CRGB eight = CHSV( 146, 0, 255);
  CRGB black  = CRGB::Black;
  
  currentPalette = CRGBPalette16( 
    black,  black,  black,  black,
    black, black, one,  one,
    one,  two,  three,  four,
    five, seven, eight,  eight );
}


void SetupFireBandPalette()
{
  CRGB one = CHSV( 0, 255, band[0]*8000);
  CRGB two  = CHSV( 0, 255, band[1]*8000);
  CRGB three = CHSV( 24, 255, band[2]*8000);
  CRGB four = CHSV( 28, 255, band[3]*8000);
  CRGB five = CHSV( 46, 255, band[4]*8000);
  CRGB six  = CHSV( 50, 255, band[5]*8000);
  CRGB seven = CHSV( 0, 0, band[6]*8000);
  CRGB eight = CHSV( 0, 0, band[7]*8000);
  CRGB nine = CHSV( 0, 0, band[8]*8000);
  CRGB ten = CHSV( 0, 0, band[9]*8000);
  CRGB black  = CRGB::Black;
  
  currentPalette = CRGBPalette16( 
    black,  black,  black,  black,
    black, black, one,  two,
    three,  four,  five,  six,
    seven, eight, nine,  ten );
}

void SetupFunkyPalette()
{
  CRGB one = CHSV( 64, 255, 255);
  CRGB two  = CHSV( 64, 255, 255);
  CRGB three = CHSV( 64, 255, 255);
  CRGB four = CHSV( 160, 255, 255);
  CRGB five = CHSV( 160, 255, 255);
  CRGB six  = CHSV( 160, 255, 255);
  CRGB seven = CHSV( 160, 255, 255);
  CRGB eight = CHSV( 160, 255, 255);
  CRGB black  = CRGB::Black;
  
  currentPalette = CRGBPalette16( 
    black,  black,  black,  black,
    black, black, black,  black,
    one,  two,  three,  four,
    five, six, seven,  eight );
}

void SetupBlurplePalette()
{
  CRGB one = CHSV( 192, 255, 255);
  CRGB two  = CHSV( 192, 255, 255);
  CRGB three = CHSV( 192, 255, 255);
  CRGB four = CHSV( 160, 255, 255);
  CRGB five = CHSV( 160, 255, 255);
  CRGB six  = CHSV( 160, 255, 255);
  CRGB seven = CHSV( 160, 255, 255);
  CRGB eight = CHSV( 160, 255, 255);
  CRGB black  = CRGB::Black;
  
  currentPalette = CRGBPalette16( 
    black,  black,  black,  black,
    black, black, black,  black,
    one,  two,  three,  four,
    five, six, seven,  eight );
}

void SetupBandPalette(){
  int high = 0;
  int mid2 = 0;
  int mid1 = 0;
  int low = 0;
  if (band[7] > 0.06){
    high = band[7]*1000;}
  if (band[5] > 0.06){
    mid2 = band[5]*1000;}
  if (band[3] > 0.06){
    mid1 = band[3]*1000;}
  if (band[1] > 0.06){
    low = band[1]*1000;} 
   
                      
  CRGB one = CHSV( 96, 0, high);
  CRGB two  = CHSV( 160, 255, mid2);
  CRGB three = CHSV( 20, 220, mid1);
  CRGB four = CHSV( 0, 255, low);  
  CRGB black  = CRGB::Black;
  
  currentPalette = CRGBPalette16( 
    one,  one,  one,  one,
    two, two, two,  three,
    three,  three,  four,  four,
    four, four, four,  four );
}

void SetupRandomPalette()
{int ran = random8();

                      
  CRGB one = CHSV( ran+90, 255, 255);
  CRGB two  = CHSV( ran+45, 255, 255);
  CRGB three = CHSV( ran+24, 255, 255);
  CRGB four = CHSV( ran, 255, 255);
  CRGB five = CHSV( ran, 255, 255);
  CRGB six  = CHSV( ran-24, 255, 255);
  CRGB seven = CHSV( ran-45, 255, 255);
  CRGB eight = CHSV( ran-90, 255, 255);
  CRGB black  = CRGB::Black;
  
  currentPalette = CRGBPalette16( 
    black,  black,  black,  black,
    black, black, one,  one,
    one,  two,  three,  four,
    five, six, seven,  eight );
}

void SetupRandomPalette2()
{
  currentPalette = CRGBPalette16( 
                      
                      CHSV( random8(), 255, 0), 
                      CHSV( random8(), 255, 255), 
                      CHSV( random8(), 255, 255), 
                      CHSV( random8(), 255, 255)); 
}

#define HOLD_PALETTES_X_TIMES_AS_LONG 2
void ChangePaletteAndSettingsPeriodically()
{
  uint8_t secondHand = ((millis() / 1000) / HOLD_PALETTES_X_TIMES_AS_LONG) % 60;
  static uint8_t lastSecond = 99;
  
  if( lastSecond != secondHand) {
    lastSecond = secondHand;
    if( secondHand ==  0)  { SetupCandyPalette();            }
    if( secondHand ==  5)  { SetupFunkyPalette();                               }
    if( secondHand == 10)  { SetupColdFirePalette();                            }
    if( secondHand == 15)  { SetupBlurplePalette();                              }
    if( secondHand == 20)  { SetupSkyPalette();                                 }
    if( secondHand == 25)  { SetupRandomPalette();                             }
    if( secondHand == 30)  { SetupRandomPalette();                              }
    if( secondHand == 35)  { SetupRandomPalette();                    }
    if( secondHand == 40)  { SetupRandomPalette();                               }
    if( secondHand == 45)  { SetupRandomPalette();                             }
    if( secondHand == 50)  { SetupRandomPalette();                              }
    if( secondHand == 55)  { SetupRandomPalette();                              }  
  }
}

void mapNoiseToLEDsUsingPalette()
{
  static uint8_t ihue=0;
  
  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      // We use the value at the (i,j) coordinate in the noise
      // array for our brightness, and the flipped value from (j,i)
      // for our pixel's index into the color palette.

      uint8_t index = noise[j][i];
      uint8_t bri =   noise[i][j];

      // if this palette is a 'loop', add a slowly-changing base value
      if( colorLoop) { 
        index += ihue;
      }

      // brighten up, as the color palette itself often contains the 
      // light/dark dynamic range desired
      if( bri > 127 ) {
        bri = 255;
      } else {
        bri = dim8_raw( bri * 2);
      }

      CRGB color = ColorFromPalette( currentPalette, index, bri);
      leds[XY(i,j)] = color;
    }
  }
  
  
  ihue+=1;
}

void mapNoiseToLEDsUsingPalette2()
{
  static uint8_t ihue=0;
  
  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      // We use the value at the (i,j) coordinate in the noise
      // array for our brightness, and the flipped value from (j,i)
      // for our pixel's index into the color palette.

      uint8_t index = (noise[j][i]/2)+(band[1]+(band[6]/4))*140;
      uint8_t bri =   (noise[i][j]/2)+(band[1]+(band[6]/4))*140;

      // if this palette is a 'loop', add a slowly-changing base value
      if( colorLoop) { 
        index += ihue;
      }

      // brighten up, as the color palette itself often contains the 
      // light/dark dynamic range desired
      if( bri > 200 ) {
        bri = 255;
      } else {
        bri = dim8_raw( bri * 3);
      }

      CRGB color = ColorFromPalette( currentPalette, index, bri);
      leds[XY(i,j)] = color;
    }
  }
  
  ihue+=1;
}

void loop(){//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 //Noise2();
 //Audio10();
 //AutoRun();
 //
 Noisy();
 //New();
 //Serial.println(LEDS.getFPS());
 
}//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


 
// basedrum/snare linked to red/green emitters
void Audio1() {
  volcheck();
ReadAudio();
if (band[0] > sensitivity*4) {leds[XY(4,4)] = CHSV (0 , 255, band[0]*1024);}
if (band[1] > sensitivity*4) {leds[XY(3,4)] = CHSV (0 , 255, band[1]*1024);}
if (band[2] > sensitivity*4) {leds[XY(3,3)] = CHSV (225 , 255, band[2]*1024);}
if (band[3] > sensitivity*4) {leds[XY(2,3)] = CHSV (240 , 255, band[3]*1024);}
if (band[4] > sensitivity*4) {leds[XY(2,2)] = CHSV (195 , 255, band[4]*1024);}
if (band[5] > sensitivity*4) {leds[XY(1,2)] = CHSV (220 , 255, band[5]*1024);}
if (band[6] > sensitivity*4) {leds[XY(1,1)] = CHSV (165 , 255, band[6]*1024);}
if (band[7] > sensitivity*4) {leds[XY(0,1)] = CHSV (200 , 255, band[7]*1024);}
if (band[8] > sensitivity*4) {leds[XY(0,0)] = CHSV (140 , 255, band[8]*1024);}
VerticalStream(60);
Caleidoscope1();
ShowFrame();
//DimAll(60);
} 

// geile Scheiße
// spectrum mandala, color linked to 160Hz band
void Audio2() {
  volcheck();
  //delay(2);
MoveOscillators();
ReadAudio();

  if (band[0] > sensitivity){
Pixel(0, 6-band[0]*7, 256-band[0]*256,255,255);}
 if (band[2] > sensitivity){
Pixel(1, 6-band[1]*7, 256-band[1]*256,255,255);}
 if (band[4] > sensitivity){
Pixel(2, 6-band[4]*7, 256-band[4]*256,255,255);}
 if (band[6] > sensitivity){
Pixel(3, 6-band[6]*7, 256-band[6]*256,255,255);}
 if (band[8] > sensitivity){
Pixel(4, 6-band[8]*7, 256-band[8]*256,255,255);}

VerticalStream(60);  

Caleidoscope1();
ShowFrame();
DimAll(60);
}


void Audio3() {
  volcheck();
  
ReadAudio();
for(int i = 0; i < 10; i++) {
leds[XY(10-i,band[i]*9)] = CHSV(100-i*i, 255, band[i]*1024);} // brightness should be divided by 4
//Caleidoscope6();
Caleidoscope2();
//SpiralStream(4, 4, 3, 120);
ShowFrame();
DimAll(60);

}


// spectrum mandala, color linked to osci
void Audio4() {
  volcheck();
  //delay(4);
MoveOscillators();
ReadAudio();
for(int i = 0; i < 5; i++) {
  if (band[0||1] > 0.02){
Pixel(8-i-band[1]*40,band[1]*20, 0,255,255);}
}
for(int i = 0; i < 5; i++) {
  if (band[6||7] > 0.02){
Pixel(i, 8-band[7]*20, 0,0,255);}
}
//Caleidoscope5();
Caleidoscope2();
//Caleidoscope3();
ShowFrame();
DimAll(60);

}


void Audio5() {
  volcheck();
  ReadAudio();
  if (band[0] > sensitivity*4){
  leds[XY(0,0)] = CHSV (160,255,band[0]*756);
  leds[XY(1,0)] = CHSV (160,255,band[0]*756);
  leds[XY(2,0)] = CHSV (160,255,band[0]*756);
  leds[XY(3,0)] = CHSV (160,255,band[0]*756);
  leds[XY(4,0)] = CHSV (160,255,band[0]*756);
  leds[XY(0,1)] = CHSV (160,255,band[0]*756);
  leds[XY(0,2)] = CHSV (160,255,band[0]*756);
  leds[XY(0,3)] = CHSV (160,255,band[0]*756);
  leds[XY(0,4)] = CHSV (160,255,band[0]*756);
  }
  if (band[1] > sensitivity){
  leds[XY(1,1)] = CHSV (160,187,band[2]*level);
  leds[XY(1,2)] = CHSV (160,187,band[2]*level);
  leds[XY(1,3)] = CHSV (160,187,band[2]*level);
  leds[XY(1,4)] = CHSV (160,187,band[2]*level);
  leds[XY(1,1)] = CHSV (160,187,band[2]*level);
  leds[XY(2,1)] = CHSV (160,187,band[2]*level);
  leds[XY(3,1)] = CHSV (160,187,band[2]*level);
  leds[XY(4,1)] = CHSV (160,187,band[2]*level);
  }
  if (band[2] > sensitivity){
  leds[XY(2,2)] = CHSV (160,127,band[4]*1200);
  leds[XY(2,3)] = CHSV (160,127,band[4]*1200);
  leds[XY(2,4)] = CHSV (160,127,band[4]*1200);
  leds[XY(2,2)] = CHSV (160,127,band[4]*1200);
  leds[XY(3,2)] = CHSV (160,127,band[4]*1200);
  leds[XY(4,2)] = CHSV (160,127,band[4]*1200);}
  if (band[3||4] > sensitivity){
  leds[XY(3,3)] = CHSV (160,67,band[6]*level);
  leds[XY(4,3)] = CHSV (160,67,band[6]*level);  
  leds[XY(3,4)] = CHSV (160,67,band[6]*level);}
  if (band[5||6] > sensitivity){
  leds[XY(4,4)] = CHSV (160,0,band[8]*level);}
//Caleidoscope1();
Caleidoscope2();
ShowFrame();
DimAll(100);

}

// analyzer x 4 (as showed on youtube)
void Audio6() {
  volcheck();
ReadAudio();
for(int i = 0; i < 10; i++) {
Pixel(7-i, 8-band[i]*20, i*20,255-(i*7),255);}
Caleidoscope1();
ShowFrame();
DimAll(120);

}

void Audio7() {
  volcheck();
  MoveOscillators();
  ReadAudio();
  //if(band[0 || 1] > 0.02){//2 lissajous dots red
  leds[XY(p[0],p[1]/4)] = CHSV (0 , 255, (band[0]+band[1])*300);
  //if(band[0||1] > 0.02){
  leds[XY(p[2],p[3]/4)] = CHSV (0 , 255, (band[0]+band[1])*300);   
  if (band[4||5] > 0.02) {
  Pixel((p[2]+p[0])/2, (p[1]+p[3])/2, 50,255,(band[4]+band[5])*1000);}
  

  ShowFrame();
  
  VerticalStream(20+(band[1]+band[3])*700);
  
}

/*void Audio8() {
  volcheck();
ReadAudio();
for(int i = 0; i < 10; i++) {
Pixel(i, (band[i]*20), 96-band[i]*240,255,255);}
//for(int i = 0; i < 10; i++) {
//Pixel(9-i, 0, 0);}
ShowFrame();
DimAll(0);

}*/
void Audio8() {
volcheck();
  ReadAudio(); 
  if (band[0] > 0.04){
  Line(0, 0, 0, band[0]*9, 96-band[0]*120);}
  if (band[1] > 0.04){
  Line(1, 0, 1, band[1]*9, 96-band[1]*120);}
  if (band[2] > 0.04){
  Line(2, 0, 2, band[2]*9, 96-band[2]*120);}
  if (band[3] > 0.04){
  Line(3, 0, 3, band[3]*9, 96-band[3]*120);}
  if (band[4] > 0.04){
  Line(4, 0, 4, band[4]*9, 96-band[4]*120);}
  if (band[5] > 0.04){
  Line(5, 0, 5, band[5]*9, 96-band[5]*120);}
  if (band[6] > 0.04){
  Line(6, 0, 6, band[6]*9, 96-band[6]*120);}
  if (band[7] > 0.04){
  Line(7, 0, 7, band[7]*9, 96-band[7]*120);}
  if (band[8] > 0.04){
  Line(8, 0, 8, band[8]*9, 96-band[8]*120);}
  if (band[9] > 0.04){
  Line(9, 0, 9, band[9]*9, 96-band[9]*120);}
  //Caleidoscope2();
  ShowFrame();
  DimAll(100);
  ShowFrame();
}

void Audio9() {
  volcheck();
MoveOscillators();
ReadAudio();
for(int i = 0; i < 10; i++) {
Pixel( i, (band[i]*9), osci[1],255,255);}
//for(int i = 0; i < 10; i++) {
//Pixel(9-i, 0, 0);}
ShowFrame();
DimAll(0);

}

void Audio10() {
  volcheck();
MoveOscillators();
ReadAudio();
for(int i = 0; i < 10; i++) {
Pixel(i,5+band[i]*4, 96+band[i]*300,255,255);}
for(int i = 0; i < 10; i++) {
Pixel(i,5-band[i]*4, 96+band[i]*300,255,255);}
//for(int i = 0; i < 10; i++) {
//Pixel(9-i, 0, 0);}
ShowFrame();
DimAll(100);

}

void Audio11() {
  volcheck();
  ReadAudio(); 
  int c = 0;
  if (band[8] > 0.04){
  Line(5 - (band[8]*5), 9, 5 + (band[8]*5), 9, 80 + c);}
  if (band[6] > 0.04){
  Line(5 - (band[6]*5), 8, 5 + (band[6]*5), 8, 60 + c);}
  if (band[4] > 0.04){
  Line(5 - (band[4]*5), 7, 5 + (band[4]*5), 7, 40 + c);}
  if (band[2] > 0.04){
  Line(5 - (band[2]*5), 6, 5 + (band[2]*5), 6, 20 + c);}
  if (band[0] > 0.04){
  Line(5 - (band[0]*5), 5, 5 + (band[0]*5), 5, 0 + c);}
  if (band[0] > 0.04){
  Line(5 - (band[0]*5), 4, 5 + (band[0]*5), 4, 0 + c);}
  if (band[2] > 0.04){
  Line(5 - (band[2]*5), 3, 5 + (band[2]*5), 3, 20 + c);}
  if (band[4] > 0.04){
  Line(5 - (band[4]*5), 2, 5 + (band[4]*5), 2, 40 + c);}
  if (band[6] > 0.04){
  Line(5 - (band[6]*5), 1, 5 + (band[6]*5), 1, 60 + c);}
  if (band[8] > 0.04){
  Line(5 - (band[8]*5), 0, 5 + (band[8]*5), 0, 80 + c);}
  //Caleidoscope2();
  ShowFrame();
  DimAll(100);
  ShowFrame();
}

void Audio12() {
  volcheck();
  ReadAudio(); 
 // Apply some blurring to whatever's already on the matrix
  // Note that we never actually clear the matrix, we just constantly
  // blur it repeatedly.  Since the blurring is 'lossy', there's
  // an automatic trend toward black -- by design.
  uint8_t blurAmount = beatsin8(2,10,90);
  blur2d( leds, kMatrixWidth, kMatrixHeight, blurAmount);

  // Use two out-of-sync sine waves
  uint8_t  i = beatsin8( 60, kBorderWidth, kMatrixHeight-kBorderWidth);
  uint8_t  j = beatsin8( 45, kBorderWidth, kMatrixWidth-kBorderWidth);
  // Also calculate some reflections
  uint8_t ni = (kMatrixWidth-1)-i;
  uint8_t nj = (kMatrixWidth-1)-j;
  
  // The color of each point shifts over time, each at a different speed.
  uint16_t ms = millis(); 
  if (band[0] > 0.1){ 
  leds[XY( i, j)] += CHSV( 0 + (ms/11), 255, band[0]*250);}
  if (band[1] > 0.1){
  leds[XY( j, i)] += CHSV( 0 + (ms/11), 255, band[1]*250);}
  if (band[2] > 0.1){
  leds[XY(ni,nj)] += CHSV( 0 + (ms/11), 255, band[4]*250);}
  if (band[4] > 0.1){
  leds[XY(nj,ni)] += CHSV( 120 + (ms/11), 255, band[5]*250);}
  if (band[5] > 0.1){
  leds[XY( i,nj)] += CHSV( 120 + (ms/11), 255, band[6]*250);}
  if (band[6] > 0.1){
  leds[XY(ni, j)] += CHSV( 120 + (ms/11), 255, band[7]*250);}
  ShowFrame();
}
// falling spectogram
void Noise1() {
  volcheck();
  ReadAudio();  
  scale = 65;
  x = x + band[1]*20;
  y = y + band[1]*40;
  if (band[4||5||6] > 0.06) z=z+1;
  if (band[7||8||9] > 0.06) z=z+1;
  else{ z=z-2;}
  FillNoise16();
  SetupBandPalette();
  colorLoop = 0;
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope1();
  ShowFrame();
}

void Noise2() {
  volcheck();
  ReadAudio();  
  scale = 60;  
  if (band[4||5||6] > 0.04) z=z+2;
  if (band[7||8||9] > 0.04) z=z+1;
  else{ z=z-2;}
  FillNoise8();
  SetupFunkyPalette();
  colorLoop = 0;  
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope1();
  ShowFrame();
}
 
void Noise3() {
  volcheck();
  ReadAudio();  
  scale = 50;  
  if (band[4||5||6] > 0.04) z=z+2;
  if (band[7||8||9] > 0.04) z=z+1;
  else{ z=z-2;}
  FillNoise8();
  ChangePaletteAndSettingsPeriodically();
  colorLoop = 0;
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope2();
  ShowFrame();
}

void Noise4() {
  volcheck();
  ReadAudio();  
  scale = 40;  
  if (band[4||5||6] > 0.04) z=z+2;
  if (band[7||8||9] > 0.04) z=z+1;
  else{ z=z-2;}
  FillNoise8();
  ChangePaletteAndSettingsPeriodically();
  colorLoop = 0;
  mapNoiseToLEDsUsingPalette2();
  ShowFrame();
}
 
void Noise5() {
  volcheck();
  ReadAudio();  
  scale = 180; 
  x = x + band[1]*20;
  y = y + band[1]*40; 
  if (band[4||5||6] > 0.06) z=z-1;
  if (band[7||8||9] > 0.06) z=z-1;
  else{ z=z-2;}  
  FillNoise16();
  ChangePaletteAndSettingsPeriodically();
  colorLoop = 0;
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope2();
  ShowFrame();
}

void Noise6() {
  volcheck();
  ReadAudio();  
  scale = 80 - (band[1]*100);  
  if (band[4||5||6] > 0.06) z=z+3;
  if (band[7||8||9] > 0.06) z=z+1;
  else{ z=z-2;}  
  FillNoise8();
  ChangePaletteAndSettingsPeriodically();
  colorLoop = 0;
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope2();
  ShowFrame();
}

void Noise7() {
  volcheck();
  ReadAudio();  
  scale = 80 - (band[1]*100);  
  if (band[4||5||6] > 0.06) z=z+3;
  if (band[7||8||9] > 0.06) z=z+1;
  else{ z=z-2;}  
  FillNoise8();
  ChangePaletteAndSettingsPeriodically();
  colorLoop = 0;
  mapNoiseToLEDsUsingPalette2();
  //Caleidoscope1();
  ShowFrame();
}

void Noise8() {
  volcheck();
  ReadAudio();  
  scale = 100 - (band[1]*100);  
  if (band[4||5||6] > 0.06) z=z+3;
  if (band[7||8||9] > 0.06) z=z+1;
  else{ z=z-2;}  
  FillNoise16();
  ChangePaletteAndSettingsPeriodically();
  colorLoop = 0;
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope1();
  ShowFrame();
}

void Noise9() {
  volcheck();
  ReadAudio();
  x = x + band[1]*20;
  y = y + band[1]*40;
  scale = 60;  
  if (band[4||5||6] > 0.06) z=z-1;
  if (band[7||8||9] > 0.06) z=z-1;
  else{ z=z-2;}  
  FillNoise8();
  ChangePaletteAndSettingsPeriodically();
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope2();
  ShowFrame();
}

void Noise10() {
  volcheck();
  ReadAudio();
  x = x + band[1]*80;
  y = y + band[1]*80;
  scale = 400;  
  if (band[4||5||6] > 0.06) z=z-1;
  if (band[7||8||9] > 0.06) z=z-1;
  else{ z=z-2;}  
  FillNoise16();
  ChangePaletteAndSettingsPeriodically();
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope1();
  ShowFrame();
}

void Noise11() {
  volcheck();
  ReadAudio();
  x = x + band[1]*80;
  y = y + band[1]*80;
  scale = 60; 
  
  
  if (band[4||5||6] > 0.06) z=z-1;
  if (band[7||8||9] > 0.06) z=z-1;
  else{ z=z-2;}  
  FillNoise8();
  ChangePaletteAndSettingsPeriodically();
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope2();
  ShowFrame();
}


void AutoRun() {
  
  for(int i = 0; i < 2000; i++) {Audio1();} 
  for(int i = 0; i < 2000; i++) {Audio2();}
  //for(int i = 0; i < 2000; i++) {Audio3();}
  for(int i = 0; i < 2000; i++) {Audio8();}
  for(int i = 0; i < 300; i++) {Audio9();}
  for(int i = 0; i < 2000; i++) {Audio4();}
  for(int i = 0; i < 2000; i++) {Audio5();}
  //for(int i = 0; i < 2000; i++) {Audio6();}
  //for(int i = 0; i < 2000; i++) {Audio7();}  
  for(int i = 0; i < 2000; i++) {Audio10();}
  for(int i = 0; i < 2000; i++) {Audio11();}
  for(int i = 0; i < 2000; i++) {Audio12();}
  for(int i = 0; i < 2000; i++) {Noise1();}
  for(int i = 0; i < 2000; i++) {Noise2();}
  for(int i = 0; i < 2000; i++) {Noise3();}
  for(int i = 0; i < 2000; i++) {Noise4();}
  for(int i = 0; i < 2000; i++) {Noise5();}
  for(int i = 0; i < 2000; i++) {Noise6();}
  for(int i = 0; i < 2000; i++) {Noise7();}
  for(int i = 0; i < 2000; i++) {Noise8();}
  for(int i = 0; i < 2000; i++) {Noise9();}
  for(int i = 0; i < 2000; i++) {Noise10();}
  for(int i = 0; i < 2000; i++) {Noise11();}
  for(int i = 0; i < random(10,4000); i++) {Audio1();} 
  for(int i = 0; i < random(10,4000); i++) {Audio2();}
  //for(int i = 0; i < random(10,4000); i++) {Audio3();}
  for(int i = 0; i < random(10,4000); i++) {Audio8();}
  for(int i = 0; i < 300; i++) {Audio9();}
  for(int i = 0; i < random(10,4000); i++) {Audio4();}
  for(int i = 0; i < random(10,4000); i++) {Audio5();}
  //for(int i = 0; i < random(10,4000); i++) {Audio6();}
  //for(int i = 0; i < random(10,4000);; i++) {Audio7();}  
  for(int i = 0; i < random(10,4000); i++) {Audio10();}
  for(int i = 0; i < random(10,4000); i++) {Audio11();}
  for(int i = 0; i < random(10,4000); i++) {Audio12();}
  for(int i = 0; i < random(10,4000); i++) {Noise1();}
  for(int i = 0; i < random(10,4000); i++) {Noise2();}
  for(int i = 0; i < random(10,4000); i++) {Noise3();}
  for(int i = 0; i < random(10,4000); i++) {Noise4();}
  for(int i = 0; i < random(10,4000); i++) {Noise5();}
  for(int i = 0; i < random(10,4000); i++) {Noise6();}
  for(int i = 0; i < random(10,4000); i++) {Noise7();}
  for(int i = 0; i < random(10,4000); i++) {Noise8();}
  for(int i = 0; i < random(10,4000); i++) {Noise9();}
  for(int i = 0; i < random(10,4000); i++) {Audio1();} 
  for(int i = 0; i < random(10,4000); i++) {Audio2();}
  //for(int i = 0; i < random(10,4000); i++) {Audio3();}
  for(int i = 0; i < random(10,4000); i++) {Audio8();}
  for(int i = 0; i < 300; i++) {Audio9();}
  for(int i = 0; i < random(10,4000); i++) {Audio4();}
  for(int i = 0; i < random(10,4000); i++) {Audio5();}
  //for(int i = 0; i < random(10,4000); i++) {Audio6();}
  //for(int i = 0; i < random(10,4000);; i++) {Audio7();}  
  for(int i = 0; i < random(10,4000); i++) {Audio10();}
  for(int i = 0; i < random(10,4000); i++) {Audio11();}
  for(int i = 0; i < random(10,4000); i++) {Audio12();}
  for(int i = 0; i < random(10,4000); i++) {Noise1();}
  for(int i = 0; i < random(10,4000); i++) {Noise2();}
  for(int i = 0; i < random(10,4000); i++) {Noise3();}
  for(int i = 0; i < random(10,4000); i++) {Noise4();}
  for(int i = 0; i < random(10,4000); i++) {Noise5();}
  for(int i = 0; i < random(10,4000); i++) {Noise6();}
  for(int i = 0; i < random(10,4000); i++) {Noise7();}
  for(int i = 0; i < random(10,4000); i++) {Noise8();}
  for(int i = 0; i < random(10,4000); i++) {Noise9();}
  
  

}

void Noisy() {
  
  
  for(int i = 0; i < 8000; i++) {Noise1();}
  for(int i = 0; i < 8000; i++) {Noise2();}
  for(int i = 0; i < 8000; i++) {Noise3();}
  for(int i = 0; i < 8000; i++) {Noise4();}
  for(int i = 0; i < 8000; i++) {Noise5();}
  for(int i = 0; i < 8000; i++) {Noise6();}
  //for(int i = 0; i < 4000; i++) {Noise7();}
  for(int i = 0; i < 8000; i++) {Noise8();}
  for(int i = 0; i < 8000; i++) {Noise9();}
  for(int i = 0; i < 8000; i++) {Noise10();}
  for(int i = 0; i < 8000; i++) {Noise11();}
   
}

void New() {
Noise11();
}

void ShowFrame() {
  // when using a matrix different than 16*16 use RenderCustomMatrix();
  //RenderCustomMatrix();
  FastLED.show();
}

