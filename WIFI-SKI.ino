#include <WiFi.h>
#include <WebServer.h>

// Access Point credentials (change these before use)
const char *AP_SSID = "JETSKI-ESP32";
const char *AP_PASS = "jetski123";

// Pin mapping from your wiring list
// NOTE: Your list uses "EN2" on GPIO25. Set M1_SECOND_PIN_IS_ENABLE to true
// only if GPIO25 is actually wired to L298N ENA/ENB instead of IN2.
const int M1_IN1 = 26;
const int M1_IN2 = 25;
const int M2_IN3 = 27;
const int M2_IN4 = 14;
const bool M1_SECOND_PIN_IS_ENABLE = false;

WebServer server(80);

unsigned long lastCommandMs = 0;
const unsigned long COMMAND_TIMEOUT_MS = 1500;  // Auto-stop if no command received
bool isMoving = false;
int m1State = 0;
int m2State = 0;

void setMotor1(int dir) {
  // dir: 1 forward, -1 backward, 0 stop
  if (M1_SECOND_PIN_IS_ENABLE) {
    // Enable-pin mode: direction reverse is not possible with this wiring.
    if (dir > 0) {
      digitalWrite(M1_IN1, HIGH);
      digitalWrite(M1_IN2, HIGH);
      m1State = 1;
    } else if (dir < 0) {
      digitalWrite(M1_IN1, LOW);
      digitalWrite(M1_IN2, LOW);
      m1State = 0;
      Serial.println("WARN: Motor1 reverse ignored in enable-pin mode. Rewire GPIO25 to IN2 for reverse.");
    } else {
      digitalWrite(M1_IN1, LOW);
      digitalWrite(M1_IN2, LOW);
      m1State = 0;
    }
  } else {
    if (dir > 0) {
      digitalWrite(M1_IN1, HIGH);
      digitalWrite(M1_IN2, LOW);
    } else if (dir < 0) {
      digitalWrite(M1_IN1, LOW);
      digitalWrite(M1_IN2, HIGH);
    } else {
      digitalWrite(M1_IN1, LOW);
      digitalWrite(M1_IN2, LOW);
    }
    m1State = dir;
  }
}

void setMotor2(int dir) {
  // dir: 1 forward, -1 backward, 0 stop
  if (dir > 0) {
    digitalWrite(M2_IN3, HIGH);
    digitalWrite(M2_IN4, LOW);
  } else if (dir < 0) {
    digitalWrite(M2_IN3, LOW);
    digitalWrite(M2_IN4, HIGH);
  } else {
    digitalWrite(M2_IN3, LOW);
    digitalWrite(M2_IN4, LOW);
  }
  m2State = dir;
}

void stopAll() {
  setMotor1(0);
  setMotor2(0);
  isMoving = false;
}

void driveCommand(const String &cmd) {
  if (cmd == "forward") {
    setMotor1(1);
    setMotor2(1);
    isMoving = true;
  } else if (cmd == "backward") {
    setMotor1(-1);
    setMotor2(-1);
    isMoving = true;
  } else if (cmd == "left") {
    setMotor1(-1);
    setMotor2(1);
    isMoving = true;
  } else if (cmd == "right") {
    setMotor1(1);
    setMotor2(-1);
    isMoving = true;
  } else if (cmd == "m1f") {
    setMotor1(1);
    isMoving = true;
  } else if (cmd == "m1b") {
    setMotor1(-1);
    isMoving = true;
  } else if (cmd == "m1s") {
    setMotor1(0);
  } else if (cmd == "m2f") {
    setMotor2(1);
    isMoving = true;
  } else if (cmd == "m2b") {
    setMotor2(-1);
    isMoving = true;
  } else if (cmd == "m2s") {
    setMotor2(0);
  } else {
    stopAll();
    return;
  }

  // Track movement via state vars instead of reading back output pins.
  isMoving = (m1State != 0 || m2State != 0);
  lastCommandMs = millis();

  Serial.print("CMD: ");
  Serial.print(cmd);
  Serial.print(" | m1=");
  Serial.print(m1State);
  Serial.print(" m2=");
  Serial.println(m2State);
}

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>DIY JetSki Control</title>
  <style>
    :root {
      --sea-deep: #083344;
      --sea: #0e7490;
      --foam: #ecfeff;
      --sun: #f59e0b;
      --coral: #ef4444;
      --lime: #22c55e;
      --panel: #ffffffee;
      --ink: #0f172a;
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      min-height: 100vh;
      font-family: "Segoe UI Variable", "Bahnschrift", "Trebuchet MS", sans-serif;
      color: var(--ink);
      background:
        radial-gradient(circle at 15% 20%, #67e8f9aa 0 18%, transparent 30%),
        radial-gradient(circle at 85% 10%, #fde68aaa 0 16%, transparent 28%),
        linear-gradient(160deg, var(--sea-deep), var(--sea));
      display: grid;
      place-items: center;
      padding: 16px;
    }

    .card {
      width: min(720px, 100%);
      background: var(--panel);
      backdrop-filter: blur(6px);
      border-radius: 22px;
      padding: 18px;
      box-shadow: 0 12px 28px #0000002e;
      animation: rise 420ms ease-out;
    }

    @keyframes rise {
      from { transform: translateY(10px); opacity: 0; }
      to { transform: translateY(0); opacity: 1; }
    }

    h1 {
      margin: 0 0 6px;
      font-size: clamp(1.4rem, 2.8vw, 2rem);
      letter-spacing: 0.04em;
      text-transform: uppercase;
    }

    .sub {
      margin: 0 0 14px;
      font-size: 0.95rem;
      opacity: 0.8;
    }

    .status {
      display: inline-block;
      margin-bottom: 14px;
      font-weight: 700;
      padding: 7px 11px;
      border-radius: 999px;
      background: #dbeafe;
      transition: background 160ms ease;
    }

    .status.live { background: #dcfce7; }
    .status.warn { background: #fee2e2; }

    .grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 14px;
    }

    .pad, .fine {
      background: #f8fafc;
      border: 1px solid #e2e8f0;
      border-radius: 16px;
      padding: 12px;
    }

    .dpad {
      display: grid;
      gap: 10px;
      grid-template-columns: repeat(3, minmax(58px, 1fr));
      justify-items: stretch;
    }

    .wide { grid-column: 1 / span 3; }

    button {
      border: 0;
      border-radius: 12px;
      min-height: 52px;
      font-size: 1rem;
      font-weight: 700;
      color: white;
      cursor: pointer;
      transition: transform 80ms ease, filter 120ms ease;
    }

    button:active { transform: translateY(1px) scale(0.99); }

    .go { background: linear-gradient(135deg, #0ea5e9, #0284c7); }
    .turn { background: linear-gradient(135deg, #f59e0b, #d97706); }
    .back { background: linear-gradient(135deg, #ef4444, #dc2626); }
    .stop { background: linear-gradient(135deg, #334155, #0f172a); }
    .motor1 { background: linear-gradient(135deg, #22c55e, #16a34a); }
    .motor2 { background: linear-gradient(135deg, #a855f7, #7e22ce); }

    .row {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 8px;
      margin-top: 8px;
    }

    h2 {
      margin: 0 0 8px;
      font-size: 1.05rem;
      letter-spacing: 0.02em;
    }

    .hint {
      margin-top: 12px;
      font-size: 0.85rem;
      opacity: 0.75;
    }

    @media (max-width: 720px) {
      .grid { grid-template-columns: 1fr; }
    }
  </style>
</head>
<body>
  <main class="card">
    <h1>DIY JetSki Control</h1>
    <p class="sub">Connected directly to your ESP32 Wi-Fi hotspot</p>
    <div id="status" class="status">Status: idle</div>

    <section class="grid">
      <div class="pad">
        <h2>Drive</h2>
        <div class="dpad">
          <div></div>
          <button class="go" data-cmd="forward">Forward</button>
          <div></div>

          <button class="turn" data-cmd="left">Left</button>
          <button class="stop" data-cmd="stop">Stop</button>
          <button class="turn" data-cmd="right">Right</button>

          <div></div>
          <button class="back" data-cmd="backward">Backward</button>
          <div></div>

          <button class="stop wide" data-cmd="stop">Emergency Stop</button>
        </div>
      </div>

      <div class="fine">
        <h2>Motor 1 (Left)</h2>
        <div class="row">
          <button class="motor1" data-cmd="m1f">M1 Fwd</button>
          <button class="motor1" data-cmd="m1s">M1 Stop</button>
          <button class="motor1" data-cmd="m1b">M1 Rev</button>
        </div>

        <h2 style="margin-top:12px">Motor 2 (Right)</h2>
        <div class="row">
          <button class="motor2" data-cmd="m2f">M2 Fwd</button>
          <button class="motor2" data-cmd="m2s">M2 Stop</button>
          <button class="motor2" data-cmd="m2b">M2 Rev</button>
        </div>

        <p class="hint">Tip: if direction is reversed, swap that motor's two wires on L298N output.</p>
      </div>
    </section>
  </main>

  <script>
    const statusEl = document.getElementById('status');

    async function send(cmd) {
      try {
        const r = await fetch(`/cmd?d=${encodeURIComponent(cmd)}`, { cache: 'no-store' });
        const text = await r.text();
        statusEl.textContent = `Status: ${text}`;
        statusEl.className = 'status ' + (cmd === 'stop' ? 'warn' : 'live');
      } catch (e) {
        statusEl.textContent = 'Status: connection error';
        statusEl.className = 'status warn';
      }
    }

    document.querySelectorAll('button[data-cmd]').forEach((btn) => {
      btn.addEventListener('click', () => send(btn.dataset.cmd));
    });
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleCmd() {
  if (!server.hasArg("d")) {
    server.send(400, "text/plain", "missing command");
    return;
  }

  String cmd = server.arg("d");
  cmd.toLowerCase();
  driveCommand(cmd);
  server.send(200, "text/plain", cmd);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void setup() {
  Serial.begin(115200);

  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);
  pinMode(M2_IN3, OUTPUT);
  pinMode(M2_IN4, OUTPUT);
  stopAll();

  WiFi.mode(WIFI_AP);
  bool started = WiFi.softAP(AP_SSID, AP_PASS);

  if (!started) {
    Serial.println("Failed to start AP");
  } else {
    Serial.print("AP started. Connect to SSID: ");
    Serial.println(AP_SSID);
    Serial.print("Open: http://");
    Serial.println(WiFi.softAPIP());
  }

  server.on("/", handleRoot);
  server.on("/cmd", handleCmd);
  server.on("/status", []() {
    String status = "m1=" + String(m1State) + ",m2=" + String(m2State) + ",moving=" + String(isMoving ? "1" : "0");
    server.send(200, "text/plain", status);
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  if (isMoving && (millis() - lastCommandMs > COMMAND_TIMEOUT_MS)) {
    stopAll();
    Serial.println("Auto-stop: command timeout");
  }
}
