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

void send(const char* path, bool value) { // и разобраться c f макро
  if (!Firebase.RTDB.setBool(&fbdo, F(path), value)) {
    Serial.print("Ошибка при отправке в ");
    Serial.print(path);
    Serial.print(" ");
    Serial.println(fbdo.errorReason());
  }
}

void send(const char* path, const char* value) {
  if (!Firebase.RTDB.setString(&fbdo, F(path), value)) {
    Serial.print("Ошибка при отправке в ");
    Serial.print(path);
    Serial.print(" ");
    Serial.println(fbdo.errorReason());
  }
}

void send(const char* path, unsigned int value) {
  if (!Firebase.RTDB.setInt(&fbdo, F(path), value)) {
    Serial.print("Ошибка при отправке в ");
    Serial.print(path);
    Serial.print(" ");
    Serial.println(fbdo.errorReason());
  }
}

void send(const char* path, unsigned long value) {
  if (!Firebase.RTDB.setInt(&fbdo, F(path), value)) {
    Serial.print("Ошибка при отправке в ");
    Serial.print(path);
    Serial.print(" ");
    Serial.println(fbdo.errorReason());
  }
}

void send(const char* path, float value) {
  if (!Firebase.RTDB.setFloat(&fbdo, F(path), value)) {
    Serial.print("Ошибка при отправке в ");
    Serial.print(path);
    Serial.print(" ");
    Serial.println(fbdo.errorReason());
  }
}

void heartbeat() {
  if (!Firebase.RTDB.setBool(&secondary, F("current/connected"), true)) {
    Serial.print(F("Ошибка в heartbeat "));
    Serial.println(secondary.errorReason());
  }
}
