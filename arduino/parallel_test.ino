#include <Adafruit_NeoPixel.h>


#define PIN 6
#define NUMPIXELS 233
// PIN 13 is PB7
// PIN 30 is PC7
// portc has reset on pc6, lol

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// target data rate is 28KB/s
// one byte at 28KHz for a burst of 1024 bytes

uint8_t buffer[2048];
uint8_t data;
int counter, i, counter2, currpixel;
int r1, g1, b1;

void setup()
{
  //Serial.begin(115200);
  strip.begin();
  strip.show();
  //Serial.println("begin");
  // output is high, input is low
  DDRB |= 0x80;
  DDRA = 0x00;
  counter2 = 0;
  //DDRC = 0xff; // test data output
  attachInterrupt(0, interrupt, RISING); // pin 2
  
  //PORTC = B00000010; // test output
  pinMode(12, OUTPUT);
  analogWrite(12, 128);
  
}

void loop()
{
  // set ready pin
  PORTB |= 0x80;
  // wait for enough data to come in
  while (counter < 2048) {delayMicroseconds(5);}
  
  PORTB &= 0x7f;
  counter = 0;
  // process data
  for (i=0; i<2048; i++)
  {
    
    if (buffer[i] == 0xff) // reset package
    {
      counter2 = 1;
    } else if (buffer[i] == 0xfe) { // enable strip
      counter2 = 0;
      strip.show();
      //digitalWrite(13, HIGH);
    } else {
      switch (counter2)
      {
        case 1:
          currpixel = buffer[i];
          break;
        case 2:
          r1 = buffer[i];
          break;
        case 3:
          g1 = buffer[i];
          break;
        case 4:
          b1 = buffer[i];
          break;
      }
      if (counter2 > 0) counter2++;
      if (counter2 == 5) // set pixel
      {
        strip.setPixelColor(currpixel, r1, g1, b1);
        counter2 = 0;
        i = 2048; // skip to end
      }
    }
  }
}

void interrupt () // 6.1 us with no content
{
  PORTB &= 0x7f;
  buffer[counter] = PINA;
  counter++;
  PORTB |= 0x80;
}
