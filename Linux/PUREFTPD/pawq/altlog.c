
/*
 * Alternative logging formats for Pure-FTPd 
 */

#include <config.h>

#ifdef WITH_ALTLOG

# include "ftpd.h"
# include "ftpwho-update.h"
# include "globals.h"
# include "altlog.h"
# include <mysql/mysql.h>
# ifdef WITH_DMALLOC
#  include <dmalloc.h>
# endif


static int altlog_write(const char *str)
{
    struct flock lock;
    ssize_t left;
    
    if (altlog_fd == -1 || str == NULL || 
        (left = (ssize_t) strlen(str)) <= (ssize_t) 0) {
        return -1;
    }
    lock.l_whence = SEEK_SET;
    lock.l_start = (off_t) 0;
    lock.l_len = (off_t) 0;
    lock.l_pid = getpid();
    lock.l_type = F_WRLCK;
    while (fcntl(altlog_fd, F_SETLKW, &lock) < 0 && errno == EINTR);
    if (lseek(altlog_fd, (off_t) 0, SEEK_END) < (off_t) 0) {
        return -1;
    }
    (void) safe_write(altlog_fd, str, (size_t) left);
    lock.l_type = F_UNLCK;
    while (fcntl(altlog_fd, F_SETLK, &lock) < 0 && errno == EINTR);    
    
    return 0;
}

/* Verbose but compact log format for ftpStats */

static int altlog_writexfer_stats(const int upload,
                                  const char * const filename,
                                  const off_t size,
                                  const double duration)
{
    /*
     * <date> <start.pid> <user> <ip> <u/d> <size> <duration> <file>
     */

    const char *host_ = *host != 0 ? host : "-";
    const char *account_ = *account != 0 ? account : "-";
    char *alloca_line;
    size_t line_size;
    
    line_size = 16U /* now */ + 1U + 16U /* start */ + 1U /* . */ +
        16U /* pid */ + 1U + strlen(account_) + 1U + strlen(host_) + 1U +
        1U /* U/D */ + 1U + 16U /* size */ + 16U /* duration */ +
        strlen(filename) + 1U /* \n */ + 1U;
    if ((alloca_line = ALLOCA(line_size)) == NULL) {
        return -1;
    }
    if (!SNCHECK(snprintf(alloca_line, line_size,
                          "%llu %llx.%lx %s %s %c %llu %lu %s\n",
                          (unsigned long long) time(NULL),
                          (unsigned long long) session_start_time,
                          (unsigned long) getpid(),
                          account_, host_,
                          upload != 0 ? 'U' : 'D',
                          (unsigned long long) size,
                          (unsigned long) (duration + 0.5),
                          filename), line_size)) {
        altlog_write(alloca_line);        
    }
                          
    ALLOCA_FREE(alloca_line);
    
    return 0;
}



static int reads(int *sock,char *buf) 
{
 int c,count=0;
 do c=read(*sock,&buf[count],1); while ((c>0)&&(buf[count++]!='\n'));
 buf[count]='\0';
 return (c<=0)?c:count;
}

static
void eat_spaces(char *str)
{
    int iX=0;
    int iY=0;

    for (iX=0; iX<strlen(str); iX++)
        if (str[iX]==' ')
            {
                // przesuwamy
                for (iY=0; iY<strlen(str)-iX; iY++)
                    str[iX+iY]=str[iX+iY+1];
            }
}

static void chop(char *pcPtr)
{
    int iX;
    for (iX=0; iX< strlen(pcPtr); iX++)
        if ( (pcPtr[iX]=='\n')||(pcPtr[iX]=='\r') )
            pcPtr[iX]=0x0;
}



/* Verbose but compact log format for ftpStatst to MYSQL Database */

static int altlog_writexfer_mysql(const int upload,
                                  const char * const filename,
                                  const off_t size,
                                  const double duration)
{
    /*
     * <date> <start.pid> <user> <ip> <u/d> <size> <duration> <file>
     */

    const char *host_ = *host != 0 ? host : "-";
    const char *account_ = *account != 0 ? account : "-";
    char *alloca_line;
    size_t line_size;
    /* Mysql Config Stuff */
    char user[50];
    char password[50];
    char dbname[50];
    char sock[255];
    char table[50];
    int iConf=0;
    int iX,iY;
    char buf[1024];    
    int iRet;
    MYSQL mysql;
//    line_size = 16U /* now */ + 1U + 16U /* start */ + 1U /* . */ +
//        16U /* pid */ + 1U + strlen(account_) + 1U + strlen(host_) + 1U +
//        1U /* U/D */ + 1U + 16U /* size */ + 16U /* duration */ +
//        strlen(filename) + 1U /* \n */ + 1U;
//    if ((alloca_line = ALLOCA(line_size)) == NULL) {
//        return -1;
//    }

    // 1 CZAS    	- time(NULL)
    // 2 USER   	 - account_
    // 3 HOST	 	- host_
    // 4 UP/DOWN 	- upload != 0 ? 'U' : 'D',
    // 5 SIZE    	- size
    // 6 FILENAME	- filename
                          
//    ALLOCA_FREE(alloca_line);

    /* Checking if there is avaible opened descriptor to conf file */
    if (altlog_fd == -1) {
        return -1;
    }
    
    /* Back to begining */
    lseek(altlog_fd, (off_t) 0, SEEK_SET);
    bzero(buf		, sizeof(buf));
    bzero(user		, sizeof(user));
    bzero(password	, sizeof(password));
    bzero(dbname	, sizeof(dbname));
    bzero(sock		, sizeof(sock));
    bzero(table		, sizeof(table));

    /* Read config file */
    while (reads(&altlog_fd, buf))
    {
        // eat spaces
        eat_spaces(buf);
        chop(buf);
        if (!strncasecmp(buf,"USER:",5)) {
            strncpy(user, buf+5, sizeof(user) - 1);
            iConf++;
        }

        if (!strncasecmp(buf,"PASSWORD:",9)) {
            strncpy(password, buf+9, sizeof(password) - 1);
            iConf++;
        }

        if (!strncasecmp(buf,"DBNAME:",7)) {
            strncpy(dbname, buf+7, sizeof(dbname) - 1);
            iConf++;
        }

        if (!strncasecmp(buf,"SOCK:",5)) {
            strncpy(sock, buf+5, sizeof(sock) - 1);
            iConf++;
        }
    
        if (!strncasecmp(buf,"TABLE:",6)) {
            strncpy(table, buf+6, sizeof(table) - 1);
            iConf++;
        }
    
        bzero(buf, sizeof(buf));

    }
 
    /* Read all needed data */	

    if (iConf == 5)
    {
	mysql_init(&mysql);
	mysql_options(&mysql, MYSQL_READ_DEFAULT_GROUP, "pure-ftpd");
	
	if (!mysql_real_connect(&mysql, sock, user, password, dbname, 0, NULL, 0))
	{
	    return -1;
	}
	
	// INSERT
		
	bzero(buf, sizeof(buf));
	snprintf(buf, sizeof(buf) - 1, "INSERT DELAYED INTO %s (DATA, TIME, USER, HOST, UPLOAD, SIZE, FILE) VALUES(NOW(), '%llu', '%s', '%s', '%c', '%llu', '%s');", 
							table,
							(unsigned long long)time(NULL), 
							account_, 
							host_, 
							upload != 0 ? 'U' : 'D',
							(unsigned long long)size,
    							filename);

	
    	iX = mysql_real_query(&mysql, buf, strlen(buf));
	mysql_close(&mysql);
	
    }

    
    return 0;
}




# define NO_URLENCODE(c) ( \
                               ((c) >= 'a' && (c) <= 'z') || \
                               ((c) >= 'A' && (c) <= 'Z') || \
                               ((c) >= '0' && (c) <= '9') || \
                               (c) == '.' || (c) == '/' || \
                               (c) == '_' || (c) == '-' \
                               )
# define HEXD(c) ((c) < 10 ? '0' + (c) : 'A' - 10 + (c))

static char *urlencode(const char *filename)
{
    const char *ptr = filename;
    char *quoted_filename;
    char *quoted_filename_ptr;
    size_t quoted_filename_size = (size_t) 1U;  
    int need_quote = 0;
    char c;

    while (*ptr != 0) {
        if (NO_URLENCODE(*ptr)) {
            quoted_filename_size++;
        } else {
            quoted_filename_size += (size_t) 3U;
            need_quote = 1;
        }
        ptr++;
    }
    if (need_quote == 0) {
        return (char *) filename;
    }
    if ((quoted_filename = malloc(quoted_filename_size)) == NULL) {
        return NULL;
    }
    quoted_filename_ptr = quoted_filename;
    ptr = filename;
    c = *ptr;
    do {
        if (NO_URLENCODE(c)) {
            if (quoted_filename_size <= (size_t) 1U) {
                return NULL;
            }
            quoted_filename_size--;         
            *quoted_filename_ptr++ = c;
        } else {
            if (quoted_filename_size <= (size_t) 3U) {
                return NULL;
            }
            quoted_filename_size -= (size_t) 3U;            
            *quoted_filename_ptr++ = '%';
            *quoted_filename_ptr++ = HEXD(((unsigned char) c) >> 4);
            *quoted_filename_ptr++ = HEXD(((unsigned char) c) & 0xf);
        }
        ptr++;
    } while ((c = *ptr) != 0);
    *quoted_filename_ptr = 0;
    
    return quoted_filename;
}

/* HTTPd-like common log format */

static int altlog_writexfer_clf(const int upload,
                                const char * const filename,
                                const off_t size)
{
    char date[sizeof "13/Apr/1975:12:34:56 +0100"];     
    struct tm *tm;
    char *alloca_line;
    const char *host_ = *host != 0 ? host : "-";
    const char *account_ = *account != 0 ? account : "-";
    char *quoted_filename;
    time_t now;
    long diff;
    int sign;
    size_t line_size;

    if ((now = time(NULL)) == (time_t) -1 ||
        (tm = localtime(&now)) == NULL ||
        tm->tm_mon > 11 || tm->tm_mon < 0) {
        return -1;
    }    
# ifdef HAVE_STRUCT_TM_TM_GMTOFF
    diff = -(tm->tm_gmtoff) / 60L;
# elif defined(HAVE_SCALAR_TIMEZONE)
    diff = -(timezone) / 60L;    
# else
    {
        struct tm gmt;
        struct tm *t;
        int days, hours, minutes;
        
        gmt = *gmtime(&now);    
        t = localtime(&now);
        days = t->tm_yday - gmt.tm_yday;
        hours = ((days < -1 ? 24 : 1 < days ? -24 : days * 24)
                 + t->tm_hour - gmt.tm_hour);
        minutes = hours * 60 + t->tm_min - gmt.tm_min;
        diff = -minutes;
    }
# endif
    if (diff > 0L) {
        sign = '+';
    } else {
        sign = '-';
        diff = -diff;
    }
    
    if (SNCHECK(snprintf(date, sizeof date, 
                         "%02d/%s/%d:%02d:%02d:%02d %c%02ld%02ld",
                         tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
                         tm->tm_hour, tm->tm_min, tm->tm_sec,
                         sign, diff / 60L, diff % 60L), sizeof date)) {
        return -1;
    }
    if ((quoted_filename = urlencode(filename)) == NULL) {
        return -1;
    }
    line_size = strlen(host_) + (sizeof " - " - 1U) + strlen(account_) +
        (sizeof " [" - 1U) + (sizeof date - 1U) + (sizeof "] \"" - 1U) +
        3U /* GET / PUT */ + (sizeof " " - 1U) + strlen(quoted_filename) +
        (sizeof "\" 200 1234567890\n" - 1U) + 1U;
    if ((alloca_line = ALLOCA(line_size)) == NULL) {
        return -1;
    }
    if (!SNCHECK(snprintf(alloca_line, line_size, 
                          "%s - %s [%s] \"%s %s\" 200 %llu\n",
                          host_, account_, date, 
                          upload == 0 ? "GET" : "PUT", quoted_filename,
                          (unsigned long long) size), line_size)) {
        altlog_write(alloca_line);
    }
    if (quoted_filename != filename) {
        free(quoted_filename);
    }
    ALLOCA_FREE(alloca_line);
    
    return 0;
}

/* WuFTPd-like log format */

static int altlog_writexfer_xferlog(const int upload,
                    const char * const filename,
                    const off_t size,
                    const double duration)
{
    char date[sizeof "Mon Apr 13 12:34:56 1975"];
    struct tm *tm;
    char *alloca_line;
    const char *host_ = *host != 0 ? host : "-";
    const char *account_ = *account != 0 ? account : "-";
    char *quoted_filename;
    size_t filename_idx;
    time_t now;
    size_t line_size;
    size_t filename_size;
    char c;

    if ((now = time(NULL)) == (time_t) -1 ||
        (tm = localtime(&now)) == NULL ||
        tm->tm_mon > 11 || tm->tm_mon < 0 ||
        tm->tm_wday > 6 || tm->tm_wday < 0) {
        return -1;
    }    
    if (SNCHECK(snprintf(date, sizeof date, 
                         "%s %s %02d %02d:%02d:%02d %d",
                         week_days[tm->tm_wday], months[tm->tm_mon],
                         tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
                         tm->tm_year + 1900), sizeof date)) {
        return -1;
    }
    if ((filename_idx = strlen(filename)) <= (size_t) 0U) {
        return -1;
    }
    filename_size = filename_idx + (size_t) 1U;
    if ((quoted_filename = ALLOCA(filename_size)) == NULL) {
        return -1;
    }
    
    quoted_filename[filename_idx] = 0;
    do {
        filename_idx--;
        c = filename[filename_idx];
        if (isspace((unsigned char) c) || ISCTRLCODE(c)) {
            c = '_';
        }
        quoted_filename[filename_idx] = c;
    } while (filename_idx > (size_t) 0U);
    
    line_size = (sizeof date - 1U) + (sizeof " " - 1U) +
        (size_t) 8U /* duration */ + (sizeof " " - 1U) +
        strlen(host_) + (sizeof " " - 1U) +
        (size_t) 16U /* size */ +
        (filename_size - 1U) + (sizeof " " - 1U) +
        (size_t) 1U /* type */ + (sizeof " _ " - 1U) +
        (size_t) 1U /* direction */ + (sizeof " " - 1U) +
        (size_t) 1U /* anonymous */ + (sizeof " " - 1U) +
        strlen(account_) + (sizeof " ftp 1 * c\n" - 1U) + (size_t) 1U;
    if ((alloca_line = ALLOCA(line_size)) == NULL) {
        ALLOCA_FREE(quoted_filename);
        return -1;
    }    
    if (!SNCHECK(snprintf(alloca_line, line_size, 
                          "%s %lu %s %llu %s %c _ %c %c %s ftp 1 * c\n",
                          date, (unsigned long) (duration + 0.5),
                          host_, (unsigned long long) size,
                          quoted_filename,
                          type == 1 ? 'a' : 'b',
                          upload != 0 ? 'i' : 'o',
                          loggedin != 0 ? 'r' : 'a',
                          account_), line_size)) {
        altlog_write(alloca_line);
    }
    ALLOCA_FREE(quoted_filename);
    ALLOCA_FREE(alloca_line);
    
    return 0;
}

static int altlog_writexfer_w3c(const int upload,
                                const char * const filename,
                                const off_t size,
                                const double duration)
{
    /*
     * <date> <time> <ip> "[]sent" <file> "226" <user>
     * date time c-ip cs-method cs-uri-stem sc-status cs-username
     */

    struct tm *tm;
    struct tm gmt;
    const char *host_ = *host != 0 ? host : "-";
    const char *account_ = *account != 0 ? account : "-";
    char *alloca_line;
    char *quoted_filename;
    time_t now;
    size_t line_size;

    (void) duration;
    if ((now = time(NULL)) == (time_t) -1 ||
        (tm = localtime(&now)) == NULL ||
        tm->tm_mon > 11 || tm->tm_mon < 0) {
        return -1;
    }
    gmt = *gmtime(&now);
    if ((quoted_filename = urlencode(filename)) == NULL) {
        return -1;
    }   
    line_size = (sizeof "13-04-1975 12:34:56 " - 1U) +
        strlen(host_) + 1U + (sizeof "[]created" - 1U) + 1U +
        strlen(quoted_filename) + 1U + (sizeof "226" - 1U) +
        strlen(account_) + 1U + 42U + 1U /* \n */ + 1U;

    if ((alloca_line = ALLOCA(line_size)) == NULL) {
        return -1;
    }
    if (!SNCHECK(snprintf(alloca_line, line_size,
                          "%d-%02d-%02d %02d:%02d:%02d %s []%s %s 226 %s %llu\n",
                          gmt.tm_year + 1900, gmt.tm_mon + 1, gmt.tm_mday,
                          gmt.tm_hour, gmt.tm_min, gmt.tm_sec,
                          host_, (upload != 0 ? "created" : "sent"),
                          quoted_filename,
                          account, (unsigned long long) size), line_size)) {
        altlog_write(alloca_line);
    }
    if (quoted_filename != filename) {
        free(quoted_filename);
    }
    ALLOCA_FREE(alloca_line);

    return 0;
}

int altlog_write_w3c_header(void)
{
    time_t now;
    struct tm *tm;
    struct tm gmt;
    char *alloca_line;
    size_t line_size;

    if ((now = time(NULL)) == (time_t) -1 ||
        (tm = localtime(&now)) == NULL ||
        tm->tm_mon > 11 || tm->tm_mon < 0) {
        return -1;
    }
    gmt = *gmtime(&now);
    line_size = sizeof "#Date: 001975-04-13 12:34:56\n";   /* be year-999999 compliant :) */
    if ((alloca_line = ALLOCA(line_size)) == NULL) {
        return -1;
    }

    altlog_write("#Software: Pure-FTPd " VERSION "\n");
    altlog_write("#Version: 1.0\n");

    if (!SNCHECK(snprintf(alloca_line, line_size,
                          "#Date: %04d-%02d-%02d %02d:%02d:%02d\n",
                          gmt.tm_year + 1900, gmt.tm_mon + 1, gmt.tm_mday,
                          gmt.tm_hour, gmt.tm_min, gmt.tm_sec),
                          line_size)) {
        altlog_write(alloca_line);
    }

    altlog_write("#Fields: date time c-ip cs-method cs-uri-stem sc-status cs-username sc-bytes\n");

    ALLOCA_FREE(alloca_line);

    return 0;
}

/* 
 * We should define a structure of function pointers, 
 * and associate a structure with each logging type in AltLogPrefixes.
 * But yet we only have *three* logging methods, and the code would be
 * complicated for nothing. So let's stick with simple tests for now. -j.
 */

int altlog_writexfer(const int upload,
                     const char * const filename,
                     const off_t size,
                     const double duration)
{
    switch (altlog_format) {
    case ALTLOG_NONE:
        return 0;
    case ALTLOG_CLF:
        return altlog_writexfer_clf(upload, filename, size);
    case ALTLOG_STATS:
        return altlog_writexfer_stats(upload, filename, size, duration);
    case ALTLOG_W3C:
        return altlog_writexfer_w3c(upload, filename, size, duration);
    case ALTLOG_XFERLOG:
        return altlog_writexfer_xferlog(upload, filename, size, duration);
    case ALTLOG_MYSQL:
        return altlog_writexfer_mysql(upload, filename, size, duration);
    }    
    return -1;
}

#endif
