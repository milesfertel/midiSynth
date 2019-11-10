//#define SERIAL_RATE 9600
#define SERIAL_RATE 31250
#define NOTE_ON_CMD 0x90
#define NOTE_OFF_CMD 0x80
#define NOTE_VELOCITY 127

#define RIBBON 16
#define PINS 32
#define NROW 8
#define NCOL 4
#define NFORCES 2
#define NCABLES 2
const int minPin = 22;
const int maxPin = 53;

// Map for each cable from input to output index
boolean keyPressed[NFORCES][NCABLES][NROW][NCOL];
// Map for each cable from input output to midi value
uint8_t keyToMidiMap[NFORCES][NCABLES][NROW][NCOL];

// Array contaning the indices in the ribbon cables of pins that
// should be written to in order to get the pressed down keys
uint8_t writePins[2][2][4] = {
  // Cable 1         Cable 2
  // F pins (Hard Press?)
  {{2, 0, 1, 7}, {6, 13, 14, 15}},
  //  S pins (Soft Press?)
  {{3, 4, 5, 6}, {7, 10, 11, 12}}
};

// The index of the pins
uint8_t readPins[2][8] = {
  {8, 9, 10, 11, 12, 13, 14, 15},
  {0, 1, 2, 3, 4, 5, 8, 9}
};

// Returns whether a certain _arduino_ pin should be in write mode
bool isWrite(int pin) {
  // Map of ribbon index to write or read status
  bool isWritePin[NCABLES][RIBBON] = {
    {true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false},
    {false, false, false, false, false, false, true, true, false, false, true, true, true, true, true, true}
  };
  return isWritePin[!(pin % 2)][(pin - minPin) / 2];
}

void setup() {
  Serial.begin(SERIAL_RATE); 

  // Goes through and assigns midi values
  for (int forceCtr = 0; forceCtr < NFORCES; forceCtr++) {
    // C2
    int note = 36;
    for (int cableCtr = 0; cableCtr < NCABLES; cableCtr++) {
      // Go through all write pins in cable
      for (int colCtr = 0; colCtr < NCOL; colCtr++)
      {
        // Go through all read pins in cable
        for (int rowCtr = 0; rowCtr < NROW; rowCtr++)
        {
          keyPressed[forceCtr][cableCtr][rowCtr][colCtr] = false;
          keyToMidiMap[forceCtr][cableCtr][rowCtr][colCtr] = note++;
        }
      }
    }
  }
  
  for (int i = minPin; i <= maxPin; i++) {
    if (!isWrite(i)) {
      pinMode(i, INPUT);
    } else {
      pinMode(i, OUTPUT);
      digitalWrite(i, LOW);
    }
  }

}

void noteOn(uint8_t midi)
{
  Serial.write(NOTE_ON_CMD);
  Serial.write(midi);
  Serial.write(NOTE_VELOCITY);
}

void noteOff(uint8_t midi)
{
  Serial.write(NOTE_OFF_CMD);
  Serial.write(midi);
  Serial.write(NOTE_VELOCITY);
}

void loop() {
  // Hard presses
  const int forceCtr = 0;
  
  for (int cableCtr = 0; cableCtr < 2; cableCtr++) {
    for (int colCtr = 0; colCtr < NCOL; colCtr++) { 
      // Get matrix row values when col is set
      int writePin = minPin + (1 - cableCtr) + 2 * writePins[forceCtr][cableCtr][colCtr];

      digitalWrite(writePin, HIGH);
      int rowValue[NROW];
      for (int rowCtr = 0; rowCtr < NROW; rowCtr++) {
        int readPin = minPin + (1 - cableCtr) + 2 * readPins[cableCtr][rowCtr];
        rowValue[rowCtr] = digitalRead(readPin);
      }

      // process keys pressed
      for (int rowCtr = 0; rowCtr < NROW; rowCtr++)
      {
        if (rowValue[rowCtr] && !keyPressed[forceCtr][cableCtr][rowCtr][colCtr])
        {
          keyPressed[forceCtr][cableCtr][rowCtr][colCtr] = true;
          noteOn(keyToMidiMap[forceCtr][cableCtr][rowCtr][colCtr]);
        }
      }

      // process keys released
      for (int rowCtr = 0; rowCtr < NROW; rowCtr++)
      {
        if (!rowValue[rowCtr] && keyPressed[forceCtr][cableCtr][rowCtr][colCtr])
        {
          keyPressed[forceCtr][cableCtr][rowCtr][colCtr] = false;
          noteOff(keyToMidiMap[forceCtr][cableCtr][rowCtr][colCtr]);
        }
      }
      
      digitalWrite(writePin, LOW);
    }
  }
}
