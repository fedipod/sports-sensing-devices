#include <M5StickCPlus.h>
#include <WiFiManager.h>

WiFiManager wifiManager;

// WiFi configuration successful flag
bool isWifiConfigSucceeded = false;

// Display message on LCD
void showMessage(String msg)
{
  M5.Lcd.setRotation(1);
  M5.Lcd.setCursor(0, 10, 2);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.println(msg);
}

// Callback when entering WiFi configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  showMessage("Please connect to this AP\nto config WiFi\nSSID: " + myWiFiManager->getConfigPortalSSID());
}

// If the power button is pressed immediately after booting, switch to WiFi configuration mode, otherwise auto-connect
void setupWiFi()
{
  wifiManager.setAPCallback(configModeCallback);

  // clicking power button at boot time to enter wifi config mode
  bool doManualConfig = false;
  showMessage("Push POWER button to enter Wifi config.");
  for(int i=0 ; i<200 ; i++) {
    M5.update();
    if (M5.BtnA.isPressed()) { // Using BtnP for M5StickCPlus
      doManualConfig = true;
      break;
    }
    delay(10);
  }

  if (doManualConfig) {
    Serial.println("wifiManager.startConfigPortal()");
    if (wifiManager.startConfigPortal()) {
      isWifiConfigSucceeded = true;
      Serial.println("startConfigPortal() connect success!");
    }
    else {
      Serial.println("startConfigPortal() connect failed!");
    }
  } else {
    showMessage("Wi-Fi connecting...");

    Serial.println("wifiManager.autoConnect()");
    if (wifiManager.autoConnect()) {
      isWifiConfigSucceeded = true;
      Serial.println("autoConnect() connect success!");
    }
    else {
      Serial.println("autoConnect() connect failed!");
    }
  }

  if (isWifiConfigSucceeded) {
    // showMessage("Wi-Fi connected.");
    String IPaddress = WiFi.localIP().toString();
    showMessage("Wi-Fi connected.\nIP Address: " + IPaddress);
  } else {
    showMessage("Wi-Fi failed.");
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("M5StickCPlus started."); // Changed to M5StickCPlus
  M5.begin();
  setupWiFi();
}

void loop() {
  // put your main code here, to run repeatedly:

}
