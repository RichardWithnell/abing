#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <math.h>     /* for sqrt() */
#include <unistd.h>     /* for close() */
#include <netdb.h>
#include <pthread.h>

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

struct recv_work_data {
	int sock;
	struct sockaddr_in dst;
	char *serverName;
	int rpt_cnt;
	int running;
};

struct send_work_data {
	int sock;
	struct sockaddr_in dst;
	int rpt_cnt;
};


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

void * do_send_work(void *data)
{
    int i,sock, rpt_cnt;
    struct sockaddr_in dst;     /* Echo server address */
    int16_t outcnt;               /* Length of received response */

    /********************************************************/
    struct timeval *tp;

    struct ABWrec *srec;
    double sleeptime = DELAYTIME, ms1, ms_iter;
    uint16_t  nc, nt, ppseq;
    int  npkt = NPKT;
    uint16_t  pktsend;
    uint16_t dcnt = 0;
    double tpole[100];
    char *ptext, sdata[MAXMESG];
    struct send_work_data *swd;

    swd = (struct send_work_data*)data;

	sock = swd->sock;
	dst = swd->dst;
	rpt_cnt = swd->rpt_cnt;

    tp = (struct timeval *)(malloc (sizeof (struct timeval)));
    sleeptime = 50;
    srec = (struct ABWrec *) sdata;
    bzero((char *) srec, sizeof (struct ABWrec));
    ptext=sdata+124;
    strcpy(ptext,klic);
    srec->rnum = htons ((u_long) 2);

    pktsend = 1;

    if (connect (sock, (struct sockaddr *) &dst, sizeof (dst)) < 0) {
        perror ("fabws: connect");
        exit(-1);
    }

    //signal(SIGALRM,time_out);

    for (i=0;i<rpt_cnt;i++) {
        for (nt = 1; nt <= ntrains; nt++) {
            srec->ppseq = htons (nt);
            for (nc = 1; nc <= npkt; nc++) {
                sprintf(ptext,"Tu je NC,NT=%d,%d a msg=%s",nc,nt,klic);
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
            tpole[nt] = msdelay(sleeptime);
        }
        pktsend = 1;
        ms_iter = (iter_t - 1) *880.0+ 30;
        ms1 = msdelay(ms_iter);

    }
}

void * do_receive_work(void *data)
{
    int i, pc, sock, rcnt, rpt_cnt;
    struct sockaddr_in dst;     /* Echo server address */
    struct sockaddr_in src;     /* Source address of echo */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    int16_t incnt, outcnt, xcnt;               /* Length of received response */

    struct timeval *tp;
    struct ABWreport *ptrep;
    struct ABWrec *rrec;
    uint16_t  np, ppseq, rnum, endcnt = 0;
    int npkt = NPKT;
    char *ptext, rdata[MAXMESG];
    char * serverName;

    double sumt,sumf,sumr,avt_abw,avr_rtt,avf_abw,
    si_t_abw, si_f_abw,si_rtt,di_t_abw, di_f_abw,di_rtt,
        arr_t_abw[NMODMAX],arr_f_abw[NMODMAX],arr_rtt[NMODMAX];

	struct recv_work_data *rwd;

	rwd = (struct recv_work_data*)data;
	sock = rwd->sock;
	dst = rwd->dst;
	serverName = rwd->serverName;
	rpt_cnt = rwd->rpt_cnt;

    tp = (struct timeval *)(malloc (sizeof (struct timeval)));
    rrec = (struct ABWrec *) rdata;
    bzero((char *) rrec, sizeof (struct ABWrec));

    npackets = ntrains * npkt;
    xcnt = 1;

    fromSize = sizeof(src);
    rcnt=0;
    while (rwd->running) {
        if ((incnt = recvfrom(sock, rdata, pktsize, 0, (struct sockaddr *) &src, &fromSize)) <0) {
            if(np < npackets){
                printf("Failed: Insufficient packets received\n");
            }
        }

        gettimeofday (tp, (struct timezone *) 0);
        if (dst.sin_addr.s_addr != src.sin_addr.s_addr) {
            if ((rcnt > 38) & (rcnt < 41)) {
                fprintf(stderr,"Error: received a packet from unknown source.\n");
                printf("Send[0..%d]pkt to Server: %s port: %d\n", rcnt, inet_ntoa(dst.sin_addr), htons(dst.sin_port));
                printf("Received[0..%d]pkt  from: %s port: %d\n", rcnt, inet_ntoa(src.sin_addr), htons(src.sin_port));
            }
        }
        np = ntohs (rrec->num);
        rnum = ntohs (rrec->rnum);
        ppseq = ntohs (rrec->ppseq);
        rcnt++;
        //printf("Received[%d] %d bytes <%s> fromServer: %s:%d %d,%d,%d\n",rcnt,incnt,ptext,inet_ntoa(src.sin_addr),htons(src.sin_port),np,rnum,ppseq);    /* Print the echoed arg */
        if (np) {
            rpackets++;
            endcnt=0;
            arr_hs_snd[np]  = (double) ntohl(rrec->snd.tv_sec);
            arr_hus_snd[np] = (double) ntohl(rrec->snd.tv_usec);
            arr_hs_mir[np]  = (double) ntohl(rrec->mirr.tv_sec);
            arr_hus_mir[np] = (double) ntohl(rrec->mirr.tv_usec);
            arr_hs_rcv[np]  = (double) tp->tv_sec;
            arr_hus_rcv[np] = (double) tp->tv_usec;
            if(np == 1) {
                sprintf(ipaddr,"%s",inet_ntoa(src.sin_addr));
                ptext=rdata+80;
                strcpy(d80,ptext);
            }
        }
        if (np == npackets) {
            //		printf("Call DiMA  np=%d  rpackets=%d, xcnt=%d rpt_cnt=%d \n",np,rpackets,xcnt,rpt_cnt);
            DiMatrix(dirname, serverName, xcnt);
            rpackets=0;
            xcnt++;
            //		ms1=msdelay(ms_iter);
        }
        if (xcnt > rpt_cnt) {
            break;
        }
    }
    close(sock);
    /********************************************************/
    // do FINAL CALC

    if (iter_n > 1) {
        sumt = 0;
        sumf = 0;
        sumr = 0;
        pc = iter_n;
        if(pc > NMOD) {
            pc = NMOD;
        }
        for(i = 0; i < pc; i++) {
            ptrep=&arr_report[i+1];
            arr_t_abw[i]=ptrep->t_abw;
            arr_f_abw[i]=ptrep->f_abw;
            arr_rtt[i]=ptrep->rtt;
            sumt += ptrep->t_abw;
            sumf += ptrep->f_abw;
            sumr += ptrep->rtt;
            // printf("FIN[%d] t_abw=%5.3f,f_abw=%5.3f,rtt=%5.3f\n",i,arr_t_abw[i],arr_f_abw[i],arr_rtt[i]);
        }
        avt_abw = dMean(arr_t_abw,pc);
        avf_abw = dMean(arr_f_abw,pc);
        avr_rtt = dMean(arr_rtt,pc);

        si_t_abw = dDisp(arr_t_abw,pc,avt_abw);
        si_f_abw = dDisp(arr_f_abw,pc,avf_abw);
        si_rtt = dDisp(arr_rtt,pc,avr_rtt);

        di_t_abw = sqrt(si_t_abw);
        di_f_abw = sqrt(si_f_abw);
        di_rtt = sqrt(si_rtt);

        printf("(Avg/Sdev) RTT: %6.3f/%-6.3fms ABW To: %6.3f/%-6.3f From: %6.3f/%-6.3fMbits/s\n",avr_rtt,di_rtt,avt_abw,di_t_abw,avf_abw,di_f_abw);
    }

}

int Do_Client_Work(char * serverName, char *bindAddress, unsigned short int serverPort, int rpt_cnt)
{
	struct timeval timeout;
	struct send_work_data swd;
	struct recv_work_data rwd;
	int pid,sock;
	struct sockaddr_in dst; /* Echo server address */
	struct in_addr dst_addr;
	char *servIP;                /* IP address of server */

	pthread_t sender;
	pthread_t receiver;

    /* DNS resolution */
    dst_addr.s_addr = gethostaddr(serverName);
    if (dst_addr.s_addr ==0) {
    	fprintf (stderr,"Destination: %s: uknown host\n",serverName);
        return -1;
    }

    /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        return -1;

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
        printf(stderr, "Set Socket Option Failed\n");
        return -1;
    }

    /* Construct the server address structure */
    memset(&dst, 0, sizeof(dst));    /* Zero out structure */
    dst.sin_family = AF_INET;                 /* Internet addr family */
    dst.sin_addr = dst_addr;  /* Server addr */
    dst.sin_port   = htons(serverPort);     /* Server port */


    if(bindAddress && strlen(bindAddress) > 0){
        struct sockaddr_in b;
        bzero((char *)&b, sizeof (struct sockaddr_in));
        b.sin_family = AF_INET;
        b.sin_addr.s_addr = inet_addr(bindAddress);
        if (bind (sock, (struct sockaddr *) &b, sizeof(b)) < 0) {
            printf(stderr, "bind Failed\n");
            return -1;
        }
    }

    swd.sock = sock;
	swd.dst = dst;
	swd.rpt_cnt = rpt_cnt;

	rwd.sock = sock;
	rwd.dst = dst;
	rwd.rpt_cnt = rpt_cnt;
	rwd.serverName = serverName;
	rwd.running = 1;

    pthread_create(&sender, 0, do_send_work, &swd);
    pthread_create(&receiver, 0, do_receive_work, &rwd);

    pthread_join(sender, 0);
    rwd.running = 0;
    pthread_join(receiver, 0);
    return 0;
}
/********************************************************/
