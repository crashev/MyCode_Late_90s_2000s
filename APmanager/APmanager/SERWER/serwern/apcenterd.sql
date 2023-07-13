--
-- PostgreSQL database dump
--

SET client_encoding = 'sql_ascii';
SET check_function_bodies = false;
SET client_min_messages = warning;

--
-- Name: SCHEMA public; Type: COMMENT; Schema: -; Owner: postgres
--

COMMENT ON SCHEMA public IS 'Standard public schema';


SET search_path = public, pg_catalog;

SET default_tablespace = '';

SET default_with_oids = true;

--
-- Name: access_lists; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE access_lists (
    alid serial NOT NULL,
    macaddr character varying(20),
    hostip character varying(15) NOT NULL,
    apid integer NOT NULL,
    stan integer DEFAULT 0,
    klientid integer NOT NULL
);


ALTER TABLE public.access_lists OWNER TO postgres;

--
-- Name: access_lists_alid_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval(pg_catalog.pg_get_serial_sequence('access_lists', 'alid'), 11, true);


--
-- Name: access_points; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE access_points (
    apid serial NOT NULL,
    apname character varying(100),
    apip character varying(15)
);


ALTER TABLE public.access_points OWNER TO postgres;

--
-- Name: access_points_apid_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval(pg_catalog.pg_get_serial_sequence('access_points', 'apid'), 3, true);


--
-- Name: klienci; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE klienci (
    klientid serial NOT NULL,
    imie character varying(10),
    nazwisko character varying(20),
    miasto character varying(30),
    adres character varying(50),
    pesel character varying(50)
);


ALTER TABLE public.klienci OWNER TO postgres;

--
-- Name: klienci_klientid_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval(pg_catalog.pg_get_serial_sequence('klienci', 'klientid'), 8, true);


--
-- Data for Name: access_lists; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY access_lists (alid, macaddr, hostip, apid, stan, klientid) FROM stdin;
1	00:0E:35:CE:07:A9	192.168.1.100	2	1	1
3	00:0e:9b:d3:bb:f1	192.168.1.102	2	1	3
4	00:12:f0:50:23:c8	192.168.1.103	2	1	4
5	00:14:a4:0b:45:7a	192.168.1.104	2	1	5
7	00:E0:98:D8:31:9B	192.168.1.105	2	1	6
2	00:0B:2B:09:5A:E0	192.168.1.101	2	1	1
8	00:14:BF:43:08:2D	192.168.1.180	2	1	7
9	00:0F:B0:46:99:62	192.168.100.100	3	1	1
10	00:40:f4:dd:b5:a0	192.168.1.106	2	1	8
11	00:13:46:b0:fa:12	192.168.1.107	2	1	1
\.


--
-- Data for Name: access_points; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY access_points (apid, apname, apip) FROM stdin;
1	PawqLinksysRouter	212.76.39.233
2	PawqLinksysRouterv3	83.145.178.186
3	PawqLinksysClient	83.145.178.186
\.


--
-- Data for Name: klienci; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY klienci (klientid, imie, nazwisko, miasto, adres, pesel) FROM stdin;
1	Pawel	Furtak	Warszawa	ul.Nowoursynowska	83032900953
2	Jaroslaw	Furtak	Warszawa	ul.Nowoursynowska	80121400953
3	Piotr	Zdanowicz	Koszalin - chotu	Bursa?	\N
4	Lukasz	Jakubik	Koszalin - szobi	Bursa?	\N
5	Lukasz	Tkacz	Koszalin - berni	Bursa?	\N
6	Marcin	Sadowski	Gorzyca	Gorzyca xx/xx	\N
7	Pawel	KlientMode	Koszalin	Yhm	\N
8	Daniel	Sorbian	\N	\N	\N
\.


--
-- Name: access_lists_macaddr_key; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY access_lists
    ADD CONSTRAINT access_lists_macaddr_key UNIQUE (macaddr);


ALTER INDEX public.access_lists_macaddr_key OWNER TO postgres;

--
-- Name: access_lists_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY access_lists
    ADD CONSTRAINT access_lists_pkey PRIMARY KEY (hostip, apid);


ALTER INDEX public.access_lists_pkey OWNER TO postgres;

--
-- Name: access_points_apname_key; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY access_points
    ADD CONSTRAINT access_points_apname_key UNIQUE (apname);


ALTER INDEX public.access_points_apname_key OWNER TO postgres;

--
-- Name: klienci_pesel_key; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY klienci
    ADD CONSTRAINT klienci_pesel_key UNIQUE (pesel);


ALTER INDEX public.klienci_pesel_key OWNER TO postgres;

--
-- Name: public; Type: ACL; Schema: -; Owner: postgres
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM postgres;
GRANT ALL ON SCHEMA public TO postgres;
GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- PostgreSQL database dump complete
--

