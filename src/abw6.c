/******************************************************************************
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
******************************************************************************/

/*
	abw6.c - The dispersion time analysis subroutines
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <math.h>

#include "abw.h"

/**************************************************************/
extern struct ABWreport arr_report[];
extern double resolution,alpha,lppb,lctb,maxtd,mintd,old_fabw[];
extern double arrS[],arrM[],arrR[],arrT[],tdr[],tdm[],tds[],tdt[],tdmx[],tdrx[];
extern double arrM[],arrR[],arrT[],tdr[],tdm[];
extern double arr_hs_snd[],arr_hus_snd[],arr_hs_mir[],arr_hus_mir[],arr_hs_rcv[],arr_hus_rcv[];
extern int pktsize,npackets,ntrains,rpackets,vbunches,nbunches,logfl,cafl;
extern char ipaddr[],date[],sstamp[],d80[],calhost[],linebuff[];

double dDisp(double *pta, int n, double xs)
{
	int i;
	double a,di,sum,mean;
	sum=0.0;
	for (i=0; i<n; i++) {
		a=*(pta+i);
		di=(xs-a)*(xs-a);
		sum +=di;
	}
	mean=sum/n;
	return mean;
}

double dMean(double *pta, int n)
{
	int i;
	double a,sum,mean;
	sum=0.0;
	for (i=0; i<n; i++) {
		a=*(pta+i);
		sum +=a;
	}
	mean=sum/n;
	return mean;
}

double dSump(double *pta, double l, double h, int n, char *what)
{
	int i,cnt;
	double a,sum;
	sum=0.0;
	cnt=0;
	for (i=0; i<n; i++) {
		a=*(pta+i);
		if((a>l)&(a<h)) {
			sum +=a;
		}
	}
	return sum;
}

double dSum(double *pta, int n)
{
	int i,cnt;
	double a,sum;
	sum=0.0;
	cnt=0;
	for (i=0; i<n; i++) {
		a=*(pta+i);
		sum +=a;
	}
	return sum;
}

double dMeanp(double *pta, double l, double h, int n)
{
	int i,cnt;
	double a,sum,mean;
	sum=0.0;
	cnt=0;
	for (i=0; i<n; i++) {
		a=*(pta+i);
		if((a>l)&(a<h)) {
			sum +=a;
			cnt++;
		}
	}
	mean=sum/cnt;
	return mean;
}

double dMax(double *pta, int n)
{
	int i;
	double a,max;
	max=*pta;
	for (i=1; i<n; i++) {
		a=*(pta+i);
		if(a > max) {
			max=a;
		}
	}
	return max;
}

double dMin(double *pta, int n)
{
	int i;
	double a,min;
	min=*pta;
	for (i=1; i<n; i++) {
		a=*(pta+i);
		if(a < min) {
			min=a;
		}
	}
	return min;
}

int xMax(double *pta, int n)
{
	int i,ix;
	double a,max;
	max=*pta;
	ix=0;
	for (i=0; i<n; i++) {
		a=*(pta+i);
		if(a > max) {
			max=a;
			ix=i;
		}
	}
	if ((ix > nbunches)|(ix<0)) {
		return 0;
	} else {
		return ix;
	}
}

void Abw6(double* td, char * tag, int itag, int rcnt, double rtt)
{
	struct  ABWreport *ptrep;
	double sumpp,meanall,pp,resolution,dil,newdil,cxm,cxl,cxa,fabw;
	double peak1,peak2,lowpeak,mpeak,rangel,rangeh;
	double pptraffic,ctbtime,ppbtime,nxcttime,busytime,qlength,
	       rog2,xtg2,cg2,*ptpk;
	double avr_rtt,dis_rtt,sig_rtt,min_rtt,max_rtt;

	double arrp1[MAXPP],arrp2[MAXPP];
	int i,hi1,hi2,maxh,bin,mbin,ipx1,ipx2,p1int,p2int,mcnt,
	    lpix,lpin,peaklevel;
	int h[MAXMBIN];

	mcnt=rcnt%NMOD;
	ptrep=&arr_report[mcnt];
	sumpp = 0.0;
	//	resolution =11;
	peaklevel=6;
	i=xMax(td,nbunches);
	sumpp=dSump(td,mintd,maxtd,nbunches,tag);
	meanall=dMeanp(td,mintd,maxtd,nbunches);
	newdil=sumpp/130;
	if(newdil > resolution) {
		dil= newdil;
	} else {
		dil=resolution/5;
	}
	mbin= (int) (sumpp/dil)+1;
	cxa=lppb/meanall;

	for (i=0; i<MAXMBIN; i++) {
		h[i]=0;
	}
	for (i=0; i<nbunches; i++) {
		pp=*(td+i);
		if((pp >= resolution)&(pp< maxtd)) {
			bin= (int) (pp/dil);
			if(bin > MAXMBIN) {
				bin=MAXMBIN-1;
			}
			h[bin]++;
			if (bin > mbin) {
				mbin=bin;
			}
		}
	}
	/* peak 1 process   */
	maxh=h[0];
	hi1=0;
	hi2=0;
	for (i=0; i<mbin; i++) {
		if(h[i] >= maxh) {
			maxh=h[i];
			hi1=i;
		}
	}
	if (hi1 > 0) {
		if (h[hi1] >=peaklevel) {
			rangel=((double)hi1)*dil;
			rangeh=rangel+dil;
		} else {
			if ((h[hi1]+h[hi1+1]+h[hi1-1]) >=peaklevel) {
				rangel=((double)hi1 - 1)*dil;
				rangeh=rangel+dil+dil+dil;
			} else {
				rangel=0.0;
				rangeh=maxtd;
			}
		}
	} else {
		if(( hi1==0)&(h[hi1]>=peaklevel)) {
			rangel=0.0;
			rangeh=rangel+dil;
		} else {
			rangel=0;
			rangeh=maxtd;
		}
	}
	ipx1=0;
	for(i=0; i<nbunches; i++) {
		pp=*(td+i);
		if ((pp >= rangel)&(pp<rangeh)) {
			arrp1[ipx1]=*(td+i);
			ipx1++;
		}
	}
	ptpk=&arrp1[0];
	p1int=ipx1;
	peak1=dMean(ptpk,ipx1);
	cxm=lppb/peak1;

/* peak 2 process   */
	peak2=(double)0;
	if((ipx1 < 15)|(h[hi1]<15)) {
		if(ipx1 < 8) {
			h[hi1]=0;
			h[hi1+1]=0;
			h[hi1-1]=0;
		} else  {
			h[hi1]=0;
		}
		maxh=h[0];
		hi2=0;
		for (i=0; i<mbin; i++) {
			if(h[i] >= maxh) {
				maxh=h[i];
				hi2=i;
			}
		}
		if(hi2 < hi1) {
			rangel=((double)hi2)*dil;
			rangeh=rangel+dil;
			ipx2=0;
			for(i=0; i<nbunches; i++) {
				pp=*(td+i);
				if ((pp >= rangel)&(pp<rangeh)) {
					arrp2[ipx2]=*(td+i);
					ipx2++;
				}
			}
			ptpk=&arrp2[0];
			p2int=ipx2;
			peak2=dMean(ptpk,ipx2);
			cxl=lppb/peak2;
		}
		if (h[hi2] < 5) {
			peak2=(double)-1.0;
		}
	}
/* after peak ident */
	mpeak=peak1;
	lowpeak=peak1;
	lpix=hi1;
	lpin=h[hi1];
	cxm=lppb/mpeak;
	cxl=cxm;
	if (peak2>0) {
		if (hi2 < hi1) {
			lowpeak=peak2;
			lpix=hi2;
			lpin=h[hi2];
			cxl=lppb/lowpeak;
		}
	}
/* Fin calculations */
	ctbtime=lctb/cxl;
	ppbtime=lppb/cxl;

	nxcttime=nbunches*ctbtime;
	pptraffic=nbunches*ppbtime;
	busytime=abs (sumpp-pptraffic);
	qlength=busytime/nxcttime;
	rog2= qlength/(1+qlength);
	xtg2= rog2*cxl;
	cg2=cxl-xtg2;
// Ext. situation !
	if (cxl<0.95*cxa) {
		xtg2=cxa;
		cg2=cxa-cxl;
	}
	if (rcnt <2 ) {old_fabw[itag]=cg2; }
	fabw=(1.0 - alpha)*cg2+alpha*old_fabw[itag];
// JUNE 11, 2004
	avr_rtt=dMean(tdt,nbunches);
	min_rtt=dMin(tdt,nbunches);
	max_rtt=dMax(tdt,nbunches);

	// nbunches+1 because 1st was rejected from analysis
	if (cg2 <1.0) {
		sprintf(linebuff,"%s %s: %s ABw-Xtr-DBC: %5.3f %5.3f %5.3f ABW: %5.3f Mbps RTT: %3.3f %3.3f %3.3f ms %d %d\n",sstamp,tag,ipaddr,cg2,xtg2,cxl,fabw,min_rtt,avr_rtt,max_rtt,npackets/2,vbunches);
	} else {
		sprintf(linebuff,"%s %s: %s ABw-Xtr-DBC: %5.1f %5.1f %5.1f ABW: %5.1f Mbps RTT: %3.3f %3.3f %3.3f ms %d %d\n",sstamp,tag,ipaddr,cg2,xtg2,cxl,fabw,min_rtt,avr_rtt,max_rtt,npackets/2,vbunches);
	}
	old_fabw[itag]=fabw;
	if (!strcmp(tag,"T")) {
		ptrep->rindex=mcnt;
		ptrep->t_abw=cg2;
		ptrep->t_xtr=xtg2;
		ptrep->t_dbc=cxl;
		ptrep->rtt=avr_rtt;
	} else {
		ptrep->f_abw=cg2;
		ptrep->f_xtr=xtg2;
		ptrep->f_dbc=cxl;
		ptrep->rtt=avr_rtt;
	}
}

int DiMatrix(char * dirname, char * localhost, int rpt_cnt)
{
	struct thread_args *apt;
	int tid,rdcnt=0;
	FILE   *Afile,*Lfile,*Cfile;

	struct  timeval now;
	double  *ptdr,*ptdm,*ptds,*ptdmx,ppdiffs,ppdiffm,ppdiffr,
	         ss,ssa,ssm,rtt,rttx,av_rtt,pktloss,problems,dfactor;

	float calfactor;
	int i,jx,jy,np,xpackets;
	int endcnt=0,okfl=0, modcnt=0;
	int discnt=0,miscnt=0,delcnt=0, len;
	char logname[MAXNAME],abwname[MAXNAME],calname[MAXNAME],
	     dtag[12],version[16],cabuff[12],what[12];

	resolution=lppb/1000.0;
	len=sscanf(d80,"%f",&calfactor);

	dfactor=(double)calfactor;
	if (resolution < dfactor)
	{resolution=dfactor; }


	ptdm=&tdm[0];
	ptdr=&tdr[0];
	ptds=&tds[0];
	ptdmx=&tdmx[0];

	sprintf(logname,"%s/%s-%s.log",dirname,"ABW",localhost);
	jx=0;
	if (rpackets > npackets) {xpackets=npackets; } else {xpackets=rpackets; }
	for (i=1; i<=xpackets; i++) {
		arrS[i]=(arr_hs_snd[i]  + arr_hus_snd[i]/1000000.0);
		arrM[i]=(arr_hs_mir[i]  + arr_hus_mir[i]/1000000.0);
		arrR[i]=(arr_hs_rcv[i]  + arr_hus_rcv[i]/1000000.0);
	}
	// REJECT 1 PP - it is unreliable because of system
	// count start 1 (PP 1,2 for second is valid i =4
	for (i=4; i<=xpackets; i++) {

		ppdiffs=1000000 * (arrS[i]-arrS[i-1]);
		ppdiffm=1000000 * (arrM[i]-arrM[i-1]);
		ppdiffr=1000000 * (arrR[i]-arrR[i-1]);

		if (!(i%2)) {
			if ((ppdiffm >mintd) & (ppdiffm<maxtd)) {

				if (ppdiffm < resolution) {
					tdm[jx]=resolution;
				} else { tdm[jx]=ppdiffm; }
				tdmx[jx]=ppdiffm;

				if (ppdiffr < resolution) {
					tdr[jx]=resolution;
				} else {
					tdr[jx]=ppdiffr;
				}
				tdrx[jx]=ppdiffr;
				tds[jx]=ppdiffs;

				rttx=(arrR[i]-arrS[i])*1000.0;
				rtt=(arrR[i]-arrS[i])* 1000.0 - ppdiffm/1000.0;
				tdt[jx]=rtt;
				jx++;
				nbunches=jx;
			}
		}
	}
	if (nbunches > 1) {
		vbunches=nbunches+1;
	} else {
		vbunches=nbunches;
	}

	for (i=2; i<=xpackets; i++) {
		if (arrS[i]==0.0) {
			miscnt++;
		} else {
			if (arrS[i-1] > arrS[i]) {
				discnt++;
			}
		}
	}

	gettimeofday(&now, NULL);

	sprintf(sstamp,"%ld",now.tv_sec);
	strcpy(dtag,"T");

	if(cafl) {
		sprintf(calname,".%s.cal",calhost);
		ss=dSump(ptdmx,0,3000,nbunches,dtag);
		ssa=dMeanp(ptdmx,0,3000,nbunches);
		ssm=dMin(ptdmx,nbunches);
		ss=(ssa+ssm)/4.0;
		printf("Local_loop_factor: %5.1f\n",ss);
		sprintf(cabuff,"%5.1f\n",ss);
		if(NULL == (Cfile = fopen(calname,"wb"))) {
			printf("Cann't open file: %s\n",calname);
			return -1;
		}
		fputs(cabuff,Cfile);
		fclose (Cfile);
		return -1;
	}
	Abw6(ptdm,dtag,0,rpt_cnt,rtt);
	if(!logfl) { printf("%s",linebuff); }
	sprintf(abwname,"%s/%s-%s.%s",dirname,dtag,ipaddr,"txt");
	if (logfl) {
		if(NULL == (Afile = fopen(abwname,"wb"))) {
			printf("Cann't open outfile %s.\n",abwname);
		}
		fputs(linebuff,Afile);
		fclose (Afile);
		//append to log.file
		if(NULL == (Lfile = fopen(logname,"a+"))) {
			printf("Cann't open outfile %s.\n",logname);
		}
		fputs(linebuff,Lfile);
	}

	if(!okfl) {
		strcpy(dtag,"F");
		Abw6(ptdr,dtag,1,rpt_cnt,rtt);
		if(!logfl) { printf("%s",linebuff); }
		if (logfl) {
			sprintf(abwname,"%s/%s-%s.%s",dirname,dtag,ipaddr,"txt");
			if(NULL == (Afile = fopen(abwname,"wb"))) {
				printf("Cann't open outfile %s.\n",abwname);
			}
			fputs(linebuff,Afile);
			fclose (Afile);
			fputs(linebuff,Lfile);
		}
	}

	if (logfl) {
		fclose (Lfile);
	}

	for (i=1; i<npackets; i++) {
		arr_hs_snd[i];
		arr_hus_snd[i];
		arr_hs_mir[i];
		arr_hus_mir[i];
		arr_hs_rcv[i];
		arr_hus_rcv[i];
		arrS[i]=0.0;
		arrM[i]=0.0;
		arrR[i]=0.0;
	}
	rdcnt++;

	return 0;
}
/******************************************************************************/
