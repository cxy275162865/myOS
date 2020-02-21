#ifndef _TIME_H
#define _TIME_H


struct Time {
    int sec;    // 00
    int min;    // 02
    int hour;   // 04

    int day;    // 07
    int mon;    // 08
    int year;   // 09, 32
};

struct Time time;

int get_cmos_time(struct Time *time);


#endif

