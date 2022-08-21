#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

#include "TimersDispatcher.h"
#include "config.h"

TimersDispatcher<unsigned long> async(millis, 4294967);

FirebaseAuth fbAuth;
FirebaseConfig fbConfig;
FirebaseData fbdo;
FirebaseData stream;

WiFiUDP ntpUDP;
NTPClient wifiTime(ntpUDP);

bool ready;
bool inited;
TaskId* servicesInitialization;
TaskId* everySecondLoopId;
TaskId* dataSendingTask;

struct {
  bool running;
  int top;
  int down;
  int cycles;
} fetchedInfo;

bool moving;
bool lowered;
byte state = 0; // 0 - начало вверху 1 - опускается 2 - внизу 3 - поднимается 4 - вверху
unsigned int completedCycles;
unsigned long currentTime;
unsigned long targetTime;
float temp = 0;

// bool aborted;
// int start;
bool completed;
unsigned long finish;
// // char* name; это кажется уже для блютуза
// // char* abortReason;
// // char* description;

void completeProcess() {
  fetchedInfo.running = false;
  completed = true;
  finish = currentTime;
  if (WiFi.status() == WL_CONNECTED) {
    if (!Firebase.RTDB.setBoolAsync(&fbdo, F("current/running"), false)) {
      Serial.println("Ошибка при отправке running: " + fbdo.errorReason());
    }
    if (!Firebase.RTDB.setBoolAsync(&fbdo, F("current/completed"), true)) {
      Serial.println("Ошибка при отправке completed: " + fbdo.errorReason());
    }
    if (!Firebase.RTDB.setIntAsync(&fbdo, F("current/finish"), finish)) {
      Serial.println("Ошибка при отправке finish: " + fbdo.errorReason());
    }
  }
  Serial.println(F("Процесс завершён"));
}

void stopProcess() {
  fetchedInfo.running = false;
  if (state < 3) {
    //функция для подъёма
  }
  state = 0;
}

void abortProcess(const char* reason) {
  fetchedInfo.running = false;
  Firebase.RTDB.setBoolAsync(&fbdo, F("current/running"), false);
  Firebase.RTDB.setBoolAsync(&fbdo, F("current/aborted"), true);
  Firebase.RTDB.setStringAsync(&fbdo, F("current/abortReason"), reason);
  Serial.println(reason);
}

void processLoop() {
  if (state == 0 && fetchedInfo.running) {
    state = 1;
    //функция для опускания
    Serial.println(F("Процесс запущен"));
  } else if (state == 1 && !moving) {
    state = 2;
    targetTime = currentTime + fetchedInfo.down;
    lowered = true;
    // отправить lowered true
  } else if (state == 2 && currentTime >= targetTime) {
    state = 3;
    //функция для подъёма
  } else if (state == 3 && !moving) {
    state = 4;
    targetTime = currentTime + fetchedInfo.top;
    lowered = false;
    // отправить lowered false
  } else if (state == 4 && currentTime >= targetTime) {
    completedCycles++;
    Serial.print(completedCycles);
    Serial.println(" цикл завершён");
    // отправить current/completedCycles
    if (completedCycles >= fetchedInfo.cycles) {
      state = 0;
      completeProcess();
    } else {
      state = 1;
      //функция для опускания
    }
  }
}

void checkInterruptSensors() {
  if (moving && (digitalRead(1) || digitalRead(2))) {
    // стоп
    moving = false;
  }
}

///////////////////////////

void getData() {
  fetchedInfo.running = true;
  if (Firebase.RTDB.getInt(&fbdo, F("/current/top"))) {
    fetchedInfo.top = fbdo.to<int>();
  }
  if (Firebase.RTDB.getInt(&fbdo, F("/current/down"))) {
    fetchedInfo.down = fbdo.to<int>();
  }
  if (Firebase.RTDB.getInt(&fbdo, F("/current/cycles"))) {
    fetchedInfo.cycles = fbdo.to<int>();
  }
}

void sendAllData() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!Firebase.RTDB.setIntAsync(&fbdo, F("current/running"), false)) {
      Serial.println("Ошибка при отправке running: " + fbdo.errorReason());
    }
    if (!Firebase.RTDB.setBoolAsync(&fbdo, F("current/completed"), true)) {
      Serial.println("Ошибка при отправке completed: " + fbdo.errorReason());
    }
    if (!Firebase.RTDB.setBoolAsync(&fbdo, F("current/lowered"), false)) {
      Serial.println("Ошибка при отправке lowered: " + fbdo.errorReason());
    }
    if (!Firebase.RTDB.setIntAsync(&fbdo, F("current/finish"), finish)) {
      Serial.println("Ошибка при отправке finish: " + fbdo.errorReason());
    }
    if (!Firebase.RTDB.setIntAsync(&fbdo, F("current/currentTime"), currentTime)) {
      Serial.println("Ошибка при отправке currentTime: " + fbdo.errorReason());
    }
    if (!Firebase.RTDB.setFloatAsync(&fbdo, F("current/temp"), temp)) {
      Serial.println("Ошибка при отправке temp: " + fbdo.errorReason());
    }
    if (!Firebase.RTDB.setIntAsync(&fbdo, F("current/completedCycles"), completedCycles)) {
      Serial.println("Ошибка при отправке completedCycles: " + fbdo.errorReason());
    }
    async.clearInterval(dataSendingTask);
    dataSendingTask = nullptr;
  }
}

void sendVolatileData() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!Firebase.RTDB.setIntAsync(&fbdo, F("current/currentTime"), currentTime)) {
      Serial.println("Ошибка при отправке данных времени: " + fbdo.errorReason());
    }
    if (!Firebase.RTDB.setFloatAsync(&fbdo, F("current/temp"), temp)) {
      Serial.println("Ошибка при отправке данных температуры: " + fbdo.errorReason());
    }
  }
}

///////////////////////////

void firebaseStream(FirebaseStream data) {
  Serial.println(F("Получены данные"));
  bool running = data.to<bool>();
  if (running) {
    if (ready) {
      getData();
      processLoop();
    } else {
      abortProcess("Прерван из-за отключения питания");
    }
  } else {
    if (fetchedInfo.running) {
      stopProcess();
    } else if (!ready) {
      ready = true;
    }
  }
}

void firebaseStreamTimeout(bool timeout) {
  if (timeout) {
    Serial.println(F("firebase stream timed out"));
  }
  if (!stream.httpConnected()) {
    Serial.println(stream.errorReason().c_str());
  }
}

void initFirebase() {
  fbConfig.host = config.host;
  fbConfig.api_key = config.apiKey;
  fbAuth.user.email = config.email;
  fbAuth.user.password = config.fbPassword;
  fbConfig.token_status_callback = tokenStatusCallback;
  Firebase.begin(&fbConfig, &fbAuth);
  Firebase.reconnectWiFi(true);
  if (!Firebase.RTDB.beginStream(&stream, F("/current/running"))) {
    Serial.println(stream.errorReason().c_str());
  }
  Firebase.RTDB.setStreamCallback(&stream, firebaseStream, firebaseStreamTimeout);
}

///////////////////////////

void heartbeat() {
  if (!Firebase.RTDB.setBoolAsync(&fbdo, F("current/connected"), true)) {
    Serial.println("Ошибка в heartbeat: " + fbdo.errorReason());
  }
}

void everySecondLoop() {
  if (fetchedInfo.running) {
    currentTime = wifiTime.getEpochTime();
    // считываем температуру
  }
  if (WiFi.status() == WL_CONNECTED) {
    if (Firebase.ready()) {
      heartbeat();
      sendVolatileData();
    }
  }
}

void initServices() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("Инициализация служб"));
    initFirebase();
    wifiTime.begin();
    inited = true;
    async.clearInterval(servicesInitialization);
    servicesInitialization = nullptr;
  }
}

void onWifiConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println(F("Подключено к wifi"));
  if (!inited) {
    servicesInitialization = async.setInterval(initServices, 500);
  } else if (completed) {
    dataSendingTask = async.setInterval(sendAllData, 500);
  }
  everySecondLoopId = async.setInterval(everySecondLoop, 1000);
}

void onWifiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println(F("Соединение с wifi разорвано, переподключение.."));
  if (servicesInitialization != nullptr) {
    async.clearInterval(servicesInitialization);
    servicesInitialization = nullptr;
  }
  if (everySecondLoopId != nullptr) {
    async.clearInterval(everySecondLoopId);
    everySecondLoopId = nullptr;
  }
  if (dataSendingTask != nullptr) {
    async.clearInterval(dataSendingTask);
    dataSendingTask = nullptr;
  }
  // WiFi.begin(config.ssid, config.wifiPassword);
}

void initWifi() {
  WiFi.setAutoReconnect(true);
  WiFi.setAutoConnect(true);
  WiFi.persistent(true);
  WiFi.onEvent(onWifiConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(onWifiDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.begin(config.ssid, config.wifiPassword);
  Serial.println(F("Подключение к wifi.."));
}

///////////////////////////

void setup() {
  Serial.begin(115200);
  // pinMode(21, OUTPUT);
  initWifi();
  //едем вверх
}

void loop() {
  wifiTime.update();
  async.loop();
  processLoop();
  checkInterruptSensors();
}
