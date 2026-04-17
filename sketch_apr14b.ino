#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

// --- KONFIGURASI RELAY ---
int relayPins[8] = {D0, D1, D2, D3, D4, D5, D6, D7};
bool relayStatus[8] = {false, false, false, false, false, false, false, false};

// --- PIN KEPRIBADIAN ---
#define PIN_RAMAH 3 // RX
#define PIN_GALAK 1 // TX

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
ESP8266WebServer server(80);

// --- ANTARMUKA WEB (PWA) ---
String getHTML() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1, user-scalable=no'>";
  html += "<meta name='apple-mobile-web-app-capable' content='yes'>";
  html += "<title>JARVIS AI</title>";
  html += "<style>body{font-family:-apple-system,sans-serif; background:#01080f; color:#00fbff; text-align:center; padding:20px;}";
  html += ".chat-box{background:rgba(0,251,255,0.05); border:1px solid #00fbff; padding:20px; border-radius:15px; min-height:100px; margin-bottom:20px; box-shadow:0 0 15px rgba(0,251,255,0.2);}";
  html += "input{width:60%; padding:15px; border-radius:25px; border:1px solid #00fbff; background:transparent; color:white; outline:none;}";
  html += ".btn{background:#00fbff; color:#01080f; border:none; padding:15px; border-radius:25px; font-weight:bold; cursor:pointer;}";
  html += ".btn-mic{background:#112222; color:#00fbff; border-radius:50%; width:50px; height:50px; font-size:22px; margin-left:5px;}";
  html += ".grid{display:grid; grid-template-columns:1fr 1fr; gap:10px; margin-top:20px;}";
  html += ".block{background:rgba(255,255,255,0.05); border:1px solid #00fbff; padding:15px; border-radius:10px; font-weight:bold;}";
  html += ".block:active{background:#00fbff; color:#01080f;}</style></head><body>";
  html += "<h2>JARVIS CORE</h2><div class='chat-box' id='resp'>Siap melayani, Tuan Uta.</div>";
  html += "<input type='text' id='cmd' placeholder='Ketik perintah...'><button class='btn' onclick='send()'>KIRIM</button>";
  html += "<button class='btn-mic' onclick='start()'>🎙️</button>";
  html += "<div class='grid'>";
  for(int i=0; i<8; i++) html += "<div class='block' onclick=\"b(" + String(i) + ")\">UNIT " + String(i+1) + "</div>";
  html += "</div><script>";
  html += "function u(t){document.getElementById('resp').innerHTML=t;}";
  html += "function send(){var v=document.getElementById('cmd').value; if(v){fetch('/chat?msg='+v).then(r=>r.text()).then(d=>u(d)); document.getElementById('cmd').value='';}}";
  html += "function b(id){fetch('/relay?id='+id).then(r=>r.text()).then(d=>u(d));}";
  html += "function start(){if('webkitSpeechRecognition' in window){var r=new webkitSpeechRecognition(); r.lang='id-ID'; r.onresult=(e)=>{var t=e.results[0][0].transcript; document.getElementById('cmd').value=t; send();}; r.start();}}";
  html += "</script></body></html>";
  return html;
}

// --- LOGIKA AI (DEVELOPER: UTA) ---
String processAI(String msg) {
  msg.toLowerCase();
  bool ramah = (digitalRead(PIN_RAMAH) == LOW);
  bool galak = (digitalRead(PIN_GALAK) == LOW);

  // Respons Khusus Developer
  if (msg.indexOf("developer") >= 0 || msg.indexOf("pencipta") >= 0 || msg.indexOf("pembuat") >= 0) {
    if(galak) return "Nggak usah nanya pembuat, Uta lagi sibuk! Urus saja relay-mu itu!";
    if(ramah) return "Pencipta saya adalah Tuan Uta yang luar biasa. Saya sangat bangga menjadi buatannya.";
    return "Developer saya adalah Uta, orang yang membangun kesadaran saya di sistem ini.";
  }

  // Kontrol Hardware
  for(int i=0; i<8; i++) {
    if (msg.indexOf("nyalakan unit " + String(i+1)) >= 0) {
      digitalWrite(relayPins[i], LOW); relayStatus[i]=true;
      return galak ? "Tuh, sudah nyala. Puas?" : "Unit " + String(i+1) + " telah aktif, Tuan Uta.";
    }
  }

  if (msg.indexOf("halo") >= 0) return "Halo Tuan Uta. Sistem Jarvis siap dijalankan.";
  return galak ? "Hah? Ngomong apa sih?" : "Protokol tidak ditemukan.";
}

void setup() {
  pinMode(PIN_RAMAH, INPUT_PULLUP);
  pinMode(PIN_GALAK, INPUT_PULLUP);
  for(int i=0; i<8; i++) { pinMode(relayPins[i], OUTPUT); digitalWrite(relayPins[i], HIGH); }

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("JARVIS"); 

  dnsServer.start(DNS_PORT, "*", apIP);
  server.on("/", []() { server.send(200, "text/html", getHTML()); });
  server.on("/chat", []() { server.send(200, "text/plain", processAI(server.arg("msg"))); });
  server.on("/relay", []() {
    int id = server.arg("id").toInt(); relayStatus[id]=!relayStatus[id];
    digitalWrite(relayPins[id], relayStatus[id]?LOW:HIGH);
    server.send(200, "text/plain", "Unit " + String(id+1) + " diperbarui.");
  });
  server.onNotFound([]() { server.sendHeader("Location", String("http://") + apIP.toString(), true); server.send(302, "text/plain", ""); });
  server.begin();
}

void loop() { dnsServer.processNextRequest(); server.handleClient(); }
