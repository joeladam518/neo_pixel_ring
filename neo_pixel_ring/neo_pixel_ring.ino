#include <Adafruit_NeoPixel.h>

#define NEO_PIN 11     // NeoPixel Pin
#define BUTTON_PIN 10  // Button Pin
#define TWINKLE_PIN 6  // Twinkle Lights Pin

//==============================================================================
// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
// NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
// NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
// NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
// NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, NEO_PIN, NEO_GRB + NEO_KHZ800);
//==============================================================================
// Global Variables

boolean looper = false;
uint8_t sce_counter = 0; // single click event counter

//==============================================================================
// MULTI-CLICK:  One Button, Multiple Events **Variables**

// Button timing variables
int debounce = 20; // ms debounce period to prevent flickering when pressing or releasing the button
int DCgap = 250; // max ms between clicks for a double click event
int holdTime = 1000; // ms hold period: how long to wait for press+hold event
int longHoldTime = 3000; // ms long hold period: how long to wait for press+hold event

// Button variables
boolean btnState = HIGH; // value read from button
boolean prevBtnState = HIGH; // buffered value of the button's previous state
boolean DCwaiting = false; // whether we're waiting for a double click (down)
boolean DConUp = false; // whether to register a double click on next release, or whether to wait and click
boolean singleOK = true; // whether it's OK to do a single click
long downTime = -1; // time the button was pressed down
long upTime = -1; // time the button was released
boolean ignoreUp = false; // whether to ignore the button release because the click+hold was triggered
boolean waitForUp = false; // when held, whether to wait for the up event
boolean holdEventPast = false; // whether or not the hold event happened already
boolean longHoldEventPast = false; // whether or not the long hold event happened already
//==============================================================================

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) 
{
    for(uint16_t i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, c);
        strip.show();
        delay(wait);
    }
}

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) 
{
    if (WheelPos < 85) {
        return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    } else if (WheelPos < 170) {
        WheelPos -= 85;
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    } else {
        WheelPos -= 170;
        return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
}

uint8_t rainbow(uint8_t wait) 
{
    uint16_t i, j;

    for (j = 0; j < 256; j++) {
        for (i = 0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, Wheel((i+j) & 255));
            if(checkButton() > 0) return 1;
        }
        strip.show();
        delay(wait);
    }

    return 0;
}

// Slightly different, this makes the rainbow equally distributed throughout
uint8_t rainbowCycle(uint8_t wait) 
{
    uint16_t i, j;
    for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
        for (i = 0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
            if(checkButton() > 0) return 1;
        }
        strip.show();
        delay(wait);
    }

    return 0;
}


// Theatre-style crawling lights.
uint8_t theaterChase(uint32_t c, uint8_t wait) 
{
    for (int j = 0; j<10; j++) {  // do 10 cycles of chasing
        for (int q = 0; q < 3; q++) {
            checkButton();
            for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
                strip.setPixelColor((i + q), c); // Turn every third pixel on
                if(checkButton() > 0) return 1;
            }
            strip.show();
            delay(wait);
            for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
                strip.setPixelColor((i + q), 0); // Turn every third pixel off
                if(checkButton() > 0) return 1;
            }
        }
    }

    return 0;
}

// Theatre-style crawling lights with rainbow effect
uint8_t theaterChaseRainbow(uint8_t wait) {
    checkButton();
    for (int j = 0; j < 256; j++) { // Cycle all 256 colors in the wheel
        for (int q = 0; q < 3; q++) {
            for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
                strip.setPixelColor((i + q), Wheel((i + j) % 255));    // Turn every third pixel on
                if(checkButton() > 0) return 1;
            }
            strip.show();
            delay(wait);
            for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
                strip.setPixelColor((i + q), 0); // Turn every third pixel off
                if(checkButton() > 0) return 1;
            }
        }
    }

    return 0;
}

int checkButton() {    
    int event = 0;
    
    btnState = digitalRead(BUTTON_PIN);
    // Serial.print("btnState = "); Serial.println(btnState);
    // Serial.print("prevBtnState = "); Serial.println(prevBtnState);

    // Button pressed down
    if (
        btnState == LOW  
    &&  prevBtnState == HIGH 
    && (millis() - upTime) > debounce
    ) {
        Serial.println(__LINE__);
        downTime = millis();
        ignoreUp = false;
        waitForUp = false;
        singleOK = true;
        holdEventPast = false;
        longHoldEventPast = false;
        DCwaiting = false;
        Serial.println(__LINE__);
        if (
            (millis() - upTime) < DCgap 
        &&  DConUp == false 
        &&  DCwaiting == true
        ) {
            DConUp = true;
        } else {
            DConUp = false;
        }
    } else if (
        btnState == HIGH  
    &&  prevBtnState == LOW  
    &&  (millis() - downTime) > debounce
    ) { 
        Serial.println(__LINE__);       
        if (not ignoreUp) {
            upTime = millis();
            
            if (DConUp == false) {
                DCwaiting = true;
            } else {
               event = 2;
               Serial.println(__LINE__);
               DConUp = false;
               DCwaiting = false;
               singleOK = false;
            }
        }
    }

    // Test for normal click event: DCgap expired
    if (
        btnState == HIGH  
    &&  (millis() - upTime) >= DCgap 
    &&  DCwaiting == true 
    &&  DConUp == false 
    &&  singleOK == true 
    &&  event != 2
    ) {
        Serial.println(__LINE__);
        event = 1;
        DCwaiting = false;
    }

    // Test for hold
    if (
        btnState == LOW  
    &&  (millis() - downTime) >= holdTime
    ) {
       // Trigger "normal" hold
       if (not holdEventPast) {
            Serial.println(__LINE__);
            event = 3;
            waitForUp = true;
            ignoreUp = true;
            DConUp = false;
            DCwaiting = false;
            //downTime = millis();
            holdEventPast = true;
        }
        // Trigger "long" hold
        if ((millis() - downTime) >= longHoldTime) {
            Serial.println(__LINE__);
            if (not longHoldEventPast) {
               event = 4;
               longHoldEventPast = true;
            }
        }
    }
   
    prevBtnState = btnState;
   
    //return event;
    if (event == 1) return singleClickEvent();
    if (event == 2) return doubleClickEvent();
    if (event == 3) return holdEvent();
    if (event == 4) return longHoldEvent();

    return 0;
}

void setup() 
{
    pinMode(BUTTON_PIN, INPUT);
    digitalWrite(BUTTON_PIN, HIGH);
    pinMode(TWINKLE_PIN, OUTPUT);
    digitalWrite(TWINKLE_PIN, LOW);

    strip.begin();
    strip.setBrightness(50); //adjust brightness here
    strip.show(); // Initialize all pixels to 'off'

    Serial.begin(9600);
    Serial.println("Start :-)");
}

void loop() 
{
   checkButton();
}

//=================================================
// Events to trigger

// Loop through the NeoPixel Modes
int singleClickEvent() 
{ 
    Serial.println("Single Click Event Fired!");
    Serial.print("looper = "); Serial.println(looper);
    Serial.print("sce_counter = "); Serial.println(sce_counter);
    
    looper = 0;
    //sce_counter = 0;

    if (digitalRead(TWINKLE_PIN) == HIGH) {
        digitalWrite(TWINKLE_PIN, LOW);
    } else {
        digitalWrite(TWINKLE_PIN, HIGH);
    }

    return 1;
}

// Turn on and off the Twinkle Lights
int doubleClickEvent() 
{
    Serial.println("Double Click Event Fired!");
    Serial.print("looper = "); Serial.println(looper);
    Serial.print("sce_counter = "); Serial.println(sce_counter);

    looper = 0;
    sce_counter = 0;

    return 2;
}

int holdEvent() 
{
    Serial.println("Hold Event Fired!");
    Serial.print("looper = "); Serial.println(looper);
    Serial.print("sce_counter = "); Serial.println(sce_counter);

    looper = 0;
    //sce_counter = 0;

    if (sce_counter == 0) {
        sce_counter++;
    }
    
    if (sce_counter == 1) {
        Serial.println("Single Click Event: ColorWipe");
        looper = 1;

        colorWipe(strip.Color(255, 0, 0), 50); // Red
        colorWipe(strip.Color(255, 255, 0), 50); // Yellow
        colorWipe(strip.Color(0, 255, 0), 50); // Green
        colorWipe(strip.Color(0, 255, 255), 50); // Cyan
        colorWipe(strip.Color(0, 0, 255), 50); // Blue
        colorWipe(strip.Color(255, 0, 255), 50); // Magenta

        sce_counter++;
    } else if (sce_counter == 2) {
        Serial.println("Single Click Event: rainbow");
        looper = 1;
        
        // while (looper == 1) {
        //     if (rainbow(25) != 0) looper = 0;
        // }

        rainbow(25);
        
        sce_counter++;
    } else if (sce_counter == 3) {
        Serial.println("Single Click Event: rainbowCycle");
        looper = 1;

        rainbowCycle(25);
        
        // while (looper == 1) {
        //     if (rainbowCycle(25) != 0) looper = 0;
        // }

        sce_counter++;
    } else if (sce_counter == 4) {
        Serial.println("Single Click Event: theaterChase");
        looper = 1;

        //while (looper == 1) {
            theaterChase(strip.Color(255, 200, 150), 50); // White
            theaterChase(strip.Color(200, 125, 0), 50); // Orange
        //}

        sce_counter++;
    } else if (sce_counter == 5) {
        Serial.println("Single Click Event: theaterChaseRainbow");
        looper = 1;
        
        // while (looper == 1) {
        //     if (theaterChaseRainbow(25) != 0) looper = 0;
        // }

        theaterChaseRainbow(25);
        
        sce_counter++;
    } else {
        sce_counter = 0;
    }

    return 3;
}

int longHoldEvent() 
{
    Serial.println("Long Hold Event Fired!");
    Serial.print("looper = "); Serial.println(looper);
    Serial.print("sce_counter = "); Serial.println(sce_counter);

    looper = 0;
    sce_counter = 0;

    return 4;
}

//=================================================
