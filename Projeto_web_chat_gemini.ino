#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ======== CONFIG WiFi ========
const char* ssid = "SUA_REDE";
const char* password = "SUA_SENHA";

// ======== CONFIG API GEMINI ========
const char* apiKey = "SUA_API_KEY";
const char* gemini_url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=";
// ======== SERVIDOR WEB ========

WebServer server(80);
// ======== PÁGINA HTML (Interface) ========

String createPage(String response = "") {
  String html = "<!DOCTYPE html><html lang='pt'><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>body{font-family:sans-serif; margin:20px;} input[type='text']{width:80%; padding:10px;} .res{background:#f0f0f0; padding:15px; border-radius:5px; margin-top:20px;}</style>";
  html += "<title>ESP32 Gemini AI</title></head><body>";
  html += "<h1>Pergunte à Gemini</h1>";
  html += "<form action='/ask' method='POST'>";
  html += "<input type='text' name='question' placeholder='Digite sua pergunta aqui...' required><br><br>";
  html += "<input type='submit' value='Enviar Pergunta'></form>";
  if (response != "") {
    html += "<div class='res'><h2>Resposta:</h2><p>" + response + "</p></div>";
  }
  html += "</body></html>";
  return html;
}

// ======== FUNÇÃO PARA CHAMAR A API (Lógica ArduinoJson) ========
String askGemini(String prompt) {
  if (WiFi.status() != WL_CONNECTED) return "Erro: WiFi desconectado";
  HTTPClient http;
  String url = String(gemini_url) + String(apiKey);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(30000); 
  // Montar JSON usando ArduinoJson
  DynamicJsonDocument doc(8192);
  doc["contents"][0]["parts"][0]["text"] = prompt;
  String requestBody;
  serializeJson(doc, requestBody);
  int httpResponseCode = http.POST(requestBody);
  String result = "";
  if (httpResponseCode > 0) {
    String payload = http.getString();
    // Interpretar Resposta
    DynamicJsonDocument responseDoc(4096);
    DeserializationError error = deserializeJson(responseDoc, payload);
    if (!error) {
      const char* geminiText = responseDoc["candidates"][0]["content"]["parts"][0]["text"];
      if (geminiText) {
        result = String(geminiText);
      } else {
        result = "Erro: Resposta vazia da API.";
      }
    } else {
      result = "Erro ao processar resposta (JSON).";
    }
  } else {
    result = "Erro na conexão HTTP: " + String(httpResponseCode);
  }
  http.end();
  return result;
}

// ======== ROTAS DO SERVIDOR ========
void handleRoot() {
  server.send(200, "text/html", createPage());
}
void handleAsk() {
  if (server.hasArg("question")) {
    String userQuestion = server.arg("question");
    Serial.println("Pergunta recebida: " + userQuestion);
    String geminiReply = askGemini(userQuestion);
    server.send(200, "text/html", createPage(geminiReply));
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

// ======== SETUP ========
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("\n--- Conectado! ---");
  Serial.print("Acesse no navegador: http://");
  Serial.println(WiFi.localIP());
  server.on("/", HTTP_GET, handleRoot);
  server.on("/ask", HTTP_POST, handleAsk);
  server.begin();
}
void loop() {
  server.handleClient();
}
