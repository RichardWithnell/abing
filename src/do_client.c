#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <math.h>     /* for sqrt() */
#include <unistd.h>     /* for close() */
#include <netdb.h>

/*------------------------------------------*/
#include  <sys/utsname.h>
#include  <signal.h>
/*------------------------------------------*/

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/time.h>
#include "abw.h"
/**************************************************************/
//char    *malloc();
extern double msdelay(),dDisp(),dMean();
extern struct ABWreport arr_report[];
extern double resolution,alpha,lppb,lctb,maxtd,mintd,old_fabw[];
extern double   arrS[],arrM[],arrR[],arrT[],tdr[],tdm[],tds[],tdt[],tdmx[],tdrx[];
extern double  arrM[],arrR[],arrT[],tdr[],tdm[];
extern double arr_hs_snd[],arr_hus_snd[],arr_hs_mir[],arr_hus_mir[],arr_hs_rcv[],arr_hus_rcv[];
extern int iter_t,iter_n;
extern int pktsize,npackets,ntrains,rpackets,nbunches,logfl,cafl;
extern char ipaddr[],date[],sstamp[],d80[],calhost[],linebuff[];
extern double S_se[],S_us[],S_at[],S_rt[];
extern char dirname[],klic[],version[];
/********************************************************/
/********************************************************/
void time_out()
{
int cas=3;
sleep(cas);
kill(getppid(),SIGTERM);
exit(-1);
}
/********************************************************/

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(-1);
}
/********************************************************/
in_addr_t gethostaddr(name)
     char *name;
{
   in_addr_t addr;
   register struct      hostent *hp;

   if ((addr = (in_addr_t)inet_addr (name)) != -1)
      return (addr);

   hp = (struct hostent*) gethostbyname(name);
   if (hp != NULL)
      return (*(in_addr_t *)hp->h_addr);
   else
      return (0);
}
/********************************************************/

int Do_Client_Work(char * serverName, char *bindAddress, unsigned short int serverPort, int rpt_cnt)
{
    int i,pid,pc,sock,rcnt;
    struct sockaddr_in dst;     /* Echo server address */
    struct sockaddr_in src;     /* Source address of echo */
    struct in_addr dst_addr;
    unsigned short echoServPort;     /* Echo server port */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    char *servIP;                    /* IP address of server */
    int16_t incnt,outcnt,xcnt;               /* Length of received response */

/********************************************************/
   struct timeval *tp;

   struct ABWreport *ptrep;

   struct ABWrec *srec,*rrec;
   double sleeptime = DELAYTIME,ms1,djn,ms_iter;
   uint16_t  nc,nt,np,ppseq,rnum,endcnt=0,okfl=0;
   int  npkt = NPKT;
   uint16_t  pktsend;
   uint16_t write_error = NO;
   uint16_t dcnt = 0;
   double tpole[100];
   char *ptext, rdata[MAXMESG],sdata[MAXMESG];
/********************************************************/
   double sumt,sumf,sumr,avt_abw,avr_rtt,avf_abw,
	si_t_abw, si_f_abw,si_rtt,di_t_abw, di_f_abw,di_rtt,
        arr_t_abw[NMODMAX],arr_f_abw[NMODMAX],arr_rtt[NMODMAX];
/********************************************************/

tp = (struct timeval *)(malloc (sizeof (struct timeval)));
sleeptime=50;
srec = (struct ABWrec *) sdata;
rrec = (struct ABWrec *) rdata;
bzero((char *) srec, sizeof (struct ABWrec));
ptext=sdata+124;
strcpy(ptext,klic);
srec->rnum = htons ((u_long) 2);

pktsend=1;
npackets=ntrains*npkt;
xcnt=1;


/* DNS resolution */
dst_addr.s_addr = gethostaddr(serverName);
if (dst_addr.s_addr ==0) {
	fprintf (stderr,"Destination: %s: uknown host\n",serverName);
	exit (1);
}

/* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

/* Construct the server address structure */
    memset(&dst, 0, sizeof(dst));    /* Zero out structure */
//    dst.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
//    dst.sin_addr.s_addr = gethostaddr(servIP);  /* Server name */
    dst.sin_family = AF_INET;                 /* Internet addr family */
    dst.sin_addr = dst_addr;  /* Server addr */
    dst.sin_port   = htons(serverPort);     /* Server port */


if(bindAddress){
    struct sockaddr_in b;
    bzero((char *)&b, sizeof (struct sockaddr_in));
    b.sin_family = AF_INET;
    b.sin_addr.s_addr = inet_addr(bindAddress);
    if (bind (sock, (struct sockaddr *) &b, sizeof(b)) < 0) {
        perror ("abw_rfl: bind");
        exit (1);
    }
}



if ((pid=fork()) == 0) {
/*********  child = sender ************************/
//        printf("I am child and I will do all sendings: %d,%d to %s \n",ntrains,npkt,serverName);



if (connect (sock, (struct sockaddr *) &dst, sizeof (dst)) < 0) {
      perror ("fabws: connect");
	exit(-1);
}

signal(SIGALRM,time_out);


for (i=0;i<rpt_cnt;i++) {
   for (nt = 1; nt <= ntrains; nt++) {
                srec->ppseq = htons (nt);
        for (nc = 1; nc <= npkt; nc++) {
    sprintf(ptext,"Tu je NC,NT=%d,%d a msg=%s",nc,nt,klic);       /* Second arg: string to echo */
                srec->num = htons (pktsend);
                gettimeofday (tp, (struct timezone *) 0);
                srec->snd.tv_sec = htonl (tp->tv_sec);
                srec->snd.tv_usec = htonl (tp->tv_usec);

	    	/* Send the string to the server */
                outcnt = write (sock, sdata, pktsize);
                if (outcnt < 0) {
		}
		pktsend++;
    	}
    	dcnt = 0;
    	tpole[nt]=msdelay(sleeptime);
   }
// new rpt_cycle need clean np
pktsend=1;
// set up delay in sec !!!!!
ms_iter=(iter_t - 1) *880.0+ 30;
ms1=msdelay(ms_iter);

} /*rpt_cnt_end*/
// start timeout now
raise(SIGALRM);
exit (-1);
} else {
/******** parent = receiver ***********************/
//        printf("I am parent and I will do receiving\n");
    		/* Recv a response */
    		fromSize = sizeof(src);
		rcnt=0;
  while (1) {
    		if ((incnt = recvfrom(sock, rdata, pktsize, 0,
         		(struct sockaddr *) &src, &fromSize)) <0)
        		DieWithError("recvfrom() failed");

        	gettimeofday (tp, (struct timezone *) 0);
    		if (dst.sin_addr.s_addr != src.sin_addr.s_addr)
    		{
			if ((rcnt >38) &(rcnt < 41)) {
        		fprintf(stderr,"Error: received a packet from unknown source.\n");
    			printf("Send[0..%d]pkt to Server: %s port: %d\n",rcnt,inet_ntoa(dst.sin_addr),htons(dst.sin_port));
    			printf("Received[0..%d]pkt  from: %s port: %d\n",rcnt,inet_ntoa(src.sin_addr),htons(src.sin_port));
			}
    		}

        	np = ntohs (rrec->num);
        	rnum = ntohs (rrec->rnum);
        	ppseq = ntohs (rrec->ppseq);
		rcnt++;
//    		printf("Received[%d] %d bytes <%s> fromServer: %s:%d %d,%d,%d\n",rcnt,incnt,ptext,inet_ntoa(src.sin_addr),htons(src.sin_port),np,rnum,ppseq);    /* Print the echoed arg */
        	if (np) {
                	rpackets++;
                	endcnt=0;
                	arr_hs_snd[np]= (double) ntohl(rrec->snd.tv_sec);
                	arr_hus_snd[np]=(double) ntohl(rrec->snd.tv_usec);
                	arr_hs_mir[np]= (double) ntohl(rrec->mirr.tv_sec);
                	arr_hus_mir[np]=(double) ntohl(rrec->mirr.tv_usec);
                	arr_hs_rcv[np]= (double) tp->tv_sec;
                	arr_hus_rcv[np]=(double) tp->tv_usec;
                	if(np==1) {
				sprintf(ipaddr,"%s",inet_ntoa(src.sin_addr));
                        	ptext=rdata+80;
                        	strcpy(d80,ptext);
			}
		}
	if (np==npackets) {
//		printf("Call DiMA  np=%d  rpackets=%d, xcnt=%d rpt_cnt=%d \n",np,rpackets,xcnt,rpt_cnt);
		DiMatrix(dirname,serverName,xcnt);
		rpackets=0;
		xcnt++;
//		ms1=msdelay(ms_iter);
		} else {
		}
//	if (np==npackets) break;
	if (xcnt > rpt_cnt) break;
   } /*endwhile*/
    close(sock);
/********************************************************/
// do FINAL CALC

if (iter_n > 1) {
	sumt=0;
	sumf=0;
	sumr=0;
	pc=iter_n;
	if(pc>NMOD) { pc=NMOD;}
	for(i=0;i<pc;i++) {
        	ptrep=&arr_report[i+1];
		arr_t_abw[i]=ptrep->t_abw;
		arr_f_abw[i]=ptrep->f_abw;
		arr_rtt[i]=ptrep->rtt;
		sumt += ptrep->t_abw;
		sumf += ptrep->f_abw;
		sumr += ptrep->rtt;
// printf("FIN[%d] t_abw=%5.3f,f_abw=%5.3f,rtt=%5.3f\n",i,arr_t_abw[i],arr_f_abw[i],arr_rtt[i]);
	}
	avt_abw=dMean(arr_t_abw,pc);
	avf_abw=dMean(arr_f_abw,pc);
	avr_rtt=dMean(arr_rtt,pc);

	si_t_abw=dDisp(arr_t_abw,pc,avt_abw);
	si_f_abw=dDisp(arr_f_abw,pc,avf_abw);
	si_rtt=dDisp(arr_rtt,pc,avr_rtt);

	di_t_abw=sqrt(si_t_abw);
	di_f_abw=sqrt(si_f_abw);
	di_rtt=sqrt(si_rtt);

        printf("(Avg/Sdev) RTT: %6.3f/%-6.3fms ABW To: %6.3f/%-6.3f From: %6.3f/%-6.3fMbits/s\n",avr_rtt,di_rtt,avt_abw,di_t_abw,avf_abw,di_f_abw);
   }
} /* end receiver - forkend */
}
/********************************************************/
