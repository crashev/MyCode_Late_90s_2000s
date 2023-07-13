#define AUTOR 		  "crashev@ebox.pl"
#define VERSION 	  "1.0"

#define LOGFILE 	  "debug/logfile"


#define APNAME 		  "Router1"		// Router's name - used in DATABASE
#define CONFIG_FILE  	  APNAME		// Name of config file for this router in DATABASE
#define CONFIG_LOCAL 	  APNAME		// Name of file which local configuration will be stored in
#define CONFIG_HOST	  "pawq.eu.org"		// Host with DATABASE where Config will be downloaded from

/* Directory with config files/sums */
#define CONFIG_DIR	  "/AP/"

#define MAIN_SERVER_HOST  "192.168.0.2"
#define MAIN_SERVER_PORT  5000

/* Connect retrying in seconds */

#define CONNECT_RETRY	  5
#define CONNECT_SSL_RETRY 5
#define SELECT_TIMEOUT    5

#define NETWORK_TIMEOUT   5		// 5 minut
