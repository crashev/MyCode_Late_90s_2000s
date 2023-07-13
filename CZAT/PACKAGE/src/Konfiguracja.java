/*
	Plik konfiguracyjny - ustawienia kolorków czatu/wygl±du

	Wykona³: Pawe³ Furtak, Daniel Sorbian, Marcin Sadowski
*/
import java.awt.*;

interface Konfiguracja
{

/* Sekcja kolorystyki */
		/* Sekcja 1 = propPANEL - przyciski od zmiany kroju/czcionki/stylu */
			final Color colAKTYWNY              = new Color(124,170,255);
			final Color colNIEAKTYWNY           = new Color(124,205,255);
			final Color colTLAPROPPANELU 	      = new Color(73,159,183);//73,159,183
			final Color colTXTPROPPANELU        = new Color(0,0,255);
			
			final Color colTLAPRZYCISKOW_FONTOW = new Color(124,205,255);
			final Color colTXTPRZYCISKOW_FONTOW = new Color(0,0,255);

			/* Pole tekstowe do wysylania tekstu i przycisk Wyslij */
			final Color colTLOENTER             = new Color(22,220,255);
			final Color colTXTENTER             = new Color(0,0,0);
			
			/* Przyciski obslugi userow - ignoruj/prywatnie itd. */
			final Color colTLOOPCJIUSEROW       = new Color(73,159,183);
			final Color colTXTOPCJIUSEROW	      = new Color(0,0,0);
			
			//new Color(22,220,255);		
			final Color colTLOOGOLNE            = new Color(22,220,255);		
				
			/* naglowki - users on line i topic */
			final Color colTLONAGLOWEK          = new Color(0,0,255);
			final Color colTXTNAGLOWEK          = new Color(255,255,255);
			
			final Color colGRANATOWY 	    = new Color(0,0,128);
			final Color colCZERWONY		    = new Color(232,0,0);
			final Color colZIELONY              = new Color(0,192,0);
			final Color colCZARNY               = new Color(0,0,0);
			final Color colROZOWY               = new Color(217,0,220);
			final Color colZOLTY                = new Color(206,208,0);
			final Color colBRAZOWY              = new Color(128,0,0);
			final Color colWRZOSOWY             = new Color(118,0,128);
			
		/* Seckja 2 - resources */
		// \u0142
			final String LANGusertekst = "Uzytkownicy";    
}
