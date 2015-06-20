/******************************************************************************/
/*
   This file is part of "Abing" package.
   Abing is Available bandwidth estimation tool based on Packet-Pair Dispersion
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
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>

#include "abw.h"

extern void *fabws();
extern void *fabwr();
extern double dMean();
extern double dDisp();

/*********************************************************/
/* Global variables */
struct ABWreport arr_report[NMODMAX];
struct  sockaddr_in src,dst,fromaddr;        /* Own address/port etc. */
struct  in_addr dst_addr;               /* for resolution */

double resolution,alpha,lppb,lppbs,lctb,maxtd,mintd,old_fabw[2];
double arrS[MAXPK],arrM[MAXPK],arrR[MAXPK],arrT[MAXPK],tdr[MAXPP],tdm[MAXPP],tds[MAXPP],tdt[MAXPP],tdmx[MAXPP],tdrx[MAXPP];
double arr_hs_snd[MAXPK],arr_hus_snd[MAXPK],arr_hs_mir[MAXPK],arr_hus_mir[MAXPK],arr_hs_rcv[MAXPK],arr_hus_rcv[MAXPK];
double S_se[100],S_us[100],S_at[100],S_rt[100];
int iter_t,iter_n;
int pktsize,npackets,rpackets,ntrains,vbunches,nbunches,logfl,cafl;
int arrN[MAXPK],arrP[MAXPK];
char ipaddr[124],date[26],sstamp[24],d80[80],calhost[MAXNAME],linebuff[120];
char hostname[MAXNAME],dirname[MAXNAME],klic[MAXNAME],version[16];
char bindAddress[MAXNAME];
/*********************************************************/
void usage()
{
	fprintf (stderr, "usage: abing [-h] <more options>\n");
	fprintf (stderr, "The options are: \n");
	fprintf (stderr, "\t -d \t destination host\n");
	fprintf (stderr, "\t -B \t bind to local address\n");
	fprintf (stderr, "\t -t \t repeating period (1,2,.. sec)\n");
	fprintf (stderr, "\t -n \t repeating factor (1,5,100,.. times)\n");
	fprintf (stderr, "\t -P \t server port \n");
	fprintf (stderr, "\t -k \t run-keyword (K324a,freD,.. - same key on Reflector)\n");
	fprintf (stderr, "\t -a \t alpha factor in EWMA (default value 0.75)\n");
	fprintf (stderr, "\t -b \t number of PP bunches (default number 20)\n");
	fprintf (stderr, "\t -c \t calibration mode \n");
	fprintf (stderr, "\t -l \t dir (where ABW-node.log and monitoring results will be located)\n");
	fprintf (stderr, "\t -h \t Help: produces this output\n");
	fprintf (stderr, "\nRemarks:\n  Don't forget that abw_rfl (reflector server) must run at destination host.\n");
	fprintf (stderr, "  The problems send to the ABW author (jiri@slac.stanford.edu)\n\t\t\t\t\t\t*jn*\n");
	exit(0);
}
/*********************************************************/
void musage()
{
	fprintf (stderr, "Sorry, the minimal options are: \n");
	fprintf (stderr, "\t -d \t destination host\n");
	fprintf (stderr, "\t -h \t Help: produces all options\n");
	fprintf (stderr, "Try following:\nabing -d iepm-resp.slac.stanford.edu\n");
	fprintf (stderr, "\nRemarks:\n  Don't forget that abw_rfl (reflector server) must run at destination host.\n");
	fprintf (stderr, "  The problems send to the ABW author (jiri@slac.stanford.edu)\n\t\t\t\t\t\t*jn*\n");
	exit(0);
}
/*********************************************************/
/*********************************************************/
int main(int argc, char *argv[])
{
	struct ABWreport *ptrep;

	double sumt,sumf,avt_abw,avf_abw, dit_abw, dif_abw,di_rtt,av_rtt,
	       ms_iter,ms1,djn,
	       arr_t_abw[NMODMAX],arr_f_abw[NMODMAX],arr_rtt[NMODMAX];
	float falpha;
	int i,pc,rc,t,jn,status,xcnt;
	int concnt,incnt,outcnt,write_error,fromlen,dstlen;
	unsigned short int rrcvport,mirrport;
	char * ptr;
/*********************/
	mirrport=MIRRPORT;
	rrcvport=RECVPORT;
	strcpy(version,"abw-v-2.2.0");
	ntrains=20;
	rpackets=0;
	alpha=0.75;
	pktsize=LPP;
	lctb=LCT*8;
	maxtd=50000.0;
	mintd=0.01;
	lppb=pktsize*8;
	resolution= lppb/1000;
	jn=350;
	iter_t=1;
	iter_n=1;
	logfl=0;
	cafl=0;
	int bindFlag = 0;
	memset(bindAddress, 0, MAXNAME);

	strcpy(hostname,"machine.domaine.edu");
	strcpy(dirname,"/tmp");
	gethostname(calhost,sizeof(calhost));
	strcpy(klic,"*jn*");
/* option processing */
	if(argc <=1) {musage(); }
	argv++; argc--;
	while (argc > 0) {
		ptr = *argv;
		while (*ptr) switch (*ptr++) {

			case '-':
				break;

			case 'a':

				if (*ptr == 0) {
					argc--; argv++;
					if (*argv == 0) {
						fprintf (stderr,
						         "abing: no alpha given with '-a'.\n");
						return 1;
					}
					sscanf(*argv,"%f",&falpha);
					alpha=(double)falpha;
				} else {
					*ptr = 0;
				}
				break;

			case 'k':

				if (*ptr == 0) {
					argc--; argv++;
					if (*argv == 0) {
						fprintf (stderr,
						         "abing: no keyword given with '-k'.\n");
						return 1;
					}
					strcpy(klic,*argv);
				} else {
					*ptr = 0;
				}
				break;

			case 'P':

				if (*ptr == 0) {
					argc--; argv++;
					if (*argv == 0) {
						fprintf (stderr,
						         "abw_rfl: no value given with '-P'.\n");
						return 1;
					}
					mirrport= (unsigned short int) atoi(*argv);
				} else {
					*ptr = 0;
				}
				break;

			case 'r':

				if (*ptr == 0) {
					argc--; argv++;
					if (*argv == 0) {
						fprintf (stderr,
						         "abing: no value given with '-r'.\n");
						return 1;
					}
					rrcvport= (unsigned short int) atoi(*argv);
				} else {
					*ptr = 0;
				}
				break;

			case 'b':

				if (*ptr == 0) {
					argc--; argv++;
					if (*argv == 0) {
						fprintf (stderr,
						         "abing: no value given with '-b'.\n");
						return 1;
					}
					ntrains= atoi(*argv);
					if (ntrains > 99) ntrains=99;
				} else {
					*ptr = 0;
				}
				break;

			case 'u':

				if (*ptr == 0) {
					argc--; argv++;
					if (*argv == 0) {
						fprintf (stderr,
						         "abing: no value given with '-u'.\n");
						return 1;
					}
					maxtd = (double) atoi(*argv);
					if (maxtd > 50000.0) maxtd=50000.0;
					if (maxtd < (2*resolution)) maxtd=2*resolution;
				} else {
					*ptr = 0;
				}
				break;

			case 'p':

				if (*ptr == 0) {
					argc--; argv++;
					if (*argv == 0) {
						fprintf (stderr,
						         "abing: no value given with '-p'.\n");
						return 1;
					}
					pktsize= atoi(*argv);
					if (pktsize <100 ) pktsize=100;
				} else {
					*ptr = 0;
				}
				break;

			case 'j':

				if (*ptr == 0) {
					argc--; argv++;
					if (*argv == 0) {
						fprintf (stderr,
						         "abing: no value given with '-j'.\n");
						return 1;
					}
					jn = atoi(*argv);
				} else {
					*ptr = 0;
				}
				break;

			case 'n':

				if (*ptr == 0) {
					argc--; argv++;
					if (*argv == 0) {
						fprintf (stderr,
						         "abing: no value given with '-n'.\n");
						return 1;
					}
					iter_n= atoi(*argv);
				} else {
					*ptr = 0;
				}
				break;

			case 't':

				if (*ptr == 0) {
					argc--; argv++;
					if (*argv == 0) {
						fprintf (stderr,
						         "abing: no value given with '-t'.\n");
						return 1;
					}
					iter_t= atoi(*argv);
				} else {
					*ptr = 0;
				}
				break;

			case 'l': /* start logging */
				if (*ptr == 0) {
					argc--; argv++;
					if (*argv == 0) {
						fprintf(stderr,"No dir given with '-l'.\n");
						exit (1);
					}
					logfl=-1;
					strcpy(dirname,*argv);
				}
				break;

			case 'd':

				if (*ptr == 0) {
					argc--; argv++;
					if (*argv == 0) {
						fprintf (stderr,
						         "abing: no destination host given with '-d'.\n");
						return 1;
					}
					strcpy(hostname, *argv);
				} else {
					strcpy(hostname, ptr);
					*ptr = 0;
				}
				break;

			case 'B':
				bindFlag = 1;
				if (*ptr == 0) {
					argc--; argv++;
					if (*argv == 0) {
						fprintf (stderr,
								"abing: no local address given with '-B'.\n");
						return 1;
					}
					strcpy(bindAddress, *argv);
				} else {
					strcpy(bindAddress, ptr);
					*ptr = 0;
				}
				break;

			case 'c':

				strcpy(hostname,"localhost");
//                        pktsize= 100;
				ntrains= 100;
				cafl=1;
				break;

			case 'h': /* help */
			case 'H':
				usage();
				break;
			default:
				fprintf (stderr,
				         "abing: Unknown option '%c'\n", ptr[-1]);
				exit (1);
			}
		argc--; argv++;
	}

// once again if pktsize changed  from CMD
	lppb=pktsize*8;
	resolution= lppb/1000;
/* Rpt Client processing  */
	if(bindFlag){
		Do_Client_Work(hostname, bindAddress, mirrport, iter_n);
	} else {
		Do_Client_Work(hostname, 0, mirrport, iter_n);
	}
}
/******************************************************************************/
