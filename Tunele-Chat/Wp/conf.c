/* Config files handling. Partialy borrowed from GNU Wget. */

#include "main.h"
#include "conf.h"
#include "magic.h"

extern int NALLOW;
extern struct config conf;
extern char *allow[MAXALLOW];

/* commands recognized by config */
const char *commands[]={
 "listen_port",
 "http_proxy_host",
 "http_proxy_port",
 "target_host",
 "target_port",
 "registered",
 "" /* indicates end of list */
};

/* copy the string formed by two pointers to a new location */
char *strdupdelim(const char *beg,const char *end) {
 char *res=(char *)malloc(end-beg+1);

 memcpy(res,beg,end-beg);
 res[end-beg]=0;

 return res;
}

/* syntax: <sp>* command <sp>* = <sp>* value <newline>
 * return values:
 *	 1	- success
 *	 0	- failure
 *	-1	- empty   */ 
int parse_line(const char *line,char **com,char **val) {
 const char *p=line;
 const char *orig_comptr,*end;
 char *new_comptr;

 while(*p&&isspace(*p)) ++p;
 if (!*p||*p=='#') return -1;
 for (orig_comptr=p;isalpha(*p)||*p=='_'||*p=='-';p++);
 if (!isspace(*p)&&(*p!='=')) return 0;
 *com=(char *)malloc(p-orig_comptr+1);
 for (new_comptr=*com;orig_comptr<p;orig_comptr++) *new_comptr++=*orig_comptr;
 *new_comptr=0;
 for (;isspace(*p);p++);
 if (*p!='=') { free(*com); return 0; } 
 for (++p;isspace(*p);p++);
 end=line+strlen(line)-1;
 while(end>p&&isspace(*end)) --end;
 *val=strdupdelim(p,end+1);

 return 1;
}

/* read a line from FP and return the pointer to allocated storage */
char *read_whole_line(FILE *fp) {
 int length=0;
 int bufsize=82;
 char *line=(char *)malloc(bufsize);

 while(fgets(line+length,bufsize-length,fp)) {
    length+=strlen(line+length);
    if (length==0) continue;
    if (line[length-1]=='\n') break;
    bufsize<<=1;
    line=realloc(line,bufsize);
 }
 if (length==0||ferror(fp)) { free (line); return NULL; }
 if (length+1<bufsize) line=realloc(line,length+1);

 return line;
}

/* parse config file. */
void parse_config() {
 char *line;
 int ln=1;
 FILE *fp;

 fp=fopen(CONFIG,"r");
 if (!fp) { printf("Can't open %s: %s.\n",CONFIG,strerror(errno)); return; }
 ln=1;
 while((line=read_whole_line(fp))) {
    char *com,*val;
    int status;
    status=parse_line(line,&com,&val);
    free(line);
    if (status==1) {
	status=0;
	while(*commands[status]) if (!strcasecmp(com,commands[status])) break; else status++;
	switch(status) {
	    case 0: conf.listen_port=atoi(val); break;
	    case 1: conf.http_proxy_host=strdup(val); break;
	    case 2: conf.http_proxy_port=strdup(val); break;
	    case 3: conf.target_host=strdup(val); break;
	    case 4: conf.target_port=strdup(val); break;
	    case 5: conf.registered=atoi(val); break;
	    default: printf("Unknown keyword in %s at line %d: %s.\n",CONFIG,ln,com);
	             finish(0);
	}
	free(com);
	free(val);
    } else if (status==0) { printf("Error in %s at line %d.\n",CONFIG,ln); finish(0); }
    ++ln;
 }
 fclose(fp);
}

/* parse allow file. */
void parse_allow() {
 FILE* data;
 int count=0;
 char tmp[BUFSIZE];

 if ((data=fopen(ALLOW,"r"))==NULL) return;
 count=0;
 do {
    if (fscanf(data,"%s",tmp)==EOF) break;
    if (*tmp) {
	allow[count]=(char *)malloc(strlen(tmp)+1);
	strcpy(allow[count++],tmp);
    }
 } while(*tmp&&(count<MAXALLOW-1));
 fclose(data);
 NALLOW=count;
}
