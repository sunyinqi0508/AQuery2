// cmvc_id = %Z% %W% %I% %E% %U%

#ifndef TimeIMPLEMENTATION
#define TimeIMPLEMENTATION

///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Morgan Stanley & Co. Incorporated, All Rights Reserved
//
// Unpublished copyright.  All rights reserved.  This material contains
// proprietary information that shall be used or copied only within Morgan
// Stanley, except with written permission of Morgan Stanley.
//
// Module Name   : Time.C
// Version       : %Z% %W% %I% %E% %U%
// Project       : 
// Description   : 
//
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include "Time.H"

Time::Time(char *startTime_)
{
  sscanf(startTime_,"%d:%d:%d", &_hrs, &_mins, &_secs);
  cout << "Hrs: " << _hrs << ",Mins: " << _mins << ",Secs: " << _secs << endl;
}

Time::~Time()
{}

Time &Time::operator++ (int)
{
  _secs++;
  adjust();
  return *this;
}

void Time::adjust(void)
{
  if (_secs >= 60) 
   {
     _mins += _secs/60;
     _secs = _secs%60;
   }

  if (_mins >= 60) 
   {
     _hrs += _mins/60;
     _mins = _mins%60;
   }
  if (_hrs >= 24) _hrs = _hrs = _hrs%24;
}

ostream &operator<< (ostream &os_, Time &that_)
{
  if (that_.hrs() < 10) os_ << "0";
  os_ << that_.hrs();
  os_ << ":" ;

  if (that_.mins() < 10) os_ << "0";
  os_ << that_.mins();
  os_ << ":" ;

  if (that_.secs() < 10) os_ << "0";
  os_ << that_.secs();
  return os_;
}


#endif
