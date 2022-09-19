// cmvc_id = %Z% %W% %I% %E% %U%

#ifndef RandGenHEADER
#define RandGenHEADER

///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Morgan Stanley & Co. Incorporated, All Rights Reserved
//
// Unpublished copyright.  All rights reserved.  This material contains
// proprietary information that shall be used or copied only within Morgan
// Stanley, except with written permission of Morgan Stanley.
//
// Module Name   : RandGen.H
// Version       : %Z% %W% %I% %E% %U%
// Project       : DataGen
// Description   : Random Number generator
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <iostream>
#include <sys/time.h>
using namespace std;

class RandNumGen
{
public:
  RandNumGen(void)
    {
      struct timeval tv;
      gettimeofday(&tv,0 );
      srand(tv.tv_sec + tv.tv_usec);
    }
  
  RandNumGen(unsigned long seed_)
    {
      srand(seed_);
    }
  
  ~RandNumGen(){}

  inline unsigned long operator() (void);
  inline int operator() (int min_, int max_);
};

// Implementation of inline functions

inline unsigned long RandNumGen::operator() (void)
{
  return rand();
}

inline int  RandNumGen::operator() (int min_, int  max_)
{
  unsigned long t = (*this)();
  int r = max_ - min_;
  return (min_ + t % r);
}
#endif

