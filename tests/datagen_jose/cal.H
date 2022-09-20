#ifndef _CAL_H_
#define _CAL_H_

#include <iostream>
using namespace std;

class Calendar
{
public:
  Calendar(void);
  ~Calendar();

  unsigned int asJulianNumber(int month_, int day_, int year_);
  int isWeekday(void);
  Calendar &operator+= (int incr_);

friend ostream &operator<< (ostream &os_,Calendar &that_);

  int dayInWeek(void);
  Calendar &nextWeekday(void);
  void split(int &mo_, int &day_, int &year_);  
private:
  unsigned int _currdate;
};
#endif

