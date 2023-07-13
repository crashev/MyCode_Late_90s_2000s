/*
	ScreenPanel.java - Klasa ScreenPanel, jedna z wa¿niejszych klas, jest to stworzony i
			   oprogramowany element do wy¶wietlania tego wszystkiego co siê dzieje
			   na czacie, wiêcej o tym w dokumentacji

	Czê¶æ projektu CZAT APPLET
	Wykona³: Pawe³ Furtak, Daniel Sorbian, Marcin Sadowski

*/

import java.awt.*;

class ekran
{
	String linia;
	int style;
	Color kolor;
	public void getstyle(char znak)
	{
		if (znak=='*') style=Font.PLAIN;
		if (znak=='&') style=Font.BOLD;
		if (znak=='$') style=Font.ITALIC;
		if (znak=='#') style=Font.PLAIN+Font.ITALIC;
	}
	public void getkolor(char znak)
	{
		
		switch (znak)
		{
			case  '0': kolor = Konfiguracja.colGRANATOWY;break;
			case  '1': kolor = Konfiguracja.colCZERWONY;break;
			case  '2': kolor = Konfiguracja.colZIELONY;break;
			case  '3': kolor = Konfiguracja.colCZARNY;break;
			case  '4': kolor = Konfiguracja.colROZOWY;break;
			case  '5': kolor = Konfiguracja.colZOLTY;break;
			case  '6': kolor = Konfiguracja.colBRAZOWY;break;
			case  '7': kolor = Konfiguracja.colWRZOSOWY;break;
			default  : kolor = Konfiguracja.colGRANATOWY;break;
		

		
		}
	}
}


class TextScreen extends Canvas
{
	Graphics gw;
	public Image bufscr=null;
	int linNum,tmp;
	private int MAXLINES=38;
	private int BUFOR=10;
	private Dimension m;
	Font GlownyFont;
	String lines[] = new String[MAXLINES+BUFOR+1];
	Dimension r=null;
//	Chat par;
	
//	public TextScreen(Chat d)
	public TextScreen()
	{
		//super();
//		this.par=d;
		GlownyFont = new Font("Verdana",Font.PLAIN,10);
		linNum = 0;
	}
	public void changeFont(Font f)
	{
		GlownyFont = f;
		repaint();

	}
	public void addLine(String s)
	{
	if (linNum==MAXLINES+BUFOR)
	{
	    for (int x = 0;x < MAXLINES+BUFOR; x++)
	    	lines[x] = lines[x+1];
		linNum--;
		lines[linNum++] = s;
	} else
	        lines[linNum++] = s;
	 
		repaint();
	}
	
	public void paint(Graphics g)
	{
		//Dimension tmpsiz = getSize();
		//System.out.println("[paint] Wymiary-> x: "+tmpsiz.width+" y: "+tmpsiz.height);
            	//if (r==null) 
		r = getSize();
	     	g.setColor(new Color(240,240,255)); // kolor tla
	     	g.fillRect(0,0,r.width,r.height);   // wypelniamy nasze tlo tekstowe
	     	g.setColor(Color.black);            // robimy ramki koloru czarnego
	     	g.drawRect(0,0,r.width,r.height);
	     	g.drawRect(1,1,r.width-1,r.height-1);
		update(g);
		//paint(g);
	}
	public void update(Graphics g)
	{
	     FontMetrics fm;
	     int countme,len;
	     int zz,line=0;
            	//if (r==null) 
	     //r = getSize();
	     Dimension tmpsiz = getSize();
	     if (r != tmpsiz) r=tmpsiz;
	     
	     int troc;
	     
	     if (bufscr == null) 
	     		bufscr = createImage(r.width,r.height);
			else {
				bufscr = null;
				bufscr = createImage(r.width,r.height);
			}
			if (gw!=null) gw.dispose();
			
			
		
	     
	     	gw = bufscr.getGraphics();           
		
	     	gw.setColor(new Color(240,240,255)); // kolor tla
	     	gw.fillRect(0,0,r.width,r.height);   // wypelniamy nasze tlo tekstowe
	     	gw.setColor(Color.black);            // robimy ramki koloru czarnego
	     	gw.drawRect(0,0,r.width,r.height);
		gw.setColor(Color.lightGray);
	     	gw.drawRect(1,1,r.width-1,r.height-1);
	     	

	     	gw.setColor(Color.blue); // kolor textu
             
	     
	     gw.setFont(GlownyFont);
	     fm = gw.getFontMetrics();

	     /*
	     	obliczanie ile lini tekstu w rozmiaerze danego fontu zmiesci sie w naszym Panelu
	     */

	     int srednia=0;
	     for (int x = linNum; x > 0; x--)
	     {
	     	
		int stylowy=getstyle(lines[x-1].charAt(0));
	     	Font tenteraz = new Font(GlownyFont.getName(),stylowy,GlownyFont.getSize());
		gw.setFont(tenteraz);
		fm = gw.getFontMetrics();
		srednia += fm.getAscent();
	     }
if (linNum>0)
{
	     srednia=srednia/linNum;
	     troc = (r.height/srednia-1);  // Obliczamy ilosc lini ktore na ekranie
} else troc=0;
	     ekran Ekran[] = new ekran[troc+5];

	     //System.out.println("gw->stringHeight() -> "+fm.getHeight());
	     //System.out.println("r.height           -> "+r.height);


	     line =0;
	     int pad=2;
	     int tmppad=pad;

	     for (int x = linNum; x > 0; x--)
	      {
	    	      if (Ekran[troc-line]==null) Ekran[troc-line]=new ekran();
	    	      Ekran[troc-line].getstyle(lines[x-1].charAt(0));
		      Ekran[troc-line].getkolor(lines[x-1].charAt(1));
		      Font tenteraz = new Font(GlownyFont.getName(),Ekran[troc-line].style,GlownyFont.getSize());
		      gw.setFont(tenteraz);
		      fm = gw.getFontMetrics();
	    if (fm.stringWidth(lines[x-1])>r.width)
	      {
		        countme = 0;
			int u =0;
			int p=0;
			for (zz = pad; zz<(lines[x-1].length()) ;zz++)
			{
				u+=fm.charWidth(lines[x-1].charAt(zz));

				if (u >= r.width-20)
				{
			//		System.out.println("-> "+lines[x].substring(countme,zz));
					if (Ekran[troc-line]==null) Ekran[troc-line]=new ekran();
					Ekran[troc-line].linia=lines[x-1].substring(countme+tmppad,zz);
					Ekran[troc-line].getstyle(lines[x-1].charAt(0));
					Ekran[troc-line].getkolor(lines[x-1].charAt(1));
					line++;
		//			gw.drawString(lines[x].substring(countme,zz),5,(x-tmp+p)*10+15);
					tmppad=0;
					countme = zz;
					u=0;
					p++;
				}
			}
			                tmppad=pad;
					if (Ekran[troc-line]==null) Ekran[troc-line]=new ekran();
					Ekran[troc-line].linia=lines[x-1].substring(countme,zz);p++;
					Ekran[troc-line].getstyle(lines[x-1].charAt(0));
					Ekran[troc-line].getkolor(lines[x-1].charAt(1));
					String tmp[] = new String[p];
					for (int tt=0;tt<p; tt++)
						tmp[p-tt-1]=Ekran[(troc-line)+tt].linia;

					for (int tt=0;tt<p; tt++) Ekran[(troc-line)+tt].linia=tmp[tt];

					line++;



	      } else {
	       	Ekran[troc-line].linia=lines[x-1].substring(pad,lines[x-1].length());

	      	line++ ;
	      }
	      if (line == troc+1) break;


	      }


	      //gw.drawString("Tekst"+x,5,((x+1)*fm.getAscent())+2);
	      //gw.drawString("Tekst1",5,2*fm.getAscent());

	      for (int x=0; x<troc+1; x++)
	      {
		      if (Ekran[x] != null)
		      {
		        Font tenteraz = new Font(GlownyFont.getName(),Ekran[x].style,GlownyFont.getSize());
				gw.setFont(tenteraz);

				gw.setColor(Ekran[x].kolor);
				gw.drawString(Ekran[x].linia,5,((x+1)*srednia-1));
		      }
	       }

	        g.drawImage(bufscr,0,0,this);
	}

	public int getstyle(char znak)
	{
		if (znak=='*') return (Font.PLAIN);
		if (znak=='&') return (Font.BOLD);
		if (znak=='$') return (Font.ITALIC);
		if (znak=='#') return (Font.BOLD+Font.ITALIC);
		return 0;
	}

/*
    public final Dimension getPreferredSize()
    {
        return getMinimumSize();
    }

    public final Dimension getMinimumSize()
    {
        return r;
    }
*/

	public void resized(Dimension newdim)
	{
		r=null;
		bufscr=null;
		r=newdim;
		bufscr = createImage(r.width,r.height);
		//resize(r.width,r.height);
	}
	public int getLineCount()
	{
		return linNum;
	}
	public String getLineAt(int at)
	{
		return lines[at];
	}

}

public class ScreenPanel extends Panel {

    TextScreen tx;
//    Chat dupa;
    //TextArea tx = new TextArea();

//    public ScreenPanel(Chat dupa)
    public ScreenPanel()
    {
//    	this.dupa=dupa;


//	tx = new TextScreen(dupa);
        tx = new TextScreen();
	setLayout(new BorderLayout());

	add("Center",tx);
	//add("East",sb);
    }
    public void addLine(String s)
    {
       tx.addLine(s);
    }

}
