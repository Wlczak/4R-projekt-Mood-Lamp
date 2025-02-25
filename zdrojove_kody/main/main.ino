#include <WiCo.h>
WiCo wico;
void setup() {
  wico.addSTA("wemos", "wemosD1R2");
  wico.connectSTA();
}

void loop() {
  // put your main code here, to run repeatedly:
}
