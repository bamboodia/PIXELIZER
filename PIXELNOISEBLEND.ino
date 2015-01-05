#include <FastLED.h>
#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>

#define DATA_PIN  1
#define CLOCK_PIN 2
#define CHIPSET   WS2801
#define COLRORDER RGB
#define BRIGHTNESS 180
#define PROGLENGTH 4000
#define CHNGSPEED 2
#define HOLD_PALETTES_X_TIMES_AS_LONG 3
#define UPDATES_PER_SECOND 120

//AUDIO//
const int myInput = AUDIO_INPUT_LINEIN;
AudioInputI2S          audioInput;         // audio shield: mic or line-in
AudioAnalyzeFFT256    myFFT;
AudioConnection patchCord1(audioInput, 0, myFFT, 0);
AudioControlSGTL5000 audioShield;

//MATRIX//
const uint8_t kMatrixWidth  = 10;
const uint8_t kMatrixHeight = 10;
const uint8_t kBorderWidth = 1;
static uint16_t x;
static uint16_t y;
static uint16_t z;
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
const bool    kMatrixSerpentineLayout = true;
CRGB leds[NUM_LEDS];
uint16_t speed = 0; // speed is set dynamically once we've started up
uint16_t scale = 2000; // scale is set dynamically once we've started up
#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight)
uint8_t noise[MAX_DIMENSION][MAX_DIMENSION];
CRGBPalette16 currentPalette( LavaColors_p );
CRGBPalette16 targetPalette( PartyColors_p );
uint8_t       colorLoop = 0;

byte osci[4]; 
byte p[4];
float band[10];

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
  
  AudioMemory(20);
  Serial.begin(115200);
  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.audioPreProcessorEnable();
  //audioShield.lineInLevel(lvl);
  audioShield.autoVolumeControl(2, 0, 0, 0.6, 0.1, 0.01);
  audioShield.autoVolumeEnable();
   
  LEDS.addLeds<CHIPSET, DATA_PIN, CLOCK_PIN, COLRORDER>(leds,NUM_LEDS); /*SPI CHIPSET (4 WIRE) */
  //LEDS.addLeds<CHIPSET, DATA_PIN, COLRORDER>(leds,NUM_LEDS); /* NON-SPI CHIPSET (3 WIRE) */
  LEDS.setBrightness(BRIGHTNESS);
  // x = random16();
 // y = random16();
 // z = random16();

}

void volcheck(){
 int val = analogRead(21);
  val = map(val, 0, 1023, 0, 15);
  audioShield.lineInLevel(15);
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

void DimAll(byte value)  
{
  for(int i = 0; i < NUM_LEDS; i++) 
  {
    leds[i].nscale8(value);
  }
}

void Caleidoscope1() {
  for(int x = 0; x < kMatrixWidth / 2 ; x++) {
    for(int y = 0; y < kMatrixHeight / 2; y++) {
      leds[XY( kMatrixWidth - 1 - x, y )] = leds[XY( y, x )];    
      leds[XY( kMatrixWidth - 1 - x, kMatrixHeight - 1 - y )] = leds[XY( x, y )];    
      leds[XY( x, kMatrixHeight - 1 - y )] = leds[XY( y, x )];    
    }
  }
}
 
void Caleidoscope2() {
  for(int x = 0; x < kMatrixWidth / 2 ; x++) {
    for(int y = 0; y < kMatrixHeight / 2; y++) {
      leds[XY( kMatrixWidth - 1 - x, y )] = leds[XY( x, y )];              
      leds[XY( x, kMatrixHeight - 1 - y )] = leds[XY( x, y )];             
      leds[XY( kMatrixWidth - 1 - x, kMatrixHeight - 1 - y )] = leds[XY( x, y )]; 
    }
  }
}

void FillNoise16() {
  for(int i = 0; i < kMatrixWidth; i++) {
    int ioffset = scale * i;
    for(int j = 0; j < kMatrixHeight; j++) {
      int joffset = scale * j;
      noise[i][j] = inoise16(x + ioffset, y + joffset);
    }
  }
}

void FillNoise8() {
  for(int i = 0; i < kMatrixWidth; i++) {
    int ioffset = scale * i;
    for(int j = 0; j < kMatrixHeight; j++) {
      int joffset = scale * j;
      noise[i][j] = inoise8(x + ioffset, y + joffset);
    }
  }
}





void ChangePalettePeriodically()
{
  uint8_t secondHand = ((millis() / 1000) / HOLD_PALETTES_X_TIMES_AS_LONG) % 60;
  static uint8_t lastSecond = 99;
  
  if( lastSecond != secondHand) {
    lastSecond = secondHand;
    CRGB r = CHSV( HUE_RED, 255, 255);
    CRGB o = CHSV( HUE_ORANGE, 255, 255);
    CRGB y = CHSV( HUE_YELLOW, 255, 255);
    CRGB g = CHSV( HUE_GREEN, 255, 255);
    CRGB a = CHSV( HUE_AQUA, 255, 255);
    CRGB b = CHSV( HUE_BLUE, 255, 255);
    CRGB p = CHSV( HUE_PURPLE, 255, 255);
    CRGB pi = CHSV( HUE_PINK, 255, 255);
    CRGB be = CRGB::Beige;
    CRGB bl = CRGB::Black;
    CRGB w = CRGB::White;
    CRGB ra = CHSV ( 75, 255, 255);
    if( secondHand ==  0)  { targetPalette = CRGBPalette16( bl,bl,bl,bl, bl,bl,bl,bl, w,w,b,b, b,b,b,bl); }
    if( secondHand ==  5)  { targetPalette = CRGBPalette16( bl,bl,bl,bl, bl,bl,bl,bl, w,a,g,g, g,r,bl,bl); }
    if( secondHand == 10)  { targetPalette = CRGBPalette16( bl,bl,bl,bl, bl,bl,bl,bl, a,p,pi,pi, pi,r,bl,bl); }
    if( secondHand == 15)  { targetPalette = CRGBPalette16( bl,bl,bl,bl, bl,bl,bl,bl, g,a,b,b, b,b,bl,bl); }
    if( secondHand == 20)  { targetPalette = CRGBPalette16( bl,bl,bl,bl, bl,bl,bl,bl, p,p,y,y, bl,bl,bl,bl); }
    if( secondHand == 25)  { targetPalette = CRGBPalette16( bl,bl,bl,bl, bl,bl,bl,bl, a,a,y,y, y,y,bl,bl); }
    if( secondHand == 30)  { targetPalette = CRGBPalette16( bl,bl,bl,bl, bl,bl,bl,bl, b,r,bl,bl, bl,bl,bl,bl); }
    if( secondHand == 35)  { targetPalette = CRGBPalette16( bl,bl,bl,bl, bl,bl,bl,bl, o,o,p,bl, bl,bl,bl,bl); }
    if( secondHand == 40)  { targetPalette = CRGBPalette16( bl,bl,bl,bl, bl,bl,bl,bl, r,o,y,g, bl,bl,bl,bl); }
    if( secondHand == 45)  { targetPalette = CRGBPalette16( bl,bl,bl,bl, bl,bl,bl,bl, b,b,a,ra, ra,bl,bl,bl); }
    if( secondHand == 50)  { targetPalette = CRGBPalette16( bl,bl,bl,bl, bl,bl,bl,bl, r,r,random8(),random8(), bl,bl,bl,bl); }
    if( secondHand == 55)  { targetPalette = CRGBPalette16( bl,bl,bl,bl, bl,bl,bl,bl, b,p,pi,r,bl,bl,bl,bl); }
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

      uint8_t index = (noise[j][i]/2)+(band[1]/2+(band[6]/4))*160;
      uint8_t bri =   (noise[i][j]/2)+(band[1]/2+(band[6]/4))*160;

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

 //Noise1(); 
 //
 Noisy(); 
 Serial.println(LEDS.getFPS());
 
}//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Noise1() {
  volcheck();
  ReadAudio();  
  scale = 55;  
  x = x + band[1]*5;
  y = y + band[1]*10;
  if (band[6] > 0.02) z=z+2;
  if (band[9] > 0.02) z=z+2;
  //else{ z=z-2;}
  FillNoise16();
  ChangePalettePeriodically();
  uint8_t maxChanges = CHNGSPEED; 
  nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);  
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope2();
  FastLED.show();
  

  //FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void Noise2() {
  volcheck();
  ReadAudio();  
  scale = 40;  
  x = x + band[1]*10;
  //y = y ;
  //if (band[4||5||6] > 0.06) z=z+2;
  //if (band[7||8||9] > 0.06) z=z+1;
  //else{ z=z-2;}
  FillNoise8();
  ChangePalettePeriodically();
  uint8_t maxChanges = CHNGSPEED; 
  nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);  
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope2();
  FastLED.show();   
  //FastLED.delay(1000 / UPDATES_PER_SECOND);
}
 
void Noise3() {
  volcheck();
  ReadAudio();  
  scale = 50;  
  x = x + band[1]*10;
  //y = y ;
  //if (band[4||5||6] > 0.06) z=z+2;
  //if (band[7||8||9] > 0.06) z=z+1;
  //else{ z=z-2;}
  FillNoise8();
  ChangePalettePeriodically();
  uint8_t maxChanges = CHNGSPEED; 
  nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);  
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope1();
  FastLED.show();   
  //FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void Noise4() {
  volcheck();
  ReadAudio();  
  scale = 40;  
  x = x + band[1]*12;
  //y = y + band[1]*40;
  if (band[4||5||6] > 0.04) z=z+2;
  if (band[7||8||9] > 0.04) z=z+1;
  //else{ z=z-2;}
  FillNoise8();
  ChangePalettePeriodically();
  uint8_t maxChanges = CHNGSPEED; 
  nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);  
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope1();
  FastLED.show();   
  //FastLED.delay(1000 / UPDATES_PER_SECOND);
}
 
void Noise5() {
  volcheck();
  ReadAudio();  
  scale = 60; 
  x = x + band[0]*10;
  y = y + band[0]*5; 
  //if (band[4||5||6] > 0.06) z=z+1;
  //if (band[7||8||9] > 0.06) z=z+1;
  z=z-2;  
  FillNoise16();
  ChangePalettePeriodically();
  uint8_t maxChanges = CHNGSPEED; 
  nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);  
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope2();
  FastLED.show();   
  //FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void Noise6() {
  volcheck();
  ReadAudio();  
  scale = 140 - (band[1]*100);  
  x = x + band[1]*20;
  y = y + band[1]*5;
  //if (band[4||5||6] > 0.06) z=z+3;
  //if (band[7||8||9] > 0.06) z=z+1;
  //else{ z=z-2;}  
  FillNoise8();
  ChangePalettePeriodically();
  uint8_t maxChanges = CHNGSPEED; 
  nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);  
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope2();
  FastLED.show();   
  //FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void Noise7() {
  volcheck();
  ReadAudio();  
  scale = 40 ;  
  x = x-1 ;
  y = y + band[0]*20;
  if (band[4||5||6] > 0.06) z=z+3;
  if (band[7||8||9] > 0.06) z=z+3;
  z=z-1;  
  FillNoise8();
  ChangePalettePeriodically();
  uint8_t maxChanges = CHNGSPEED; 
  nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);  
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope2();
  FastLED.show();   
  //FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void Noise8() {
  volcheck();
  ReadAudio();  
  scale = 100 - (band[1]*100);  
  x = x + band[1]*5;
  y = y + band[1]*10;
  if (band[4||5||6] > 0.06) z=z+3;
  if (band[7||8||9] > 0.06) z=z+1;
  else{ z=z-2;}  
  FillNoise16();
  ChangePalettePeriodically();
  uint8_t maxChanges = CHNGSPEED; 
  nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);  
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope1();
  FastLED.show();   
  //FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void Noise9() {
  volcheck();
  ReadAudio();
  x = x + band[1]*20;
  y = y + band[1]*10;
  scale = 60;  
  //if (band[4||5||6] > 0.06) z=z-1;
  //if (band[7||8||9] > 0.06) z=z-1;
  z=z-2;  
  FillNoise8();
  ChangePalettePeriodically();
  uint8_t maxChanges = CHNGSPEED; 
  nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);  
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope2();
  FastLED.show();   
  //FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void Noise10() {
  volcheck();
  ReadAudio();
  x = x + band[1]*20;
  y = y + band[1]*20;
  scale = 200;  
  if (band[4||5||6] > 0.06) z=z-1;
  if (band[7||8||9] > 0.06) z=z-1;
  else{ z=z-2;}  
  FillNoise16();
  ChangePalettePeriodically();
  uint8_t maxChanges = CHNGSPEED; 
  nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);  
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope1();
  FastLED.show();   
  //FastLED.delay(1000 / UPDATES_PER_SECOND);
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
  ChangePalettePeriodically();
  uint8_t maxChanges = CHNGSPEED; 
  nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);  
  mapNoiseToLEDsUsingPalette2();
  Caleidoscope2();
  FastLED.show();   
  //FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void Noisy() {
  
  
  for(int i = 0; i < PROGLENGTH; i++) {Noise1();}
  for(int i = 0; i < PROGLENGTH; i++) {Noise2();}
  for(int i = 0; i < PROGLENGTH; i++) {Noise3();}
  for(int i = 0; i < PROGLENGTH; i++) {Noise4();}
  for(int i = 0; i < PROGLENGTH; i++) {Noise5();}
  for(int i = 0; i < PROGLENGTH; i++) {Noise6();}
  for(int i = 0; i < PROGLENGTH; i++) {Noise7();}
  for(int i = 0; i < PROGLENGTH; i++) {Noise8();}
  for(int i = 0; i < PROGLENGTH; i++) {Noise9();}
  for(int i = 0; i < PROGLENGTH; i++) {Noise10();}
  //for(int i = 0; i < 1000; i++) {Noise11();}
   
}

void ShowFrame() {
  // when using a matrix different than 16*16 use RenderCustomMatrix();
  //RenderCustomMatrix();
  FastLED.show();
}
