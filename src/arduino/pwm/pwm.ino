#define PWMPIN 3

void setup() {
  // put your setup code here, to run once:
  //analogWrite(PWMPin, 0xA0);
  //set the PWM pin to output
  pinMode(PWMPIN, OUTPUT);
  TIMSK2 = (1<<OCIE2A)| (0<<TOIE2);
  OCR2A = 0x50;
  TCNT2=0x00;
  TCCR2A = (1<<COM2A1) | (0<<COM2A0) | (1<<WGM21) | (1<<WGM20);
  TCCR2B = (0<<WGM22) | (1<<CS22) | (0<<CS21) | (0<<CS20);
}

uint8_t changed=0;
uint8_t compval=0;

void loop() {
  compval
  //0x10 full reverser
  //0x30 stop
  //0x90 full forward
  
  // put your main code here, to run repeatedly:
  /*delay(5000);
  compval=0x80;
  changed=1;*/
}

ISR(TIMER2_COMPA_vect){
  if(changed){
    OCR2A=compval;
    changed=0;
  }
}
