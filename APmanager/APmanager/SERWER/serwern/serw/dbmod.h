#ifndef _DBMOD_H
#define _DBMOD_H

#include <libpq-fe.h>

extern void pgsql_zakoncz(PGconn *pgconn);
extern PGconn *pgsql_polacz(char *pcUser, char *pcPass, char *pcBaza);
extern PGresult *pgsql_query(PGconn *pgconn, const char *spcFormat, ...);

#endif
