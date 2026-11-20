#include "WebServerManager.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "BoardProfile.h"

namespace {
const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Fan Controller</title><style>
:root{color-scheme:dark;font-family:system-ui;background:#101827;color:#edf2f7}body{max-width:900px;margin:auto;padding:18px}
section{background:#1c293b;border:1px solid #34445b;border-radius:12px;padding:16px;margin:14px 0}
h1,h2{margin-top:0}label{display:block;margin:10px 0 4px;color:#b8c6d9}input,select,button{box-sizing:border-box;width:100%;padding:10px;border-radius:8px;border:1px solid #52647c;background:#101827;color:#fff}
button{background:#2563eb;border:0;margin-top:10px;font-weight:600}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px}
small{color:#9fb0c7}.switch{width:auto}.fan{border-top:1px solid #40516a;margin-top:14px;padding-top:10px}
</style></head><body>
<h1>ESP32 Fan Controller</h1><p><small>WebUI is always enabled. BLE Presence and MQTT Control are independent toggles.</small></p>
<section><h2>Control modes</h2><div class="grid">
<label><input class="switch" id="blePresence" type="checkbox"> BLE Presence</label>
<label><input class="switch" id="mqttEnabled" type="checkbox"> MQTT Control</label></div></section>
<section><h2>Wi-Fi AP + STA</h2><div class="grid">
<div><label>Wi-Fi SSID</label><input id="wifiSsid"></div><div><label>Wi-Fi password (leave blank to keep)</label><input id="wifiPassword" type="password"></div>
<div><label>Setup AP SSID</label><input id="apSsid"></div><div><label>Setup AP password (min 8 chars)</label><input id="apPassword" type="password"></div></div></section>
<section><h2>WebUI authentication</h2><div class="grid"><div><label>Username</label><input id="webUsername"></div><div><label>New password (leave blank to keep)</label><input id="webPassword" type="password"></div></div></section>
<section><h2>MQTT</h2><div class="grid">
<div><label>Transport</label><select id="mqttTransport"><option value="0">WS</option><option value="1">WSS</option><option value="2">TCP / MQTT</option><option value="3">TLS / MQTTS</option></select></div>
<div><label>Broker host</label><input id="mqttHost"></div><div><label>Port</label><input id="mqttPort" type="number"></div>
<div><label>WebSocket path</label><input id="mqttPath"></div><div><label>Username</label><input id="mqttUsername"></div>
<div><label>Password (leave blank to keep)</label><input id="mqttPassword" type="password"></div></div>
<label>Root CA PEM for WSS/TLS (leave blank to keep)</label><textarea id="mqttCaPem" rows="5" style="width:100%;background:#101827;color:white"></textarea>
<div class="grid"><div><label>Base topic</label><input id="mqttBaseTopic"></div><div><label>Device ID</label><input id="deviceId"></div></div></section>
<section><h2>Fan GPIO configuration</h2><p><small>Target: ESP32-C3 Super Mini. Relay outputs and button inputs use separate pin lists. GPIO2 is a boot strapping pin; preserve its HIGH level during reset. Direct GPIO supports one complete fan only; more fans need an I/O expander or a larger board. Pins must be unique. Save then restart to apply hardware changes.</small></p>
<div class="grid"><div><label>Number of fans (max 8 logical profiles; hardware GPIO budget applies)</label><select id="fanCount"><option>1</option><option>2</option><option>3</option><option>4</option><option>5</option><option>6</option><option>7</option><option>8</option></select></div></div>
<div id="fans"></div></section>
<section><h2>BLE iTag</h2><button onclick="scanBle()">Scan BLE devices</button><div id="bleDevices"></div>
<div class="grid"><div><label>Registered iTag address</label><input id="registeredItag"></div><div><label>RSSI threshold (calibrate around 2 m)</label><input id="presenceRssiThreshold" type="number"></div>
<div><label>Absence timeout (seconds)</label><input id="presenceTimeoutSeconds" type="number"></div></div><label>Fans controlled by this iTag</label><div id="bleFanMapping" class="grid"></div></section>
<section><h2>Manual fan control</h2><div class="grid"><div><label>Fan index</label><select id="manualFan"><option value="0">Fan 1</option><option value="1">Fan 2</option><option value="2">Fan 3</option><option value="3">Fan 4</option><option value="4">Fan 5</option><option value="5">Fan 6</option><option value="6">Fan 7</option><option value="7">Fan 8</option></select></div>
<div><label>Speed</label><select id="manualSpeed"><option value="0">OFF</option><option value="1">Speed 1</option><option value="2">Speed 2</option><option value="3">Speed 3</option><option value="4">Speed 4</option></select></div></div><button onclick="sendFan()">Apply speed</button><button onclick="refreshStatus()">Refresh fan / MQTT status</button><p id="fanStatus"></p></section>
<section><button onclick="save()">Save configuration</button><button onclick="restart()">Restart ESP32 to apply hardware/Wi-Fi changes</button><p id="result"></p></section>
<script>
let cfg={},outputPins=[],inputPins=[];
async function api(path,method='GET',body){let r=await fetch(path,{method,headers:{'Content-Type':'application/json'},body:body?JSON.stringify(body):undefined});if(!r.ok)throw new Error(await r.text());return r.status===204?{}:r.json();}
function gpioSelect(id,value,availablePins){let s='<select id="'+id+'"><option value="-1">Not assigned</option>';availablePins.forEach(p=>s+='<option value="'+p+'" '+(p===value?'selected':'')+'>GPIO '+p+'</option>');return s+'</select>';}
function renderFans(){let n=+document.getElementById('fanCount').value;let html='';for(let i=0;i<n;i++){let f=cfg.fans?.[i]||{};html+='<div class="fan"><h3>Fan '+(i+1)+'</h3><label>Fan ID</label><input id="fanId'+i+'" value="'+(f.id||('fan-00'+(i+1)))+'"><label>Available speeds</label><select id="speedCount'+i+'">'+[2,3,4].map(x=>'<option '+((f.availableSpeeds||4)===x?'selected':'')+'>'+x+'</option>').join('')+'</select><div class="grid">';for(let j=0;j<4;j++)html+='<div><label>Speed '+(j+1)+' GPIO</label>'+gpioSelect('speedPin'+i+'_'+j,f.speedPins?.[j]??-1,outputPins)+'</div>';for(let j=0;j<5;j++)html+='<div><label>Button '+(j===0?'OFF':'Speed '+j)+' GPIO</label>'+gpioSelect('buttonPin'+i+'_'+j,f.buttonPins?.[j]??-1,inputPins)+'</div>';html+='</div><div class="grid"><div><label>Relay active level</label><select id="relayLow'+i+'"><option value="true" '+(f.relayActiveLow!==false?'selected':'')+'>Active LOW</option><option value="false" '+(f.relayActiveLow===false?'selected':'')+'>Active HIGH</option></select></div><div><label>Button active level</label><select id="buttonLow'+i+'"><option value="true" '+(f.buttonActiveLow!==false?'selected':'')+'>Active LOW</option><option value="false" '+(f.buttonActiveLow===false?'selected':'')+'>Active HIGH</option></select></div><div><label>Break-before-make (ms)</label><input id="dead'+i+'" type="number" value="'+(f.deadTimeMs||1000)+'"></div></div></div>';}document.getElementById('fans').innerHTML=html;}
function updateManualFanOptions(){let n=+document.getElementById('fanCount').value;document.getElementById('manualFan').querySelectorAll('option').forEach((o,i)=>o.hidden=i>=n);}
async function load(){[outputPins,inputPins,cfg]=await Promise.all([api('/api/gpio?type=output'),api('/api/gpio?type=input'),api('/api/config')]);['blePresence','mqttEnabled','wifiSsid','apSsid','mqttTransport','mqttHost','mqttPort','mqttPath','mqttUsername','mqttBaseTopic','webUsername','deviceId','registeredItag','presenceRssiThreshold','presenceTimeoutSeconds','fanCount'].forEach(k=>{let e=document.getElementById(k);if(e)e.type==='checkbox'?e.checked=!!cfg[k]:e.value=cfg[k]??'';});renderFans();renderBleFanMapping();updateManualFanOptions();}
function renderBleFanMapping(){let n=+document.getElementById('fanCount').value;let html='';for(let i=0;i<n;i++){let checked=(cfg.bleFanMask??1)&(1<<i);html+='<label><input class="switch bleFan" type="checkbox" value="'+i+'" '+(checked?'checked':'')+'> '+(cfg.fans?.[i]?.id||('Fan '+(i+1)))+'</label>';}document.getElementById('bleFanMapping').innerHTML=html;}
document.getElementById('fanCount').addEventListener('change',()=>{renderFans();renderBleFanMapping();updateManualFanOptions();});
document.getElementById('mqttPort').addEventListener('input',()=>{document.getElementById('mqttPort').dataset.touched='1';});
document.getElementById('mqttTransport').addEventListener('change',e=>{let p=document.getElementById('mqttPort');if(!p.dataset.touched){p.value=({0:80,1:443,2:1883,3:8883})[e.target.value];}});
function collect(){let n=+document.getElementById('fanCount').value;let fans=[];for(let i=0;i<n;i++){let speedPins=[],buttonPins=[];for(let j=0;j<4;j++)speedPins.push(+document.getElementById('speedPin'+i+'_'+j).value);for(let j=0;j<5;j++)buttonPins.push(+document.getElementById('buttonPin'+i+'_'+j).value);fans.push({id:document.getElementById('fanId'+i).value,availableSpeeds:+document.getElementById('speedCount'+i).value,speedPins,buttonPins,relayActiveLow:document.getElementById('relayLow'+i).value==='true',buttonActiveLow:document.getElementById('buttonLow'+i).value==='true',deadTimeMs:+document.getElementById('dead'+i).value});}
return {blePresenceEnabled:document.getElementById('blePresence').checked,bleFanMask:Array.from(document.querySelectorAll('.bleFan:checked')).reduce((mask,e)=>mask|(1<<Number(e.value)),0),mqttEnabled:document.getElementById('mqttEnabled').checked,wifiSsid:document.getElementById('wifiSsid').value,wifiPassword:document.getElementById('wifiPassword').value,apSsid:document.getElementById('apSsid').value,apPassword:document.getElementById('apPassword').value,mqttTransport:+document.getElementById('mqttTransport').value,mqttHost:document.getElementById('mqttHost').value,mqttPort:+document.getElementById('mqttPort').value,mqttPath:document.getElementById('mqttPath').value,webUsername:document.getElementById('webUsername').value,webPassword:document.getElementById('webPassword').value,mqttUsername:document.getElementById('mqttUsername').value,mqttPassword:document.getElementById('mqttPassword').value,mqttCaPem:document.getElementById('mqttCaPem').value,mqttBaseTopic:document.getElementById('mqttBaseTopic').value,deviceId:document.getElementById('deviceId').value,registeredItag:document.getElementById('registeredItag').value,presenceRssiThreshold:+document.getElementById('presenceRssiThreshold').value,presenceTimeoutSeconds:+document.getElementById('presenceTimeoutSeconds').value,fanCount:n,fans};}
async function save(){try{let result=await api('/api/config','POST',collect());document.getElementById('result').textContent=result.message||'Saved. Restart if requested.';cfg=await api('/api/config');}catch(e){document.getElementById('result').textContent='Error: '+e.message;}}
async function scanBle(){try{await api('/api/ble/scan','POST',{});setTimeout(async()=>{let d=await api('/api/ble/scan');document.getElementById('bleDevices').innerHTML=d.map(x=>'<button onclick="document.getElementById(\'registeredItag\').value=\''+x.address+'\'">'+(x.name||'Unknown')+' · '+x.address+' · '+x.rssi+' dBm · data '+x.manufacturer+'</button>').join('');},2500);}catch(e){alert(e.message);}}
async function refreshStatus(){try{let s=await api('/api/status');let lines=['MQTT: '+(s.mqttConnected?'Connected':'Disconnected')];s.fans.forEach(f=>lines.push(f.fanId+': '+(f.configured?f.state+' · speed '+f.speed+' · target '+f.targetSpeed:'GPIO profile incomplete')));document.getElementById('fanStatus').textContent=lines.join(' | ');}catch(e){document.getElementById('fanStatus').textContent='Status error: '+e.message;}}
async function sendFan(){try{await api('/api/fan/command','POST',{fanIndex:+document.getElementById('manualFan').value,speed:+document.getElementById('manualSpeed').value});document.getElementById('fanStatus').textContent='Command queued';setTimeout(refreshStatus,300);}catch(e){alert(e.message);}}
async function restart(){try{await api('/api/restart','POST',{});}catch(e){document.getElementById('result').textContent='Restart requested.';}}
load().catch(e=>document.getElementById('result').textContent=e.message);
</script></body></html>
)HTML";
}

bool WebServerManager::authenticate() {
    if (!config_) return false;
    const AppConfig& cfg = config_->get();
    if (server_.authenticate(cfg.webUsername, cfg.webPassword)) return true;
    server_.requestAuthentication();
    return false;
}

void WebServerManager::begin(ConfigManager& config, MqttManager& mqtt, BleRemoteManager& ble,
                             CommandManager& commands, FanController* fans) {
    config_ = &config;
    mqtt_ = &mqtt;
    ble_ = &ble;
    commands_ = &commands;
    fans_ = fans;

    server_.on("/", HTTP_GET, [this]() {
        if (!authenticate()) return;
        server_.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
    });
    server_.on("/api/config", HTTP_GET, [this]() { handleConfigGet(); });
    server_.on("/api/config", HTTP_POST, [this]() { handleConfigPost(); });
    server_.on("/api/gpio", HTTP_GET, [this]() { handleGpioOptions(); });
    server_.on("/api/ble/scan", HTTP_GET, [this]() {
        if (!authenticate()) return;
        server_.send(200, "application/json", ble_->scanResultsJson());
    });
    server_.on("/api/ble/scan", HTTP_POST, [this]() {
        if (!authenticate()) return;
        server_.send(202, "application/json", "{\"scanning\":true}");
    });
    server_.on("/api/ble/register", HTTP_POST, [this]() { handleBleRegister(); });
    server_.on("/api/fan/command", HTTP_POST, [this]() { handleFanCommand(); });
    server_.on("/api/status", HTTP_GET, [this]() { handleStatusGet(); });
    server_.on("/api/restart", HTTP_POST, [this]() {
        if (!authenticate()) return;
        server_.send(202, "application/json", "{\"restarting\":true}");
        delay(100);
        ESP.restart();
    });
    server_.begin();
}

void WebServerManager::service() {
    server_.handleClient();
}

void WebServerManager::handleConfigGet() {
    if (!authenticate()) return;
    server_.send(200, "application/json", config_->toJson());
}

void WebServerManager::handleConfigPost() {
    if (!authenticate()) return;
    JsonDocument doc;
    if (deserializeJson(doc, server_.arg("plain"))) {
        server_.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    AppConfig next = config_->get();
    if (doc["blePresenceEnabled"].is<bool>()) next.blePresenceEnabled = doc["blePresenceEnabled"];
    if (doc["bleFanMask"].is<uint8_t>()) next.bleFanMask = doc["bleFanMask"];
    if (doc["webUsername"].is<const char*>()) strlcpy(next.webUsername, doc["webUsername"], sizeof(next.webUsername));
    if (doc["webPassword"].is<const char*>() && String(doc["webPassword"].as<const char*>()).length()) strlcpy(next.webPassword, doc["webPassword"], sizeof(next.webPassword));
    if (doc["mqttEnabled"].is<bool>()) next.mqttEnabled = doc["mqttEnabled"];
    if (doc["wifiSsid"].is<const char*>()) strlcpy(next.wifiSsid, doc["wifiSsid"], sizeof(next.wifiSsid));
    if (doc["wifiPassword"].is<const char*>() && String(doc["wifiPassword"].as<const char*>()).length()) strlcpy(next.wifiPassword, doc["wifiPassword"], sizeof(next.wifiPassword));
    if (doc["apSsid"].is<const char*>()) strlcpy(next.apSsid, doc["apSsid"], sizeof(next.apSsid));
    if (doc["apPassword"].is<const char*>() && String(doc["apPassword"].as<const char*>()).length()) strlcpy(next.apPassword, doc["apPassword"], sizeof(next.apPassword));
    if (doc["mqttTransport"].is<uint8_t>()) next.mqttTransport = static_cast<MqttTransport>(doc["mqttTransport"].as<uint8_t>());
    if (doc["mqttHost"].is<const char*>()) strlcpy(next.mqttHost, doc["mqttHost"], sizeof(next.mqttHost));
    if (doc["mqttPort"].is<uint16_t>()) next.mqttPort = doc["mqttPort"];
    if (doc["mqttPath"].is<const char*>()) strlcpy(next.mqttPath, doc["mqttPath"], sizeof(next.mqttPath));
    if (doc["mqttUsername"].is<const char*>()) strlcpy(next.mqttUsername, doc["mqttUsername"], sizeof(next.mqttUsername));
    if (doc["mqttPassword"].is<const char*>() && String(doc["mqttPassword"].as<const char*>()).length()) strlcpy(next.mqttPassword, doc["mqttPassword"], sizeof(next.mqttPassword));
    if (doc["mqttCaPem"].is<const char*>() && String(doc["mqttCaPem"].as<const char*>()).length()) strlcpy(next.mqttCaPem, doc["mqttCaPem"], sizeof(next.mqttCaPem));
    if (doc["mqttBaseTopic"].is<const char*>()) strlcpy(next.mqttBaseTopic, doc["mqttBaseTopic"], sizeof(next.mqttBaseTopic));
    if (doc["deviceId"].is<const char*>()) strlcpy(next.deviceId, doc["deviceId"], sizeof(next.deviceId));
    if (doc["registeredItag"].is<const char*>()) strlcpy(next.registeredItag, doc["registeredItag"], sizeof(next.registeredItag));
    if (doc["presenceRssiThreshold"].is<int8_t>()) next.presenceRssiThreshold = doc["presenceRssiThreshold"];
    if (doc["presenceTimeoutSeconds"].is<uint16_t>()) next.presenceTimeoutSeconds = doc["presenceTimeoutSeconds"];
    if (doc["fanCount"].is<uint8_t>()) next.fanCount = doc["fanCount"];
    if (next.fanCount < 1 || next.fanCount > MAX_FANS) {
        server_.send(400, "application/json", "{\"error\":\"Fan count must be 1-8\"}");
        return;
    }

    JsonArrayConst fans = doc["fans"].as<JsonArrayConst>();
    for (uint8_t i = 0; i < next.fanCount && i < fans.size(); ++i) {
        JsonObjectConst fan = fans[i];
        if (fan["id"].is<const char*>()) strlcpy(next.fans[i].id, fan["id"], sizeof(next.fans[i].id));
        if (fan["availableSpeeds"].is<uint8_t>()) next.fans[i].availableSpeeds = fan["availableSpeeds"];
        if (fan["relayActiveLow"].is<bool>()) next.fans[i].relayActiveLow = fan["relayActiveLow"];
        if (fan["buttonActiveLow"].is<bool>()) next.fans[i].buttonActiveLow = fan["buttonActiveLow"];
        if (fan["deadTimeMs"].is<uint16_t>()) next.fans[i].deadTimeMs = fan["deadTimeMs"];
        JsonArrayConst speedPins = fan["speedPins"].as<JsonArrayConst>();
        JsonArrayConst buttonPins = fan["buttonPins"].as<JsonArrayConst>();
        for (uint8_t j = 0; j < MAX_SPEEDS && j < speedPins.size(); ++j) next.fans[i].speedPins[j] = speedPins[j].as<int8_t>();
        for (uint8_t j = 0; j < BUTTON_COUNT && j < buttonPins.size(); ++j) next.fans[i].buttonPins[j] = buttonPins[j].as<int8_t>();
    }

    String error;
    if (!config_->validate(next, error)) {
        JsonDocument response;
        response["error"] = error;
        String body;
        serializeJson(response, body);
        server_.send(400, "application/json", body);
        return;
    }
    if (!config_->save(next)) {
        server_.send(500, "application/json", "{\"error\":\"Unable to save configuration\"}");
        return;
    }
    mqtt_->applyConfig(config_->get());
    ble_->applyConfig(config_->get());
    server_.send(200, "application/json", "{\"saved\":true,\"message\":\"Configuration saved. Restart ESP32 to apply Wi-Fi and GPIO changes.\"}");
}

void WebServerManager::handleGpioOptions() {
    if (!authenticate()) return;
    JsonDocument doc;
    JsonArray pins = doc.to<JsonArray>();
    const bool input = server_.arg("type") == "input";
    const auto availablePins = input ? BoardProfile::inputCapablePins()
                                     : BoardProfile::outputCapablePins();
    for (int pin : availablePins) pins.add(pin);
    String body;
    serializeJson(pins, body);
    server_.send(200, "application/json", body);
}

void WebServerManager::handleBleRegister() {
    if (!authenticate()) return;
    JsonDocument doc;
    if (deserializeJson(doc, server_.arg("plain")) || !doc["address"].is<const char*>()) {
        server_.send(400, "application/json", "{\"error\":\"Missing BLE address\"}");
        return;
    }
    AppConfig next = config_->get();
    strlcpy(next.registeredItag, doc["address"], sizeof(next.registeredItag));
    if (!config_->save(next)) {
        server_.send(400, "application/json", "{\"error\":\"Configuration rejected\"}");
        return;
    }
    ble_->applyConfig(config_->get());
    server_.send(200, "application/json", "{\"registered\":true}");
}

void WebServerManager::handleStatusGet() {
    if (!authenticate()) return;
    JsonDocument doc;
    doc["mqttConnected"] = mqtt_ && mqtt_->connected();
    JsonArray fans = doc["fans"].to<JsonArray>();
    const uint8_t fanCount = config_->get().fanCount;
    for (uint8_t i = 0; i < fanCount; ++i) {
        JsonObject item = fans.add<JsonObject>();
        item["fanId"] = config_->get().fans[i].id;
        const bool configured = fans_ && fans_[i].isConfigured();
        item["configured"] = configured;
        if (!configured) {
            item["state"] = "unconfigured";
            item["speed"] = 0;
            item["targetSpeed"] = 0;
            continue;
        }
        switch (fans_[i].state()) {
            case FanRunState::Off: item["state"] = "off"; break;
            case FanRunState::Transition: item["state"] = "transition"; break;
            case FanRunState::Running: item["state"] = "running"; break;
            case FanRunState::Error: item["state"] = "error"; break;
            case FanRunState::Unconfigured: item["state"] = "unconfigured"; break;
        }
        item["speed"] = fans_[i].currentSpeed();
        item["targetSpeed"] = fans_[i].targetSpeed();
    }
    String body;
    serializeJson(doc, body);
    server_.send(200, "application/json", body);
}

void WebServerManager::handleFanCommand() {
    if (!authenticate()) return;
    JsonDocument doc;
    if (deserializeJson(doc, server_.arg("plain")) ||
        !doc["fanIndex"].is<uint8_t>() || !doc["speed"].is<uint8_t>()) {
        server_.send(400, "application/json", "{\"error\":\"Invalid fan command\"}");
        return;
    }
    uint8_t fanIndex = doc["fanIndex"];
    uint8_t speed = doc["speed"];
    if (fanIndex >= config_->get().fanCount || speed > config_->get().fans[fanIndex].availableSpeeds) {
        server_.send(400, "application/json", "{\"error\":\"Fan or speed is unavailable\"}");
        return;
    }
    if (!fans_ || !fans_[fanIndex].isConfigured()) {
        server_.send(409, "application/json", "{\"error\":\"Fan GPIO profile is incomplete. Configure all speed GPIOs and restart.\"}");
        return;
    }
    if (!commands_->enqueue({fanIndex, speed, CommandSource::WebUi})) {
        server_.send(503, "application/json", "{\"error\":\"Command queue full\"}");
        return;
    }
    server_.send(202, "application/json", "{\"queued\":true}");
}
