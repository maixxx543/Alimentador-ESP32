#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <Preferences.h>

const int pinServo = 13;    

const int pinLedDispensando = 18; 
const int pinLedAutomatico = 21;  
const int pinLedWifi = 22;        
const int pinLedsExtras = 19;     // Pin para controlar los 4 LEDs

unsigned long tiempoInicioLedWifi = 0;
bool ledWifiEncendido = false;
bool estadoLedsExtras = false;    // Estado actual de los 4 LEDs

Servo miServo;
WebServer server(80);
Preferences preferences; 

unsigned long tiempoEspera = 0;        
unsigned long tiempoDispensado = 2000;
unsigned long tiempoUltimaComida = 0;
bool modoAutomatico = false;
unsigned long totalDispensaciones = 0; 

String ssidAP = "Comedero_ESP32";
String passwordAP = "12345678";
unsigned long valorTiempoConfig = 0;
String unidadTiempoConfig = "s";

String obtenerHTML() {
    String html = "<!DOCTYPE html>\n<html lang=\"es\">\n<head>\n<meta charset=\"UTF-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html += "<title>Smart Comedero - Control Visual</title>\n<style>\n";
    html += ":root { --bg-gradient: linear-gradient(135deg, #f5f7fa, #c3cfe2); --card-bg: #ffffff; --text-main: #2d3748; --text-muted: #4a5568; --section-bg: #f8f9fa; --border-color: #e2e8f0; --input-bg: #ffffff; --input-border: #cbd5e0; --input-text: #2d3748; --feeder-body: #2d3748; --feeder-lid: #4a5568; --panel-bg: #1a202c; --status-bg: #edf2f7; --status-text: #2d3748; }\n";
    html += ".dark-mode { --bg-gradient: linear-gradient(135deg, #1a202c, #111827); --card-bg: #1f2937; --text-main: #f9fafb; --text-muted: #9ca3af; --section-bg: #111827; --border-color: #374151; --input-bg: #374151; --input-border: #4b5563; --input-text: #f9fafb; --feeder-body: #111827; --feeder-lid: #374151; --panel-bg: #0b0f19; --status-bg: #374151; --status-text: #f9fafb; }\n";
    html += "* { box-sizing: border-box; margin: 0; padding: 0; }\n";
    html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: var(--bg-gradient); color: var(--text-main); display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 15px; transition: background 0.3s ease, color 0.3s ease; }\n";
    html += ".card { width: 100%; max-width: 440px; background: var(--card-bg); padding: 25px; border-radius: 24px; box-shadow: 0 14px 35px rgba(0, 0, 0, 0.15); transition: background 0.3s ease, box-shadow 0.3s ease; position: relative; }\n";
    html += ".theme-toggle { position: absolute; top: 22px; right: 22px; background: var(--section-bg); border: 1px solid var(--border-color); color: var(--text-main); width: 36px; height: 36px; border-radius: 50%; display: flex; justify-content: center; align-items: center; cursor: pointer; font-size: 16px; transition: transform 0.2s, background 0.3s; }\n";
    html += ".theme-toggle:hover { transform: scale(1.1); }\n";
    html += "h2 { font-size: 22px; color: var(--text-main); margin-bottom: 18px; text-align: center; font-weight: 700; }\n";
    html += ".feeder-graphic-container { display: flex; flex-direction: column; align-items: center; background: var(--section-bg); border: 1px solid var(--border-color); border-radius: 16px; padding: 20px; margin-bottom: 14px; position: relative; overflow: hidden; transition: background 0.3s ease, border-color 0.3s ease; }\n";
    html += ".feeder-structure { position: relative; width: 130px; height: 140px; background: var(--feeder-body); border-radius: 12px 12px 20px 20px; display: flex; flex-direction: column; align-items: center; box-shadow: 0 8px 20px rgba(0,0,0,0.2); transition: background 0.3s ease; }\n";
    html += ".feeder-lid { position: absolute; top: -12px; width: 90px; height: 16px; background: var(--feeder-lid); border-radius: 6px 6px 0 0; transition: background 0.3s ease; }\n";
    html += ".feeder-panel { position: absolute; top: 22px; display: flex; gap: 10px; background: var(--panel-bg); padding: 6px 10px; border-radius: 20px; transition: background 0.3s ease; }\n";
    html += ".led-item { display: flex; flex-direction: column; align-items: center; gap: 3px; font-size: 8px; color: #a0aec0; font-weight: 600; }\n";
    html += ".led-bulb { width: 8px; height: 8px; border-radius: 50%; background: #4a5568; box-shadow: inset 0 1px 2px rgba(0,0,0,0.4); transition: all 0.3s ease; }\n";
    html += ".led-bulb.blue-on { background: #3182ce; box-shadow: 0 0 8px #3182ce, inset 0 1px 2px rgba(0,0,0,0.2); }\n";
    html += ".led-bulb.green-on { background: #38a169; box-shadow: 0 0 8px #38a169, inset 0 1px 2px rgba(0,0,0,0.2); }\n";
    html += ".led-bulb.red-on { background: #e53e3e; box-shadow: 0 0 8px #e53e3e, inset 0 1px 2px rgba(0,0,0,0.2); }\n";
    html += ".led-bulb.yellow-on { background: #ecc94b; box-shadow: 0 0 8px #ecc94b, inset 0 1px 2px rgba(0,0,0,0.2); }\n";
    html += ".feeder-door-container { position: absolute; top: 72px; width: 50px; height: 50px; background: var(--panel-bg); border-radius: 50%; display: flex; justify-content: center; align-items: center; border: 2px solid var(--border-color); transition: background 0.3s ease, border-color 0.3s ease; }\n";
    html += ".feeder-door { width: 40px; height: 40px; background: #e2e8f0; border-radius: 50%; position: relative; transition: transform 0.4s ease; }\n";
    html += ".feeder-door::after { content: ''; position: absolute; top: 2px; left: 17px; width: 6px; height: 36px; background: #2d3748; border-radius: 3px; }\n";
    html += ".rotating { transform: rotate(180deg); }\n";
    html += ".feeder-bowl { width: 160px; height: 20px; background: #dd6b20; border-radius: 0 0 12px 12px; margin-top: 4px; box-shadow: 0 4px 10px rgba(0,0,0,0.15); }\n";
    html += ".food-pellet { position: absolute; width: 8px; height: 8px; background: #d35400; border-radius: 50%; opacity: 0; z-index: 10; }\n";
    html += "@keyframes fallFood { 0% { transform: translateY(115px) scale(0.6); opacity: 1; } 100% { transform: translateY(175px) scale(1); opacity: 0; } }\n";
    html += ".animating-pellet { animation: fallFood 0.5s linear infinite; }\n";
    html += ".btn-mini-dispense { margin-top: 14px; background: #38a169; color: white; border: none; padding: 7px 16px; border-radius: 20px; font-size: 12px; font-weight: bold; cursor: pointer; box-shadow: 0 3px 8px rgba(56, 161, 105, 0.3); transition: background 0.2s, transform 0.1s; }\n";
    html += ".btn-mini-dispense:hover { background: #2f855a; }\n";
    html += ".btn-mini-dispense:active { transform: scale(0.96); }\n";
    html += ".config-section { background: var(--section-bg); border: 1px solid var(--border-color); border-radius: 16px; padding: 16px; margin-bottom: 14px; transition: background 0.3s ease, border-color 0.3s ease; }\n";
    html += ".section-title { font-size: 11px; text-transform: uppercase; letter-spacing: 0.8px; color: var(--text-muted); margin-bottom: 12px; font-weight: 700; display: flex; align-items: center; gap: 6px; }\n";
    html += "label { font-size: 12px; font-weight: 600; display: block; margin-bottom: 5px; color: var(--text-main); }\n";
    html += "input, select { width: 100%; padding: 10px 14px; margin-bottom: 12px; border: 1px solid var(--input-border); border-radius: 10px; font-size: 13px; background-color: var(--input-bg); color: var(--input-text); transition: all 0.2s ease; }\n";
    html += "input:focus, select:focus { border-color: #3182ce; outline: none; box-shadow: 0 0 0 3px rgba(49, 130, 206, 0.2); }\n";
    html += ".row-tiempo { display: flex; gap: 8px; }\n";
    html += ".row-tiempo input { flex: 2; }\n";
    html += ".row-tiempo select { flex: 1; }\n";
    html += ".btn-config { background: linear-gradient(135deg, #3182ce, #2b6cb0); color: white; width: 100%; padding: 12px; border: none; border-radius: 10px; font-weight: bold; font-size: 13px; cursor: pointer; box-shadow: 0 4px 12px rgba(49, 130, 206, 0.3); display: flex; justify-content: center; align-items: center; gap: 8px; transition: transform 0.1s, opacity 0.2s; }\n";
    html += ".btn-config:hover { opacity: 0.92; }\n";
    html += ".btn-config:active { transform: scale(0.98); }\n";
    html += ".btn-toggle-on { background: linear-gradient(135deg, #38a169, #2f855a); box-shadow: 0 4px 12px rgba(56, 161, 105, 0.3); }\n";
    html += ".btn-toggle-off { background: linear-gradient(135deg, #e53e3e, #c53030); box-shadow: 0 4px 12px rgba(229, 62, 62, 0.3); }\n";
    html += "#estado { font-weight: 600; margin-top: 14px; padding: 10px; background: var(--status-bg); border-radius: 10px; color: var(--status-text); font-size: 12px; text-align: center; transition: background 0.3s, color 0.3s; }\n";
    html += "</style>\n</head>\n<body>\n<div class=\"card\" id=\"mainCard\">\n";
    html += "<button class=\"theme-toggle\" id=\"themeBtn\" onclick=\"toggleTheme()\" title=\"Cambiar modo claro/oscuro\">🌙</button>\n";
    html += "<h2>Smart Comedero</h2>\n";
    
    html += "<div class=\"config-section\">\n<div class=\"section-title\">📡 Configuración Wi-Fi (AP)</div>\n";
    html += "<label for=\"ssid\">Nombre de Red (SSID):</label>\n<input type=\"text\" id=\"ssid\" value=\"" + ssidAP + "\">\n";
    html += "<label for=\"password\">Contraseña (Mín. 8 caract.):</label>\n<input type=\"text\" id=\"password\" value=\"" + passwordAP + "\">\n";
    html += "<button class=\"btn-config\" onclick=\"guardarWifi()\">💾 Guardar Wi-Fi</button>\n</div>\n";

    html += "<div class=\"feeder-graphic-container\">\n<div class=\"feeder-structure\">\n<div class=\"feeder-lid\"></div>\n";
    html += "<div class=\"feeder-panel\">\n<div class=\"led-item\"><div id=\"ledAuto\" class=\"led-bulb\"></div>Auto</div>\n";
    html += "<div class=\"led-item\"><div id=\"ledDisp\" class=\"led-bulb\"></div>Servo</div>\n";
    html += "<div class=\"led-item\"><div id=\"ledWifi\" class=\"led-bulb\"></div>Wi-Fi</div>\n";
    html += "<div class=\"led-item\"><div id=\"ledExtras\" class=\"led-bulb" + String(estadoLedsExtras ? " yellow-on" : "") + "\"></div>LEDs</div>\n</div>\n";
    html += "<div class=\"feeder-door-container\"><div id=\"door\" class=\"feeder-door\"></div></div>\n</div>\n";
    html += "<div id=\"pellet1\" class=\"food-pellet\" style=\"left: calc(50% - 15px);\"></div>\n";
    html += "<div id=\"pellet2\" class=\"food-pellet\" style=\"left: calc(50% + 8px); animation-delay: 0.25s;\"></div>\n";
    html += "<div class=\"feeder-bowl\"></div>\n<button class=\"btn-mini-dispense\" onclick=\"dispensarManual()\">🍖 Dispensar Ahora</button>\n</div>\n";

    // NUEVA SECCIÓN: Botón de Encendido/Apagado de los 4 LEDs
    html += "<div class=\"config-section\">\n<div class=\"section-title\">💡 Control de 4 LEDs Externos</div>\n";
    html += "<button id=\"btnLeds\" class=\"btn-config " + String(estadoLedsExtras ? "btn-toggle-on" : "btn-toggle-off") + "\" onclick=\"alternarLeds()\">" + String(estadoLedsExtras ? "🟢 LEDs Encendidos (Apagar)" : "🔴 LEDs Apagados (Encender)") + "</button>\n</div>\n";

    html += "<div class=\"config-section\">\n<div class=\"section-title\">⚙️ Configuración del Sistema</div>\n";
    html += "<label for=\"duracionGiro\">Duración de Movimiento (Segundos):</label>\n<input type=\"number\" id=\"duracionGiro\" min=\"1\" max=\"60\" value=\"" + String(tiempoDispensado / 1000) + "\">\n";
    html += "<label for=\"valorTiempo\">Intervalo de Espera Automático:</label>\n<div class=\"row-tiempo\">\n";
    html += "<input type=\"number\" id=\"valorTiempo\" min=\"0\" value=\"" + String(valorTiempoConfig) + "\">\n";
    html += "<select id=\"unidadTiempo\">\n";
    html += "<option value=\"s\"" + String(unidadTiempoConfig == "s" ? " selected" : "") + ">Seg</option>\n";
    html += "<option value=\"m\"" + String(unidadTiempoConfig == "m" ? " selected" : "") + ">Min</option>\n";
    html += "<option value=\"h\"" + String(unidadTiempoConfig == "h" ? " selected" : "") + ">Horas</option>\n";
    html += "</select>\n</div>\n";
    html += "<label for=\"auto\">Modo Automático:</label>\n<select id=\"auto\" onchange=\"actualizarModoAuto()\">\n";
    html += "<option value=\"0\"" + String(!modoAutomatico ? " selected" : "") + ">Desactivado (Solo Manual)</option>\n";
    html += "<option value=\"1\"" + String(modoAutomatico ? " selected" : "") + ">Activado</option>\n";
    html += "</select>\n<button class=\"btn-config\" style=\"margin-top: 15px;\" onclick=\"guardarConfig()\">💾 Guardar Sistema</button>\n</div>\n";
    
    html += "<div id=\"estado\">Estado: Listo para usar</div>\n</div>\n<script>\n";
    html += "function toggleTheme() { const body = document.body; const themeBtn = document.getElementById('themeBtn'); body.classList.toggle('dark-mode'); themeBtn.innerText = body.classList.contains('dark-mode') ? '☀️' : '🌙'; }\n";
    html += "function actualizarModoAuto() { const autoVal = document.getElementById('auto').value; const ledAuto = document.getElementById('ledAuto'); if (autoVal == '1') { ledAuto.classList.add('blue-on'); } else { ledAuto.classList.remove('blue-on'); } }\n";
    
    // Función de JavaScript para alternar el estado con el botón y actualizar el indicador visual
    html += "let estadoActualLeds = " + String(estadoLedsExtras ? "1" : "0") + ";\n";
    html += "async function alternarLeds() {\n";
    html += "  estadoActualLeds = estadoActualLeds == 1 ? 0 : 1;\n";
    html += "  const btn = document.getElementById('btnLeds');\n";
    html += "  const ledExtras = document.getElementById('ledExtras');\n";
    html += "  const estadoEl = document.getElementById('estado');\n";
    html += "  if (estadoActualLeds == 1) {\n";
    html += "    btn.className = 'btn-config btn-toggle-on';\n";
    html += "    btn.innerText = '🟢 LEDs Encendidos (Apagar)';\n";
    html += "    ledExtras.classList.add('yellow-on');\n";
    html += "  } else {\n";
    html += "    btn.className = 'btn-config btn-toggle-off';\n";
    html += "    btn.innerText = '🔴 LEDs Apagados (Encender)';\n";
    html += "    ledExtras.classList.remove('yellow-on');\n";
    html += "  }\n";
    html += "  try {\n";
    html += "    await fetch('/leds?estado=' + estadoActualLeds, { method: 'POST' });\n";
    html += "    estadoEl.innerText = estadoActualLeds == 1 ? '🟢 LEDs encendidos' : '🔴 LEDs apagados';\n";
    html += "  } catch (error) {\n";
    html += "    estadoEl.innerText = '🔴 Error de conexión';\n";
    html += "  }\n";
    html += "}\n";

    html += "function activarAnimacionDispensar() {\n";
    html += "  const ledDisp = document.getElementById('ledDisp'); const door = document.getElementById('door'); const p1 = document.getElementById('pellet1'); const p2 = document.getElementById('pellet2');\n";
    html += "  ledDisp.classList.add('green-on'); door.classList.add('rotating'); p1.classList.add('animating-pellet'); p2.classList.add('animating-pellet');\n";
    html += "  setTimeout(() => {\n";
    html += "    ledDisp.classList.remove('green-on'); door.classList.remove('rotating'); p1.classList.remove('animating-pellet'); p2.classList.remove('animating-pellet');\n";
    html += "  }, " + String(tiempoDispensado) + ");\n";
    html += "}\n";

    html += "async function dispensarManual() { const estadoEl = document.getElementById('estado'); estadoEl.innerText = '⏳ Moviendo servo y dispensando comida...'; activarAnimacionDispensar(); try { await fetch('/dispensar', { method: 'POST' }); estadoEl.innerText = '🟢 ¡Comida dispensada con éxito!'; } catch (error) { estadoEl.innerText = '🔴 Error de conexión'; } }\n";
    
    html += "let ultimoContador = -1;\n";
    html += "setInterval(async () => {\n";
    html += "  try {\n";
    html += "    let res = await fetch('/estado');\n";
    html += "    let data = await res.json();\n";
    html += "    if (ultimoContador === -1) { ultimoContador = data.contador; }\n";
    html += "    else if (data.contador > ultimoContador) {\n";
    html += "      ultimoContador = data.contador;\n";
    html += "      activarAnimacionDispensar();\n";
    html += "      document.getElementById('estado').innerText = '🟢 ¡Comida dispensada automáticamente!';\n";
    html += "    }\n";
    html += "  } catch (e) {}\n";
    html += "}, 200);\n";
    
    html += "async function guardarWifi() { const newSsid = document.getElementById('ssid').value; const newPass = document.getElementById('password').value; if (newPass.length > 0 && newPass.length < 8) { alert('La contraseña debe tener al menos 8 caracteres.'); return; } let confirmar = confirm('⚠️ ADVERTENCIA: Al cambiar la red Wi-Fi, el LED rojo se encenderá y el dispositivo aplicará los cambios. ¿Desea continuar?'); const ledWifi = document.getElementById('ledWifi'); const estadoEl = document.getElementById('estado'); if (confirmar) { ledWifi.classList.add('red-on'); estadoEl.innerText = '⏳ Aplicando nueva configuración Wi-Fi...'; const data = `ssid=${encodeURIComponent(newSsid)}&password=${encodeURIComponent(newPass)}`; try { await fetch('/config', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: data }); } catch (error) {} setTimeout(() => { ledWifi.classList.remove('red-on'); estadoEl.innerText = '🟢 ¡Wi-Fi actualizado!'; }, 5000); } }\n";
    html += "async function guardarConfig() { const estadoEl = document.getElementById('estado'); const duracion = Number(document.getElementById('duracionGiro').value || 2); const valor = Number(document.getElementById('valorTiempo').value || 0); const unidad = document.getElementById('unidadTiempo').value; const auto = document.getElementById('auto').value; const data = `duracion=${duracion}&valor=${valor}&unidad=${unidad}&auto=${auto}`; estadoEl.innerText = '⏳ Guardando configuración...'; try { await fetch('/config', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: data }); estadoEl.innerText = '🟢 ¡Configuración guardada!'; } catch (error) { estadoEl.innerText = '🔴 Error al guardar'; } }\n";
    html += "window.onload = function() { actualizarModoAuto(); }\n</script>\n</body>\n</html>";
    return html;
}

void dispensarComida() {
  Serial.println(">> ¡Agitando servo para dispensar...");
  digitalWrite(pinLedDispensando, HIGH);
  
  unsigned long inicioMovimiento = millis();
  while (millis() - inicioMovimiento < tiempoDispensado) {
    for (int pos = 0; pos <= 110; pos += 5) {
      miServo.write(pos);
      delay(15); 
      if (millis() - inicioMovimiento >= tiempoDispensado) break;
    }
    for (int pos = 90; pos >= 0; pos -= 5) {
      miServo.write(pos);
      delay(15);
      if (millis() - inicioMovimiento >= tiempoDispensado) break;
    }
  }
  
  miServo.write(0); 
  digitalWrite(pinLedDispensando, LOW);
  
  totalDispensaciones++; 
  tiempoUltimaComida = millis(); 
  Serial.println(">> Movimiento finalizado.");
}

void handleRoot() {
  server.send(200, "text/html", obtenerHTML());
}

void handleEstadoJSON() {
  String json = "{";
  json += "\"contador\":" + String(totalDispensaciones);
  json += "}";
  server.send(200, "application/json", json);
}

void handleDispensar() {
  dispensarComida();
  server.send(200, "text/plain", "OK");
}

void handleLeds() {
  if (server.hasArg("estado")) {
    estadoLedsExtras = (server.arg("estado") == "1");
    digitalWrite(pinLedsExtras, estadoLedsExtras ? HIGH : LOW);
  }
  server.send(200, "text/plain", "OK");
}

void handleConfig() {
  if (server.hasArg("duracion")) {
    unsigned long segundosGiro = server.arg("duracion").toInt();
    if (segundosGiro < 1) segundosGiro = 1;
    tiempoDispensado = segundosGiro * 1000UL; 
    
    preferences.begin("red-config", false);
    preferences.putULong("tGiro", tiempoDispensado);
    preferences.end();
  }

  if (server.hasArg("valor") && server.hasArg("unidad")) {
    valorTiempoConfig = server.arg("valor").toInt();
    unidadTiempoConfig = server.arg("unidad");
    if (valorTiempoConfig < 0) valorTiempoConfig = 0;

    if (unidadTiempoConfig == "s") {
      tiempoEspera = valorTiempoConfig * 1000UL;                 
    } else if (unidadTiempoConfig == "m") {
      tiempoEspera = valorTiempoConfig * 60UL * 1000UL;          
    } else if (unidadTiempoConfig == "h") {
      tiempoEspera = valorTiempoConfig * 60UL * 60UL * 1000UL;   
    }

    preferences.begin("red-config", false);
    preferences.putULong("vTiempo", valorTiempoConfig);
    preferences.putString("uTiempo", unidadTiempoConfig);
    preferences.end();

    tiempoUltimaComida = millis();
  }

  if (server.hasArg("auto")) {
    modoAutomatico = (server.arg("auto") == "1");
    preferences.begin("red-config", false);
    preferences.putBool("modoAuto", modoAutomatico);
    preferences.end();
    
    tiempoUltimaComida = millis();
  }

  bool reiniciarAP = false;
  
  if (server.hasArg("ssid")) {
    String nuevoSsid = server.arg("ssid");
    if (nuevoSsid.length() > 0 && nuevoSsid != ssidAP) {
      ssidAP = nuevoSsid;
      preferences.begin("red-config", false);
      preferences.putString("ssid", ssidAP); 
      preferences.end();
      reiniciarAP = true;
    }
  }

  if (server.hasArg("password")) {
    String nuevoPass = server.arg("password");
    if (nuevoPass.length() >= 8 && nuevoPass != passwordAP) {
      passwordAP = nuevoPass;
      preferences.begin("red-config", false);
      preferences.putString("password", passwordAP); 
      preferences.end();
      reiniciarAP = true;
    }
  }

  if (reiniciarAP) {
    digitalWrite(pinLedWifi, HIGH);
    tiempoInicioLedWifi = millis();
    ledWifiEncendido = true;
  }

  server.send(200, "text/plain", "OK");

  if (reiniciarAP) {
    Serial.println(">> Credenciales actualizadas. Reiniciando AP...");
    delay(500);
    WiFi.softAP(ssidAP.c_str(), passwordAP.c_str());
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  preferences.begin("red-config", true);
  ssidAP = preferences.getString("ssid", "Comedero_ESP32");
  passwordAP = preferences.getString("password", "12345678");
  tiempoDispensado = preferences.getULong("tGiro", 2000); 
  modoAutomatico = preferences.getBool("modoAuto", false);
  valorTiempoConfig = preferences.getULong("vTiempo", 0);
  unidadTiempoConfig = preferences.getString("uTiempo", "s");
  preferences.end();

  if (unidadTiempoConfig == "s") tiempoEspera = valorTiempoConfig * 1000UL;
  else if (unidadTiempoConfig == "m") tiempoEspera = valorTiempoConfig * 60UL * 1000UL;
  else if (unidadTiempoConfig == "h") tiempoEspera = valorTiempoConfig * 60UL * 60UL * 1000UL;

  pinMode(pinLedDispensando, OUTPUT);
  pinMode(pinLedAutomatico, OUTPUT);
  pinMode(pinLedWifi, OUTPUT);
  pinMode(pinLedsExtras, OUTPUT); 

  digitalWrite(pinLedDispensando, LOW);
  digitalWrite(pinLedAutomatico, LOW);
  digitalWrite(pinLedWifi, LOW);
  digitalWrite(pinLedsExtras, LOW); 
  
  ESP32PWM::allocateTimer(0);
  miServo.setPeriodHertz(50);
  miServo.attach(pinServo, 500, 2400);
  miServo.write(0); 

  WiFi.softAP(ssidAP.c_str(), passwordAP.c_str());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/estado", HTTP_GET, handleEstadoJSON);
  server.on("/dispensar", HTTP_POST, handleDispensar);
  server.on("/leds", HTTP_POST, handleLeds);
  server.on("/config", HTTP_POST, handleConfig);
  server.begin();
  tiempoUltimaComida = millis(); 
}

void loop() {
  server.handleClient();

  if (modoAutomatico) {
    digitalWrite(pinLedAutomatico, HIGH);
  } else {
    digitalWrite(pinLedAutomatico, LOW);
  }

  if (ledWifiEncendido && (millis() - tiempoInicioLedWifi >= 5000)) {
    digitalWrite(pinLedWifi, LOW);
    ledWifiEncendido = false;
  }

  if (modoAutomatico && tiempoEspera > 0) {
    if (millis() - tiempoUltimaComida >= tiempoEspera) {
      dispensarComida();
    }
  }
}