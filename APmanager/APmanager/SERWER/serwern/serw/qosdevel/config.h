#ifndef _CONFIG_H
#define _CONFIG_H

int config_make_ethers(char *pcHostIp, char *pcName);
int config_make_clientsconfig(char *pcHostIp, char *pcName) ;
int config_make_update(char *pcHostIp, char *pcName);
int config_make_config(char *pcHostIp, char *pcName);
int config_make_additional(char *pcHostIp, char *pcFile, char *pcDir, char *pcName);

#define DB_USER "apcenterd"
#define DB_PASSWORD "ap.bunczuczny.$%^"
#define DB_BAZA "komunix_pawelf_apcenterd"

#endif
