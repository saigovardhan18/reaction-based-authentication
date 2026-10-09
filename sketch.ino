/*
  Human Reaction Signature Authentication — Arduino Uno (one light + LCD, multi-user)
  Educational behavioral-authentication prototype (not a validated biometric).

  HARDWARE (Arduino Uno)
    D2  -> pushbutton -> GND            (INPUT_PULLUP: HIGH = released, LOW = pressed)
    D8  -> 220 ohm -> LED -> GND        (the only light)
    D3  -> CONTROL button (red) -> GND  (INPUT_PULLUP)
    LCD 16x2 I2C:  GND -> GND, VCC -> 5V, SDA -> A4, SCL -> A5
    A0  -> leave unconnected (seeds random())

  LIBRARY: "LiquidCrystal I2C" (Wokwi: Library Manager). I2C address 0x27 (try 0x3F if blank).

  CONTROL BUTTON (D3, red):
    1 click                          = register a new user
    2 clicks within 5 seconds        = authenticate
    hold for 2 seconds               = erase all users
  The green button on D2 is only used to react to the light.

  SERIAL MONITOR (optional, 9600 baud) still works:
    r = register a NEW user (max 3 users, 5 tests each)
    a = authenticate (5 tests, compared with every registered user)
    p = show registered profiles
    x = erase all users
  Profiles live in RAM only: they are lost on reset or power-off.
*/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const uint8_t LED_PIN = 8;
const uint8_t BTN_PIN = 2;                 // reaction button
const uint8_t CTRL_PIN = 3;                // control button

LiquidCrystal_I2C lcd(0x27, 16, 2);

const uint8_t N = 5;                       // reaction tests per session
const uint8_t MAX_USERS = 3;
const float SPAN_MS = 300.0;               // difference that maps to 0% match
const float THRESHOLD = 70.0;              // experimental demo threshold, not validated
const unsigned long LONG_MS = 2000;           // hold this long to erase all
const unsigned long CLICK_WINDOW_MS = 5000;   // time allowed for the 2nd click
const unsigned long TIMEOUT_MS = 2000;     // no press within 2 s = too slow, retry

int     reg[MAX_USERS][N];                 // registered reactions per user (ms)
uint8_t users = 0;

// ---------- helpers ----------
void show(const char *a, const char *b) {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(a);
  lcd.setCursor(0, 1); lcd.print(b);
}

void flushSerial() {
  while (Serial.available()) Serial.read();
}

void waitRelease() {
  while (digitalRead(BTN_PIN) == LOW) delay(5);
  delay(30);                               // simple debounce
}

// ---------- one valid reaction test, returns ms ----------
int reactionTest(uint8_t i) {
  char l1[17];
  while (true) {
    waitRelease();                         // a held button can never start a trial
    digitalWrite(LED_PIN, LOW);
    delay(400);

    snprintf(l1, sizeof l1, "TEST %d/%d WAIT", i, N);
    show(l1, "STAY FOCUSED");
    Serial.println(F("  Wait for the LED, then press the button..."));

    unsigned long waitMs = random(2000, 5001);   // 2000..5000 ms
    unsigned long t = millis();
    bool early = false;
    while (millis() - t < waitMs) {
      if (digitalRead(BTN_PIN) == LOW) { early = true; break; }
    }
    if (early) {                           // pressed before the LED: not valid
      Serial.println(F("  FALSE START - repeating this test"));
      show("FALSE START!", "WAIT FOR LIGHT");
      delay(1200);
      continue;
    }

    digitalWrite(LED_PIN, HIGH);
    show("PRESS NOW!", "0 ms");
    unsigned long start = micros();        // timer starts when the LED turns ON
    unsigned long stop = start;
    unsigned long lastUi = 0;
    bool pressed = false;
    while (true) {
      if (digitalRead(BTN_PIN) == LOW) { stop = micros(); pressed = true; break; }
      unsigned long el = micros() - start; // unsigned subtraction is overflow-safe
      if (el > TIMEOUT_MS * 1000UL) break;
      if (el / 1000 - lastUi >= 50) {      // live timer on the LCD
        lastUi = el / 1000;
        lcd.setCursor(0, 1); lcd.print(lastUi); lcd.print(" ms     ");
      }
    }
    digitalWrite(LED_PIN, LOW);            // LED off right after the response

    if (!pressed) {
      Serial.println(F("  TOO SLOW - repeating this test"));
      show("TOO SLOW!", "TRY AGAIN");
      delay(1200);
      continue;
    }
    return (int)((stop - start) / 1000);   // microseconds -> milliseconds
  }
}

// ---------- N tests into out[] ----------
void collect(int *out) {
  char l1[17], l2[17];
  for (uint8_t i = 1; i <= N; i++) {
    Serial.print(F("Test ")); Serial.print(i); Serial.print('/'); Serial.print(N);
    Serial.println(F(" - get ready..."));
    int rt = reactionTest(i);
    out[i - 1] = rt;
    Serial.print(F("  Reaction: ")); Serial.print(rt); Serial.println(F(" ms"));
    long sum = 0; for (uint8_t k = 0; k < i; k++) sum += out[k];
    snprintf(l1, sizeof l1, "T%d: %d MS", i, rt);
    snprintf(l2, sizeof l2, "AVG %ld MS", sum / i);
    show(l1, l2);
    delay(1200);
  }
}

// mean of the N per-reaction matches between a[] and b[] (0..100)
float similarity(const int *a, const int *b) {
  float total = 0;
  for (uint8_t i = 0; i < N; i++) {
    float m = 100.0 - abs(a[i] - b[i]) * 100.0 / SPAN_MS;
    if (m < 0) m = 0;
    if (m > 100) m = 100;
    total += m;
  }
  return total / N;
}

float average(const int *a) {
  long sum = 0;
  for (uint8_t i = 0; i < N; i++) sum += a[i];
  return sum / (float)N;
}

// ---------- r : register a new user ----------
void registerUser() {
  if (users >= MAX_USERS) {
    Serial.println(F("All user slots are full. Send x to erase."));
    show("USERS FULL", "SEND x TO RESET"); delay(2500); return;
  }
  char l1[17], l2[17];
  Serial.print(F("\n=== REGISTER USER ")); Serial.print(users + 1); Serial.println(F(" ==="));
  snprintf(l1, sizeof l1, "NEW USER %d", users + 1);
  show(l1, "GET READY..."); delay(900);

  collect(reg[users]);                     // user only counts after all N tests succeed

  float avg = average(reg[users]);
  users++;
  snprintf(l1, sizeof l1, "USER %d SAVED", users);
  snprintf(l2, sizeof l2, "AVG %d MS", (int)(avg + 0.5));
  show(l1, l2);
  Serial.print(F("USER ")); Serial.print(users);
  Serial.print(F(" SAVED  average = ")); Serial.print(avg, 1); Serial.println(F(" ms"));
  delay(3000);
}

// ---------- a : authenticate ----------
void authenticateUser() {
  if (users == 0) {
    Serial.println(F("No users registered. Send r first."));
    show("NO USERS", "SEND r TO REG"); delay(2500); return;
  }
  Serial.println(F("\n=== AUTHENTICATION ==="));
  show("AUTHENTICATE", "GET READY..."); delay(900);

  int cur[N];
  collect(cur);

  float best = -1; uint8_t bestUser = 0;
  for (uint8_t u = 0; u < users; u++) {
    float s = similarity(cur, reg[u]);
    Serial.print(F("User ")); Serial.print(u + 1);
    Serial.print(F(" similarity: ")); Serial.print(s, 1); Serial.println('%');
    if (s > best) { best = s; bestUser = u; }
  }

  Serial.print(F("Test  Registered  Current  Delta  Match   (best: user "));
  Serial.print(bestUser + 1); Serial.println(')');
  for (uint8_t i = 0; i < N; i++) {
    int d = abs(cur[i] - reg[bestUser][i]);
    float m = 100.0 - d * 100.0 / SPAN_MS; if (m < 0) m = 0;
    Serial.print(i + 1); Serial.print(F("     ")); Serial.print(reg[bestUser][i]);
    Serial.print(F(" ms      ")); Serial.print(cur[i]); Serial.print(F(" ms   "));
    Serial.print(d); Serial.print(F(" ms  ")); Serial.print(m, 1); Serial.println('%');
  }

  bool ok = best >= THRESHOLD;
  char l1[17];
  if (ok) {
    snprintf(l1, sizeof l1, "USER %d  %d.%d%%", bestUser + 1, (int)best, (int)(best * 10) % 10);
    Serial.print(F(">>> ACCESS GRANTED - USER ")); Serial.println(bestUser + 1);
  } else {
    snprintf(l1, sizeof l1, "%d.%d%% MATCH", (int)best, (int)(best * 10) % 10);
    Serial.println(F(">>> ACCESS DENIED"));
  }
  show(l1, ok ? "ACCESS GRANTED" : "ACCESS DENIED");
  delay(5000);
}

// ---------- p : show profiles ----------
void showProfiles() {
  if (users == 0) { Serial.println(F("No users registered.")); return; }
  for (uint8_t u = 0; u < users; u++) {
    Serial.print(F("User ")); Serial.print(u + 1); Serial.print(F(": "));
    for (uint8_t i = 0; i < N; i++) { Serial.print(reg[u][i]); Serial.print(F(" ms  ")); }
    Serial.print(F("| avg ")); Serial.print(average(reg[u]), 1); Serial.println(F(" ms"));
  }
}

// ---------- control button gestures ----------
// returns 0 = nothing, 1 = single click, 2 = double click, 3 = long press
uint8_t readGesture() {
  if (digitalRead(CTRL_PIN) == HIGH) return 0;
  delay(30);                                   // debounce
  if (digitalRead(CTRL_PIN) == HIGH) return 0;

  unsigned long t = millis();
  bool warned = false;
  while (digitalRead(CTRL_PIN) == LOW) {       // still held?
    unsigned long held = millis() - t;
    if (held >= LONG_MS) {
      while (digitalRead(CTRL_PIN) == LOW);
      delay(30);
      return 3;                                // long press
    }
    if (held >= 500 && !warned) { show("HOLD TO ERASE", "KEEP HOLDING..."); warned = true; }
  }
  delay(30);

  unsigned long w = millis();                  // wait for a possible 2nd click
  int lastSec = -1;
  while (millis() - w < CLICK_WINDOW_MS) {
    int left = (CLICK_WINDOW_MS - (millis() - w)) / 1000 + 1;
    if (left != lastSec) {
      lastSec = left;
      char l2[17]; snprintf(l2, sizeof l2, "2nd CLICK=AUTH %d", left);
      show("1 CLICK=REGISTER", l2);
    }
    if (digitalRead(CTRL_PIN) == LOW) {
      delay(30);
      while (digitalRead(CTRL_PIN) == LOW);
      delay(30);
      return 2;                                // double click
    }
  }
  return 1;                                    // single click
}

void eraseAll() {
  users = 0;
  Serial.println(F("All users erased."));
  show("ALL USERS", "ERASED");
  delay(1500);
}

void printMenu() {
  char l1[17];
  snprintf(l1, sizeof l1, "USERS: %d/%d", users, MAX_USERS);
  Serial.println(F("\n=== Arduino Uno Reaction Signature ==="));
  Serial.println(F("r = register new user | a = authenticate | p = show profiles | x = erase all"));
  show(l1, "1x REG 2x AUTH");
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(CTRL_PIN, INPUT_PULLUP);
  Serial.begin(9600);
  Wire.begin();
  Wire.setClock(400000);
  lcd.init();
  lcd.backlight();
  randomSeed(analogRead(A0));              // floating pin A0 seeds random()
  printMenu();
}

void loop() {
  uint8_t g = readGesture();                   // control button first
  if (g) {
    if      (g == 1) registerUser();
    else if (g == 2) authenticateUser();
    else             eraseAll();
    while (digitalRead(CTRL_PIN) == LOW);
    flushSerial();
    printMenu();
    return;
  }
  if (!Serial.available()) return;
  char c = Serial.read();
  if (c >= 'A' && c <= 'Z') c += 32;       // accept upper case too
  if (c == '\n' || c == '\r' || c == ' ') return;

  if      (c == 'r') registerUser();
  else if (c == 'a') authenticateUser();
  else if (c == 'p') showProfiles();
  else if (c == 'x') eraseAll();
  else Serial.println(F("Unknown command. Use r, a, p or x."));

  flushSerial();                           // drop leftover characters typed during a session
  printMenu();
}
