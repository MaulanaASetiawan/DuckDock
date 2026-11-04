#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BleKeyboard.h>
#include <Preferences.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

BleKeyboard bleKeyboard("StreamDock ESP32", "Pembuat", 100);
Preferences pref;

const int btnMenu = 4;
const int btnEnter = 5;
const int btnDown = 18;
const int btnUp = 19;

const int btnS[6] = { 32, 33, 25, 26, 27, 14 };

// DATABASE FITUR
const int TOTAL_FITUR = 11;
String namaFitur[TOTAL_FITUR] = {
  "1. Audio Toggle", "2. Screenshot", "3. Nvidia Record",
  "4. Task Manager", "5. Mute Mic", "6. Ghost F13 (OBS)",
  "7. Toggle HDR", "8. Focus Mode", "9. Restart VGA",
  "10. Next Track", "11. Prev Track"
};

String namaPendek[TOTAL_FITUR] = {
  "SND", "PIC", "REC", "TSK", "MIC", "F13", "HDR", "FOC", "VGA", "NXT", "PRV"
};

int mappingTombol[6] = { 0, 1, 2, 3, 4, 5 };
int appState = 0;
int indexMenuUtama = 0;
int indexFiturDipilih = 0;
int indexSlotDipilih = 0;
int indexMusicMenu = 0;
int scrollMenu = 0;

bool isAudioMonitor = false;
bool lastBtStatus = false;

static const unsigned char PROGMEM icon_bluetooth[] = { 0x20, 0xb0, 0x68, 0x30, 0x30, 0x68, 0xb0, 0x20 };

void muatKonfigurasi() {
  pref.begin("streamdock", true);
  for (int i = 0; i < 6; i++) {
    String key = "slot" + String(i);
    int val = pref.getInt(key.c_str(), i);
    if (val >= 0 && val < TOTAL_FITUR) mappingTombol[i] = val;
    else mappingTombol[i] = i;
  }
  pref.end();
}

void simpanSlot(int slotIndex, int fiturIndex) {
  mappingTombol[slotIndex] = fiturIndex;
  pref.begin("streamdock", false);
  String key = "slot" + String(slotIndex);
  pref.putInt(key.c_str(), fiturIndex);
  pref.end();
}

void resetKonfigurasiDefault() {
  pref.begin("streamdock", false);
  pref.clear();
  for (int i = 0; i < 6; i++) {
    mappingTombol[i] = i;
    String key = "slot" + String(i);
    pref.putInt(key.c_str(), i);
  }
  pref.end();
}

void setup() {
  Serial.begin(115200);

  pinMode(btnMenu, INPUT_PULLUP);
  pinMode(btnUp, INPUT_PULLUP);
  pinMode(btnDown, INPUT_PULLUP);
  pinMode(btnEnter, INPUT_PULLUP);

  for (int i = 0; i < 6; i++) pinMode(btnS[i], INPUT_PULLUP);

  muatKonfigurasi();

  Wire.begin(21, 22);
  Wire.setClock(100000);
  delay(100);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;)
      ;
  }

  display.clearDisplay();
  display.drawRoundRect(0, 0, 128, 64, 4, WHITE);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(16, 20);
  display.println("SYSTEM BOOTING");
  display.display();
  delay(800);

  bleKeyboard.begin();
  updateOLED();
}

bool tekanValid(int pin) {
  if (digitalRead(pin) == LOW) {
    delay(30);
    if (digitalRead(pin) == LOW) return true;
  }
  return false;
}

void tungguTombolDilepas(int pin) {
  while (digitalRead(pin) == LOW) { delay(10); }
  delay(30);
}

void loop() {
  bool currentBtStatus = bleKeyboard.isConnected();
  if (currentBtStatus != lastBtStatus) {
    lastBtStatus = currentBtStatus;
    updateOLED();
  }

  // ==========================================
  // STATE 0: DASHBOARD
  // ==========================================
  if (appState == 0) {
    if (tekanValid(btnMenu)) {
      tungguTombolDilepas(btnMenu);
      appState = 1;
      indexMenuUtama = 0;
      updateOLED();
    }
    // for (int i = 0; i < 6; i++) {
    //   if (tekanValid(btnS[i])) {
    //     tungguTombolDilepas(btnS[i]);
    //     eksekusiFitur(mappingTombol[i]);
    //   }
    // }
    for (int i = 0; i < 5; i++) {
      if (tekanValid(btnS[i])) {
        tungguTombolDilepas(btnS[i]);
        eksekusiFitur(mappingTombol[i]);
      }
    }
    if (tekanValid(btnUp)) {
      tungguTombolDilepas(btnUp);
      if (bleKeyboard.isConnected()) {
        bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
        showFeedback("VOLUME UP +");
      }
    } else if (tekanValid(btnDown)) {
      tungguTombolDilepas(btnDown);
      if (bleKeyboard.isConnected()) {
        bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
        showFeedback("VOLUME DOWN -");
      }
    } else if (tekanValid(btnEnter)) {
      tungguTombolDilepas(btnEnter);
      if (bleKeyboard.isConnected()) {
        bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
        showFeedback("PLAY / PAUSE");
      }
    }
  }

  // ==========================================
  // STATE 1: MENU UTAMA
  // ==========================================
  else if (appState == 1) {
    if (tekanValid(btnMenu)) {
      tungguTombolDilepas(btnMenu);
      appState = 0;
      updateOLED();
    } else if (tekanValid(btnUp)) {
      tungguTombolDilepas(btnUp);
      indexMenuUtama--;
      if (indexMenuUtama < 0) indexMenuUtama = 3;
      updateOLED();
    } else if (tekanValid(btnDown)) {
      tungguTombolDilepas(btnDown);
      indexMenuUtama++;
      if (indexMenuUtama > 3) indexMenuUtama = 0;
      updateOLED();
    } else if (tekanValid(btnEnter)) {
      tungguTombolDilepas(btnEnter);
      if (indexMenuUtama == 0) {
        appState = 2;
        indexFiturDipilih = 0;
        scrollMenu = 0;
        updateOLED();
      } else if (indexMenuUtama == 1) {
        appState = 4;
        indexMusicMenu = 0;
        updateOLED();
      } else if (indexMenuUtama == 2) {
        resetKonfigurasiDefault();
        appState = 0;
        showFeedback("DEFAULT RESET!");
      } else {
        appState = 0;
        updateOLED();
      }
    }
  }

  // ==========================================
  // STATE 2: PILIH FITUR
  // ==========================================
  else if (appState == 2) {
    if (tekanValid(btnMenu)) {
      tungguTombolDilepas(btnMenu);
      appState = 1;
      updateOLED();
    } else if (tekanValid(btnUp)) {
      tungguTombolDilepas(btnUp);
      indexFiturDipilih--;
      if (indexFiturDipilih < 0) indexFiturDipilih = TOTAL_FITUR - 1;
      updateOLED();
    } else if (tekanValid(btnDown)) {
      tungguTombolDilepas(btnDown);
      indexFiturDipilih++;
      if (indexFiturDipilih >= TOTAL_FITUR) indexFiturDipilih = 0;
      updateOLED();
    } else if (tekanValid(btnEnter)) {
      tungguTombolDilepas(btnEnter);
      appState = 3;
      indexSlotDipilih = 0;
      updateOLED();
    }
  }

  // ==========================================
  // STATE 3: PILIH SLOT TARGET
  // ==========================================
  else if (appState == 3) {
    if (tekanValid(btnMenu)) {
      tungguTombolDilepas(btnMenu);
      appState = 2;
      updateOLED();
    } else if (tekanValid(btnUp)) {
      tungguTombolDilepas(btnUp);
      indexSlotDipilih--;
      if (indexSlotDipilih < 0) indexSlotDipilih = 5;
      updateOLED();
    } else if (tekanValid(btnDown)) {
      tungguTombolDilepas(btnDown);
      indexSlotDipilih++;
      if (indexSlotDipilih > 5) indexSlotDipilih = 0;
      updateOLED();
    } else if (tekanValid(btnEnter)) {
      tungguTombolDilepas(btnEnter);
      simpanSlot(indexSlotDipilih, indexFiturDipilih);
      appState = 0;
      showFeedback("SLOT " + String(indexSlotDipilih + 1) + " SAVED!");
    }

    for (int i = 0; i < 5; i++) {
      if (tekanValid(btnS[i])) {
        tungguTombolDilepas(btnS[i]);
        simpanSlot(i, indexFiturDipilih);
        appState = 0;
        showFeedback("SLOT " + String(i + 1) + " SAVED!");
      }
    }
  }

  // ==========================================
  // STATE 4: MUSIC PLAYER
  // ==========================================
  else if (appState == 4) {
    if (tekanValid(btnMenu)) {
      tungguTombolDilepas(btnMenu);
      appState = 0;
      updateOLED();
    } else if (tekanValid(btnUp)) {
      tungguTombolDilepas(btnUp);
      indexMusicMenu--;
      if (indexMusicMenu < 0) indexMusicMenu = 3;
      updateOLED();
    } else if (tekanValid(btnDown)) {
      tungguTombolDilepas(btnDown);
      indexMusicMenu++;
      if (indexMusicMenu > 3) indexMusicMenu = 0;
      updateOLED();
    } else if (tekanValid(btnEnter)) {
      tungguTombolDilepas(btnEnter);
      if (bleKeyboard.isConnected()) {
        if (indexMusicMenu == 0) {
          bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
          showFeedback("PLAY / PAUSE");
        } else if (indexMusicMenu == 1) {
          bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
          showFeedback("NEXT TRACK >>");
        } else if (indexMusicMenu == 2) {
          bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
          showFeedback("<< PREV TRACK");
        } else if (indexMusicMenu == 3) {
          bleKeyboard.write(KEY_MEDIA_MUTE);
          showFeedback("MUTE AUDIO");
        }
      } else showFeedback("NO BLUETOOTH");
    }
  }
}

void eksekusiFitur(int kodeFitur) {
  if (!bleKeyboard.isConnected()) {
    showFeedback("NO BLUETOOTH");
    return;
  }
  switch (kodeFitur) {
    case 0:
      {
        isAudioMonitor = !isAudioMonitor;
        bleKeyboard.press(KEY_LEFT_GUI);
        bleKeyboard.press(KEY_LEFT_CTRL);
        bleKeyboard.press('v');
        delay(50);
        bleKeyboard.releaseAll();
        delay(400);
        for (int i = 0; i < 4; i++) {
          bleKeyboard.press(KEY_UP_ARROW);
          delay(20);
          bleKeyboard.releaseAll();
          delay(20);
        }
        int turun = isAudioMonitor ? 2 : 1;
        for (int i = 0; i < turun; i++) {
          bleKeyboard.press(KEY_DOWN_ARROW);
          delay(30);
          bleKeyboard.releaseAll();
          delay(30);
        }
        delay(50);
        bleKeyboard.press(KEY_RETURN);
        delay(50);
        bleKeyboard.releaseAll();
        delay(50);
        bleKeyboard.press(KEY_ESC);
        delay(50);
        bleKeyboard.releaseAll();
        showFeedback(isAudioMonitor ? "AUDIO: MON" : "AUDIO: IEM");
        break;
      }
    case 1:
      bleKeyboard.press(KEY_LEFT_GUI);
      bleKeyboard.press(KEY_PRTSC);
      delay(100);
      bleKeyboard.releaseAll();
      showFeedback("SCREENSHOT");
      break;
    case 2:
      bleKeyboard.press(KEY_LEFT_ALT);
      bleKeyboard.press(KEY_F10);
      delay(100);
      bleKeyboard.releaseAll();
      showFeedback("RECORDING");
      break;
    case 3:
      bleKeyboard.press(KEY_LEFT_CTRL);
      bleKeyboard.press(KEY_LEFT_SHIFT);
      bleKeyboard.press(KEY_ESC);
      delay(100);
      bleKeyboard.releaseAll();
      showFeedback("TASK MANAGER");
      break;
    case 4:
      bleKeyboard.press(KEY_LEFT_GUI);
      bleKeyboard.press(KEY_LEFT_ALT);
      bleKeyboard.press('k');
      delay(100);
      bleKeyboard.releaseAll();
      showFeedback("MIC TOGGLED");
      break;
    case 5:
      bleKeyboard.press(KEY_F13);
      delay(100);
      bleKeyboard.releaseAll();
      showFeedback("F13 DITEKAN");
      break;
    case 6:
      bleKeyboard.press(KEY_LEFT_GUI);
      bleKeyboard.press(KEY_LEFT_ALT);
      bleKeyboard.press('b');
      delay(100);
      bleKeyboard.releaseAll();
      showFeedback("HDR TOGGLED");
      break;
    case 7:
      bleKeyboard.press(KEY_LEFT_GUI);
      bleKeyboard.press(KEY_HOME);
      delay(100);
      bleKeyboard.releaseAll();
      showFeedback("FOCUS MODE");
      break;
    case 8:
      bleKeyboard.press(KEY_LEFT_GUI);
      bleKeyboard.press(KEY_LEFT_CTRL);
      bleKeyboard.press(KEY_LEFT_SHIFT);
      bleKeyboard.press('b');
      delay(100);
      bleKeyboard.releaseAll();
      showFeedback("RESET VGA");
      break;
    case 9:
      bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
      showFeedback("NEXT TRACK");
      break;
    case 10:
      bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
      showFeedback("PREV TRACK");
      break;
  }
}

void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.drawRect(0, 0, 128, 64, WHITE);
  display.setCursor(4, 4);

  if (appState == 0) display.print("DASHBOARD");
  else if (appState == 1) display.print("MENU UTAMA");
  else if (appState == 2) display.print("PILIH FITUR:");
  else if (appState == 3) display.print("PILIH SLOT (1-6):");
  else if (appState == 4) display.print("MUSIC PLAYER");

  if (bleKeyboard.isConnected()) display.drawBitmap(118, 3, icon_bluetooth, 5, 8, WHITE);
  else {
    display.drawLine(117, 3, 123, 9, WHITE);
    display.drawLine(123, 3, 117, 9, WHITE);
  }
  display.drawLine(0, 14, 128, 14, WHITE);

  if (appState == 0) {
    int xPos[3] = { 6, 47, 88 };
    int yPos[2] = { 18, 41 };
    for (int i = 0; i < 6; i++) {
      int col = i % 3;
      int row = i / 3;
      display.drawRoundRect(xPos[col], yPos[row], 34, 20, 3, WHITE);
      display.setCursor(xPos[col] + 8, yPos[row] + 6);
      display.print(namaPendek[mappingTombol[i]]);
    }
  } else if (appState == 1) {
    String opsiMenu[4] = { "1. Ubah Shortcut", "2. Music Player", "3. Reset Default", "4. Kembali" };
    int startM = (indexMenuUtama >= 2) ? (indexMenuUtama - 2) : 0;
    for (int i = 0; i < 3; i++) {
      int item = startM + i;
      if (item >= 4) break;
      int yPos = 18 + (i * 14);
      if (item == indexMenuUtama) {
        display.fillRect(2, yPos - 2, 124, 12, WHITE);
        display.setTextColor(BLACK);
      } else {
        display.setTextColor(WHITE);
      }
      display.setCursor(6, yPos);
      display.print(opsiMenu[item]);
    }
  } else if (appState == 2) {
    if (indexFiturDipilih < scrollMenu) scrollMenu = indexFiturDipilih;
    if (indexFiturDipilih >= scrollMenu + 3) scrollMenu = indexFiturDipilih - 2;
    for (int i = 0; i < 3; i++) {
      int item = scrollMenu + i;
      if (item >= TOTAL_FITUR) break;
      int yPos = 18 + (i * 14);
      if (item == indexFiturDipilih) {
        display.fillRect(2, yPos - 2, 124, 12, WHITE);
        display.setTextColor(BLACK);
      } else {
        display.setTextColor(WHITE);
      }
      display.setCursor(6, yPos);
      display.print(namaFitur[item]);
    }
  } else if (appState == 3) {
    int xPos[3] = { 6, 47, 88 };
    int yPos[2] = { 18, 41 };
    for (int i = 0; i < 6; i++) {
      int col = i % 3;
      int row = i / 3;
      if (i == indexSlotDipilih) {
        display.fillRoundRect(xPos[col], yPos[row], 34, 20, 3, WHITE);
        display.setTextColor(BLACK);
      } else {
        display.drawRoundRect(xPos[col], yPos[row], 34, 20, 3, WHITE);
        display.setTextColor(WHITE);
      }
      display.setCursor(xPos[col] + 8, yPos[row] + 6);
      display.print(namaPendek[mappingTombol[i]]);
    }
  } else if (appState == 4) {
    String musicMenu[4] = { "1. Play / Pause", "2. Next Track >>", "3. << Prev Track", "4. Mute Audio" };
    int startM = (indexMusicMenu >= 2) ? (indexMusicMenu - 2) : 0;
    for (int i = 0; i < 3; i++) {
      int item = startM + i;
      if (item >= 4) break;
      int yPos = 18 + (i * 14);
      if (item == indexMusicMenu) {
        display.fillRect(2, yPos - 2, 124, 12, WHITE);
        display.setTextColor(BLACK);
      } else {
        display.setTextColor(WHITE);
      }
      display.setCursor(6, yPos);
      display.print(musicMenu[item]);
    }
  }
  display.display();
}

void showFeedback(String text) {
  display.fillRect(10, 22, 108, 22, BLACK);
  display.drawRoundRect(10, 22, 108, 22, 4, WHITE);
  int textWidth = text.length() * 6;
  int cursorX = 10 + ((108 - textWidth) / 2);
  if (cursorX < 14) cursorX = 14;
  display.setTextColor(WHITE);
  display.setCursor(cursorX, 29);
  display.print(text);
  display.display();
  delay(700);
  updateOLED();
}