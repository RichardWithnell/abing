/*****************************************************************************/
/*
This file is part of "Abing" package.
ABing is Available bandwidth estimation tool based on Packet-Pair Dispersion
Technique.
Copyright (C) 2003 Jiri Navratil
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details (COPYING).
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
ABing is result of the research in frame of the Project INCITE (Edge-based
Traffic Processing and Service Interface for High-Performance Network)
It is common project of RICE University, Los Alamos National Labs and
SLAC - Stanford Linear Accelerator (see http://incite.rice.edu)
supported from DoE in frame of SCIDAC program.
*/
/*****************************************************************************/

/*
abw_rfl.c
This program is packets reflector running on the remote host
It receives packets from the sender adds its own timestamp
and sends packet back to the sender. For backward measurements
regenerate packets into back-to-back form.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#define MAXSEQ 100
//int exit ();
/**************************************/
#include "abw.h"

void usage()
{
    fprintf (stderr, "usage: abw_rfl &\n");
    fprintf (stderr, "The options are: \n");
    fprintf (stderr, "\t -c \t calibration mode \n");
    fprintf (stderr, "\t -k \t run-keyword\n");
    fprintf (stderr, "\t -s \t 0,1..n (server mode: default 0 - running in continues mode) \n");
    fprintf (stderr, "\t -m \t reflector port number (default 8176)\n");
    fprintf (stderr, "\t -h \t Help: produces this output\n");
    fprintf (stderr, "The problems send to the ABing author (jiri@slac.stanford.edu)\n\t\t\t\t\t\t*jn*\n");

    exit(1);
}
/**************************************/
/**************************************/
int main(int argc, char * argv[])
{
    FILE *caf;

    struct	sockaddr_in Server;	/* Own address/port etc. */
    struct	sockaddr_in Client;	/* Reciver's address/port */
    struct	timeval *tp;		/* local timestamp */
    struct	ABWrec *abwrec;		/* log record for the packet */
    struct	ABWrec *ptrec;		/* log record for the packet */
    struct	ABWrec arec[MAXSEQ];	/* ARRAY of record for the packet */
    int	cli_len;		/* size of sender's address/port */
    int	so1;			/* socket id for incoming pkts */
    int	so2;			/* socket id for outgoing pkts */
    int	rcv_size = MAXBUFF;	/* socket receive buffer size */
    uint16_t	np;			/* sender's packet number */
    uint16_t	np1;			/* sender's P1packet number */
    uint16_t	np2;			/* sender's P2packet number */
    uint16_t	train;			/* packets in sequence */
    uint16_t	nrcv = 0;		/* number of packets rcvd */
    int 	i,lindex;
    int 	okfl=0;
    int 	sqfl=0;
    int 	cafl=0;
    int 	stfl=0;
    int 	stflc=0;
    int 	endcnt=0;
    int 	totcnt=0;
    int 	seqno1,nwpp,len;
    static 	int incnt, outcnt;
    int pktsize = 1450;
    double sleeptime;
    float  calfactor;
    unsigned short int rcvport,rrcvport,mirrport;
    char	*ptr,*ptdata,*ptext,*err;
    char	vzor[16],version[16],sstamp[20],ca_filename[80],buffer[80],calhost[MAXNAME],data[MAXMESG];
    /***********************************************************************/
    tp = (struct timeval *)(malloc (sizeof (struct timeval)));
    strcpy(vzor,"*jn*");
    strcpy(version,"abw-v-2.2.0");
    mirrport=MIRRPORT;
    rrcvport = RECVPORT;
    rcvport = RECVPORT;

    argc--; argv++;

    /* go through the arguments */
    while (argc > 0) {
        ptr = *argv;
        while (*ptr) switch (*ptr++) {
            case '-':
                break;
            case 'k':
                if (*ptr == 0) {
                    argc--; argv++;
                    if (*argv == 0) {
                        fprintf (stderr,
                            "abw_rfl: no keyword given with '-k'.\n");
                        return 1;
                    }
                    strcpy(vzor,*argv);
                } else {
                    strcpy(vzor, ptr);
                    *ptr = 0;
                }
                break;
            case 'c':
                cafl=1;
                break;
            case 's':
                if (*ptr == 0) {
                    argc--; argv++;
                    if (*argv == 0) {
                        fprintf (stderr,
                            "abw_rfl: no value given with '-s'.\n");
                        return 1;
                    }
                    stfl = 1;
                    stflc = atoi(*argv);
                } else {
                    *ptr = 0;
                }
                break;

            case 'm':

            if (*ptr == 0) {
                argc--; argv++;
                if (*argv == 0) {
                    fprintf (stderr,
                        "abw_rfl: no value given with '-m'.\n");
                    return 1;
                }
                mirrport= (unsigned short int) atoi(*argv);
            } else {
                *ptr = 0;
            }
            break;

            default:
                fprintf (stderr,
                    "abw_rfl: Unknown option '%c'\n", ptr[-1]);

                case 'h':		/* help */
                case 'H':
                usage();
                break;
        }
        argc--; argv++;
    }
    gethostname(calhost,sizeof(calhost));
    sprintf(ca_filename,".%s.cal",calhost);
    if (!(cafl)) {
        if(NULL == (caf = fopen(ca_filename,"rb"))) {
            printf("Cann't open initfile %s.\n",ca_filename);
            printf("Please, run \nabw_rfl -c &  and abing -c \n   - on this host \n   - from the directory in which you want to run abw_rfl permanently\n");
            printf("After this re-run abw_rfl& \n");
            exit(1);
        }
        while ((err=fgets(buffer,80,caf))!=NULL) {
            len=sscanf(buffer,"%f",&calfactor);
            //			printf("Local_loop_factor: %5.1f\n",calfactor);
        }
        fclose (caf);
    }
    ptrec = &arec[0];
    ptext=(char *)ptrec+80;
    sprintf(ptext,"%5.1f",calfactor);
    ptrec = &arec[1];
    ptext=(char *)ptrec+80;
    sprintf(ptext,"%5.1f",calfactor);
    //	printf("CAL-R-factor: %s\n",ptext);

    abwrec = (struct ABWrec *) data;
    cli_len = sizeof (Client);
    so1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (so1 < 0) {
        perror("abw_rfl: socket");
        exit(1);
    }
    so2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (so2 < 0) {
        perror("abw_rfl: socket");
        exit(1);
    }
    bzero((char *)&Server, sizeof (Server));
    Server.sin_family = AF_INET;
    Server.sin_port = htons(mirrport);

    if (bind (so1, (struct sockaddr *) &Server, sizeof(Server)) < 0) {
        perror ("abw_rfl: bind");
        exit (1);
    }

    bzero((char *)&Client, sizeof (Client));
    Client.sin_family = AF_INET;
    Client.sin_port = htons(RECVPORT);

    train = 2;

    while (1) {
        incnt = recvfrom (so1, data, MAXMESG, 0,
        (struct sockaddr *)&Client, &cli_len);
        if (incnt < 0) {
            perror ("abw_rfl: read");
            break;
        }
        gettimeofday (tp, (struct timezone *) 0);
        nrcv++;
        //      rrcvport = ntohs(abwrec->rrcvport);
        //      rcvport = rrcvport;
        //	Client.sin_port = htons(rcvport);
        np = ntohs (abwrec->num);
        if(np==1) {
            Client.sin_addr = Client.sin_addr;
            Client.sin_port = Client.sin_port;
        }

        // backward compatibility (don't know what is there)
        train=2;
        lindex=np%(train);
        ptrec = &arec[lindex];
        ptrec->mirr.tv_sec = htonl (tp->tv_sec);
        ptrec->mirr.tv_usec = htonl (tp->tv_usec);
        ptrec->rnum = htons (nrcv);
        ptrec->num = abwrec->num;

        //printf("Reflector Received Packet: %d\n", ntohs (abwrec->num));

        ptrec->ppseq = abwrec->ppseq;
        ptrec->snd.tv_sec = abwrec->snd.tv_sec;
        ptrec->snd.tv_usec = abwrec->snd.tv_usec;
        ptext=data+124;
        okfl=strcmp(ptext,vzor);
        ptext=(char *)ptrec+88;
        strcpy(ptext,version);
        np2=np;
        if(!(lindex)){
            np2 = np;

            if(seqno1 == abwrec->ppseq) {
                sqfl=1;
            } else {
                sqfl=0;
            };

            if(1) {
                for (i=1;i<train;i++) {
                    ptrec = &arec[i];
                    ptdata= (char *) ptrec;
                    outcnt = sendto (so1, ptdata, pktsize, 0,
                    (struct sockaddr *)&Client, sizeof (Client));
                    if (outcnt < 0) perror ("RFL sendto:");
                }
                i=0;
                ptrec = &arec[i];
                ptdata= (char *) ptrec;
                outcnt = sendto (so1, ptdata, pktsize, 0,
                (struct sockaddr *)&Client, sizeof (Client));
                if (outcnt < 0) perror ("abw_rfl echo: sendto");
                endcnt=0;
                if ((cafl)&(np>199)) {
                    printf("Init finished OK: np=%d\nNow you can run abw_rfl&\n",np);
                    exit(0);
                }

            } else {
                sprintf(sstamp,"%d",(int)(tp->tv_sec));
                printf("Seq. or Key error T: %s from: %s\n",sstamp,inet_ntoa(Client.sin_addr));
                nwpp=(np2 - np1)/2;
                printf ("PP report: %d wrong PPairs, seqno=%d,p1=%d,p2=%d,okfl=%d,sqfl=%d\n",nwpp,ntohs(seqno1),np1,np2,okfl,sqfl);
            }
            if((stfl)&(nrcv>=stflc*40)) {
                exit(0);
            }
        } else {
            np1=np;
            seqno1= abwrec->ppseq;
            //		printf("ABW_RFL send: to %s:%d\n",inet_ntoa(Client.sin_addr),htons(Client.sin_port));
        }
    } /*endwhile*/
    exit(0);
}
