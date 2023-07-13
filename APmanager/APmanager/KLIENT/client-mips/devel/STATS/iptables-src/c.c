/*


iptables -t mangle -A PREROUTING -j CONNMARK --restore-mark
iptables -t mangle -A PREROUTING -m mark ! --mark 0 -j ACCEPT
iptables -t mangle -A PREROUTING -m ipp2p --edk -j MARK --set-mark 0x10
iptables -t mangle -A PREROUTING -m ipp2p --dc -j MARK --set-mark 0x11
iptables -t mangle -A PREROUTING -m ipp2p --gnu -j MARK --set-mark 0x12
iptables -t mangle -A PREROUTING -m ipp2p --kazaa -j MARK --set-mark 0x13
iptables -t mangle -A PREROUTING -m ipp2p --bit -j MARK --set-mark 0x14
iptables -t mangle -A PREROUTING -j CONNMARK --save-mark

iptables -t mangle -A POSTROUTING -m mark --mark 0x10 -j ACCEPT
iptables -t mangle -A POSTROUTING -m mark --mark 0x11 -j ACCEPT
iptables -t mangle -A POSTROUTING -m mark --mark 0x12 -j ACCEPT
iptables -t mangle -A POSTROUTING -m mark --mark 0x13 -j ACCEPT
iptables -t mangle -A POSTROUTING -m mark --mark 0x14 -j ACCEPT

# pp2p (0x80-...)
example three:


01# iptables -t mangle -A PREROUTING -j CONNMARK --restore-mark
02# iptables -t mangle -A PREROUTING -m mark ! --mark 0 -j ACCEPT
03# iptables -t mangle -A PREROUTING -m ipp2p --edk -j MARK --set-mark 0x10
04# iptables -t mangle -A PREROUTING -m ipp2p --dc -j MARK --set-mark 0x11
05# iptables -t mangle -A PREROUTING -m ipp2p --gnu -j MARK --set-mark 0x12
06# iptables -t mangle -A PREROUTING -m ipp2p --kazaa -j MARK --set-mark 0x13
07# iptables -t mangle -A PREROUTING -m ipp2p --bit -j MARK --set-mark 0x14
08# iptables -t mangle -A PREROUTING -j CONNMARK --save-mark

09# iptables -t mangle -A POSTROUTING -m mark --mark 0x10 -j ACCEPT
10# iptables -t mangle -A POSTROUTING -m mark --mark 0x11 -j ACCEPT
11# iptables -t mangle -A POSTROUTING -m mark --mark 0x12 -j ACCEPT
12# iptables -t mangle -A POSTROUTING -m mark --mark 0x13 -j ACCEPT
13# iptables -t mangle -A POSTROUTING -m mark --mark 0x14 -j ACCEPT

Finally an example where IPP2P is used for statistical purposes. Packets of 
different P2P networks get different marks (03-07). Like in example two 
CONNMARK is used to save (08) and restore (01&02) marks but then we add 
rules to the POSTROUTING chain (09-13) and use the iptables byte and packet 
counters to sum up P2P usage. 
Use "iptables -t mangle -L -n -v -x" then to print out the counters for all 
rules and get an overview of P2P traffic at your link. You could add other 
supported P2P networks as well - I think through this example you understand 
how! ;-)

*/
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/wait.h>

char iptables[255];

char *iptables_path[] = {
  "/usr/sbin/iptables",
  "/usr/local/sbin/iptables",
  NULL
};


void
chop (char *pcPtr)
{
      int iX = 0;
      for (iX = 0; iX < strlen (pcPtr); iX++)
	    if ((pcPtr[iX] == '\r') || (pcPtr[iX] == '\n'))
	          pcPtr[iX] = 0;
}

int
locate (char *pcPath)
{
  struct stat st;
  fflush (0);
  bzero (&st, sizeof (st));
  return (stat (pcPath, &st));
}

static void
find_iptables ()
{
  int iX = 0;
  bzero (iptables, sizeof (iptables));
  fflush (0);
  do
    {
      if (locate (iptables_path[iX]) == 0)
	strncpy (iptables, iptables_path[iX], sizeof (iptables) - 1);
    }
  while (iptables_path[++iX]);
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
	    unsigned long in;
	    unsigned long out;
	    char cIP[15];
      };
      struct 		IpClientStruct **IpClient;

//    if (!find_iptables())
//	return 0;	

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
	IpClient[iX]->in = atol (pcArgs[1]);
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
	      IpClient[iC]->out = atol (pcArgs[1]);
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
    snprintf(ctmpBuf, sizeof(ctmpBuf) - 1 ,"+STATS TRAFFIC %s %s-client.rrd %lu %lu\n", pcApName, IpClient[iC]->cIP, IpClient[iC]->in, IpClient[iC]->out);
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

int
main ()
{
  char *ptr;
 int status;
  find_iptables ();
  printf ("[?] Iptables found in %s \n", iptables);
      ptr = getClientTraffic ("TestowyAP");
      printf("\n-------------------------------------------------\n");
	printf("%s", ptr);
//    waitpid(-1,NULL,WNOHANG); //    waitpid(-1,NULL,WNOHANG);
    getchar();    
      printf("\n-------------------------------------------------\n");
      
  return 0;
}
