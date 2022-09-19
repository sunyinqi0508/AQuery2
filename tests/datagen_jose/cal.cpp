#include <sys/types.h>
#include <time.h>
#include <iostream>

#include "cal.H"

Calendar::Calendar(void)
{
  time_t clk = time(0);
  struct tm *now = localtime(&clk);
  _currdate = asJulianNumber(now->tm_mon+1, now->tm_mday, now->tm_year+1900);
}

Calendar::~Calendar()
{}

// year_ in yyyy format
unsigned int Calendar::asJulianNumber(int month_,int day_,int year_)
{
  unsigned long c,ya;
  
  if (month_>2) month_-=3;
  else { month_+=9; year_--; } 
  c=year_/100;
  ya=year_-100*c;
  return ((146097*c)>>2)+((1461*ya)>>2)+(153*month_+2)/5+day_+1721119;
} 

void Calendar::split(int& month_,int& day_,int& year_)
{
  unsigned long d;
  unsigned long j=_currdate-1721119;
  year_=(int) (((j<<2)-1)/146097);
  j=(j<<2)-1-146097*year_;
  d=(j>>2);
  j=((d<<2)+3)/1461;
  d=(d<<2)+3-1461*j;
  d=(d+4)>>2;
  month_=(int)(5*d-3)/153;
  d=5*d-3-153*month_;
  day_=(int)((d+5)/5);
  year_=(int)(100*year_+j);
  if (month_<10) month_+=3;
  else { month_-=9; year_++; } 
} 

int Calendar::dayInWeek(void)
{ 
  return ((((_currdate+1)%7)+6)%7)+1; 
}

Calendar &Calendar::nextWeekday(void)
{
  (*this) += 1;
  while (!isWeekday()) (*this)+= 1;
  return *this;
}

int Calendar::isWeekday(void)
{
  return (dayInWeek()<6)?1:0;
}

Calendar &Calendar::operator+= (int incr_)
{
  _currdate += incr_;
  return *this;
}

ostream &operator<< (ostream &os_, Calendar &that_)
{
  int mo, day, year;
  that_.split(mo,day,year);
  os_ << mo << "/" << day << "/" << year;
  return os_;
}
