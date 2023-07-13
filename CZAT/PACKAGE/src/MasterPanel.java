/*
	MasterPanel.java - Klasa MasterPanel - panel kontroli wygl±du czcionki, koloru, stylu...
	
	Cze¶æ projektu CZAT APPLET
	Wykona³: Pawe³ Furtak, Daniel Sorbian, Marcin Sadowski


*/

import java.awt.*;
import java.awt.event.*;


public class MasterPanel extends Panel implements ActionListener, ItemListener
{

	ScreenPanel parentPanel;

	Panel propPanel;
	Button pb1,pb2,pb3;
	Choice wyborFontow,wyborRozmiaruFontu,wyborKOLOROW;
	Panel tmp103;
	
	private boolean bold=false,italic=false;
	private int KOLOR;

	MasterPanel(ScreenPanel scr)
	{
		
		parentPanel = scr;
		
		
		propPanel = new Panel();
		propPanel.setBackground(Konfiguracja.colTLAPROPPANELU);
	
		pb1 = new Button("B");pb1.setFont(new Font("Arial",Font.BOLD,0));pb1.addActionListener(this);
		pb2 = new Button("I");pb2.setFont(new Font("Arial",Font.ITALIC,0));pb2.addActionListener(this);
		pb3 = new Button("N");pb3.setFont(new Font("Arial",Font.PLAIN,0));pb3.addActionListener(this);
	
		wyborFontow = new Choice();
			wyborFontow.addItem("Verdana");
			wyborFontow.addItem("Arial");
			wyborFontow.addItem("Helvetica");
			wyborFontow.addItem("TimesRoman");
			wyborFontow.addItem("Courier");
                        wyborFontow.addItemListener(this);
		wyborRozmiaruFontu = new Choice();
			wyborRozmiaruFontu.addItem("10");
			wyborRozmiaruFontu.addItem("11");
			wyborRozmiaruFontu.addItem("13");
			wyborRozmiaruFontu.addItem("14");
			wyborRozmiaruFontu.addItem("16");
			wyborRozmiaruFontu.addItem("18");
			wyborRozmiaruFontu.addItemListener(this);
		wyborKOLOROW = new Choice();
			wyborKOLOROW.addItem("Granatowy");
			wyborKOLOROW.addItem("Czerwony");
			wyborKOLOROW.addItem("Zielony");
			wyborKOLOROW.addItem("Czarny");
			wyborKOLOROW.addItem("Rozowy");
			wyborKOLOROW.addItem("Zolty");
			wyborKOLOROW.addItem("Brazowy");
			wyborKOLOROW.addItem("Wrzosowy");
			
			
			
			wyborKOLOROW.addItemListener(this);
		/* ustawiamy kolorki przyciskow manipulacji czcionka */
		
			pb1.setBackground(Konfiguracja.colNIEAKTYWNY);pb1.setForeground(Konfiguracja.colTXTPROPPANELU);
			pb2.setBackground(Konfiguracja.colNIEAKTYWNY);pb2.setForeground(Konfiguracja.colTXTPROPPANELU);
			pb3.setBackground(Konfiguracja.colNIEAKTYWNY);pb3.setForeground(Konfiguracja.colTXTPROPPANELU);
		
			wyborFontow.setBackground(Konfiguracja.colTLAPRZYCISKOW_FONTOW);
			wyborFontow.setForeground(Konfiguracja.colTXTPRZYCISKOW_FONTOW);
			wyborRozmiaruFontu.setBackground(Konfiguracja.colTLAPRZYCISKOW_FONTOW);
			wyborRozmiaruFontu.setForeground(Konfiguracja.colTXTPRZYCISKOW_FONTOW);
		
			wyborKOLOROW.setBackground(Konfiguracja.colNIEAKTYWNY);
			//wyborKOLOROW.setForeground(Konfiguracja.colTXTPROPPANELU);
			wyborKOLOROW.setForeground(Konfiguracja.colGRANATOWY);

			setLayout(new BorderLayout());
			setBackground(Konfiguracja.colTLAPROPPANELU);

			tmp103 = new Panel();
			tmp103.setBackground(Konfiguracja.colTLAPROPPANELU);

			propPanel.add(wyborFontow);
			propPanel.add(wyborRozmiaruFontu);

			tmp103.add(wyborKOLOROW);
			tmp103.add(pb1);
			tmp103.add(pb2);
			tmp103.add(pb3);

			add("West",propPanel);
			add("East",tmp103);

	}

	public void actionPerformed(ActionEvent e) {

	if (e.getSource().equals(pb1))
			{
				if (!bold) {
						bold=true;
					 	pb1.setBackground(Konfiguracja.colAKTYWNY);
					 }  else {
					  	bold=false;
					  	pb1.setBackground(Konfiguracja.colNIEAKTYWNY);

					  }
			}
		if (e.getSource().equals(pb2))
			{
				if (!italic) {
						italic=true;
					 	pb2.setBackground(Konfiguracja.colAKTYWNY);
					 }  else {
					  	italic=false;
					  	pb2.setBackground(Konfiguracja.colNIEAKTYWNY);
					  }
			}

		if (e.getSource().equals(pb3))
		{
			italic=false;bold=false;
			pb1.setBackground(Konfiguracja.colNIEAKTYWNY);
			pb2.setBackground(Konfiguracja.colNIEAKTYWNY);
		}
			

	
	
	}
	
	
	
	
	
	
	public void itemStateChanged(ItemEvent item) 
	{
	
		if (item.getSource().equals(wyborFontow))
		{
			Font nowy;
			String nowyFont;
			int rozmiar;
			nowyFont = (String)item.getItem();
			
				rozmiar = Integer.parseInt(wyborRozmiaruFontu.getSelectedItem());
				nowy = new Font(nowyFont,Font.PLAIN,rozmiar);
				
				parentPanel.tx.changeFont(nowy);
		}
		
		
		if (item.getSource().equals(wyborRozmiaruFontu))
		{
				Font nowy;
				int nowyRozmiar;
				String nowyFont;
				
				nowyRozmiar = Integer.parseInt((String)item.getItem());
				nowyFont = (String)wyborFontow.getSelectedItem();
				nowy = new Font(nowyFont,Font.PLAIN,nowyRozmiar);
				
				parentPanel.tx.changeFont(nowy);
		}
		
		if (item.getSource().equals(wyborKOLOROW))
		{
			try {
				String kolor = (String)item.getItem();
				
				if (kolor.equalsIgnoreCase("Granatowy")) KOLOR=0;
				if (kolor.equalsIgnoreCase("Czerwony"))  KOLOR=1;
				if (kolor.equalsIgnoreCase("Zielony"))   KOLOR=2;
				if (kolor.equalsIgnoreCase("Czarny"))    KOLOR=3;
				if (kolor.equalsIgnoreCase("Rozowy"))    KOLOR=4;
				if (kolor.equalsIgnoreCase("Zolty"))    KOLOR=5;
				if (kolor.equalsIgnoreCase("Brazowy"))    KOLOR=6;
				if (kolor.equalsIgnoreCase("Wrzosowy"))    KOLOR=7;
				wyborKOLOROW.setForeground(getkolor(KOLOR));
				/*
				
						
		        final Color colGRANATOWY 	    = new Color(0,0,128);
			final Color colCZERWONY		    = new Color(0,0,128);
			final Color colZIELONY              = new Color(0,192,0);
			final Color colCZARNY               = new Color(0,0,0);
			final Color colROZOWY               = new Color(217,0,220);
			final Color colZOLTY                = new Color(206,208,0);
			final Color colBRAZOWY              = new Color(128,0,0);
			final Color colWRZOSOWY             = new Color(118,0,128);
				*/
			
			} catch (Exception ex) {}
		
		}
		
	}
	
	public boolean isBold() { 
		return bold; 
	}
	
	public boolean isItalic() {
		return italic;
	}

	public int getColor() {
		return KOLOR;
	}
	public char getStyle ()
	{
		if ( (!isBold()) && (!isItalic()) ) return '*';
		if ( (isBold() ) && (!isItalic()) ) return '&';
		if ( (!isBold()) && (isItalic())  ) return '$';
		if ( (isBold() ) && (isItalic())  ) return '#';
		
		return '*';
	}
	
	public Color getkolor(int znak)
	{
		
		switch (znak)
		{
			case  0: return Konfiguracja.colGRANATOWY;
			case  1: return Konfiguracja.colCZERWONY;
			case  2: return Konfiguracja.colZIELONY;
			case  3: return Konfiguracja.colCZARNY;
			case  4: return Konfiguracja.colROZOWY;
			case  5: return Konfiguracja.colZOLTY;
			case  6: return Konfiguracja.colBRAZOWY;
			case  7: return Konfiguracja.colWRZOSOWY;
			default  : return Konfiguracja.colWRZOSOWY;


		
		}
	}
	
}
