DROP TABLE ACCESS_POINTS;
DROP TABLE ACCESS_LISTS;
DROP TABLE KLIENCI;
DROP TABLE LOKALIZACJA;
DROP TABLE QUEUES;

CREATE TABLE ACCESS_POINTS (
	ID      VARCHAR(32)     NOT NULL,
	APNAME  VARCHAR(100) UNIQUE,
	APIP	VARCHAR(15),
	LID	VARCHAR(32)
);

CREATE TABLE ACCESS_LISTS (
	ID       VARCHAR(32)     NOT NULL,
	MACADDR	 VARCHAR(20) NOT NULL,
	HOSTIP	 VARCHAR(15) NOT NULL,
	AID	 VARCHAR(32) NOT NULL,
	QID	 VARCHAR(32),
	KID	 VARCHAR(32) NOT NULL,
	Active	 SmallInt DEFAULT '1',
	PRIMARY KEY(HOSTIP, AID)
);

CREATE TABLE KLIENCI (
	ID 			Varchar(32)     NOT NULL PRIMARY KEY, 
	Imie    		Varchar(60)     NOT NULL,        
	Nazwisko        	Varchar(60)     NOT NULL,     
	Login   		Varchar(20),     
	Haslo   		Varchar(32),     
	Email   		Varchar(160),    
	Ulica   		Varchar(160),   
	NumerDomuMieszkania     Varchar(30),  
	kod     		Varchar(6), 
	MID     		Varchar(32),
	Telefon 		Varchar(30),
	Tel_kom 		Varchar(30),            
	PESEL   		Varchar(12)     
);

/*
Lokalizacja AP
*/
CREATE TABLE LOKALIZACJA (
	ID      Varchar(32)     NOT NULL,     
	Miasto  Varchar(60)     NOT NULL,       
	KOD     Varchar(10)     NOT NULL,       
	Ulica   Varchar(100)    NOT NULL,       
	MID     Varchar(32)
);


CREATE TABLE QUEUES (
	ID      	VARCHAR(32)     NOT NULL,
	BW      	INT	  	NOT NULL,        /* BW size (128, 256),*/
	BW_MAX  	INT	  	NOT NULL DEFAULT 0,        /*BW max jump (1024, 2056),*/
	BW_TRAFFIC 	INT	        NOT NULL,        /*Amount of MB after MAX_BW (moves to)-> BW,*/
	Descr   	Text    	NOT NULL
);


CREATE TABLE admin 
(
	id      serial,
	login   character varying(32) not null,
	haslo   character varying(32) not null
);


CREATE TABLE miasta 
(
	id varchar(32),
	name varchar(20)
);


CREATE TABLE historiarach
(
 id     varchar(32)  not null,
 uid    varchar(32) not null,
 kwota  double precision not null,
 data   timestamp  not null,
 opisid varchar(32) not null
);

CREATE TABLE optrans (
	 id     varchar(32) not null,
	 text   varchar(100) not null
);



