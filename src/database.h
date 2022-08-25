#pragma once
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

FirebaseAuth fbAuth;
FirebaseConfig fbConfig;
FirebaseData fbdo;
FirebaseData secondary;
FirebaseData stream;

bool send(const char* path, bool value) { // и разобраться c f макро
  bool ok = Firebase.RTDB.setBool(&fbdo, F(path), value);
  if (!ok) {
    Serial.print("Ошибка при отправке в ");
    Serial.print(path);
    Serial.print(" ");
    Serial.println(fbdo.errorReason());
  }
  return ok;
}

bool send(const char* path, const char* value) {
  bool ok = Firebase.RTDB.setString(&fbdo, F(path), value);
  if (!ok) {
    Serial.print("Ошибка при отправке в ");
    Serial.print(path);
    Serial.print(" ");
    Serial.println(fbdo.errorReason());
  }
  return ok;
}

bool send(const char* path, unsigned int value) {
  bool ok = Firebase.RTDB.setInt(&fbdo, F(path), value);
  if (!ok) {
    Serial.print("Ошибка при отправке в ");
    Serial.print(path);
    Serial.print(" ");
    Serial.println(fbdo.errorReason());
  }
  return ok;
}

bool send(const char* path, unsigned long value) {
  bool ok = Firebase.RTDB.setInt(&fbdo, F(path), value);
  if (!ok) {
    Serial.print("Ошибка при отправке в ");
    Serial.print(path);
    Serial.print(" ");
    Serial.println(fbdo.errorReason());
  }
  return ok;
}

bool send(const char* path, float value) {
  bool ok = Firebase.RTDB.setFloat(&fbdo, F(path), value);
  if (!ok) {
    Serial.print("Ошибка при отправке в ");
    Serial.print(path);
    Serial.print(" ");
    Serial.println(fbdo.errorReason());
  }
  return ok;
}

void heartbeat() {
  if (!Firebase.RTDB.setBool(&secondary, F("current/connected"), true)) {
    Serial.print(F("Ошибка в heartbeat "));
    Serial.println(secondary.errorReason());
  }
}
