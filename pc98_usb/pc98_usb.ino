// PC98 To USB Keyboard Adapter
// By Bleach

#define REPEAT_TIMEOUT 20 // How many ms to wait before releasing a key (The PC98 Internal repeat is ~60ms per cycle but as low as 5 seems to work?)
#define STANDARDIZE_LAYOUT // Replaces some key actions to make it more closer to a standard keyboard
//#define STANDARDIZE_DELETE // Switches the place of Delete and Page Up to be the same as standard keyboards (You probably want to swap the keycaps if you enable this)

#define RST 2 // Reset request
#define RXD 3 // Data
#define RDY 4 // Ready
#define RTY 5 // Retransmission request

uint8_t usbData[8] = { 0 };
unsigned long releaseData[8] = { 0 };

void setup() {
  // Hardware serial initialization
  Serial.begin(9600);

  // Pinmode setup
  pinMode(RXD, INPUT_PULLUP);

  pinMode(RST, INPUT_PULLUP);
  pinMode(RST, OUTPUT);

  pinMode(RDY, INPUT_PULLUP);
  pinMode(RDY, OUTPUT);

  pinMode(RTY, INPUT_PULLUP);
  pinMode(RTY, OUTPUT);

  // PC98 reset
  // https://archive.org/stream/PC9800TechnicalDataBookHARDWARE1993/PC-9800TechnicalDataBook_HARDWARE1993#page/n359
  digitalWrite(RDY, HIGH);
  digitalWrite(RST, LOW);
  delayMicroseconds(15);
  digitalWrite(RST, HIGH);

  delay(50);

  // Attach byte listener
  attachInterrupt(digitalPinToInterrupt(RXD), processKey, FALLING);

  // ready to receive from keyboard
  digitalWrite(RDY, LOW);
}

void loop() {
  uint8_t tempData[6] = { 0 };
  unsigned long tempRData[6] = { 0 };
  uint8_t tempI = 0;
  unsigned long ms = millis();
  for (int i = 2; i < 8; i++) {
    if ((releaseData[i] == 0) || ((releaseData[i] + REPEAT_TIMEOUT) > ms)) {
      tempData[tempI] = usbData[i];
      tempRData[tempI] = releaseData[i];
      tempI++;
    }
  }
  if (tempI != 6) {
    for (int i = 2; i < 8; i++) {
      usbData[i] = tempData[i - 2];
      releaseData[i] = tempRData[i - 2];
    }
    Serial.write(usbData, 8);
  }
  delay(1);
}


void processKey() {
  uint8_t code = readByte();
  if (code == 0xFF) {
    return;
  }

  uint8_t usbCode = pc98toUsb(code & 0x7F);
  uint8_t originalUsbCode = usbCode;
  #ifdef STANDARDIZE_LAYOUT
    usbCode = standardizeUsb(usbCode);
  #endif
  #ifdef STANDARDIZE_DELETE
    usbCode = swapDelete(usbCode);
  #endif
  
  // The CAPS & KANA keys don't send release scancodes so we press and release them at the same time.
  bool lockKey = (originalUsbCode == 0x39) || (originalUsbCode == 0x88);

  // MODIFIERS
  // Modifiers don't send repeat make/break codes so we don't
  // worry about release timings for them.
  if ((originalUsbCode >= 0xE0) && (originalUsbCode <= 0xE7)) {
    usbData[0] ^= (1 << (usbCode - 0xE0));
    Serial.write(usbData, 8);
  } else {
    // MAKE
    if (!(code & 0x80) || lockKey) {
      for (uint8_t i = 2; i < 8; i++) {
        if ((usbData[i] == 0) || (usbData[i] == usbCode)) {
          usbData[i] = usbCode;
          if (releaseData[i] == 0) {
            Serial.write(usbData, 8);
          }
          releaseData[i] = 0;
          break;
        }
      }
    }

    // BREAK
    if ((code & 0x80) || lockKey) {
      for (uint8_t i = 2; i < 8; i++) {
        if (usbData[i] == usbCode) {
          releaseData[i] = millis();
          break;
        }
      }
    }
  }

  // Keyboards require RDY pulse >=37us to send next data
  // https://archive.org/stream/PC9800TechnicalDataBookHARDWARE1993/PC-9800TechnicalDataBook_HARDWARE1993#page/n157
  digitalWrite(RDY, HIGH);
  delayMicroseconds(40);
  digitalWrite(RDY, LOW);
}

// Reads a byte from the RXD input
// Very conc
uint8_t readByte() {
  uint8_t data = 0;
  uint8_t mask = 0x01;
  uint8_t parity = 0;

  // We probably should center the start bit but every digitalRead takes
  // a few micros which could offset the later bits
  const uint8_t startBit = digitalRead(RXD);
  // Return if the start bit is high to filter out noise
  if (startBit) {
    return 0xFF;
  }

  do {
    // To center of next bit
    delayMicroseconds(52);

    if (digitalRead(RXD)) {
      data |= mask;
      parity ^= 1;
    }
    mask <<= 1;
  } while (mask & 0xFF);

  // to center of parity bit
  delayMicroseconds(52);
  const uint8_t parityBit = digitalRead(RXD);

  // to center of stop bit
  delayMicroseconds(52);
  const uint8_t stopBit = digitalRead(RXD);

  /*if (!(parity ^ parityBit)) {
    Serial.print("WRONG PARITY: ");
    Serial.print(startBit);
    Serial.print("|");
    Serial.print(data, HEX);
    Serial.print("|");
    Serial.print(parityBit);
    Serial.println(stopBit);
  }*/
  
  return data;
}

uint8_t pc98toUsb(uint8_t scancode) {
  switch (scancode) {
    // MAIN ROW 1
    case 0x00: return 0x29; /* ESC */
    case 0x01: return 0x1E; /* 1 */
    case 0x02: return 0x1F; /* 2 */
    case 0x03: return 0x20; /* 3 */
    case 0x04: return 0x21; /* 4 */
    case 0x05: return 0x22; /* 5 */
    case 0x06: return 0x23; /* 6 */
    case 0x07: return 0x24; /* 7 */
    case 0x08: return 0x25; /* 8 */
    case 0x09: return 0x26; /* 9 */
    case 0x0A: return 0x27; /* 0 */
    case 0x0B: return 0x2D; /* - */
    case 0x0C: return 0x2E; /* = */
    case 0x0D: return 0x89; /* YEN */
    case 0x0E: return 0x2A; /* BACKSPACE */
    // MAIN ROW 2
    case 0x0F: return 0x2B; /* TAB */
    case 0x10: return 0x14; /* Q */
    case 0x11: return 0x1A; /* W */
    case 0x12: return 0x08; /* E */
    case 0x13: return 0x15; /* R */
    case 0x14: return 0x17; /* T */
    case 0x15: return 0x1C; /* Y */
    case 0x16: return 0x18; /* U */
    case 0x17: return 0x0C; /* I */
    case 0x18: return 0x12; /* O */
    case 0x19: return 0x13; /* P */
    case 0x1A: return 0x2F; /* [ */
    case 0x1B: return 0x30; /* ] */
    case 0x1C: return 0x28; /* ENTER */
    // MAIN ROW 3 (EXCEPT CTRL/CAPS)
    case 0x1D: return 0x04; /* A */
    case 0x1E: return 0x16; /* S */
    case 0x1F: return 0x07; /* D */
    case 0x20: return 0x09; /* F */
    case 0x21: return 0x0A; /* G */
    case 0x22: return 0x0B; /* H */
    case 0x23: return 0x0D; /* J */
    case 0x24: return 0x0E; /* K */
    case 0x25: return 0x0F; /* L */
    case 0x26: return 0x33; /* ; */
    case 0x27: return 0x34; /* ' */
    case 0x28: return 0x31; /* \ */
    // MAIN ROW 4 (EXCEPT SHIFT)
    case 0x29: return 0x1D; /* Z */
    case 0x2A: return 0x1B; /* X */
    case 0x2B: return 0x06; /* C */
    case 0x2C: return 0x19; /* V */
    case 0x2D: return 0x05; /* B */
    case 0x2E: return 0x11; /* N */
    case 0x2F: return 0x10; /* M */
    case 0x30: return 0x36; /* , */
    case 0x31: return 0x37; /* . */
    case 0x32: return 0x38; /* / */
    case 0x33: return 0x87; /* RO */
    // SPACE + XFER
    case 0x34: return 0x2C; /* SPACE */
    case 0x35: return 0x8A; /* XFER */
    // NAV CLUSTER
    case 0x36: return 0x4B; /* ROLL/PAGE UP */
    case 0x37: return 0x4E; /* ROLL/PAGE DOWN */
    case 0x38: return 0x49; /* INSERT */
    case 0x39: return 0x4C; /* DELETE */
    // ARROWS
    case 0x3A: return 0x52; /* ARROW UP */
    case 0x3B: return 0x50; /* ARROW LEFT */
    case 0x3C: return 0x4F; /* ARROW RIGHT */
    case 0x3D: return 0x51; /* ARROW DOWN */
    // NUMPAD
    case 0x3E: return 0x4A; /* HOME/CLEAR */
    case 0x3F: return 0x75; /* HELP */
    case 0x40: return 0x56; /* NUM - */
    case 0x41: return 0x54; /* NUM / */
    case 0x42: return 0x5F; /* NUM 7 */
    case 0x43: return 0x60; /* NUM 8 */
    case 0x44: return 0x61; /* NUM 9 */
    case 0x45: return 0x55; /* NUM * */
    case 0x46: return 0x5C; /* NUM 4 */
    case 0x47: return 0x5D; /* NUM 5 */
    case 0x48: return 0x5E; /* NUM 6 */
    case 0x49: return 0x57; /* NUM + */
    case 0x4A: return 0x59; /* NUM 1 */
    case 0x4B: return 0x5A; /* NUM 2 */
    case 0x4C: return 0x5B; /* NUM 3 */
    case 0x4D: return 0x67; /* NUM = */
    case 0x4E: return 0x62; /* NUM 0 */
    case 0x4F: return 0x85; /* NUM , */
    case 0x50: return 0x63; /* NUM . */
    // NFER
    case 0x51: return 0x8B; /* NFER */
    // VF KEYS
    case 0x52: return 0x44; /* VF1 */
    case 0x53: return 0x45; /* VF2 */
    case 0x54: return 0x46; /* VF3 */
    case 0x55: return 0x47; /* VF4 */
    case 0x56: return 0x48; /* VF5 */
    // HOME
    case 0x5E: return 0x4A; /* HOME */
    // TOP ROW
    case 0x60: return 0x78; /* STOP */
    case 0x61: return 0x7C; /* COPY */
    case 0x62: return 0x3A; /* F1 */
    case 0x63: return 0x3B; /* F2 */
    case 0x64: return 0x3C; /* F3 */
    case 0x65: return 0x3D; /* F4 */
    case 0x66: return 0x3E; /* F5 */
    case 0x67: return 0x3F; /* F6 */
    case 0x68: return 0x40; /* F7 */
    case 0x69: return 0x41; /* F8 */
    case 0x6A: return 0x42; /* F9 */
    case 0x6B: return 0x43; /* F10 */
    // MODIFIER KEYS
    case 0x70: return 0xE1; /* SHIFT */
    case 0x71: return 0x39; /* CAPS LOCK */
    case 0x72: return 0x88; /* KANA */
    case 0x73: return 0xE2; /* GRPH (LALT) */
    case 0x74: return 0xE0; /* CTRL */
    case 0x75: return 0xE5; /* RSHIFT */
    case 0x77: return 0xE3; /* LGUI */
    case 0x78: return 0xE7; /* RGUI */
    case 0x79: return 0xE4; /* APPLICATION (RCTRL) */
    default: return 0;
  }
}

uint8_t standardizeUsb(uint8_t scancode) {
  switch (scancode) {
    case 0xE2: return 0xE3; /* GRPH (LALT) -> LGUI */
    case 0x8B: return 0xE2; /* NFER -> LALT */
    case 0x8A: return 0xE6; /* XFER -> RALT */
    case 0x29: return 0x35; /* ESC -> GRAVE ACCENT */
    case 0x7C: return 0x29; /* COPY -> ESC */
    case 0x89: return 0x2A; /* YEN -> BACKSPACE (For a wider backspace) */
    case 0x87: return 0x64; /* RO -> ISO \| */
    default: return scancode;
  }
}

uint8_t swapDelete(uint8_t scancode) {
  switch (scancode) {
    case 0x4B: return 0x4C; /* ROLL/PAGE UP -> DELETE */
    case 0x4C: return 0x4B; /* DELETE -> ROLL/PAGE UP */
    default: return scancode;
  }
}

// WRITING FUNCTIONS
// Writing is technically supported but everytime i've tried
// to send commands it either returns 0xFC or nothing at all so TODO i guess
// https://geekhack.org/index.php?topic=110094.msg3164441#msg3164441 < relevant maybe
/*void pc98_send(uint8_t data) {
  digitalWrite(RDY, HIGH);
  delay(1);

  // START BIT
  digitalWrite(RST, LOW);
  delayMicroseconds(52);

  // DATA
  uint8_t mask = 0x01;
  uint8_t parity = 0;
  while (mask&0xFF) {
    if (data&mask) {
      digitalWrite(RST, HIGH);
      parity ^= 1;
    } else {
      digitalWrite(RST, LOW);
    }
    delayMicroseconds(52);
    mask <<= 1;
  }

  // ODD PARITY BIT
  if (parity != 1) {
    digitalWrite(RST, HIGH);
  } else {
    digitalWrite(RST, LOW);
  }
  delayMicroseconds(52);

  // END BIT
  digitalWrite(RST, HIGH);
  delayMicroseconds(52);

  delay(1);
  digitalWrite(RDY, LOW);
}

uint8_t pc98_wait_response() {
  while (digitalRead(RXD));
  return readByte();
}*/
