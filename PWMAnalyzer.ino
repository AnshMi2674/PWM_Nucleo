void setup() {
    Serial.begin(115200);
    pinMode(8, INPUT);
}

void loop() {
    // Longer timeout = 100ms, won't miss pulse
    unsigned long high_time = pulseIn(8, HIGH, 100000);
    unsigned long period    = pulseIn(8, LOW,  100000) + high_time;

    if(high_time > 0) {
        Serial.print("Pulse width (us): ");
        Serial.println(high_time);
        Serial.print("Period (us): ");
        Serial.println(period);
        Serial.print("Frequency (Hz): ");
        Serial.println(1000000.0 / period);
    } else {
        Serial.println("No pulse detected");
    }
    Serial.println("---");
    delay(200);
}
