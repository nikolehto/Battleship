#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "lcd.h"
#include "sounds.h"

#define ButtonCenter (!(PINA & (1 << PA2)))

void nyancat()	{
	char nyan1[2][51] = {								// alustuksessa määritetty jaksoksi 15625 hz nuottiin kirjoitetaan jakaja
	{20,18,25,24,30,25,27,30,30,27,25,25,27,30,27,24,20,18,24,20,27,24,30,27,30,24,20,18,24,20,27,24,30,25,24,25,27,30,27,25,30,27,24,18,25,24,27,30,27,30,27},
	{2,2,1,2,1,1,1,2,2,2, 2,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 2,2,1,1,1,1,1,1,1,1,1,1,1,1, 2,1,1,1,1,1,1,1,1,2,2,2} };		//kesto sadoissa millisekunneissa

	
	char nyan2[2][55] = {								
	{30,39,36,30,39,36,30,27,24,30,22,24,22,20, 30,30,39,36,30,39,22,24,27,30,39,47,45,39, 30,39,36,30,39,36,30,30,27,24,30,39,36,39, 30,30,32,30,39,36,30,22,24,22,20,30,32},
	{2,1,1,2,1,1,1,1,1,1,1,1,1,1, 2,1,1,2,1,1,1,1,1,1,1,1,1,1, 2,1,1,2,1,1,1,1,1,1,1,1,1,1, 2,1,1,1,1,1,1,1,1,1,1,2,2} };

	while (1) {	
		for (int i = 0; i<51; i++) {
			soitanuotti(nyan1[0][i], nyan1[1][i]);
			_delay_ms(3); // erottaa nuotit
			if (ButtonCenter) {
				while (ButtonCenter)
					_delay_ms(50);
				
				return;
			}
		}

		for (int i = 0; i<55; i++) {
			soitanuotti(nyan2[0][i], nyan2[1][i]);
			_delay_ms(3);
			if (ButtonCenter) {
				while (ButtonCenter)
					_delay_ms(50);
				
				return;
			}
		}

		for (int i = 0; i<55; i++) {
			soitanuotti(nyan2[0][i], nyan2[1][i]);
			_delay_ms(3);
			if (ButtonCenter) {
				while (ButtonCenter)
					_delay_ms(50);
				
				return;
			}
		}

		for (int i = 0; i<51; i++) {
			soitanuotti(nyan1[0][i], nyan1[1][i]);
			_delay_ms(3);
			if (ButtonCenter) {
				while (ButtonCenter)
					_delay_ms(50);
				
				return;
			}
		}
	}

}

void havio() {
	soitanuotti(32, 4);
	_delay_ms(5);
	soitanuotti(32, 2);
	_delay_ms(5);
	soitanuotti(47, 8);
	_delay_ms(2); 
}

void soitanuotti(char taajuus,char kesto) {
// E =47 F =45 F# = 42, G = 39, G# = 38, 36, 34, 32, 30, 28, 27, 25, 24, 22,  G =20, G# =19, A = 18

	TCNT3 = 0;
	OCR3AL = taajuus;
	kesto = kesto * 90;
	
	SREG |= (1 << 7);
	//delay ja variable
	//http://www.avrfreaks.net/index.php?name=PNphpBB2&file=printview&t=74093&start=0
	while ((kesto)>1) {
		kesto -= 5;
		_delay_ms(5);		
	}
	//while (pulssit > 0); Käytin toista laskuria mutta tämä lause ei toiminut optimoinnin kanssa. Ratkaisin delaylla
	
	SREG &= ~(1 << 7);
}


ISR(TIMER3_COMPA_vect) {
	PORTE ^= (1 << PE4) | (1 << PE5);
}

//delay ja variable
	//http://www.avrfreaks.net/index.php?name=PNphpBB2&file=printview&t=74093&start=0

//Helpompi ja tarkempi summerin toteutus jos jää aikaa pieni prescaling ja sävelten kertoimet http://en.wikipedia.org/wiki/Twelfth_root_of_two
