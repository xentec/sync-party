#define PWMPIN 11


void setupTIMER2(){
  //enable the compare interrupt
  TIMSK2 = (1<<OCIE2A);
  //set the compare register to the stop value
  OCR2A = 0x30;
  TCNT2=0x00;
  //set up the TIMER2 FastPWM Mode (WGM22-WGM20) and the clear ~OC2A on compare match, set OC2A at BOTTOM (COM2A1-COM2A0)
  TCCR2A = (1<<COM2A1) | (0<<COM2A0) | (1<<WGM21) | (1<<WGM20);
  //set the prescaler of TIMER2 to 256
  TCCR2B = (0<<WGM22) | (1<<CS22) | (1<<CS21) | (0<<CS20);
}

uint8_t compval=0x30;
void setSpeed(unsigned char newSpeed){
  //0x10 full reverse
  //0x30 stop
  //0x90 full forward  
  compval=newSpeed;
  //enable the compare interrupt 
  TIMSK2=(1<<OCIE2A);  
}

void setup() {
  pinMode(PWMPIN, OUTPUT);
  setupTIMER2();
}

void loop() {

}

ISR(TIMER2_COMPA_vect){
  //set the compare register to the new value
  OCR2A=compval;
  //disable the compare interrupt
  TIMSK2&=~(1<<OCIE2A);
}
