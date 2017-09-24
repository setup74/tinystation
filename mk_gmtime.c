/*
 * (C)2012 Michael Duane Rice All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer. Redistributions in binary
 * form must reproduce the above copyright notice, this list of conditions
 * and the following disclaimer in the documentation and/or other materials
 * provided with the distribution. Neither the name of the copyright holders
 * nor the names of contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* $Id$ */

/*
    'Break down' a y2k time stamp into the elements of struct tm.
    Unlike mktime(), this function does not 'normalize' the elements of timeptr.

*/

#include <inttypes.h>
#include <time.h>

/**
 Enumerated labels for the days of the week.
*/
enum _WEEK_DAYS_ {
    SUNDAY,
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY
};

/**
    Enumerated labels for the months.
*/
enum _MONTHS_ {
    JANUARY,
    FEBRUARY,
    MARCH,
    APRIL,
    MAY,
    JUNE,
    JULY,
    AUGUST,
    SEPTEMBER,
    OCTOBER,
    NOVEMBER,
    DECEMBER
};

/** One hour, expressed in seconds */
#define ONE_HOUR 3600

/** Angular degree, expressed in arc seconds */
#define ONE_DEGREE 3600

/** One day, expressed in seconds */
#define ONE_DAY 86400

/** Difference between the Y2K and the UNIX epochs, in seconds. To convert a Y2K
    timestamp to UNIX...
 */
#define UNIX_OFFSET 946684800

/** Difference between the Y2K and the NTP epochs, in seconds. To convert a Y2K
    timestamp to NTP...
    */
#define NTP_OFFSET 3155673600

unsigned char
is_leap_year(int year)
{
    /* year must be divisible by 4 to be a leap year */
    if (year & 3)
        return 0;

    /* If theres a remainder after division by 100, year is not divisible by 100 or 400 */
    if (year % 100)
        return 1;

    /* If the quotient is divisible by 4, then year is divisible by 400 */
    if (year % 400 == 0)
        return 1;

    return 0;
}

time_t
mk_gmtime(const struct tm * timeptr)
{

    time_t          ret;
    uint32_t        tmp;
    int             n, m, d, leaps;

    /*
        Determine elapsed whole days since the epoch to the beginning of this year. Since our epoch is
        at a conjunction of the leap cycles, we can do this rather quickly.
        */
    n = timeptr->tm_year - 100;
    leaps = 0;
    if (n) {
        m = n - 1;
        leaps = m / 4;
        leaps -= m / 100;
        leaps++;
    }
    tmp = 365UL * n + leaps;

    /*
                Derive the day of year from month and day of month. We use the pattern of 31 day months
                followed by 30 day months to our advantage, but we must 'special case' Jan/Feb, and
                account for a 'phase change' between July and August (153 days after March 1).
            */
    d = timeptr->tm_mday - 1;   /* tm_mday is one based */

    /* handle Jan/Feb as a special case */
    if (timeptr->tm_mon < 2) {
        if (timeptr->tm_mon)
            d += 31;

    } else {
        n = 59 + is_leap_year(timeptr->tm_year + 1900);
        d += n;
        n = timeptr->tm_mon - MARCH;

        /* account for phase change */
        if (n > (JULY - MARCH))
            d += 153;
        n %= 5;

        /*
         * n is now an index into a group of alternating 31 and 30
         * day months... 61 day pairs.
         */
        m = n / 2;
        m *= 61;
        d += m;

        /*
         * if n is odd, we are in the second half of the
         * month pair
         */
        if (n & 1)
            d += 31;
    }

    /* Add day of year to elapsed days, and convert to seconds */
    tmp += d;
    tmp *= ONE_DAY;
    ret = tmp;

    /* compute 'fractional' day */
    tmp = timeptr->tm_hour;
    tmp *= ONE_HOUR;
    tmp += timeptr->tm_min * 60UL;
    tmp += timeptr->tm_sec;

    ret += tmp;

    return ret;
}

