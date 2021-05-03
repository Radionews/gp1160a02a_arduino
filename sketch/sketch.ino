//библиотека для работы с экраном
#include <vfd.h>
VFD vfd_display;

void setup() {
  Serial.begin(19200,SERIAL_8O1);
  
  vfd_display.clear();
  vfd_display.set_mode(MODE_3_ROWS);
  vfd_display.set_dimming(1);

}

void loop() {
  //vfd_display.send_str("ПРОСТО строка",0,2);
  delay(2000);
  
  //vfd_display.set_scroll("СПАСИБО ВСЕМ ЗА ПОДДЕРЖКУ! ",1); 
  delay(2000);

  vfd_display.print_graph(0,0,DISPLAY_HEIGHT,DISPLAY_WIDTH,test_fire);
  delay(2000);
}
