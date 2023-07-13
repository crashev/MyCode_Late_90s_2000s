/*

    Iptables Module
    Komunix (c) - Pawel Furtak '2005 <pawelf@bioinfo.pl>

*/

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "common.h"


char iptables[255];

char *iptables_path[]={
    "/usr/sbin/iptables",
    "/usr/local/sbin/iptables", 
    NULL
};


static int 
locate(char *pcPath)
{
	struct stat st;
	bzero(&st, sizeof(struct stat));
        return (stat(pcPath, &st));
}


static int
find_iptables()
{
        int iX = 0;
        bzero(iptables, sizeof(iptables));

        do {
		if ( locate(iptables_path[iX])==0 ) {
		    strncpy(iptables, iptables_path[iX], sizeof(iptables) - 1);
		    return 1;
		}
        } while (iptables_path[++iX]);
	
	return 0;
}


char *
get_mark(char *chain, char *markno) {
	FILE *fPo;
	char cBuf[1024];
        char mod[16];	
	char *ptr;
	char mark[10];
	unsigned long pkts,bytes;
	static char retBuf[200];

        if (!find_iptables())
			return NULL;	
	bzero(cBuf, sizeof(cBuf));
	snprintf(cBuf, sizeof(cBuf) - 1, "%s -n -t mangle -x -v -L %s", iptables, chain);
	fPo = popen(cBuf, "r");
	if (!fPo) {
	    return NULL;
	}

	/* skip first 2 lines */
	fscanf(fPo, "%*[^\n]\n%*[^\n]\n"); 
        bzero(cBuf, sizeof(cBuf));
	while (fgets(cBuf, sizeof(cBuf) - 1, fPo)) {
		ptr = NULL;
		sscanf(cBuf,"%lu %lu %15s %*[^\n]\n", &pkts, &bytes, mod);
		chop(cBuf);
	    if (!strcmp(mod,"MARK"))
		if (strstr(cBuf,"MARK set")) {
		    ptr = strstr(cBuf,"MARK set")+9;
		    bzero(mark, sizeof(mark));
		    strncpy(mark, ptr, sizeof(mark) - 1);
		    strtok(mark," ");
			if (!strcmp(mark,markno)) {
			    bzero(retBuf, sizeof(retBuf));
			    snprintf(retBuf, sizeof(retBuf) - 1, "%lu", bytes);
			    pclose(fPo);
			    return retBuf;
			}		
		}
	} 
	pclose(fPo);
	
	return NULL;
	
}




FILE *
OpenIpTables (char *pcArg)
{
  FILE *fPo = popen (pcArg, "r");
  if (!fPo)
    {
      return NULL;
    }
  /* skip first 2 lines */
  fscanf (fPo, "%*[^\n]\n%*[^\n]\n");
  waitpid(-1,NULL,WNOHANG); //    waitpid(-1,NULL,WNOHANG);
  return fPo;
}

static int
IptablesCount (char *pcChain)
{
  int iCount = 0;
  char cBuf[1024];
  FILE *fPo;

  bzero (cBuf, sizeof (cBuf));
  snprintf (cBuf, sizeof (cBuf) - 1, "%s -n -x -v -L %s", iptables, pcChain);
  fPo = OpenIpTables (cBuf);
  if (!fPo)
    return 0;

  /* Count records out_records */
  while (fgets (cBuf, sizeof (cBuf) - 1, fPo))
    iCount++;
  pclose (fPo);

  return iCount;

}



char *
getClientTraffic (char *pcApName)
{
      FILE 		*fPo;
      int 		iCount = 0, iCount2 = 0, iX = 0, iC = 0;
      char 		cBuf[1024];
      static char 	retBuf[4048];
      char 		ctmpBuf[100];
      char 		*pcArgs[10];
      char 		*ptr;

      struct IpClientStruct
      {
	    unsigned long long in;
	    unsigned long long out;
	    char cIP[30];
      };
      struct 		IpClientStruct **IpClient;

    if (!find_iptables())
	return NULL;	

  iCount = IptablesCount ("in_counter");
  iCount2 = IptablesCount ("out_counter");

  /* more entries in one chain that the other? - cant take it */
  if (iCount != iCount2)
    return NULL;
  if ((iCount == 0) || (iCount2 == 0))
    return NULL;

  IpClient =
    (struct IpClientStruct **) calloc (iCount,
				       sizeof (struct IpClientStruct *));
  if (!IpClient)
    return NULL;

  /* Allocate memory for our strctures */
  for (iC = 0; iC < iCount; iC++)
    if ((IpClient[iC] =
	 (struct IpClientStruct *) calloc (1,
					   sizeof (struct IpClientStruct))) ==
	NULL)
      {
	for (iX = 0; iX < iCount; iX++)
	  if (IpClient[iX])
	    free (IpClient[iX]);
	free (IpClient);
	return NULL;
      }

  /* Read data for in_counter */

  bzero (cBuf, sizeof (cBuf));
  snprintf (cBuf, sizeof (cBuf) - 1, "%s -n -x -v -L in_counter", iptables);
  fPo = OpenIpTables (cBuf);
  if (!fPo)
    return 0;


  iX = 0;
  while (iX < iCount)
    {
      bzero (cBuf, sizeof (cBuf));
      fgets (cBuf, sizeof (cBuf) - 1, fPo);
      chop (cBuf);
      if (strlen (cBuf) <= 5)
	{
	  pclose (fPo);
	  for (iC = 0; iC < iCount; iC++)
	    if (IpClient[iC])
	      free (IpClient[iC]);

	  if (IpClient)
	    free (IpClient);
	  return NULL;
	}

      bzero (&pcArgs, sizeof (pcArgs));
      ptr = strtok (cBuf, " ");
      if (!ptr)
	{
	  pclose (fPo);
	  for (iC = 0; iC < iCount; iC++)
	    if (IpClient[iC])
	      free (IpClient[iC]);

	  if (IpClient)
	    free (IpClient);
	  return NULL;
	}

      pcArgs[0] = ptr;
      iC = 1;
      while ((ptr = strtok (NULL, " ")) && (&pcArgs[iC + 1] != &pcArgs[10]))
	pcArgs[iC++] = ptr;

      if (pcArgs[1] != NULL)
	IpClient[iX]->in = atoll (pcArgs[1]);
      else
	{
	  for (iC = 0; iC < iCount; iC++)
	    if (IpClient[iC])
	      free (IpClient[iC]);
	  if (IpClient)
	    free (IpClient);
	  pclose (fPo);
	  return NULL;
	}

      if (pcArgs[7] != NULL)
	strncpy (IpClient[iX]->cIP, pcArgs[7],
		 sizeof (IpClient[iX]->cIP) - 1);
      else
	{
	  for (iC = 0; iC < iCount; iC++)
	    if (IpClient[iC])
	      free (IpClient[iC]);
	  if (IpClient)
	    free (IpClient);
	  pclose (fPo);
	  return NULL;
	}
      iX++;
    }
  pclose (fPo);

  /* Wczytujemy out_counter */

  bzero (cBuf, sizeof (cBuf));
  snprintf (cBuf, sizeof (cBuf) - 1, "%s -n -x -v -L out_counter", iptables);
  fPo = OpenIpTables (cBuf);
  if (!fPo)
    return 0;

  iX = 0;
  while (iX < iCount)
    {
      bzero (cBuf, sizeof (cBuf));
      fgets (cBuf, sizeof (cBuf) - 1, fPo);
      chop (cBuf);
      if (strlen (cBuf) <= 5)
	{
	  pclose (fPo);
	  for (iC = 0; iC < iCount; iC++)
	    if (IpClient[iC])
	      free (IpClient[iC]);

	  if (IpClient)
	    free (IpClient);
	  return NULL;
	}

      bzero (&pcArgs, sizeof (pcArgs));
      ptr = strtok (cBuf, " ");
      if (!ptr)
	{
	  pclose (fPo);
	  for (iC = 0; iC < iCount; iC++)
	    if (IpClient[iC])
	      free (IpClient[iC]);

	  if (IpClient)
	    free (IpClient);
	  return NULL;
	}
      pcArgs[0] = ptr;
      iC = 1;
      while ((ptr = strtok (NULL, " ")) && (&pcArgs[iC + 1] != &pcArgs[10]))
	pcArgs[iC++] = ptr;

      if ((pcArgs[1] != NULL) && (pcArgs[6] != NULL))
	{
	  for (iC = 0; iC < iCount; iC++)
	    if (!strcmp (IpClient[iC]->cIP, pcArgs[6]))
	      IpClient[iC]->out = atoll (pcArgs[1]);
	}
      else
	{
	  for (iC = 0; iC < iCount; iC++)
	    if (IpClient[iC])
	      free (IpClient[iC]);

	  if (IpClient)
	    free (IpClient);
	  pclose (fPo);
	  return NULL;
	}



      iX++;
    }
  pclose (fPo);

  /* Genearate whole retBuf */
  bzero(retBuf, sizeof(retBuf));
  for (iC = 0; iC < iCount; iC++) {
    bzero(ctmpBuf, sizeof(ctmpBuf));
    snprintf(ctmpBuf, sizeof(ctmpBuf) - 1 ,"+STATS TRAFFIC %s %s-client.rrd %llu %llu\n", pcApName, IpClient[iC]->cIP, IpClient[iC]->in, IpClient[iC]->out);
    strncat(retBuf, ctmpBuf, sizeof(retBuf) - 1);     
  }
  /* Free memory */
  for (iC = 0; iC < iCount; iC++)
    if (IpClient[iC])
      free (IpClient[iC]);

  if (IpClient)
    free (IpClient);

  return retBuf;
}
