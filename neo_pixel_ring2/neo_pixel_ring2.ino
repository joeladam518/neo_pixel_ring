/**
 *  Arduino script for the neopixel ring lantern
 *
 *  multiClick code from
 *  http://jmsarduino.blogspot.com/2009/10/4-way-button-click-double-click-hold.html
 *
 *  the modified by Joel Haker
 *
 *  Start of project: 2018-05-25
**/
#include <Adafruit_NeoPixel.h>

#define NEO_PIN 11     // NeoPixel Pin
#define BUTTON_PIN 10  // Button Pin
#define TWINKLE_PIN 6  // Twinkle Lights Pin

//==============================================================================

/**
 *  Adafruit NeoPixel ring object
 *
 *  Parameter 1 = number of pixels in strip
 *  Parameter 2 = pin number (most are valid)
 *  Parameter 3 = pixel type flags, add together as needed:
 *      NEO_KHZ800  800KHz  bitstream (most NeoPixel products w/WS2812 LEDs)
 *      NEO_KHZ400  400KHz  (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
 *      NEO_GRB              Pixels are wired for GRB bitstream (most NeoPixel products)
 *      NEO_RGB              Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
**/
Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, NEO_PIN, NEO_GRB + NEO_KHZ800);

//==============================================================================
// MULTI-CLICK Button Variables

// Button timing variables
int debounce = 20; // ms debounce period to prevent flickering when pressing or releasing the button
int DCgap = 250;   // max ms between clicks for a double click event
int longPressTime = 1000;  // ms hold period: how long to wait for press+hold event
int vlongPressTime = 4000; // ms long hold period: how long to wait for press+hold event

// Button variables
long upTime = -1;   // time the button was released
long downTime = -1; // time the button was pressed down
boolean btnState = HIGH;  // value read from button
boolean btnStateLast = HIGH; // buffered value of the button's previous state
boolean dblClickWaiting = false; // whether we're waiting for a double click (down)
boolean dblClickOnNextUp = false;    // whether to register a double click on next release, or wait and click
boolean singleClickOK = true;   // whether it's OK to do a single click
boolean ignoreUp = false;  // ignore the button release because the click+hold was triggered
boolean waitForUp = false;     // when held, whether to wait for the up event
boolean longPressHappened = false; // whether or not the hold event happened already
boolean vLongPressHappened = false; // whether or not the long hold event happened already

//==============================================================================
// Gobal Variables

int global_event_state_current;
int global_event_state_previous;

//==============================================================================
// Functions

void clearPixels()
{
    colorWipe(strip.Color(0, 0, 0), 1);
    strip.show();
}

// Print line number
void prln(int line_no)
{
    Serial.print("(#");
    Serial.print(line_no);
    Serial.println(")");
}

void pr_global_event_status()
{
    Serial.print("global_event_state_current: "); Serial.println(global_event_state_current);
    Serial.print("global_event_state_previous: "); Serial.println(global_event_state_previous);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait)
{
    for(uint16_t i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, c);
        strip.show();
        delay(wait);
    }
}

void rainbow(uint8_t wait)
{
    uint16_t i, j;
    for(j=0; j<256; j++) {
        for(i=0; i<strip.numPixels(); i++) {
            strip.setPixelColor(i, Wheel((i+j) & 255));
        }
        strip.show();
        delay(wait);
    }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait)
{
    uint16_t i, j;
    for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
        for(i=0; i< strip.numPixels(); i++) {
            strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
        }
        strip.show();
        delay(wait);
    }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait)
{
    for (int j=0; j<10; j++) {  //do 10 cycles of chasing
        for (int q=0; q < 3; q++) {
            for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
                strip.setPixelColor(i+q, c);    //turn every third pixel on
            }

            strip.show();

            delay(wait);

            for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
                strip.setPixelColor(i+q, 0);        //turn every third pixel off
            }
        }
    }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait)
{
    for (int j=0; j < 256; j++) { // cycle all 256 colors in the wheel
        for (int q=0; q < 3; q++) {
            for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
                strip.setPixelColor(i+q, Wheel((i+j) % 255)); //turn every third pixel on
            }

            strip.show();

            delay(wait);

            for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
                strip.setPixelColor(i+q, 0); //turn every third pixel off
            }
        }
    }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos)
{
    WheelPos = 255 - WheelPos;
    if(WheelPos < 85) {
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if(WheelPos < 170) {
        WheelPos -= 85;
        return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

int readButtonEvent()
{
    int event = 0;
    long chckbtn_millis = millis();
    btnState = digitalRead(BUTTON_PIN);

    if ( // Button pressed down
        btnState == LOW
    &&  btnStateLast == HIGH
    &&  (chckbtn_millis - upTime) > debounce
    ) {
        downTime = millis();
        ignoreUp = false;
        waitForUp = false;
        singleClickOK = true;
        longPressHappened = false;
        vLongPressHappened = false;

        if (
           (chckbtn_millis - upTime) < DCgap
        &&  dblClickOnNextUp == false
        &&  dblClickWaiting == true
        ) {
            dblClickOnNextUp = true;
        } else {
            dblClickOnNextUp = false;
        }

        dblClickWaiting = false;

    } else if ( // Button released
        btnState == HIGH
    &&  btnStateLast == LOW
    &&  (chckbtn_millis - downTime) > debounce
    ) {
        if (ignoreUp == false) {

            upTime = millis();

            if (dblClickOnNextUp == false) {
                dblClickWaiting = true;
            } else {
                event = 2;
                dblClickOnNextUp = false;
                dblClickWaiting = false;
                singleClickOK = false;
            }
        }
    }

    if ( // Test for normal click event: DCgap expired
        btnState == HIGH
    &&  (chckbtn_millis - upTime) >= DCgap
    &&  dblClickWaiting == true
    &&  dblClickOnNextUp == false
    &&  singleClickOK == true
    &&  event != 2
    ) {
        event = 1;
        dblClickWaiting = false;
    }

    if ( // added code
        btnState == HIGH
    &&  longPressHappened
    &&  !vLongPressHappened
    ) {
        event = 3; // trigger event after button release
        longPressHappened = false;
    } else if (
        btnState == HIGH
    &&  !longPressHappened
    &&  vLongPressHappened
    ) {
        //event = 4; // trigger event after button release
        vLongPressHappened = false;
    } else {
        if ( // Test for long press event
            btnState == LOW
        && (chckbtn_millis - downTime) >= longPressTime
        ) {
            if (!longPressHappened) {
                //event = 3; // trigger event during button hold
                //downTime = millis(); // Why is this here?
                waitForUp = true;
                ignoreUp = true;
                dblClickOnNextUp = false;
                dblClickWaiting = false;
                longPressHappened = true;
                vLongPressHappened = false;
            }

            // Test for very long press event
            if ((chckbtn_millis - downTime) >= vlongPressTime) {
                if (!vLongPressHappened) {
                    event = 4; // trigger event during button press
                    longPressHappened = false;
                    vLongPressHappened = true;
                }
            }
        }
    }

    btnStateLast = btnState;

    if (event != 0) {
        global_event_state_previous = global_event_state_current;
        global_event_state_current = event;
    }

    return event;
}

int handleButtonEvent(int event)
{
    //return event;
    if (event == 1) {
        singleClickEvent();
    } else if (event == 2) {
        doubleClickEvent();
    } else if (event == 3) {
        holdEvent();
    } else if (event == 4) {
        longHoldEvent();
    } else {
        return 0;
    }

    return 1;
}

int checkButton()
{
    int b_event = readButtonEvent();
    handleButtonEvent(b_event);
}

//=================================================
// Main

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
    Serial.println("");
    Serial.println("Single Click Event Fired!");
    pr_global_event_status();
    Serial.println("");

    return 1;
}

// Turn on and off the Twinkle Lights
int doubleClickEvent()
{
    Serial.println("");
    Serial.println("Double Click Event Fired!");
    pr_global_event_status();
    Serial.println("");

    return 2;
}

int holdEvent()
{
    Serial.println("");
    Serial.println("Hold Event Fired!");
    pr_global_event_status();
    Serial.println("");

    if (digitalRead(TWINKLE_PIN) == HIGH) {
        digitalWrite(TWINKLE_PIN, LOW);
    } else {
        digitalWrite(TWINKLE_PIN, HIGH);
    }

    return 3;
}

int longHoldEvent()
{
    Serial.println("");
    Serial.println("Long Hold Event Fired!");
    pr_global_event_status();
    Serial.println("");

    colorWipe(strip.Color(0, 0, 0), 1);

    if (digitalRead(TWINKLE_PIN) == HIGH) {
        digitalWrite(TWINKLE_PIN, LOW);
    }

    return 4;
}

//=================================================
