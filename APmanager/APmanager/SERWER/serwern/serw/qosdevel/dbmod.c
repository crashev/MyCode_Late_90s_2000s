/*
    Modul komunikacji z baza danych
    gcc -lpq my.c -o my    
*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "dbmod.h"


void
pgsql_zakoncz(PGconn *pgconn) 
{
    if (pgconn)
	PQfinish(pgconn);
}


PGconn *
pgsql_polacz(char *pcUser, char *pcPass, char *pcBaza)
{
    PGconn *pgconn;
    char cConnStr[100];
    bzero(cConnStr, sizeof(cConnStr));

    snprintf(cConnStr, sizeof(cConnStr) - 1,"dbname=%s user=%s password=%s", pcBaza, pcUser, pcPass);
    pgconn = PQconnectdb(cConnStr);
    
    if (PQstatus( pgconn ) == CONNECTION_BAD) {
	    printf("[!] Connection to database failed\n");
	    printf("[!] %s \n", PQerrorMessage( pgconn ));
	    PQfinish(pgconn);
	    return NULL;
    }
    
    return pgconn;
}


PGresult *
pgsql_query(PGconn *pgconn, const char *spcFormat, ...)
{
            static char scBuf[1024];
	    va_list ap;
	    PGresult *pgres;
	    
	    if (!pgconn)
		return NULL;
		
            memset(scBuf, 0x0, sizeof(scBuf));
            va_start(ap, spcFormat);
            vsnprintf(scBuf, sizeof(scBuf) - 1, spcFormat, ap);
            va_end(ap);
	    
	    pgres = PQexec(pgconn, scBuf);
	    if (!pgres || PQresultStatus(pgres) != PGRES_TUPLES_OK) {
		printf("[!] Error executing SELECT: pgres:  and result: %s\n", PQresultErrorMessage(pgres));
		PQclear(pgres);
		return NULL;
	    }

	    return pgres;
}
