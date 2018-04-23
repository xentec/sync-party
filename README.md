## Sync-Party

Synchrongesteurte RC-Autos.

### Kompileren

Zum Kompilieren des Projektes reicht es die `build.sh` auszuführen. Die fertigen Binaries liegen direkt im `build`-Ordner.

### Settings

**Pololu Motor Controller**

The Pololu Motor Controller recieves a PWM signal via the RC1 pin from the arduino. If the PWM signal was generated with the following compare values (in register ORC2A) on the arduino the motor will speed up to the direction
	0x10 means full reverse speed
	0x30 means stop the motor
	0x90 means full forward speed

