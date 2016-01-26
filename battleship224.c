#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <util/delay.h>
#include "lcd.h"
#include "sounds.h"

#define ButtonUp (!(PINA & (1 << PA0)))
#define ButtonLeft (!(PINA & (1 << PA1)))
#define ButtonCenter (!(PINA & (1 << PA2)))
#define ButtonRight (!(PINA & (1 << PA3)))
#define ButtonDown (!(PINA & (1 << PA4)))
#define Buzz (PORTE ^= (1 << PE4) | (1 << PE5))
#define LedOff (PORTA &= ~(1 << PA6))
#define LedOn (PORTA |=  (1 << PA6))


int pelifunktio(int vaikeus, int seed);
int luokentta(int (*kentta)[10], int seed);
int asetalaiva(int pituus, int (*kentta)[10]);
void ammufunktio(int (*kentta)[10], int x, int y, int *ammusmaara);
int tarkastavt(int (*kentta)[10], int ammukset);
void piirtofunktio(int (*kentta)[10], int vasenyk, int x , int y, int ammusmaara);
void liikutakursoria(int *x, int *y, char suunta, int *vasenyk);
void lcd_gotoxy (int x, int y);
int annaseed(void);
void lcd_write_str(char *sana, int rivi);


void init(void) {	//Alustus
		cli(); //Estet‰‰n keskeytykset

        DDRE  |=  (1 << PE4) | (1 << PE5);	//kaiutin pinnit ulostuloksi
        PORTE &= ~(1 << PE4);				// pinni PE4 nollataan
        PORTE |=  (1 << PE5);  				// pinni PE5 asetetaan
        	
		TCCR3A &= ~( (1 << WGM31) | (1 << WGM30) ); // ajastin nollautuu, kun sen ja OCR3A rekisterin arvot ovat samat
        TCCR3B |=    (1 << WGM32);
        TCCR3B &=   ~(1 << WGM33);

		ETIMSK |= (1 << OCIE3A);

        OCR3AH = 0x00;	// asetetaan OCR1A rekisterin arvoksi 0x22
        OCR3AL = 0x22;
		
		TCCR3B |= (1 << CS32) | (1 << CS30);

			
		DDRA &= ~(1 << PA0); // n‰pp‰in pinnit sis‰‰ntuloksi
		DDRA &= ~(1 << PA2);
		DDRA &= ~(1 << PA4);

		DDRA |= (1 << PA6);	// led pinni ulostuloksi

		lcd_init();		//LCD-n‰ytˆn alustaminen
		lcd_write_ctrl(LCD_ON);
		lcd_write_ctrl(LCD_CLEAR);
}


int main(void)  {
/*Valikot*/
	init();
	int vaikeus = 25;	// Oletus = helppo
	char merkki = 0b00111110;
	int seed = annaseed();
	char y = 0;		// valikko muuttujia
	char z = 0;
	
	while (1) {		
		y = 0;
		z = 0;
		
		lcd_write_ctrl(LCD_CLEAR);
		lcd_write_str("  Pelaa nyt", 0);					//Main menu
		lcd_write_str("  Vaikeustaso", 1);
		while (1) {
			lcd_gotoxy(0,y);
			lcd_write_data(' ');
			if (ButtonDown && y < 1)
				y++;
			
			if (ButtonUp && y > 0)
				y--;
		
			if (ButtonCenter) {
				while (ButtonCenter)	// ei poistuta funktiosta nappipohjassa
					_delay_ms(50);
					
				break;
			}
		
			lcd_gotoxy(0,y);
			lcd_write_data(merkki);
			_delay_ms(50);
		}
	
		while (y == 0)  {						// Aloita peli
			switch (pelifunktio(vaikeus, seed)) {
				case 0: 								// Voitto
					lcd_write_ctrl(LCD_CLEAR);
					lcd_write_str("*****VOITTO*****", 0);
					while (ButtonCenter)				 // Odotetaan ett‰ nappi p‰‰stet‰‰n irti
						_delay_ms(50);				

					nyancat();
					break;
				case 1: 								// H√§vi√∂
					lcd_write_ctrl(LCD_CLEAR);
					lcd_write_str("*****HAVIO*****", 0);
					havio();
					_delay_ms(500);
 
					while (!(ButtonCenter))	// Voitossa t‰t‰ ei tarvita koska nyan huolehtii napin p‰‰stˆn
						_delay_ms(50);
					while (ButtonCenter)
						_delay_ms(50);
					break;
				case 2:
					continue;
				}
			
			lcd_write_ctrl(LCD_CLEAR);
			lcd_write_str("  Uusi peli", 0);
			lcd_write_str("  Valikkoon", 1);
			while (1) {
				lcd_gotoxy(0, z);
				lcd_write_data(' ');
				if (ButtonDown && z < 1)
					z++;
			
				if (ButtonUp && z > 0)
					z--;
		
				if (ButtonCenter) {
					while (ButtonCenter)
						_delay_ms(50);
					
					break;
				}
		
				lcd_gotoxy(0,z);
				lcd_write_data(merkki);			
				_delay_ms(50);
			}
			if (z == 0) {
				y = 0;
				seed++;
			}
			
			else
				y = 5;	//hyp‰t‰‰n seuraavien ehtojen yli
		}
		
		if (y == 1) {
			vaikeus = 0;
			lcd_write_ctrl(LCD_CLEAR);
			lcd_write_str("  Helppo (25)", 0);
			lcd_write_str("  Vaikea (10)", 1);
	
			while (1) {
				lcd_gotoxy(0, vaikeus);
				lcd_write_data(' ');
				if (ButtonDown && vaikeus < 1)
					vaikeus++;
			
				if (ButtonUp && vaikeus > 0)
					vaikeus--;
		
				if (ButtonCenter) {
					while (ButtonCenter)
						_delay_ms(50);
					
					break;
				}
				lcd_gotoxy(0,vaikeus);
				lcd_write_data(merkki);			
				_delay_ms(50);
			}
			
			if (vaikeus == 0)
				vaikeus = 25;
			else					// Jos lis‰t‰‰n tasoja muuta ifiksi
				vaikeus = 10;
			
		}
	}
}

int pelifunktio(int vaikeus, int seed) {	
/* N√§pp√§imien kuuntelu, Funktion hallinta */      
	int kentta[10][10] = {{ 0 }};	////Tuplasulkeilla varoituspois
	int vasenyk = 0;
	int	x = 0;
	int y = 0;
	int ammusmaara = vaikeus;
	int kursori = 1;
	int testi = 0;	//testausta varten
	
	if (luokentta(kentta, seed) == 0) {
		return 2;	// Palataan p‰‰valikkoon (ei koskaan tapahtunut)
	}

    while (1) {
		
		if (ButtonCenter) {
			testi++;
			if (testi > 40)	{//testausta varten
				return 0;	
			}	
			ammufunktio(kentta, x, y, &ammusmaara);
			switch (tarkastavt(kentta, ammusmaara)) {
				case 0: 
					break;
				case 1:
					return 1;
					break;
				case 2:
					return 0;
					break;
			}
		}	
		else {
			testi = 0;
		

		}

		if (ButtonUp){
			liikutakursoria(&x, &y, 'y', &vasenyk);
			kursori = 9;	// Pelattavuus paranee 100%
		}
						
		if (ButtonLeft){
			liikutakursoria(&x, &y, 'v', &vasenyk);
			kursori = 9;
		}

		if (ButtonRight){
			liikutakursoria(&x, &y, 'o', &vasenyk);
			kursori = 9;
		}
				
		if (ButtonDown){
			liikutakursoria(&x, &y, 'a', &vasenyk);
			kursori = 9;
		}
			
		piirtofunktio(kentta, vasenyk, x, y, ammusmaara);			//	Jos j‰‰ aikaa muuta kursorin laskemninen counter 2:lle ja piirr‰ koko kentt‰ vain jos nappia painetaan
																	// Myˆs nappi viiveet voi silloin ottaa pois ja napin uuteen painallukseen vaatia aika tai napin k‰yttˆ ylh‰‰ll‰
		if (kursori > 8) {	//  kursori muuttaa tilaa n. 400ms v√§lein
			lcd_gotoxy(x,(y - vasenyk));
			lcd_write_data('_');
			if (kursori > 15)
				kursori = 0;
				
		}
		_delay_ms(50); //Kuuntelu  20 kertaa sekunnissa. Aina kun nappia painetaan tulee lis√§√§ viivett√§ kursori- tai ammufunktiosta
		
		kursori++;
	}
}

int luokentta(int (*kentta)[10], int seed) {
/* Ohjaa kent√§n luomisen*/

    int lca = 0;	//looppirajoitin
    srand(seed);
    int laivat[] = {5,4,3,3,2,1};   
    int i = 0;

    while(1) {
        if((asetalaiva(laivat[i], kentta)) == 1) {
            i++;
        }     
        lca++;
        
        if(i == 6) {
        return 1; //luonti onnistui
        }
        if(lca > 200) {
        return 0; //luonti ep√§onnistui, Palataan mainiin asti
        }
    }
	



}

int asetalaiva(int pituus, int (*kentta)[10]) {
/*Pyrkii arpomaan laivan taulukkoon, palauttaa onnistuiko*/

    int x = rand() % 10;
    int y = rand() % 10;
    int suunta = rand() % 2 ; //vaaka 0 pysty 1

    switch (suunta) {
    case 0 :
		if((x+pituus) >10) 
            return 0;//ei onnistu
                
		for(int i = -1; i<pituus+2; i++) { //on otettava laivan pituus +2 kierrosta
            for(int j = -1; j<2; j++){ //pyˆr‰hdett‰v‰ 3 kertaa
                if(((x+i) <0) || ((y+j) <0) || ((x+i)>9) || ((y+j)>9)) { //tarkistuksia onko ulkona alueelta
                    continue;
                }
                               
                if(kentta[x+i][y+j] == 1) {////tarkistus onko kentt√É¬§ varattu
                    return 0;
                }
                
            }
        }

		for(int i = 0; i < pituus; i++) {     //Selvittiin
			kentta[x+i][y] = 1;
		}

    return 1;//onnistui
    break;
	
    case 1:		//Samaan tyyliin
        if((y+pituus) >10) {
            return 0;//ei onnistu
        }
        for(int j = -1; j<pituus+2; j++) { //on otettava laivan pituus +2 kierrosta
            for(int i = -1; i<2; i++){ //py√É¬∂r√É¬§hdett√É¬§v√É¬§ 3 kertaa
                if(((x+i) <0) || ((y+j) <0) || ((x+i)>9) || ((y+j)>9)) { //tarkistuksia onko ulkona alueelta
                    continue;
                }
                              
                if(kentta[x+i][y+j] == 1) { //tarkistuksia onko kentt√É¬§ varattu
                    return 0;
                }
                
            }
        }

    for(int j = 0; j < pituus; j++) {    //Selvittiin
        kentta[x][y+j] = 1;
    }
    return 1;
    break;
	}
	return 0;
}


void ammufunktio(int (*kentta)[10], int x, int y, int *ammusmaara){
/* Muokkaa kentt‰‰ ammuttaessa, v‰hent‰‰ ja lis‰‰ panokset*/

	_delay_ms(150);
    if (*ammusmaara>0) {
		
		switch(kentta[x][y]) {
         	case 0 ://jos kiinni ja tyhj‰‰
				Buzz;
         	   	kentta[x][y] = 2;
				--(*ammusmaara);
    			break;
      	 	case 1 ://jos kiinni ja t‰ysi
				LedOn;
				for(int i = 0; i<15; i++) {
					Buzz;
					_delay_us(500);
					}
         	 	kentta[x][y] = 3;
				++(*ammusmaara);
				LedOff;
   	    		break;
    	   	case 2 ://jos auki ja tyhj√É¬§ ei siis reagoi ampumalla tyhj√É¬§√É¬§ ruutua
  	    	case 3 ://jos auki ja t√É¬§ysi
        		break;
    	}
	}   
}

int tarkastavt(int (*kentta)[10], int ammukset) {	
/* Tarkistaa onko voitto tai tappio tapahtunut */

    int piilotetut = 0;
    
    for(int i=0; i<10; i++) {
        for(int j=0; j<10; j++) {
            if(kentta[i][j] == 1) {
                piilotetut = 1;
            }
        }
    
    }
    
    if(piilotetut) {
        if(ammukset < 1) {
            return 1; //tappio
        } else {
            return 0; //ei mit‰‰n
        }
    }
    
    return 2; //voitto
      
}

void piirtofunktio(int (*kentta)[10], int vasenyk, int x, int y, int ammusmaara) {	
/* Ei muokkaa mit√§√§n, piirt√§√§ pelikent√§n */
    lcd_write_ctrl(LCD_CLEAR); //piirret√É¬§√É¬§n varmmaan n.10x sekunnissa TAI ainakun k√É¬§ytt√É¬§j√É¬§ painaa nappia
    char avaamatonruutu = '^';
	char tyhjaruutu = '.';
	char taysiruutu = '*';

    lcd_gotoxy(0,0);
    
    for(int i = 0; i<10; i++) {
  
        switch(kentta[i][vasenyk]) {
            case 1:
            case 0:
                lcd_write_data(avaamatonruutu);
            break;
            case 2:
                lcd_write_data(tyhjaruutu);
            break;
            case 3:
                lcd_write_data(taysiruutu);
            break;
        
        }        
    }
    //yksinkertaisuuden takia en k√É¬§yt√É¬§ sis√É¬§kk√É¬§ist√É¬§ silmukkaa kahdella kierroksella
    lcd_write_data(' ');
    lcd_write_data(' ');
    lcd_write_data('A');
    lcd_write_data('=');
 	 
	//http://stackoverflow.com/questions/1114741/how-to-convert-int-to-char-c
	if(ammusmaara > 9)
		lcd_write_data((char)(((int)'0')+(ammusmaara / 10)));
	
	else 
        lcd_write_data(' ');

	lcd_write_data((char)(((int)'0')+(ammusmaara % 10)));

    //kaikki 16 k√É¬§ytetty
    //alemman rivin piirto

	lcd_gotoxy(0,1);
    for(int i = 0; i<10; i++) {
  
        switch(kentta[i][vasenyk+1]) {
            case 1:
            case 0:
                lcd_write_data(avaamatonruutu);
            break;
            case 2:
                lcd_write_data(tyhjaruutu);
            break;
            case 3:
                lcd_write_data(taysiruutu);
            break;
        
        }
		
        
        
    }
	
	lcd_write_data(' ');
	lcd_write_data(' ');
    lcd_write_data('L');
    lcd_write_data('=');

	if(y > 8) { 			//Rivin korkeus, pelaajalle n√§ytet√§√§n rivi + 1
        lcd_write_data('1');
        y = y - 10;
    }
	else {
       lcd_write_data(' ');
	}

	lcd_write_data((char)(((int)'0')+(y+1)));
}

void liikutakursoria(int *x, int *y, char suunta, int *vasenyk){	
/* Muuttaa n√§pp√§imen x:n y:n sek√§ yl√§kulman muutokseksi. */
	_delay_ms(250);			// Nappi pohjassa liikutaan noin 3 kertaa sekunnissa.
    switch(suunta) {
        case 'y':
            if(*y > 0) {
                --(*y);
            }
        break;
        case 'a':
            if(*y < 9) {
                ++(*y);
            }
        break;
        case 'v':
            if(*x > 0) {
                --(*x);
            }
        break;
        case 'o':
            if(*x < 9) {
                ++(*x);
            }
        break;
    
    }
    if(suunta == 'a' && abs(*vasenyk - *y) >1) 
		++(*vasenyk);

    if(suunta == 'y' && (*vasenyk - *y) >0)
		--(*vasenyk);	
}

void lcd_gotoxy (int x, int y) {
    
       if ( y==0 )
          lcd_write_ctrl(LCD_DDRAM | (0x00+x));
       else
          lcd_write_ctrl(LCD_DDRAM | (0x40+x));
	
}

int annaseed() {
	lcd_write_ctrl(LCD_CLEAR);
	lcd_write_str("***Battleship***", 0);
	lcd_write_str("****************",1);
	int nappiylos = 0;
	int nappipohjassa = 0;
	int seed = 0;

	while(!(nappipohjassa && nappiylos)) {
	
		if(ButtonCenter){		
			nappipohjassa = 1;
			seed = seed +6;
		} 
	
		else {
			if(nappipohjassa == 1){
				nappiylos = 1;
				seed = seed + 4;
			}

		}
		seed++;
		if (seed > 542)
			seed = 0;

	}
	lcd_write_ctrl(LCD_CLEAR);
	_delay_ms(250); // Aikaa reagoida pelin alkamiseen
	return seed;
}


void lcd_write_str(char *sana, int rivi) {
	/*Funktio ottaa rivin jolle kirjoitus aloitetaan, rivinvaihto automaattisesti 16 merkin j√§lkeen*/
	int i = 0;	//jotta olisi aina mÔøΩÔøΩritelty toisellekin.loopille
	if (rivi == 0) {
		lcd_gotoxy(0,0);
		for(;(i<strlen(sana)) && (i<16);i++) {
			lcd_write_data(sana[i]);
		}
	}
	
		lcd_gotoxy(0,1);
		for(;i<strlen(sana);i++){
			lcd_write_data(sana[i]);
		}
}




/* L√§hteet koottuna 
delay ja variable:
	http://www.avrfreaks.net/index.php?name=PNphpBB2&file=printview&t=74093&start=0
int to char:
	//http://stackoverflow.com/questions/1114741/how-to-convert-int-to-char-c
nyan notes
	// notes from http://fc05.deviantart.net/fs71/i/2011/274/d/2/nyan_cat_clarinet_music_by_bobtheclarinet-d3f0x42.jpg
freq note chart
	http://donrathjr.com/wp-content/uploads/2010/07/Table-of-Frequencies1.png
*/
