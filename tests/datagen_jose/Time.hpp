// cmvc_id = %Z% %W% %I% %E% %U%

#ifndef TimeHEADER
#define TimeHEADER

///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Morgan Stanley & Co. Incorporated, All Rights Reserved
//
// Unpublished copyright.  All rights reserved.  This material contains
// proprietary information that shall be used or copied only within Morgan
// Stanley, except with written permission of Morgan Stanley.
//
// Module Name   : Time.H
// Version       : %Z% %W% %I% %E% %U%
// Project       : 
// Description   : 
//
///////////////////////////////////////////////////////////////////////////////
#include <iostream>

using namespace std;

class Time 
{
public:
  Time(char *startTime_);
  ~Time();
  
  Time &operator++(int);
  void adjust(void);

  int hrs(void) { return _hrs; }
  int mins(void) { return _mins; }
  int secs(void) { return _secs; }

friend ostream &operator<< (ostream &os_, Time &that_);

private:
  int _hrs, _mins, _secs;
};
#endif

