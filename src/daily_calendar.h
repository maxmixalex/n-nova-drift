#pragma once
#include <stdint.h>

namespace nova {
struct CalendarDate { unsigned year,month,day; };
inline bool calendar_leap(unsigned year) {return year%4==0&&(year%100!=0||year%400==0);}
inline unsigned calendar_days(unsigned year,unsigned month) {
    static constexpr unsigned days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    return month>=1&&month<=12?days[month-1]+unsigned(month==2&&calendar_leap(year)):0;
}
inline uint32_t calendar_day(CalendarDate date) {
    if(date.year<2020||date.year>2099||!date.day||date.day>calendar_days(date.year,date.month))return 0;
    uint32_t result=0;
    for(unsigned year=1970;year<date.year;++year)result+=365+unsigned(calendar_leap(year));
    for(unsigned month=1;month<date.month;++month)result+=calendar_days(date.year,month);
    return result+date.day-1;
}
inline CalendarDate calendar_date(uint32_t day) {
    if(day<18262||day>47481)return CalendarDate{2026,1,1};
    CalendarDate result{1970,1,1};
    while(day>=365+unsigned(calendar_leap(result.year)))day-=365+unsigned(calendar_leap(result.year++));
    while(day>=calendar_days(result.year,result.month))day-=calendar_days(result.year,result.month++);
    result.day=day+1;return result;
}
inline void calendar_text(char (&text)[11],uint32_t day) {
    CalendarDate date=calendar_date(day);
    text[0]=char('0'+date.year/1000);text[1]=char('0'+date.year/100%10);
    text[2]=char('0'+date.year/10%10);text[3]=char('0'+date.year%10);text[4]='-';
    text[5]=char('0'+date.month/10);text[6]=char('0'+date.month%10);text[7]='-';
    text[8]=char('0'+date.day/10);text[9]=char('0'+date.day%10);text[10]=0;
}
}
