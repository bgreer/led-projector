
/*  TEENSY 3.1 CODE  */

#define BUFFERSIZE 2048

#define PIN 6
#define CLOCK 23
#define RDY 2
#define NUMPIXELS 233

int ii, ij;
int currpixel, r1, g1, b1, counter;
int n, count;
int newframes;
float fps;
uint8_t buffer[BUFFERSIZE];
uint32_t lasttime;

uint8_t pinmapping[8] = {3,4,5,6,9,10,20,21};

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(0);
  pinMode(13, OUTPUT);
  ij = 0;
  counter = 0;
  delay(2000);
  Serial.println("prog begin");
  digitalWrite(13, LOW);
  lasttime = millis();
  newframes = 0;
  // set parallel out
  pinMode(CLOCK, OUTPUT);
  for (ii=0; ii<8; ii++)
    pinMode(pinmapping[ii],OUTPUT);
  pinMode(RDY, INPUT);
}

void loop()
{
  int i, index;
  digitalWrite(13, LOW);
  // get some data, wait until buffer fills

  count = 0;
  while (count < BUFFERSIZE)
  {
    n = Serial.readBytes((char*)buffer+count, BUFFERSIZE-count);
    if (n == 0)
      while (!Serial.available());
    count = count + n;
  }
  
  // send data out all parallel-like
  
  // TODO: check for mega-ready
  while (!digitalReadFast(RDY)) {}
  digitalWrite(13, HIGH);
  for (i=0; i<BUFFERSIZE; i++)
  {
    // set clock low
    digitalWriteFast(CLOCK, LOW);
    // set data, takes about 1.125 us
    for (index=0; index<8; index++)
      digitalWriteFast(pinmapping[index], (buffer[i]>>index)&0x01);
    // tick clock
    digitalWriteFast(CLOCK, HIGH);
    delayMicroseconds(10); // tune this to the arduino data rate
    // 1 -> 460 KHz
    // 20 -> 47 KHz
    // 30 -> 32 KHz
  }
  digitalWriteFast(CLOCK, LOW);
  digitalWrite(13, LOW);
  
  // parse the data buffer
  // counter is persistent over loops
  //digitalWrite(13, LOW);
  
  /*
  for (i=0; i<BUFFERSIZE; i++)
  {
    
    if (buffer[i] == 0xff) // reset package
    {
      counter = 1;
    } else if (buffer[i] == 0xfe) { // enable strip
      counter = 0;
      digitalWrite(13, HIGH);
      //strip.show();
      newframes++;
      if (millis() - lasttime > 2000)
      {
        Serial.print("FPS: ");
        fps = newframes / 2.0;
        Serial.println(fps);
        lasttime = millis();
        newframes = 0;
      }
    } else {
      switch (counter)
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
      if (counter > 0) counter++;
      if (counter == 5) // set pixel
      {
        //strip.setPixelColor(currpixel, r1, g1, b1);
        counter = 0;
      }
    }
  }
  */
  //digitalWrite(13, HIGH);
}
