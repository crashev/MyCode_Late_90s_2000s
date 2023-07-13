/*
	PrivateCzat.java - Klasa PrivateCzat - czyli okienko do rozmów prywatnych pomiêdzy
			   u¿ytkownikami czatu
	Zale¿no¶ci - MasterPanel, ScreenPanel

	Czê¶æ projektu CZAT APPLET
	Wykona³: Pawe³ Furtak, Daniel Sorbian, Marcin Sadowski

*/
import java.awt.*;
import java.awt.event.*;

public class PrivateCzat extends Frame implements ActionListener
{
	String querydo;
	String queryod;
	Chat par;
	TextField enter;
	ScreenPanel okno;
	String fd;
	MasterPanel master;

	PrivateCzat(Chat p,String odkogo,String dokogoto,String fd, String fdmyself)
	{
		this.querydo=dokogoto;
		this.par=p;
		this.fd=fd;
		this.queryod=odkogo;
		//setResizable(false);
		setTitle("Rozmowa prywatna DO "+dokogoto);
		Toolkit tool = Toolkit.getDefaultToolkit();
		Dimension ssize = tool.getScreenSize();
		setSize((ssize.width*2)/3, (ssize.height*2)/3);


//		okno = new ScreenPanel(par);
 	        okno = new ScreenPanel();
		okno.tx.changeFont(new Font("Arial",Font.PLAIN,12));

		setLayout(new BorderLayout());

		enter = new TextField();enter.addActionListener(this);
		Panel tss = new Panel(new BorderLayout());
		tss.add("Center",okno);
		tss.add("South",enter);

		enter.setBackground(Konfiguracja.colTLOENTER);
		enter.setForeground(Konfiguracja.colTXTENTER);

		master = new MasterPanel(okno);

		add("Center",tss);
		add("South",master);



		show();

		/* zamykanie okna */
		addWindowListener(new WindowAdapter(){
	        public void windowClosing(WindowEvent windowevent)
		{
		  if (par.userSTAT.get(querydo).equals("privmsg")) par.userSTAT.remove(querydo);
		  par.ileotwartych--;
	         dispose();
		 return;
		}
	});
	}
	public void actionPerformed(ActionEvent e)
		{
			if (e.getSource().equals(enter))
			{
				String wyslij = enter.getText();
				enter.setText("");
				if (wyslij.length()>0)
				{
					//par.out.println("+PRIVMSG "+fd+" :=PRIVMSG:"+queryod+":*12<"+queryod+"> "+wyslij+"\n");
					par.out.println("+PRIVMSG "+fd+" :"+master.getStyle()+master.getColor()+" "+wyslij+"\n");
					par.out.flush();
					add(""+master.getStyle()+master.getColor()+"<"+queryod+"> "+wyslij);
				}
			}
		}
	public void sendPriv(String a,String b)
	{
		if ( (a.equals(querydo)) || (a.equals(queryod)) )
		{
			okno.addLine(b);
		}
	}
	public void add(String co)
	{
		okno.addLine(co);
	}
	public void shutPriv(String s)
	{
		if (s.equals(querydo))
		{
			enter.setEnabled(false);
			enter.removeActionListener(this);
			add("*+++ User "+s+" opuscil czata....");
		}
	}
	public void shutPriv()
	{
		
		
			enter.setEnabled(false);
			enter.removeActionListener(this);
			add("*Uzytkownik opuscil czata");
		
	}
}
