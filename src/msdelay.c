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

/*
    msdelay.c
        This is JN routine for ms delay
        (Delayloop is split between cpu and calling system function !)
        standard usleep() is unpredictable on different machines
 */

#include  <sys/types.h>
#include  <sys/times.h>
#include  <sys/time.h>
#include <stdlib.h>

double msdelay(double howmuch) {
	struct timeval *tp;
	static int i,j,k;
	double cs,cus;
	static double cas1,cas2,oldcas2,rozdil_c12,cinit,xd;

	tp = (struct timeval *) (malloc (sizeof (struct timeval)));

	gettimeofday(tp,(struct timezone *)0);
	cs = (double) tp->tv_sec;
	cus= (double) tp->tv_usec;
	cas1= cs + cus/1000000.0;
	cas2=cas1;
	oldcas2=cas2;
	rozdil_c12=0.0;
	i=0;
	while (howmuch > rozdil_c12) {
		gettimeofday(tp,(struct timezone *)0);
		cs = (double) tp->tv_sec;
		cus = (double) tp->tv_usec;
		cas2 = cs + cus/1000000.0;
		rozdil_c12=(cas2-cas1)*1000.0;
// use CPU cycle in user level to avoid overloading system calls !!
// it gives ~ 1ms delay anywhy even if system omitt do correct system call !!
		for(j=0; j<1000; j++) {
			for(k=0; k<10; k++) {
				xd=3*k/(j+1);
			}
		}
		oldcas2=cas2;
		i++;
	}
	return rozdil_c12;
}
/******************************************************************************/
