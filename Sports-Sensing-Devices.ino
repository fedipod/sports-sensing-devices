#include <M5StickCPlus.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>

const int httpsPort = 443; 

WiFiManager wifiManager;
WiFiManagerParameter custom_host("host", "Host", "", 40);
WiFiManagerParameter custom_token("token", "Access Token", "", 80);
WiFiClientSecure client;

String host;
String accessToken;
float bodyWeight = 70.0; 
int stepCount = 0;
int sitUpCount = 0;
float caloriesBurned = 0;
float previousAy = 0; 
float previousAz = 0; 
bool dataChanged = false;

unsigned long lastCheckTime = 0;  // 上一次检查的时间
const int CHECK_INTERVAL = 10000; // 10秒，单位为毫秒
String lastPostedStatus;          // 上一次发布到Mastodon的状态


const int MAX_RETRIES = 3;

void showMessage(String msg) {
  M5.Lcd.setRotation(1);
  M5.Lcd.setCursor(0, 10, 2);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.println(msg);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  showMessage("Please connect to this AP\nto config WiFi\nSSID: " + myWiFiManager->getConfigPortalSSID());
}

void setupWiFi() {
  wifiManager.addParameter(&custom_host);
  wifiManager.addParameter(&custom_token);
  
  wifiManager.setAPCallback(configModeCallback);
  bool doManualConfig = false;
  showMessage("Push POWER button to enter Wifi config.");
  for(int i=0 ; i<200 ; i++) {
    M5.update();
    if (M5.Axp.GetBtnPress()) {
      doManualConfig = true;
      break;
    }
    delay(10);
  }

  if (doManualConfig) {
    Serial.println("wifiManager.startConfigPortal()");
    if (!wifiManager.startConfigPortal()) {
      Serial.println("startConfigPortal() connect failed!");
      showMessage("Wi-Fi failed.");
      return;
    }
  } else {
    showMessage("Wi-Fi connecting...");
    Serial.println("wifiManager.autoConnect()");
    if (!wifiManager.autoConnect()) {
      Serial.println("autoConnect() connect failed!");
      showMessage("Wi-Fi failed.");
      return;
    }
  }

  // Retrieve custom parameters
  host = custom_host.getValue();
  accessToken = custom_token.getValue();

  String IPaddress = WiFi.localIP().toString();
  showMessage("Wi-Fi connected.\nIP Address: " + IPaddress);
}

void postToMastodon(String status) {
  int retries = 0;
  
  while(retries < MAX_RETRIES) {
    if (!client.connect(host.c_str(), httpsPort)) {
      Serial.println("Connection failed! Retrying...");
      retries++;
      delay(1000);  // wait a second before retry
      continue;
    }

    String postData = "status=" + status;
    client.println("POST /api/v1/statuses HTTP/1.1");
    client.println("Host: " + String(host));
    client.println("User-Agent: M5StickCPlus");
    client.println("Authorization: Bearer " + accessToken);
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(postData.length());
    client.println();
    client.println(postData);

    // Read the full response from Mastodon for debugging and to ensure the buffer is cleared
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
          break;
      }
      Serial.println(line);
    }
    
    break;  // If everything goes well, break the while loop
  }
}

void setup() {
  M5.begin();
  M5.Lcd.begin();
  M5.Lcd.setRotation(3); 
  M5.IMU.Init();

  Serial.begin(115200);
  client.setInsecure();

  setupWiFi();
}

void loop() {
  float ax, ay, az, gx, gy, gz;
  M5.IMU.getAccelData(&ax, &ay, &az);
  M5.IMU.getGyroData(&gx, &gy, &gz);

  if (ay > 0.5 && previousAy < 0.5) {
    stepCount++;
    caloriesBurned += (0.035 * bodyWeight / 30);
    dataChanged = true;
  }

  if (az > 0.5 && previousAz < 0.5 && gy > 0.5) {
    sitUpCount++;
    caloriesBurned += 0.8; 
    dataChanged = true;
  }

  previousAy = ay;
  previousAz = az;

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2); 
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("Steps: " + String(stepCount));
  M5.Lcd.println("Sit-ups: " + String(sitUpCount));
  M5.Lcd.println("Calories: " + String(caloriesBurned, 2));

  if (millis() - lastCheckTime >= CHECK_INTERVAL) { // 检查是否已经过去10秒
    lastCheckTime = millis(); // 重置计时器

    String currentStatus = "Steps: " + String(stepCount) + ", Sit-ups: " + String(sitUpCount) + ", Calories: " + String(caloriesBurned, 2);

    // 只有当当前状态与上一次发布到Mastodon的状态不同时才发布
    if (currentStatus != lastPostedStatus) {
      postToMastodon(currentStatus);
      lastPostedStatus = currentStatus; // 更新上次发布的状态
    }
  }

  delay(1000); 
}
