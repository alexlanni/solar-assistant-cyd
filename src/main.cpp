#include <Arduino.h>
/*
  CYD Solar Monitor – Solar Assistant via MQTT
  -------------------------------------------------
  Scheda : ESP32-2432S028R "Cheap Yellow Display" (ILI9341 320x240 + touch XPT2046)
  Progetto PlatformIO: librerie e configurazione display in platformio.ini
  Pagine : 1 = LIVE (potenze, SOC)   2 = OGGI E METEO (energie giornaliere, previsioni)
  Tocca lo schermo in un punto qualsiasi per cambiare pagina.
*/
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <time.h>
#include <math.h>

// ======================= CONFIGURAZIONE =======================
#include "secrets.h"   // WIFI_SSID, WIFI_PASS, MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS

// Convenzioni di segno: verificale guardando un valore noto e inverti se serve
const bool BATT_POSITIVE_IS_CHARGE = true;   // battery_power > 0 = in carica
const bool GRID_POSITIVE_IS_IMPORT = true;   // grid_power   > 0 = prelievo dalla rete

const uint32_t AUTO_PAGE_MS = 30000;  // scorrimento automatico: ogni pagina resta 30 s (0 = solo touch)
const uint32_t IDLE_MS      = 30000;  // lo scorrimento parte dopo 30 s senza tocchi
const uint32_t STALE_MS     = 60000;  // dati "vecchi" dopo 60 s senza messaggi (pallino arancione)
const char*    TZ_INFO      = "CET-1CEST,M3.5.0,M10.5.0/3";  // Europe/Rome
const uint16_t TOUCH_MIN_Z  = 600;    // pressione minima per un tocco valido (alza se cambia ancora da solo)
const bool     TOUCH_DEBUG  = false;  // true = stampa le letture del touch sul monitor seriale
// Topic della tensione batteria (relativo a "solar_assistant/", senza "/state").
// Verifica il nome esatto in MQTT Explorer: es. "battery_1/voltage" o "inverter_1/battery_voltage"
#define BATT_V_KEY "battery_1/voltage"
const float    GAUGE_MAX_W  = 7500;   // fondo scala del gauge fotovoltaico (pagina 3)

// LED RGB sul retro: "respiro" verde in carica, rosso in scarica, spento a riposo
const bool     LED_ENABLED     = true;
const uint8_t  LED_MAX         = 255;    // luminosità massima (0-255)
const uint16_t LED_PERIOD_SLOW = 4000;   // ms per ciclo a bassa potenza
const uint16_t LED_PERIOD_FAST = 1200;   // ms per ciclo alla potenza LED_FULL_POWER_W
const float    LED_FULL_POWER_W = 3000;  // potenza batteria a cui il respiro è più veloce
const float    LED_IDLE_W       = 20;    // sotto questa potenza la batteria è "a riposo" (LED spento)
// ===============================================================

// Pin touch del CYD (bus SPI separato dal display)
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

TFT_eSPI tft;
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
WiFiClient net;
PubSubClient mqtt(net);

// Colori (RGB565)
#define C_BG     0x0000
#define C_HEAD   0x10A2
#define C_CARD   0x18E3
#define C_TEXT   0xFFFF
#define C_DIM    0x9CF3
#define C_SOLAR  0xFEA0
#define C_LOAD   0x5DDF
#define C_GRID   0xC618
#define C_GREEN  0x07E8
#define C_RED    0xF8A2
#define C_ORANGE 0xFC60
#define C_LINE   0x39C7

// ----------------------- Dati MQTT -----------------------
enum Val { PV_P, LOAD_P, GRID_P, BATT_P, SOC, BATT_T, GRID_V, GRID_F, LOAD_PCT,
           E_PV, E_LOAD, E_GIN, E_GOUT, E_BIN, E_BOUT,
           W_CLOUD, W_TEMP, W_REMAIN, W_PRED, W_CODE, BATT_V, N_VALS };

const char* KEYS[N_VALS] = {
  "total/pv_power", "total/load_power", "total/grid_power", "total/battery_power",
  "total/battery_state_of_charge", "total/battery_temperature", "total/grid_voltage",
  "total/grid_frequency", "total/load_percentage",
  "total/pv_energy", "total/load_energy", "total/grid_energy_in", "total/grid_energy_out",
  "total/battery_energy_in", "total/battery_energy_out",
  "weather/cloud_cover", "weather/outside_temperature", "weather/pv_energy_remaining_today",
  "weather/pv_power_predicted", "weather/weather_code",
  BATT_V_KEY   // tensione batteria (nei topic "total" non c'è)
};

float    val[N_VALS];
bool     has[N_VALS];
volatile bool dirty = true;
bool     saOnline = false;
uint32_t lastMsgMs = 0;
const uint8_t N_PAGES = 3;
const char* PAGE_TITLES[N_PAGES] = {"LIVE", "OGGI E METEO", "BATTERIA E FV"};
uint8_t  page = 0;
uint32_t lastPageSwitch = 0;
uint32_t lastTouchMs = 0;
uint32_t lastWifiTry = 0;

void onMqtt(char* topic, byte* payload, unsigned int len) {
  char buf[32];
  unsigned int n = len < sizeof(buf) - 1 ? len : sizeof(buf) - 1;
  memcpy(buf, payload, n);
  buf[n] = 0;

  const char* prefix = "solar_assistant/";
  size_t pl = strlen(prefix);
  if (strncmp(topic, prefix, pl) != 0) return;
  const char* t = topic + pl;

  if (strcmp(t, "status") == 0) {
    saOnline = (strcmp(buf, "online") == 0);
    dirty = true;
    return;
  }

  char key[64];
  strncpy(key, t, sizeof(key) - 1);
  key[sizeof(key) - 1] = 0;
  size_t kl = strlen(key);
  if (kl < 7 || strcmp(key + kl - 6, "/state") != 0) return;
  key[kl - 6] = 0;

  for (int i = 0; i < N_VALS; i++) {
    if (strcmp(key, KEYS[i]) == 0) {
      val[i] = atof(buf);
      has[i] = true;
      lastMsgMs = millis();
      dirty = true;
      return;
    }
  }
}

// ----------------------- Helper grafici -----------------------
#define NF 32
char     cacheStr[NF][28];
uint16_t cacheCol[NF];

void clearCache() {
  for (int i = 0; i < NF; i++) { cacheStr[i][0] = '\x01'; cacheStr[i][1] = 0; cacheCol[i] = 0; }
}

// true se il contenuto del campo è cambiato (e aggiorna la cache)
bool changed(int id, const char* s, uint16_t col) {
  if (strcmp(cacheStr[id], s) == 0 && cacheCol[id] == col) return false;
  strncpy(cacheStr[id], s, sizeof(cacheStr[id]) - 1);
  cacheStr[id][sizeof(cacheStr[id]) - 1] = 0;
  cacheCol[id] = col;
  return true;
}

void text(const char* s, int x, int y, uint8_t font, uint16_t fg, uint16_t bg, uint8_t datum, int pad) {
  tft.setTextDatum(datum);
  tft.setTextColor(fg, bg);
  tft.setTextPadding(pad);
  tft.drawString(s, x, y, font);
  tft.setTextPadding(0);
}

void field(int id, const char* s, int x, int y, uint8_t font, uint16_t fg, uint16_t bg, uint8_t datum, int pad) {
  if (changed(id, s, fg)) text(s, x, y, font, fg, bg, datum, pad);
}

// Temperatura con simbolo di grado disegnato a mano (i font integrati non hanno °)
void tempField(int id, const char* prefix, bool ok, float t, int x, int y, uint8_t font,
               uint16_t fg, uint16_t bg, uint8_t datum, int pad) {
  char num[24], s[28];
  if (!ok) { snprintf(s, sizeof s, "%s--", prefix); field(id, s, x, y, font, fg, bg, datum, pad); return; }
  snprintf(num, sizeof num, "%s%.1f ", prefix, t);
  snprintf(s, sizeof s, "%s C", num);
  if (!changed(id, s, fg)) return;
  text(s, x, y, font, fg, bg, datum, pad);
  int x0 = (datum == TR_DATUM) ? x - tft.textWidth(s, font) : x;
  int r = font >= 4 ? 3 : 2;
  tft.drawCircle(x0 + tft.textWidth(num, font) + r, y + r + 1, r, fg);
}

void fmtPower(char* b, size_t n, float w) {
  float a = fabsf(w);
  if (a < 1000)       snprintf(b, n, "%d W", (int)lroundf(a));
  else if (a < 10000) snprintf(b, n, "%.2f kW", a / 1000.0f);
  else                snprintf(b, n, "%.1f kW", a / 1000.0f);
}

const char* wmoText(int c, uint16_t& col) {
  col = C_TEXT;
  if (c == 0)  { col = C_SOLAR; return "Sereno"; }
  if (c == 1)  { col = C_SOLAR; return "Prev. sereno"; }
  if (c == 2)  return "Parz. nuvoloso";
  if (c == 3)  { col = C_DIM; return "Coperto"; }
  if (c == 45 || c == 48) { col = C_DIM; return "Nebbia"; }
  if (c >= 51 && c <= 55) { col = C_LOAD; return "Pioviggine"; }
  if (c == 56 || c == 57) { col = C_LOAD; return "Pioviggine gelata"; }
  if (c >= 61 && c <= 65) { col = C_LOAD; return "Pioggia"; }
  if (c == 66 || c == 67) { col = C_LOAD; return "Pioggia gelata"; }
  if (c >= 71 && c <= 75) return "Neve";
  if (c == 77) return "Nevischio";
  if (c >= 80 && c <= 82) { col = C_LOAD; return "Rovesci"; }
  if (c == 85 || c == 86) return "Rovesci di neve";
  if (c == 95) { col = C_ORANGE; return "Temporale"; }
  if (c == 96 || c == 99) { col = C_RED; return "Temporale/grandine"; }
  return nullptr;
}

// ----------------------- Header -----------------------
uint16_t statusColor() {
  if (WiFi.status() != WL_CONNECTED || !mqtt.connected()) return C_RED;
  if (!saOnline || millis() - lastMsgMs > STALE_MS) return C_ORANGE;
  return C_GREEN;
}

void updateHeader(bool force) {
  static int lastMin = -3;
  static uint16_t lastCol = 1;
  struct tm ti;
  char b[8] = "--:--";
  int m = -2;
  if (getLocalTime(&ti, 0)) { strftime(b, sizeof b, "%H:%M", &ti); m = ti.tm_hour * 60 + ti.tm_min; }
  if (force || m != lastMin) { text(b, 312, 4, 2, C_TEXT, C_HEAD, TR_DATUM, 44); lastMin = m; }
  uint16_t c = statusColor();
  if (force || c != lastCol) { tft.fillCircle(256, 12, 5, c); lastCol = c; }
}

void drawHeader() {
  tft.fillRect(0, 0, 320, 24, C_HEAD);
  text(PAGE_TITLES[page], 8, 4, 2, C_TEXT, C_HEAD, TL_DATUM, 0);
  const int dx0 = 160 - (N_PAGES - 1) * 8;
  for (int i = 0; i < N_PAGES; i++) {
    if (i == page) tft.fillCircle(dx0 + i * 16, 12, 4, C_TEXT);
    else           tft.drawCircle(dx0 + i * 16, 12, 4, C_DIM);
  }
  updateHeader(true);
}

// ----------------------- Blocco SOC (pagine 1 e 3) -----------------------
// y0 = bordo superiore della card (alta 68 px, larga tutto lo schermo)
void drawSocStatic(int y0) {
  tft.fillRoundRect(4, y0, 312, 68, 6, C_CARD);
  text("CARICA BATTERIA", 124, y0 + 6, 2, C_DIM, C_CARD, TL_DATUM, 0);
}

void drawSocValues(int y0) {
  if (has[SOC]) {
    int soc = constrain((int)lroundf(val[SOC]), 0, 100);
    uint16_t col = soc < 20 ? C_RED : (soc < 50 ? C_ORANGE : C_GREEN);
    char n[8];
    snprintf(n, sizeof n, "%d", soc);
    if (changed(8, n, col)) {
      tft.fillRect(8, y0 + 2, 112, 48, C_CARD);
      text(n, 12, y0 + 2, 6, col, C_CARD, TL_DATUM, 0);
      text("%", 12 + tft.textWidth(n, 6) + 4, y0 + 22, 4, col, C_CARD, TL_DATUM, 0);
      const int bx = 124, by = y0 + 26, bw = 184, bh = 20;
      tft.drawRoundRect(bx, by, bw, bh, 4, C_DIM);
      int fw = (bw - 4) * soc / 100;
      tft.fillRect(bx + 2, by + 2, fw, bh - 4, col);
      tft.fillRect(bx + 2 + fw, by + 2, bw - 4 - fw, bh - 4, C_BG);
    }
  } else if (changed(8, "--", C_DIM)) {
    tft.fillRect(8, y0 + 2, 112, 48, C_CARD);
    text("--", 12, y0 + 14, 4, C_DIM, C_CARD, TL_DATUM, 0);
  }
  // Tensione batteria: piccola, sotto la percentuale (stessa riga della temperatura)
  char vb[16];
  if (has[BATT_V]) snprintf(vb, sizeof vb, "%.1f V", val[BATT_V]); else strcpy(vb, "-- V");
  field(10, vb, 12, y0 + 50, 2, C_DIM, C_CARD, TL_DATUM, 108);
  tempField(9, "Temp. ", has[BATT_T], val[BATT_T], 124, y0 + 50, 2, C_DIM, C_CARD, TL_DATUM, 184);
}

// ----------------------- Pagina 1: LIVE -----------------------
struct Card { int x, y; const char* label; uint16_t accent; };
const Card CARDS[4] = {
  {4, 28, "SOLARE", C_SOLAR}, {164, 28, "CASA", C_LOAD},
  {4, 98, "RETE", C_GRID},    {164, 98, "BATTERIA", C_GREEN}
};
const int CW = 152, CH = 66;

void drawPage1Static() {
  for (const Card& c : CARDS) {
    tft.fillRoundRect(c.x, c.y, CW, CH, 6, C_CARD);
    tft.fillRect(c.x, c.y + 8, 3, CH - 16, c.accent);
    text(c.label, c.x + 10, c.y + 4, 2, C_DIM, C_CARD, TL_DATUM, 0);
  }
  drawSocStatic(168);
}

void cardValue(int idx, int idV, int idS, const char* v, uint16_t vc, const char* sub, uint16_t sc) {
  const Card& c = CARDS[idx];
  field(idV, v,   c.x + 10, c.y + 20, 4, vc, C_CARD, TL_DATUM, CW - 16);
  field(idS, sub, c.x + 10, c.y + 48, 2, sc, C_CARD, TL_DATUM, CW - 16);
}

void drawPage1Values() {
  char v[24], s[32], p[16];

  // SOLARE
  if (has[PV_P]) fmtPower(v, sizeof v, val[PV_P]); else strcpy(v, "--");
  if (has[W_PRED]) { fmtPower(p, sizeof p, val[W_PRED]); snprintf(s, sizeof s, "previsto %s", p); }
  else strcpy(s, "previsto --");
  cardValue(0, 0, 1, v, C_SOLAR, s, C_DIM);

  // CASA
  if (has[LOAD_P]) fmtPower(v, sizeof v, val[LOAD_P]); else strcpy(v, "--");
  if (has[LOAD_PCT]) snprintf(s, sizeof s, "carico %d%%", (int)lroundf(val[LOAD_PCT]));
  else strcpy(s, "carico --");
  cardValue(1, 2, 3, v, C_LOAD, s, C_DIM);

  // RETE
  uint16_t gc = C_TEXT, sc = C_DIM;
  const char* dir = "--";
  if (has[GRID_P]) {
    float g = val[GRID_P];
    bool imp = GRID_POSITIVE_IS_IMPORT ? g > 0 : g < 0;
    fmtPower(v, sizeof v, g);
    if (fabsf(g) < 30)  { dir = "--"; }
    else if (imp)       { dir = "prel."; gc = C_RED; }
    else                { dir = "imm.";  gc = C_GREEN; }
  } else strcpy(v, "--");
  if (has[GRID_V] && val[GRID_V] < 100) { strcpy(s, "RETE ASSENTE"); sc = C_RED; }
  else if (has[GRID_V] && has[GRID_F])
    snprintf(s, sizeof s, "%s  %dV %.1fHz", dir, (int)lroundf(val[GRID_V]), val[GRID_F]);
  else snprintf(s, sizeof s, "%s", dir);
  cardValue(2, 4, 5, v, gc, s, sc);

  // BATTERIA (potenza)
  uint16_t bc = C_TEXT;
  const char* bs = "--";
  if (has[BATT_P]) {
    float b = val[BATT_P];
    bool chg = BATT_POSITIVE_IS_CHARGE ? b > 0 : b < 0;
    fmtPower(v, sizeof v, b);
    if (fabsf(b) < 20) { bs = "a riposo"; }
    else if (chg)      { bs = "in carica";  bc = C_GREEN; }
    else               { bs = "in scarica"; bc = C_ORANGE; }
  } else strcpy(v, "--");
  cardValue(3, 6, 7, v, bc, bs, bc == C_TEXT ? C_DIM : bc);

  drawSocValues(168);
}

// ----------------------- Pagina 2: OGGI E METEO -----------------------
const int ROW0 = 54, ROWH = 22;
const char* LEFT_LABELS[8]  = {"Solare", "Casa", "Prelievo", "Immissione",
                               "Batt. carica", "Batt. scarica", "Autosuff.", "Autocons."};
const char* RIGHT_LABELS[7] = {"", "Temperatura", "Nuvole", "Resto oggi",
                               "PV previsto", "PV reale", "Resa"};

void drawPage2Static() {
  tft.fillRoundRect(4, 28, 154, 208, 6, C_CARD);
  tft.fillRoundRect(162, 28, 154, 208, 6, C_CARD);
  text("ENERGIA OGGI kWh", 12, 32, 2, C_SOLAR, C_CARD, TL_DATUM, 0);
  text("METEO", 170, 32, 2, C_LOAD, C_CARD, TL_DATUM, 0);
  for (int i = 0; i < 8; i++) text(LEFT_LABELS[i], 12, ROW0 + i * ROWH, 2, C_DIM, C_CARD, TL_DATUM, 0);
  tft.drawFastHLine(12, ROW0 + 6 * ROWH - 3, 138, C_LINE);
  for (int i = 1; i < 7; i++) text(RIGHT_LABELS[i], 170, ROW0 + i * ROWH, 2, C_DIM, C_CARD, TL_DATUM, 0);
}

void leftRow(int i, const char* s, uint16_t col) {
  field(10 + i, s, 150, ROW0 + i * ROWH, 2, col, C_CARD, TR_DATUM, 56);
}
void rightRow(int i, const char* s, uint16_t col) {
  field(18 + i, s, 308, ROW0 + i * ROWH, 2, col, C_CARD, TR_DATUM, 64);
}

void drawPage2Values() {
  char b[24];
  const Val ev[6] = {E_PV, E_LOAD, E_GIN, E_GOUT, E_BIN, E_BOUT};
  const uint16_t ec[6] = {C_SOLAR, C_LOAD, C_RED, C_GREEN, C_GREEN, C_ORANGE};
  for (int i = 0; i < 6; i++) {
    if (has[ev[i]]) snprintf(b, sizeof b, "%.1f", val[ev[i]]); else strcpy(b, "--");
    leftRow(i, b, ec[i]);
  }
  // Autosufficienza: quota dei consumi non presa dalla rete
  if (has[E_LOAD] && has[E_GIN] && val[E_LOAD] > 0.05f) {
    int a = constrain((int)lroundf(100.0f * (1.0f - val[E_GIN] / val[E_LOAD])), 0, 100);
    snprintf(b, sizeof b, "%d %%", a);
  } else strcpy(b, "--");
  leftRow(6, b, C_TEXT);
  // Autoconsumo: quota della produzione non immessa in rete
  if (has[E_PV] && has[E_GOUT] && val[E_PV] > 0.05f) {
    int a = constrain((int)lroundf(100.0f * (1.0f - val[E_GOUT] / val[E_PV])), 0, 100);
    snprintf(b, sizeof b, "%d %%", a);
  } else strcpy(b, "--");
  leftRow(7, b, C_TEXT);

  // Condizione meteo (riga intera)
  uint16_t wc = C_DIM;
  if (has[W_CODE]) {
    const char* w = wmoText((int)lroundf(val[W_CODE]), wc);
    if (w) strncpy(b, w, sizeof b); else snprintf(b, sizeof b, "Codice %d", (int)lroundf(val[W_CODE]));
    b[sizeof b - 1] = 0;
  } else strcpy(b, "--");
  field(18, b, 170, ROW0, 2, wc, C_CARD, TL_DATUM, 140);

  tempField(19, "", has[W_TEMP], val[W_TEMP], 308, ROW0 + 1 * ROWH, 2, C_TEXT, C_CARD, TR_DATUM, 64);

  if (has[W_CLOUD]) snprintf(b, sizeof b, "%d %%", (int)lroundf(val[W_CLOUD])); else strcpy(b, "--");
  rightRow(2, b, C_TEXT);

  if (has[W_REMAIN]) snprintf(b, sizeof b, "%.1f kWh", val[W_REMAIN]); else strcpy(b, "--");
  rightRow(3, b, C_SOLAR);

  if (has[W_PRED]) fmtPower(b, sizeof b, val[W_PRED]); else strcpy(b, "--");
  rightRow(4, b, C_DIM);

  if (has[PV_P]) fmtPower(b, sizeof b, val[PV_P]); else strcpy(b, "--");
  rightRow(5, b, C_SOLAR);

  uint16_t rc = C_TEXT;
  if (has[PV_P] && has[W_PRED] && val[W_PRED] > 50) {
    int r = (int)lroundf(100.0f * val[PV_P] / val[W_PRED]);
    rc = r >= 90 ? C_GREEN : (r >= 60 ? C_ORANGE : C_RED);
    snprintf(b, sizeof b, "%d %%", r);
  } else strcpy(b, "--");
  rightRow(6, b, rc);
}

// ----------------------- Pagina 3: BATTERIA E FV -----------------------
// Gauge a semicerchio della produzione FV (0 .. GAUGE_MAX_W) + barra SOC
const int G_CX = 160, G_CY = 144, G_RO = 104, G_RI = 84;

// Punto sul semicerchio superiore: f = 0 a sinistra, f = 1 a destra
void gaugePoint(float f, int r, int& x, int& y) {
  x = G_CX - (int)lroundf(r * cosf(PI * f));
  y = G_CY - (int)lroundf(r * sinf(PI * f));
}

void drawPage3Static() {
  tft.fillRoundRect(4, 28, 312, 136, 6, C_CARD);
  text("FOTOVOLTAICO", 12, 32, 2, C_SOLAR, C_CARD, TL_DATUM, 0);
  // tacche ogni 25 %
  for (int i = 0; i <= 4; i++) {
    int x0, y0, x1, y1;
    gaugePoint(i / 4.0f, G_RO + 2, x0, y0);
    gaugePoint(i / 4.0f, G_RO + 7, x1, y1);
    tft.drawLine(x0, y0, x1, y1, C_DIM);
  }
  char b[16];
  text("0", G_CX - (G_RO + G_RI) / 2, G_CY + 4, 2, C_DIM, C_CARD, TC_DATUM, 0);
  snprintf(b, sizeof b, "%.1f kW", GAUGE_MAX_W / 1000.0f);
  text(b, G_CX + (G_RO + G_RI) / 2, G_CY + 4, 2, C_DIM, C_CARD, TC_DATUM, 0);
  drawSocStatic(168);
}

void drawPage3Values() {
  char v[24], s[32];
  float pv = has[PV_P] ? fmaxf(val[PV_P], 0.0f) : 0.0f;
  float f = constrain(pv / GAUGE_MAX_W, 0.0f, 1.0f);
  int pm = (int)lroundf(f * 1000);   // risoluzione 0,1 %
  uint16_t col = has[PV_P] ? C_SOLAR : C_DIM;

  snprintf(s, sizeof s, "%d", has[PV_P] ? pm : -1);
  if (changed(0, s, col)) {
    uint32_t a = 90 + (uint32_t)lroundf(f * 180);   // 90 = ore 9, 270 = ore 3
    if (a > 90)  tft.drawArc(G_CX, G_CY, G_RO, G_RI, 90, a, col, C_CARD, true);
    if (a < 270) tft.drawArc(G_CX, G_CY, G_RO, G_RI, a, 270, C_LINE, C_CARD, true);
  }

  if (has[PV_P]) fmtPower(v, sizeof v, val[PV_P]); else strcpy(v, "--");
  field(1, v, G_CX, G_CY - 36, 4, col, C_CARD, MC_DATUM, 2 * G_RI - 24);

  if (has[PV_P]) snprintf(s, sizeof s, "%d %% del massimo", (int)lroundf(f * 100));
  else strcpy(s, "nessun dato");
  field(2, s, G_CX, G_CY - 10, 2, C_DIM, C_CARD, MC_DATUM, 2 * G_RI - 24);

  drawSocValues(168);
}

// ----------------------- Gestione pagine -----------------------
void drawValues() {
  if (page == 0)      drawPage1Values();
  else if (page == 1) drawPage2Values();
  else                drawPage3Values();
}

void drawPage() {
  clearCache();
  tft.fillScreen(C_BG);
  drawHeader();
  if (page == 0)      drawPage1Static();
  else if (page == 1) drawPage2Static();
  else                drawPage3Static();
  drawValues();
}

void switchPage() {
  page = (page + 1) % N_PAGES;
  lastPageSwitch = millis();
  drawPage();
}

// Lettura touch filtrata: scarta i "tocchi fantasma" (rumore, valori saturi a 4095,
// pressione troppo bassa) che altrimenti fanno cambiare pagina di continuo.
bool validTouch() {
  if (!ts.touched()) return false;
  TS_Point p = ts.getPoint();
  bool ok = p.z >= TOUCH_MIN_Z && p.z < 4000 &&
            p.x > 150 && p.x < 3950 && p.y > 150 && p.y < 3950;
  if (TOUCH_DEBUG) Serial.printf("touch x=%d y=%d z=%d %s\n", p.x, p.y, p.z, ok ? "OK" : "scartato");
  return ok;
}

// Cambio pagina solo con un tocco stabile (3 letture valide consecutive, ~60 ms)
// e dopo che il dito è stato sollevato (5 letture a vuoto consecutive).
void handleTouch() {
  static uint32_t lastSample = 0, lastSwitch = 0;
  static uint8_t pressed = 0, released = 0;
  static bool armed = true;
  if (millis() - lastSample < 20) return;
  lastSample = millis();

  if (validTouch()) {
    released = 0;
    lastTouchMs = millis();   // ogni tocco valido mette in pausa lo scorrimento automatico
    if (pressed < 255) pressed++;
    if (armed && pressed >= 3 && millis() - lastSwitch > 700) {
      armed = false;
      lastSwitch = millis();
      switchPage();
    }
  } else {
    pressed = 0;
    if (released < 255) released++;
    if (released >= 5) armed = true;
  }
}

// ----------------------- LED RGB (retro) -----------------------
// Sul CYD il LED RGB è ad ANODO COMUNE: si accende con livello BASSO
#define LED_R_PIN 4
#define LED_G_PIN 16
#define LED_B_PIN 17
const uint8_t LED_CH[3] = {1, 2, 3};   // canali LEDC (il canale 0 resta libero)

void ledWrite(uint8_t r, uint8_t g, uint8_t b) {
  ledcWrite(LED_CH[0], 255 - r);       // logica invertita (attivo basso)
  ledcWrite(LED_CH[1], 255 - g);
  ledcWrite(LED_CH[2], 255 - b);
}

void ledSetup() {
  const uint8_t pins[3] = {LED_R_PIN, LED_G_PIN, LED_B_PIN};
  for (int i = 0; i < 3; i++) {
    ledcSetup(LED_CH[i], 5000, 8);
    ledcAttachPin(pins[i], LED_CH[i]);
  }
  ledWrite(0, 0, 0);
}

void updateLed() {
  static uint32_t last = 0, phaseMs = 0;
  uint32_t now = millis();
  uint32_t dt = now - last;
  if (dt < 20) return;
  last = now;

  bool fresh = mqtt.connected() && has[BATT_P] && now - lastMsgMs < STALE_MS;
  float p = fresh ? val[BATT_P] : 0;
  if (!LED_ENABLED || !fresh || fabsf(p) < LED_IDLE_W) { ledWrite(0, 0, 0); phaseMs = 0; return; }

  bool charging = BATT_POSITIVE_IS_CHARGE ? p > 0 : p < 0;

  // Più potenza = respiro più veloce
  float k = constrain(fabsf(p) / LED_FULL_POWER_W, 0.0f, 1.0f);
  float period = LED_PERIOD_SLOW - k * (LED_PERIOD_SLOW - LED_PERIOD_FAST);
  phaseMs += dt;
  float x = fmodf(phaseMs, period) / period;               // 0..1
  float sv = 0.5f - 0.5f * cosf(2.0f * PI * x);            // onda morbida 0..1
  uint8_t v = (uint8_t)(LED_MAX * (0.03f + 0.97f * sv * sv)); // correzione gamma, minimo appena acceso

  if (charging) ledWrite(0, v, 0);
  else          ledWrite(v, 0, 0);
}

// ----------------------- Connessioni -----------------------
void ensureConnections() {
  static uint32_t lastMqttTry = 0;
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiTry > 10000) {
      lastWifiTry = millis();
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
    return;
  }
  if (!mqtt.connected() && millis() - lastMqttTry > 5000) {
    lastMqttTry = millis();
    char id[32];
    snprintf(id, sizeof id, "cyd-solar-%06lx", (unsigned long)(ESP.getEfuseMac() & 0xFFFFFF));
    bool ok = strlen(MQTT_USER) ? mqtt.connect(id, MQTT_USER, MQTT_PASS) : mqtt.connect(id);
    if (ok) {
      mqtt.subscribe("solar_assistant/total/+/state");
      mqtt.subscribe("solar_assistant/weather/+/state");
      mqtt.subscribe("solar_assistant/" BATT_V_KEY "/state");
      mqtt.subscribe("solar_assistant/status");
    }
  }
}

// ----------------------- Setup / Loop -----------------------
void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);   // orizzontale; usa 3 se lo vuoi ruotato di 180°
  tft.fillScreen(C_BG);
#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif

  ledSetup();

  touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchSPI);
  ts.setRotation(1);

  text("Solar Monitor", 160, 100, 4, C_SOLAR, C_BG, MC_DATUM, 0);
  text("Connessione al WiFi...", 160, 140, 2, C_DIM, C_BG, MC_DATUM, 0);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 15000) delay(200);
  lastWifiTry = millis();

  configTzTime(TZ_INFO, "pool.ntp.org", "time.google.com");

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqtt);
  mqtt.setSocketTimeout(3);
  mqtt.setKeepAlive(30);

  drawPage();
}

void loop() {
  ensureConnections();
  mqtt.loop();
  handleTouch();
  updateLed();

  // Scorrimento automatico solo dopo IDLE_MS senza tocchi
  if (AUTO_PAGE_MS && millis() - lastTouchMs > IDLE_MS &&
      millis() - lastPageSwitch > AUTO_PAGE_MS) switchPage();

  static uint32_t lastDraw = 0;
  if (millis() - lastDraw > 500) {
    lastDraw = millis();
    updateHeader(false);
    if (dirty) { dirty = false; drawValues(); }
  }
}
