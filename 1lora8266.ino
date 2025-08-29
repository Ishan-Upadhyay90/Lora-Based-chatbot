#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <SoftwareSerial.h>

// ========== LoRa UART (RYLR896) ==========
#define LORA_RX 13   // D7
#define LORA_TX 15   // D8
SoftwareSerial lora(LORA_RX, LORA_TX); // RX, TX

// ========== Wi-Fi AP ==========
const char* ssid     = "ESP8266_Chat";
const char* password = "12345678";

// ========== Web ==========
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// ========== Buffers ==========
String lineBuf;

// ======== Pro UI (shared HTML) ========
const char index_html[] PROGMEM = R"HTML(
<!DOCTYPE html><html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>LoRa Chat</title>
<style>
:root{
  --bg:#F3F5F7; --card:#FFFFFF; --primary:#0B72E7; --primary-d:#0858B5;
  --bubble-me:#0B72E7; --bubble-other:#FFFFFF; --text:#1F2937; --muted:#6B7280; --ring:#E5E7EB;
}
*{box-sizing:border-box} body{margin:0;background:var(--bg);font-family:"Segoe UI",Roboto,Arial,sans-serif;display:flex;flex-direction:column;height:100vh}
header{background:linear-gradient(90deg,#0B72E7,#4AA3FF);color:#fff;padding:14px 16px;font-weight:600;font-size:18px;display:flex;align-items:center;justify-content:center;box-shadow:0 2px 6px rgba(0,0,0,.15)}
.container{flex:1;display:flex;justify-content:center;padding:12px}
.chat{background:var(--card);width:100%;max-width:820px;border-radius:16px;box-shadow:0 10px 30px rgba(0,0,0,.08);display:flex;flex-direction:column;overflow:hidden}
.stream{flex:1;padding:16px;overflow-y:auto;display:flex;flex-direction:column;gap:10px;background:linear-gradient(#F9FAFB,#EFF2F6)}
.msg{max-width:75%;padding:10px 14px;border-radius:16px;line-height:1.45;font-size:15px;word-wrap:break-word;box-shadow:0 1px 2px rgba(0,0,0,.08);position:relative;animation:fade .18s ease-out}
.msg.me{align-self:flex-end;background:var(--bubble-me);color:#fff;border-bottom-right-radius:6px}
.msg.other{align-self:flex-start;background:var(--bubble-other);color:var(--text);border:1px solid var(--ring);border-bottom-left-radius:6px}
.time{display:block;font-size:11px;opacity:.75;margin-top:4px;text-align:right}
footer{display:flex;gap:8px;padding:12px;background:#fff;border-top:1px solid var(--ring)}
input[type=text]{flex:1;padding:12px 14px;border:1px solid var(--ring);border-radius:999px;font-size:15px;outline:none}
button{padding:10px 16px;border:none;border-radius:999px;background:var(--primary);color:#fff;font-weight:600;cursor:pointer;transition:background .15s ease}
button:hover{background:var(--primary-d)}
@keyframes fade{from{opacity:0;transform:translateY(4px)}to{opacity:1;transform:translateY(0)}}
</style></head>
<body>
<header>📡 LoRa Chat (ESP8266)</header>
<div class="container">
  <div class="chat">
    <div id="stream" class="stream"></div>
    <footer>
      <input id="msg" type="text" placeholder="Type a message…" autocomplete="off">
      <button id="send">Send</button>
    </footer>
  </div>
</div>
<script>
const ws = new WebSocket(`ws://${location.hostname}:81/`);
const stream = document.getElementById('stream');
const input  = document.getElementById('msg');
const sendBtn= document.getElementById('send');

ws.onopen = ()=> input.focus();
ws.onmessage = (e)=> addMsg(e.data,'other');

sendBtn.onclick = send;
input.addEventListener('keypress',e=>{ if(e.key==='Enter') send(); });

function send(){
  const t=input.value.trim();
  if(!t) return;
  ws.send(t);
  addMsg(t,'me');
  input.value='';
  input.focus();
}

function addMsg(text, who){
  const d=document.createElement('div');
  d.className='msg '+who;
  const time=new Date().toLocaleTimeString([],{hour:'2-digit',minute:'2-digit'});
  d.innerHTML = escapeHtml(text)+`<span class="time">${time}</span>`;
  stream.appendChild(d);
  stream.scrollTop=stream.scrollHeight;
}

// Prevent HTML injection in chat
function escapeHtml(s){return s.replace(/[&<>"']/g,m=>({ '&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#039;'}[m]));}
</script>
</body></html>
)HTML";

// ======= LoRa helpers (original style) =======
void sendCommand(const String& cmd){
  lora.println(cmd);
  Serial.print(">> "); Serial.println(cmd);
  delay(600);
  while(lora.available()){
    String r = lora.readStringUntil('\n'); r.trim();
    if(r.length()) Serial.println(r);
  }
  delay(300);
}

void waitForLoRa(){
  Serial.println("Waiting for LoRa module to respond...");
  while(true){
    lora.println("AT");
    delay(500);
    if(lora.available()){
      String r=lora.readStringUntil('\n'); r.trim();
      Serial.print("Response: "); Serial.println(r);
      if(r.startsWith("+OK")){ Serial.println("✅ LoRa ready\n"); break; }
    }
    Serial.println("Retrying...");
  }
}

// Parse +RCV=<addr>,<len>,<msg>,<rssi>,<snr> → <msg>
bool parseRcvLine(const String& s, String& outMsg){
  if(!s.startsWith("+RCV")) return false;
  int p1=s.indexOf(','); if(p1<0) return false;
  int p2=s.indexOf(',',p1+1); if(p2<0) return false;
  int p3=s.indexOf(',',p2+1); if(p3<0) return false;
  outMsg = s.substring(p2+1, p3);
  return true;
}

// ======= WebSocket handler =======
void wsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len){
  if(type==WStype_TEXT){
    String msg = String((char*)payload).substring(0,len);
    // Send to peer (ESP32 address 2)
    String cmd = "AT+SEND=2," + String(msg.length()) + "," + msg;
    sendCommand(cmd);
  }
}

void setup(){
  Serial.begin(115200);
  lora.begin(115200);

  delay(2500);
  Serial.println("\n--- ESP8266 RYLR896 WebChat ---\n");

  // LoRa init
  waitForLoRa();
  sendCommand("AT+ADDRESS=1");
  sendCommand("AT+NETWORKID=10");
  sendCommand("AT+BAND=868000000");

  // Wi-Fi AP
  WiFi.softAP(ssid, password);
  Serial.print("AP: "); Serial.println(ssid);

  // Web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req){ req->send_P(200,"text/html",index_html); });
  server.begin();

  webSocket.begin();
  webSocket.onEvent(wsEvent);

  Serial.println("✅ ESP8266 WebChat ready at http://192.168.4.1");
}

void loop(){
  webSocket.loop();

  // Read LoRa lines and forward only the clean message
  while(lora.available()){
    String ln = lora.readStringUntil('\n'); ln.trim();
    if(ln.length()){
      String msg;
      if(parseRcvLine(ln,msg)){
        Serial.println("📥 " + msg);
        webSocket.broadcastTXT(msg);
      }
    }
  }
}
