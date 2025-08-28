#include "Portal.h"
#include "../../config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <FS.h>
#include <Preferences.h>

namespace {
  WebServer* serverPtr = nullptr;
  bool active = false;
  bool spiffsReady = false;
  Preferences prefs;

  void ensureSPIFFS(){ if (!spiffsReady) spiffsReady = SPIFFS.begin(true); }

  void writeDefaultIndexHtmlIfMissing(){
    ensureSPIFFS(); if (!spiffsReady) return;
    if (!SPIFFS.exists("/index.html")){
      File f = SPIFFS.open("/index.html", FILE_WRITE);
      if (!f) return;
      const char* html = R"HTML(
<!doctype html><html><head><meta name=viewport content="width=device-width,initial-scale=1"><title>DogePet Setup</title><style>body{font-family:sans-serif;margin:16px}label{display:block;margin-top:10px}input,select{width:100%;padding:8px;margin-top:4px}button{margin-top:12px;padding:10px 14px}pre{white-space:pre-wrap}</style></head><body><h2>DogePet Setup</h2><div id=stat></div><form id=wifiForm><label>SSID</label><select name=ssid id=ssid></select><label>Password</label><input type=password name=pass id=pass><button type=submit>Connect WiFi</button></form><hr/><h3>Config</h3><form id=cfgForm></form><button id=saveBtn>Save Config</button><script>
async function scan(){let r=await fetch('/scan');let j=await r.json();let s=document.getElementById('ssid');s.innerHTML='';j.forEach(x=>{let o=document.createElement('option');o.textContent=x;s.appendChild(o)});}
async function loadCfg(){let r=await fetch('/config');let j=await r.json();let f=document.getElementById('cfgForm');f.innerHTML='';Object.keys(j).forEach(k=>{let v=j[k];let L=document.createElement('label');L.textContent=k;f.appendChild(L);let i;if(typeof v==='boolean'){i=document.createElement('select');['false','true'].forEach(b=>{let o=document.createElement('option');o.value=b;o.textContent=b;o.selected=(String(v)===b);i.appendChild(o);});} else {i=document.createElement('input');i.type='text';i.value=v;}i.name=k;f.appendChild(i);});}
document.getElementById('wifiForm').addEventListener('submit',async(e)=>{e.preventDefault();let ssid=document.getElementById('ssid').value;let pass=document.getElementById('pass').value;let r=await fetch('/wifi/connect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid,pass})});document.getElementById('stat').textContent=await r.text();});
document.getElementById('saveBtn').addEventListener('click',async()=>{let f=document.getElementById('cfgForm');let o={};[...f.elements].forEach(el=>{if(!el.name)return;let v=el.value;if(v==='true'||v==='false'){o[el.name]=(v==='true');} else if(/^-?\d+$/.test(v)){o[el.name]=parseInt(v);} else {o[el.name]=v;}});let r=await fetch('/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(o)});document.getElementById('stat').textContent=await r.text();});
scan();loadCfg();
</script></body></html>
)HTML";
      f.print(html);
      f.close();
    }
  }

  void handleRoot(){
    if (!serverPtr) return;
    ensureSPIFFS(); writeDefaultIndexHtmlIfMissing();
    if (!spiffsReady) { serverPtr->send(500, "text/plain", "SPIFFS mount failed"); return; }
    File f = SPIFFS.open("/index.html", FILE_READ);
    if (!f) { serverPtr->send(404, "text/plain", "index.html missing"); return; }
    serverPtr->streamFile(f, "text/html");
    f.close();
  }

  void handleConfigGet(){
    if (!serverPtr) return;
    ensureSPIFFS();
    if (spiffsReady && SPIFFS.exists("/config.json")){
      File f = SPIFFS.open("/config.json", FILE_READ);
      serverPtr->streamFile(f, "application/json");
      f.close();
      return;
    }
    // Defaults from compile-time config
    String json = "{";
    json += "\"ENABLE_GEMINI_AI\":"; json += (ENABLE_GEMINI_AI?"true":"false"); json += ",";
    json += "\"ENABLE_AI_CHATTER\":"; json += (ENABLE_AI_CHATTER?"true":"false"); json += ",";
    json += "\"AI_CHATTER_INTERVAL_MS\":"; json += AI_CHATTER_INTERVAL_MS; json += ",";
    json += "\"GEMINI_COOLDOWN_MIN_MS\":"; json += GEMINI_COOLDOWN_MIN_MS; json += ",";
    json += "\"GEMINI_COOLDOWN_MAX_MS\":"; json += GEMINI_COOLDOWN_MAX_MS; json += ",";
    json += "\"ENABLE_IDLE_AUDIO_CHATTER\":"; json += (ENABLE_IDLE_AUDIO_CHATTER?"true":"false"); json += ",";
    json += "\"ENABLE_BINARY_CHATTER\":"; json += (ENABLE_BINARY_CHATTER?"true":"false"); json += ",";
    json += "\"ENABLE_LAZY_MODE\":"; json += (ENABLE_LAZY_MODE?"true":"false"); json += ",";
    json += "\"LAZY_AFTER_MS\":"; json += LAZY_AFTER_MS; json += ",";
    json += "\"LAZY_JINGLE_MIN_MS\":"; json += LAZY_JINGLE_MIN_MS; json += ",";
    json += "\"LAZY_JINGLE_MAX_MS\":"; json += LAZY_JINGLE_MAX_MS;
    json += "}";
    serverPtr->send(200, "application/json", json);
  }

  void handleConfigPost(){
    if (!serverPtr) return;
    ensureSPIFFS(); if (!spiffsReady){ serverPtr->send(500, "text/plain", "SPIFFS mount failed"); return; }
    String body = serverPtr->arg("plain");
    File f = SPIFFS.open("/config.json", FILE_WRITE);
    if (!f){ serverPtr->send(500, "text/plain", "Write failed"); return; }
    f.print(body);
    f.close();
    serverPtr->send(200, "text/plain", "Config saved");
  }

  void handleScan(){
    if (!serverPtr) return;
    WiFi.scanDelete();
    WiFi.scanNetworks();
    int n = WiFi.scanComplete();
    String out = "[";
    for (int i=0;i<n;i++){ if (i) out += ","; out += '"'; out += WiFi.SSID(i); out += '"'; }
    out += "]";
    serverPtr->send(200, "application/json", out);
  }
}

namespace Sectors { namespace Portal {
  void begin() {
    ensureSPIFFS();
  }

  void update() {
    if (active && serverPtr) serverPtr->handleClient();
  }

  void start() {
    if (active) return;
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP("DogePet-Setup");
    if (!serverPtr) serverPtr = new WebServer(80);
    serverPtr->on("/", handleRoot);
    serverPtr->on("/index.html", handleRoot);
    serverPtr->on("/scan", handleScan);
    serverPtr->on("/config", HTTP_GET, handleConfigGet);
    serverPtr->on("/config", HTTP_POST, handleConfigPost);
    serverPtr->on("/wifi/connect", HTTP_POST, [](){
      String body = serverPtr->arg("plain");
      String ssid, pass;
      int s = body.indexOf("\"ssid\"");
      if (s>=0){ s = body.indexOf(':', s); int q=body.indexOf('"', s+1); int q2=body.indexOf('"', q+1); ssid = body.substring(q+1,q2); }
      int p = body.indexOf("\"pass\"");
      if (p>=0){ p = body.indexOf(':', p); int q=body.indexOf('"', p+1); int q2=body.indexOf('"', q+1); pass = body.substring(q+1,q2); }
      prefs.begin("cfg", false);
      prefs.putString("wifi_ssid", ssid);
      prefs.putString("wifi_pass", pass);
      prefs.end();
      serverPtr->send(200, "text/plain", "Saved WiFi, triple-press FUNC to connect.");
    });
    serverPtr->begin();
    active = true;
  }

  bool stopAndConnect(uint32_t timeoutMs) {
    if (!active) return WiFi.status() == WL_CONNECTED;
    if (serverPtr) { serverPtr->stop(); }
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    prefs.begin("cfg", true);
    String ssid = prefs.getString("wifi_ssid", "");
    String pass = prefs.getString("wifi_pass", "");
    prefs.end();
    if (ssid.length() == 0) {
      active = false;
      return false;
    }
    WiFi.begin(ssid.c_str(), pass.c_str());
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) { delay(250); }
    active = false;
    return WiFi.status() == WL_CONNECTED;
  }

  bool isActive() { return active; }
}}
