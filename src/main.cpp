#include <Arduino.h>
#include <WiFi.h>

#include "TimersDispatcher.h"
#include "database.h"
#include "config.h"

TimersDispatcher<unsigned long> async(millis, 4294967); //проверить так ли это

bool ready;
bool inited;
TaskId* servicesInitialization;
TaskId* dataSending;

bool moving;
byte state = 0; // 0 - не запущен 1 - опускается 2 - внизу 3 - поднимается 4 - вверху
void state0(), state1(), state2(), state3(), state4();
void (*processLoop[5])() = {state0, state1, state2, state3, state4};
unsigned long targetTime;

struct {
  bool running;
  unsigned int top;
  unsigned int down;
  unsigned int cycles;
} fetched;

struct {
  float temp = 0;
  unsigned long currentTime;
  unsigned int completedCycles;
  bool lowered;
  unsigned long start;
  unsigned long finish;
  bool completed;
} published;

// bool aborted;
// // char* name; это кажется уже для блютуза
// // char* abortReason;
// // char* description;

void checkIntersectionSensors();
bool everySecondLoop();

void completeProcess();
void stopProcess();
void abortProcess(const char* reason);

void getData();
bool sendAllData();
void updateWifiTime();
bool sendLowered();

void initWifi();
void onWifiConnected(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
bool initServices();

void initFirebase();
void firebaseStream(FirebaseStream data);
void firebaseStreamTimeout(bool timeout);

void setup() {
  Serial.begin(115200);
  // pinMode(21, OUTPUT);
  initWifi();
  async.setInterval(everySecondLoop, 1000);
}

void loop() {
  async.loop();
  // checkIntersectionSensors(); // можно повесить на прерывания, причём оба на один пин
}

void checkIntersectionSensors() {
  if (moving && (digitalRead(1) || digitalRead(2))) {
    // стоп
    moving = false;
  }
}

bool everySecondLoop() {
  if (fetched.running) {
    // считываем температуру
    if (WiFi.status() == WL_CONNECTED) {
      if (Firebase.ready()) {
        heartbeat();
        send("current/temp", published.temp);
        updateWifiTime();
      }
    }
    processLoop[state]();
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      if (Firebase.ready()) {
        heartbeat();
      }
    }
  }
  return false;
}

void state0() { // не запущен
  if (fetched.running) {
    state = 1;
    //функция для опускания
    published.completedCycles = 0;
    published.start = published.currentTime; // возможно отправлять для точности
    Serial.print(F("Процесс запущен sec: "));
    Serial.println(published.currentTime);
    Serial.print(F("top: "));
    Serial.println(fetched.top);
    Serial.print(F("down: "));
    Serial.println(fetched.down);
    Serial.print(F("cycles: "));
    Serial.println(fetched.cycles);
  } else {
    Serial.println(F("Сработал тот самый if")); // очень редко, но срабатывает из-за многопоточности
  }
}

void state1() { // опускается
  if (!moving) {
    state = 2;
    targetTime = published.currentTime + fetched.down;
    published.lowered = true;
    Serial.print(F("Опустился: "));
    Serial.println(millis());
    send("current/lowered", true); // отправлять только если подключено
  }
}

void state2() { // внизу
  if (published.currentTime >= targetTime) {
    state = 3;
    // функция для подъёма
    Serial.print(F("Поднимается: "));
    Serial.println(millis());
  }
}

void state3() { // поднимается
  if (!moving) {
    state = 4;
    targetTime = published.currentTime + fetched.top;
    published.lowered = false;
    Serial.print(F("Поднялся: "));
    Serial.println(millis());
    send("current/lowered", false); // отправлять только если подключено
  }
}

void state4() { // вверху
  if (published.currentTime >= targetTime) {
    published.completedCycles++;
    Serial.print(published.completedCycles);
    Serial.print(F(" цикл завершён sec: "));
    Serial.print(published.currentTime);
    Serial.print(F(" millis: "));
    Serial.println(millis());
    send("current/completedCycles", published.completedCycles); // отправлять только если подключено
    if (published.completedCycles == fetched.cycles) {
      state = 0;
      completeProcess();
    } else {
      state = 1;
      //функция для опускания
    }
  }
}

/////////////////////////// process helpers

void completeProcess() {
  fetched.running = false;
  published.completed = true;
  published.finish = published.currentTime;
  if (WiFi.status() == WL_CONNECTED) {
    send("current/running", false);
    send("current/completed", true);
    send("current/finish", published.finish);
  }
  Serial.print(F("Процесс завершён sec: "));
  Serial.println(published.currentTime);
}

void stopProcess() {
  fetched.running = false;
  published.lowered = false;
  if (state < 3) {
    //функция для подъёма
  }
  state = 0;
  Serial.println(F("Процесс остановлен"));
}

void abortProcess(const char* reason) {
  send("current/running", false);
  send("current/aborted", true);
  send("current/abortReason", reason);
  Serial.println(reason);
}

/////////////////////////// database

void getData() {
  if (Firebase.RTDB.getInt(&fbdo, F("/current/top"))) {
    fetched.top = fbdo.to<int>();
  }
  if (Firebase.RTDB.getInt(&fbdo, F("/current/down"))) {
    fetched.down = fbdo.to<int>();
  }
  if (Firebase.RTDB.getInt(&fbdo, F("/current/cycles"))) {
    fetched.cycles = fbdo.to<int>();
  }
  fetched.running = true;
}

bool sendAllData() {
  if (WiFi.status() == WL_CONNECTED) {
    if (Firebase.ready()) {
      send("current/running", false);
      send("current/completed", true);
      send("current/lowered", false);
      // возможно отправлять start, но будет актуально только с блютузом
      send("current/finish", published.finish);
      send("current/currentTime", published.currentTime);
      send("current/temp", published.temp);
      send("current/completedCycles", published.completedCycles);
      dataSending = nullptr;
      return true;
    }
  }
  return false;
}

void updateWifiTime() {
  Firebase.RTDB.setTimestamp(&secondary, F("/current/currentTime"));
  published.currentTime = secondary.to<int>();
}

/////////////////////////// network init functions

void initWifi() {
  WiFi.setAutoReconnect(true);
  WiFi.setAutoConnect(true);
  WiFi.persistent(true);
  WiFi.onEvent(onWifiConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(onWifiDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.begin(config.ssid, config.wifiPassword);
  Serial.println(F("Подключение к wifi.."));
}

void onWifiConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (!inited) {
    servicesInitialization = async.setInterval(initServices, 500);
  } else if (published.completed) {
    dataSending = async.setInterval(sendAllData, 500);
  }
  Serial.println(F("Подключено к wifi"));
}

void onWifiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (servicesInitialization != nullptr) {
    async.clearInterval(servicesInitialization);
    servicesInitialization = nullptr;
  }
  if (dataSending != nullptr) {
    async.clearInterval(dataSending);
    dataSending = nullptr;
  }
  Serial.println(F("Соединение с wifi разорвано, переподключение.."));
  // WiFi.begin(config.ssid, config.wifiPassword);
}

bool initServices() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("Инициализация сетевых служб"));
    initFirebase();
    inited = true;
    servicesInitialization = nullptr;
    Serial.println(F("Сетевые службы проинициализированы"));
    return true;
  } else {
    Serial.println(F("Попытка инициализации сетевых служб провалена, плохое соединение.."));
    return false;
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

// по идее вызывается на втором ядре параллельно с loop
void firebaseStream(FirebaseStream data) {
  bool running = data.to<bool>();
  Serial.print(F("Получено running: "));
  Serial.println(running);
  if (running) {
    if (ready) {
      getData();
    } else {
      abortProcess("Прерван из-за отключения питания");
    }
  } else {
    if (fetched.running) {
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
