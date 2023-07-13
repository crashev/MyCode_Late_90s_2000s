/*
 	CZAT APPLET 1.0
	Wykona³: Pawe³ Furtak, Daniel Sorbian,Marcin Sadowski

	Plik g³ówny - inicjalizuje applet, obs³uguje wszystko co z nim zwi±zane(zdarzenia,w±tki)
	

*/

import java.applet.*;
import java.awt.*;

import java.net.*;
import java.io.*;

import java.net.MalformedURLException;

import java.awt.event.*;
import java.util.Hashtable;








class Reklama extends Canvas
{
	Image glowny,bufscr;
	Dimension size;
	Graphics gw;

	Reklama(Image I1)
	{
		glowny = I1;
		//size = new Dimension(64,88);
		size = new Dimension(669,88);
		setSize(669,88);
		
	}
	public final Dimension getPreferredSize()
	{
		return getMinimumSize();
	}
	public final Dimension getMinimumSize()
	{
		return size;
	}
	public void paint(Graphics g)
	{
		update(g);
	}
	
	public void update(Graphics g)
	{
	   Dimension isize = getSize();
	   if (glowny!=null)
	   {
	   	
		if (bufscr==null) bufscr=createImage(size.width,size.height);

			gw  = bufscr.getGraphics();

	   			gw.setColor(Color.white);
				gw.fillRect(0,0,size.width,size.height);

		gw.drawImage(glowny,0,0,size.width,size.height,this);
		g.drawImage(bufscr,(isize.width/2)-size.width/2,0,size.width,size.height,this);

	   }
	}
}








class Powieksz extends Frame
{
	Applet ten;
	Powieksz(Applet p,ScreenPanel txt)
	{
	Toolkit tool = Toolkit.getDefaultToolkit();
	Dimension ssize = tool.getScreenSize();
	setSize((ssize.width), (ssize.height));
	p.resize((ssize.width), (ssize.height));
        //Dimension dim2 = new Dimension(((ssize.width*1)/2),((ssize.height*2)/3));
	//txt.tx.resized(dim2);
	//setResizable(false);
	setLayout(new BorderLayout());
	add("Center",p);
	setTitle("WWW Czat");
	//showStatus("check this out in full window");
	show();

	addWindowListener(new WindowAdapter(){
	        public void windowClosing(WindowEvent windowevent)
		{
	         dispose();
		 return;
		}
	});

	}
	public void zakoncz()
	{
	        dispose();
		return;
	}
}


class Autorzy extends Frame {

	Autorzy() {
		Panel p1 = new Panel(new GridLayout(3,1));
		Panel p2 = new Panel();
		Label l1 = new Label("Autorzy");

		l1.setBackground(new Color(73,159,183));
		l1.setForeground(new Color(255,255,255));
		p2.add("Center",l1);
		p1.add(new Label("+ Pawel Furtak"));
		p1.add(new Label("+ Daniel Sorbian"));
		p1.add(new Label("+ Marcin Sadowski"));
		add("North",p2);
		add("Center",p1);
		setTitle("Autorzy...");
		setSize(300,200);
		setResizable(false);
		//setPosition(100,100);

	addWindowListener(new WindowAdapter(){
	        public void windowClosing(WindowEvent windowevent)
		{
	         dispose();
		 return;
		}
	});

		// Poka¿ okno Centralnie
		Dimension size = getSize();
	    	Dimension screen = getToolkit().getScreenSize();

		/* Ma³e zabezpieczenie przed dziwnymi zdarzeniami */
		if (size.width  > screen.width)
      			size.width  = screen.width;
    		if (size.height > screen.height)
      			size.height = screen.height;

		setLocation(screen.width/2-size.width/2,screen.height/2-size.height/2);
		show();
	}
	
}








class Historia extends Frame
{
	TextArea text;

	Historia(ScreenPanel t)
	{
		Toolkit tool = Toolkit.getDefaultToolkit();
		Dimension ssize = tool.getScreenSize();
		setSize((ssize.width*5)/6, (ssize.height*5)/6);
		setLayout(new BorderLayout());

		text = new TextArea();
		add("Center",text);
		text.setBackground(new Color(244,244,255));
		text.setForeground(new Color(0,0,255));
		text.setEditable(false);
		//text.append("asdasd");
		int num = t.tx.getLineCount();
		for (int z = 0; z<num; z++)
		{
			String wklejana = t.tx.getLineAt(z);
			text.append(wklejana.substring(2,wklejana.length())+'\n');
		}
		/* handle window closing */
		addWindowListener(new WindowAdapter(){
	        	public void windowClosing(WindowEvent windowevent)
			{
	         		dispose();
		 		return;
			}
		});
		show();
	}
}


















public class Chat extends Applet implements ActionListener, ItemListener, Runnable
{
	Thread self = null;
	boolean run=false;

	BufferedReader in;
	PrintWriter out;
	Socket sock;

	/* Elementy czatu - wszystkie! */
	ScreenPanel txt;

	TextField enter;
	Button wyslij;

	List lista;                             // Lista userow
	Panel pan,blue,panelOPCJI_UZYTKOWNIKOW; // Panel z lista uzytkownikow i guziczkami do nich
	Button b1,b2,b3,b4,b5,zamknij;          // Przyciski na panelu do userow

	String nickname;
	String kanal;
	int port;
	Powieksz powieksz;
				
	Panel test20;
	Label lab1,lab2;
	int usersONLINE=0;


	int KOLOR=0;


	MasterPanel master;

	/* Private Czat stuff */
	public Hashtable fdUSEROW  = new Hashtable();  // To jest hashtable zawierajacy numerki deskryptorow
	public Hashtable userSTAT  = new Hashtable();  // a to zawiera informacje o ignore/privmsg
	public Hashtable idOkienka = new Hashtable();
	PrivateCzat priv[] = new PrivateCzat[50];
	int ileotwartych=0;




	/* Sekcja kolorystyki */

			/* Pole tekstowe do wysylania tekstu i przycisk Wyslij */
			Color colTLOENTER             = new Color(22,220,255);
			Color colTXTENTER             = new Color(0,0,0);

			/* Przyciski obslugi userow - ignoruj/prywatnie itd. */
			Color colTLOOPCJIUSEROW       = new Color(73,159,183);
			Color colTXTOPCJIUSEROW	      = new Color(0,0,0);

			//new Color(22,220,255);
			Color colTLOOGOLNE            = new Color(22,220,255);

			/* naglowki - users on line i topic */
			Color colTLONAGLOWEK          = new Color(0,0,255);
			Color colTXTNAGLOWEK          = new Color(255,255,255);

		/* Seckja 2 - resources */
		// \u0142
			String LANGusertekst = "Uzytkownicy";













	public void actionPerformed(ActionEvent e) {
		if ( (e.getSource().equals(enter)) || (e.getSource().equals(wyslij))){
				String wyslij = enter.getText();
				enter.setText("");
				KOLOR = master.getColor();
				if (wyslij.length()>0)
				{
				if ((!master.isBold() )&&(!master.isItalic() ))
					out.println("+SEND:NORMAL:"+KOLOR+"="+wyslij+"\n");
				else if ((master.isBold() )&&(!master.isItalic() ))
					out.println("+SEND:BOLDDD:"+KOLOR+"="+wyslij+"\n");
				else if (( master.isItalic() )&&( !master.isBold() ))
					out.println("+SEND:ITALIC:"+KOLOR+"="+wyslij+"\n");
				else if ((master.isBold() )&&( master.isItalic() ))
					out.println("+SEND:BOLITA:"+KOLOR+"="+wyslij+"\n");
					out.flush();
				}
			}

		if ( e.getSource().equals(b4)){
			showStatus(":))");
			Autorzy autorzy = new Autorzy();
		}

		if ( e.getSource().equals(b5)){
				b5.setEnabled(false);
				powieksz = new Powieksz(this,txt);
			}
	
		
		if ( e.getSource().equals(zamknij)){
				wyjdz_z_czata();
		}
		
		/* Private Chat */
		if (e.getSource().equals(b1))
		{
			String dokogo = lista.getSelectedItem();
			if (dokogo==null) return;
			//System.out.println("[PRIV]-> "+dokogo+" i jego FD: "+fdUSEROW.get(dokogo));


			if (dokogo.equals(nickname)) return;
			if ( (userSTAT.get(dokogo)==null)||((!userSTAT.get(dokogo).equals("privmsg"))) )
			{


					userSTAT.put(dokogo,"privmsg");
					if (ileotwartych+1==49) {
							 showStatus("Maksymalnie mozna miec 50 okienek z priv msg");
							 return;
						}
					priv[ileotwartych] = new PrivateCzat(this,nickname,dokogo,(String)fdUSEROW.get(dokogo),(String)fdUSEROW.get(nickname));
					idOkienka.put(dokogo,new Integer(ileotwartych));
					ileotwartych++;


			} else {}
		}

		if (e.getSource().equals(b2))
		{
			String kogo = lista.getSelectedItem();
			if (kogo == null) return;
			if (kogo.equals(nickname)) return;
			if ( (userSTAT.get(kogo)==null)||(userSTAT.get(kogo).equals("privmsg")) )
			{
				userSTAT.put(kogo,"ignore");
				notifyMSG("+++ User "+kogo+" zostal dodany do listy ignorowanych");
				b2.setLabel("Nie ignoruj");
			} else {
				userSTAT.remove(kogo);
				notifyMSG("+++ User "+kogo+" zostal usuniety z listy ignorowanych");
				b2.setLabel("Ignoruj");
			        }
		}

		if (e.getSource().equals(b3))
		{
		   Historia hist = new Historia(txt);
		}
}
	













	public	void itemStateChanged(ItemEvent item)
	{
		
		if (item.getSource().equals(lista))
		{

			//Powiazania -> userSTAT,b2
			try {
				String kto = lista.getSelectedItem();
				
				if (userSTAT.get(kto)==null) { b2.setLabel("Ignoruj");return;}
				if (userSTAT.get(kto).equals("ignore")) b2.setLabel("Nie ignoruj"); else b2.setLabel("Ignoruj");

			} catch (Exception ex) {}

		}
		

	}
	

        
	
	

	

	/* Inicjuemy wyglad naszego apleciku */
	
	
	
	public void init()
	{

        	showStatus("wersja - ciagle beta....");
        		GridBagLayout gridbag = new GridBagLayout();
        		GridBagConstraints c = new GridBagConstraints();

	
        	/* Set Gridbag Layout */
		Panel glowny = new Panel();
		setLayout(new BorderLayout());


		glowny.setLayout(gridbag);
	
//		Image img = getImage(getDocumentBase(),"ban/reklama.php");
		System.out.println("Wczytywanie reklamy: "+getCodeBase()+"ban/logomalcom.jpg");
		//Image img = getImage(getCodeBase(),"ban/reklama.gif");
		Image img = getImage(getCodeBase(),"ban/logo2.jpg");
				try {
		//Image img = getImage(new URL("http://malechowo.com/reklama/czat.php"));
		//Image img = getImage(new URL("http://malechowo.com/reklama/czat.php"));
		//Image img = getImage(new url("http://malechowo.com);
        	Reklama reklama = new Reklama(img);
		test20 = new Panel();
		test20.setLayout(new BorderLayout());
		test20.add("Center",reklama);
		test20.setBackground(Color.white);
	add("North",test20);
	} catch (Exception ex) { System.out.println("Error with banner: "+ex);}
	add("Center",glowny);


        setBackground(colTLOOGOLNE);
        	//natural height, maximum width
        //c.fill = GridBagConstraints.HORIZONTAL; 
	

		lista = new List(2,false);
		lista.addItemListener(this);
		lista.setForeground(new Color(255,0,0));
		lista.setBackground(new Color(244,244,255));
		
	/* Add Users List Component */
	
	c.ipady = 10;      //make this component tall
	int old = c.ipadx;
	c.ipadx=0;
        c.weightx = 0.0; // decyduje o powiekszeniu lub nie przy rozciaganiu okna
        c.weighty = 1.0;   //request any extra vertical space
        c.anchor = GridBagConstraints.CENTER; //bottom of space
	c.insets = new Insets(2,5,0,5);
        c.gridx = 0;
        c.gridy = 0;
	c.gridheight=1;
        c.fill = GridBagConstraints.VERTICAL;
	
	


	pan = new Panel(new GridLayout(0,1));
	blue = new Panel();
	panelOPCJI_UZYTKOWNIKOW = new Panel(new GridLayout(3,2));
	

	b1 	= new Button("Prywatnie");    b1.addActionListener(this);b1.setBackground(colTLOOPCJIUSEROW);b1.setForeground(colTXTOPCJIUSEROW);
	b2 	= new Button("Ignoruj")  ;    b2.addActionListener(this);b2.setBackground(colTLOOPCJIUSEROW);b2.setForeground(colTXTOPCJIUSEROW);
	b3      = new Button("Historia") ;    b3.addActionListener(this);b3.setBackground(colTLOOPCJIUSEROW);b3.setForeground(colTXTOPCJIUSEROW);
	b4      = new Button("Autorzy")  ;    b4.addActionListener(this);b4.setBackground(colTLOOPCJIUSEROW);b4.setForeground(colTXTOPCJIUSEROW);
	b5      = new Button("PowiekszOkno");b5.addActionListener(this);b5.setBackground(colTLOOPCJIUSEROW);b5.setForeground(colTXTOPCJIUSEROW);
	zamknij = new Button("Wyjdz");	      zamknij.addActionListener(this);
	
		panelOPCJI_UZYTKOWNIKOW.add(b1);
		panelOPCJI_UZYTKOWNIKOW.add(b2);
		panelOPCJI_UZYTKOWNIKOW.add(b3);
		panelOPCJI_UZYTKOWNIKOW.add(b4);
		panelOPCJI_UZYTKOWNIKOW.add(b5);
		panelOPCJI_UZYTKOWNIKOW.add(zamknij);
		panelOPCJI_UZYTKOWNIKOW.setBackground(new Color(73,159,183));
		
	pan.setBackground(new Color(0,0,255));
		
	
	pan.add(lista);
	pan.add(panelOPCJI_UZYTKOWNIKOW);
	blue.setBackground(new Color(73,159,183));
	//blue.setBackground(new Color(73,159,0));
	Panel pann = new Panel();
	pann.setLayout(new BorderLayout());
	
	lab1 = new Label(""+LANGusertekst+" [0]");
	pann.add("North",lab1);
	lab1.setBackground(colTLONAGLOWEK);
	lab1.setForeground(colTXTNAGLOWEK);

	pann.add("Center",lista);
	pann.add("South",panelOPCJI_UZYTKOWNIKOW);
	pann.setBackground(new Color(0,0,255));
	gridbag.setConstraints(pann, c);
        glowny.add(pann);
	
	c.fill = GridBagConstraints.HORIZONTAL; 
	
	

	/* Add Useres Text Window Component */

	c.ipady = 340;      //make this component tall
	c.ipadx=old;
	c.weighty=0.0;
	c.weightx = 1.0;
	c.insets = new Insets(2,0,0,2);
	c.gridx = 1;
        c.gridy = 0;
	c.gridwidth=1;
        c.gridheight=1;
//	txt = new ScreenPanel(this);
	txt = new ScreenPanel();
        Panel ptxt = new Panel(new BorderLayout());
	lab2 = new Label("Czat pawq - No Topic Set - Kanal: ");
	ptxt.add("North",lab2);
		lab2.setBackground(colTLONAGLOWEK);
		lab2.setForeground(colTXTNAGLOWEK);
	ptxt.add("Center",txt);
	
	enter = new TextField();
	enter.setBackground(colTLOENTER);
	enter.setForeground(colTXTENTER);

	enter.addActionListener(this);

	
	Panel south = new Panel(new BorderLayout());
	south.add("Center",enter);
	
	wyslij = new Button("Wyslij");
	wyslij.addActionListener(this);
	wyslij.setBackground(new Color(0,0,255));
	wyslij.setForeground(new Color(124,205,255));
	
	south.add("East",wyslij);
	ptxt.add("South",south);

	gridbag.setConstraints(ptxt, c);
	glowny.add(ptxt);

	
	

	
        /* Pole wprowadzania tekstu na czacika */
	c.ipadx=20;
	c.ipady=2;
	c.gridwidth=1;
	c.gridheight=1;
	
	c.weighty=0.0;
	c.weightx = 1.0;		
	c.gridx = 1;
	c.gridy = 1;
	c.insets = new Insets(10,0,10,2);
	c.anchor = GridBagConstraints.NORTH; //bottom of space
	

	
	/* Rodzzaje fontow */
	/* Toolbar z wyborem Italic/Bold/Underline/Normal */
	
	c.weightx = 0.0;
	c.weighty = 0.0;
	c.anchor = GridBagConstraints.WEST; //bottom of space
	c.insets = new Insets(0,0,0,0);
	c.ipady=0;
	c.ipadx=0;
	c.gridx=1;
	c.gridy=1;
	c.gridheight=1;
	c.gridwidth=1;

		master = new MasterPanel(txt);
	
	gridbag.setConstraints(master,c);
	glowny.add(master);
	
	
		Panel testuj = new Panel();
		//testuj.add(new Button("test"));
		testuj.setBackground(Konfiguracja.colTLAPROPPANELU);
		//testuj.add(new Button("Konfiguracja"));
		
		c.weightx = 0.0;
		c.weighty = 0.0;
		c.anchor = GridBagConstraints.WEST; //bottom of space
		c.insets = new Insets(5,5,5,0);
		c.ipady=6;
		c.ipadx=0;
		c.gridx=0;
		c.gridy=1;
		c.gridheight=1;
		c.gridwidth=1;
		gridbag.setConstraints(testuj,c);
		glowny.add(testuj);

	
	notifyMSG("Witamy na czacie !");

	
}

        
	 
	
	
	
	
	

	
	
	
	  
	
	/* Metoda wysylania prostych komunikatow do okna JtexPane */
	
	public void notifyMSG(String wysylany) 
	{
		txt.addLine("&1"+wysylany);

	}
	public void chatSEND(String wysylany) 
	{
		txt.addLine(wysylany);

	}
	

	
	
	
	
	
	
	
	

	
	
	public void start()
	{
	
                notifyMSG("+ Lacze sie z CHATEM....");
		
		try {
		/* ksywka */

		nickname = getParameter("KSYWKA");
		kanal = getParameter("KANAL");
		if (nickname == null)
		{
		  nickname = new String("~~NOBODY");
		}
		if (kanal == null) {
			kanal = new String("#malechowo");
		}

			port = Integer.parseInt(getParameter("PORT"));

			if (port==0) port = 1900;

		} catch (Exception ex) { System.out.println("[LACZE-Exception]-> "+ex); }

		try
		{
		sock = new Socket(getCodeBase().getHost(),port);
		in = new BufferedReader(new InputStreamReader(sock.getInputStream()));

		out = new PrintWriter(sock.getOutputStream(),true);
		} catch (IOException e)
		{
			System.out.println("Error "+e);
			notifyMSG("Niestety nie moge nawiazac polaczenia z chatem ,sprobuj pozniej!! ");
			zablokuj_wszystko();
			return;

		}
			/* protokol - ustawianie ksywki i kanalu */
			out.println(nickname+":"+kanal);
			out.flush();
		lab2.setText("Czat pawq - No Topic Set - Kanal: "+kanal);
		self = new Thread(Chat.this);
		run=true;
		self.start();
	}







	
	public void run()
	{
		String line;
		int zzz=0;
		
		
			try 
			{
			do {
			line = in.readLine();
			if (line == null) 
			{
				System.out.println("Polaczenie przerwane przez serwer !!  ");
				notifyMSG("Polaczenie zerwane przez serwer!! Sprobuj sie ponownie polaczyzc, w przypadku bledow ,skorzystaj z pomocy... ");
				zablokuj_wszystko();
				return;
				}
			
			/* rozpatrujemy co mamy */
			if  ( (line.length() > 0)&&(line.charAt(0)=='=') )
			{
				
				int p = line.indexOf(':');
				int pp = line.indexOf(':',p+1);
				if ((p>1)&&(pp>1))
				{
					String protoCMD=line.substring(1,p);
					String protoARG1=line.substring(p+1,pp);
					String protoARG2=line.substring(pp+1,line.length());
					
					//System.out.println("wholeLine: "+line);
					//System.out.println("protoCMD: "+protoCMD);
					////System.out.println("protoARG1: "+protoARG1);
					///System.out.println("protoARG2: "+protoARG2);
					if (protoCMD.equals("addlist")) { lista.add(protoARG1);fdUSEROW.put(protoARG1,protoARG2);
									   usersONLINE++;lab1.setText(""+LANGusertekst+" ["+usersONLINE+"]"); }
					if (protoCMD.equals("join"))    { lista.add(protoARG1);notifyMSG("+++ Przychodzi "+protoARG1);
									   fdUSEROW.put(protoARG1,protoARG2);	
									   usersONLINE++;lab1.setText(""+LANGusertekst+" ["+usersONLINE+"]");
									   }
					if (protoCMD.equals("part"))    { lista.remove(protoARG1);
									  if ((userSTAT.get(protoARG1)!=null)&&(userSTAT.get(protoARG1).equals("privmsg")) )
									  {
									  	for (int licznik=0; licznik<ileotwartych; licznik++)
											priv[licznik].shutPriv(protoARG1);
									  }
									  fdUSEROW.remove(protoARG1);
									  notifyMSG("+++ Odchodzi "+protoARG1);
									  usersONLINE--;lab1.setText(""+LANGusertekst+" ["+usersONLINE+"]");
									  }
					if (protoCMD.equals("PRIVMSG"))    
					{ 
						if ( (userSTAT.get(protoARG1)==null)||
							( (!userSTAT.get(protoARG1).equals("privmsg"))
									&&(!userSTAT.get(protoARG1).equals("ignore")) ))
					{
					userSTAT.put(protoARG1,"privmsg");
					if (ileotwartych+1==49) {
							 showStatus("Maksymalnie mozna miec 50 okienek z priv msg");
							 userSTAT.remove(protoARG1);
							 return;
						}
					priv[ileotwartych] = new PrivateCzat(this,nickname,protoARG1,(String)fdUSEROW.get(protoARG1),(String)fdUSEROW.get(nickname));

					priv[ileotwartych].add(protoARG2);
					ileotwartych++;
					// dodac tekst ktory otrzymal
					} else if (!userSTAT.get(protoARG1).equals("ignore"))
						{
						for (int licznik=0; licznik<ileotwartych; licznik++)
							priv[licznik].sendPriv(protoARG1,protoARG2);
						
						//String fuck = (String)idOkienka.get((String)protoARG1);
						//int id = Integer.parseInt(fuck);
						//System.out.println("[ODEBRANY] Powinno wyslac do okienka "+idOkienka.get((String)protoARG1));
						//int id = Integer.parseInt((String)idOkienka.get(protoARG1));
						//System.out.println("Powinno wyslac do okienka "+id);
						//PrivateCzat tmpp = priv[id];
						
						}
					}	
				}
				else System.out.println("Protocol ERROR!!!");
			}
			else
				chatSEND(line);
			
		
		
		} while ( run );
		
		} catch (IOException e)
			{
				System.out.println("Error : "+e);
				notifyMSG("Error: "+e);
				run=false;
				
			}
		
		System.out.println("[RUN] Stopped!!! \n");
			try {

		String wracamydo = getParameter("WRACAMYDO");
	        if (wracamydo == null) { wracamydo = new String("http://"+getCodeBase().getHost()+"/czat");}
		getAppletContext().showDocument(new URL(wracamydo),"_self");

		} catch (MalformedURLException ex ) {}
	}
	public void stop()
	{
		System.out.println("[STOP] Metoda STOP WYWOLANA!!");
		run=false;
		//sock = null;
		self = null;

	}
	
	public void destroy()
	{
//			System.out.println("[DESTROY] Wracamy do http://pawq.eu.org ");
			wyjdz_z_czata();

	}
	
	public void wyjdz_z_czata()
	{
				notifyMSG("--- Opuszczam czata......");
				for (int licznik=0; licznik<ileotwartych; licznik++)
							priv[licznik].shutPriv();
				run = false;
				zamknij.setEnabled(false);
				zablokuj_wszystko();
			
			try {
				if (sock != null ) {
					out.println("+QUIT"+"\n");
					out.flush();
					sock.close();
					sock = null;
				}
			    } catch (IOException ee) { System.out.println("sock.close "+ee);}	
	
			if (powieksz!=null) powieksz.zakoncz();
			try {
				zniszcz_wszystko();
		    		String wracamydo = getParameter("WRACAMYDO");
			        if (wracamydo == null) { wracamydo = new String("http://"+getCodeBase().getHost()+"/czat");}
				getAppletContext().showDocument(new URL(wracamydo),"_self");
//				getAppletContext().showDocument(new URL("http://"+getCodeBase().getHost()+"/czat"),"_self");
				return;
			} catch (Exception ex ) {}
	
	}
	
	
	public void zablokuj_wszystko()
	{
	
		enter.setEnabled(false); // textfield
		wyslij.setEnabled(false); // przycisk wyslij
		lista.removeAll();                             // Lista userow
		b1.setEnabled(false);
		b2.setEnabled(false);
		b3.setEnabled(false);
		b4.setEnabled(false);
		b5.setEnabled(false);
		
		
		master.pb1.setEnabled(false);
		master.pb2.setEnabled(false);
		master.pb3.setEnabled(false);
		master.wyborKOLOROW.setEnabled(false);
		usersONLINE=0;
		lab1.setText(""+LANGusertekst+" ["+usersONLINE+"]");		
	}
	public void zniszcz_wszystko()
	{
		/*remove(enter);remove(wyslij);
		remove(pb1);remove(pb2);remove(pb3);
		remove(b1);remove(b2);remove(b3);remove(b4);remove(b5);remove(zamknij);
		remove(lista);
		remove(propPanel);
		remove(pan);
		remove(blue);
		remove(panelOPCJI_UZYTKOWNIKOW);
		remove(txt);*/
	}
}

/*

Otwieranie nowego okna przegladarki

Use getAppletContext().ShowDocument(url, target); from within the code of your applet (start() method for example).
What yerkan gave is correct but not accurate, the getAppletContext() method is not static and getContext() is not defined at all. Example:

import java.net.*;
import java.applet.*;

public class MyApplet{

 public void start(){
    try{
        getAppletContext().showDocument(new URL("http://www.yahoo.com"),"_blank");
    } catch (MalformedURLException e){
          e.printStackTrace();
      }
 }

}





*/
