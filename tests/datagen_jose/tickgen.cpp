// cmvc_id = %Z% %W% %I% %E% %U%

#ifndef histgenIMPLEMENTATION
#define histgenIMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sys/time.h>

#include "RandGen.hpp"
#include "cal.hpp"
#include "Time.hpp"

using namespace std;

inline int max(int a, int b) 
{
  return (a>b)?a:b;
}

inline int min(int a, int b) 
{
  return (a<b)?a:b;
}

int main(int ac, char *av[])
{
  RandNumGen rg;
  int i, j, d, k;
  ofstream basefile("../../data/tick-base-file.csv");
  ofstream pricefile("../../data/tick-price-file.csv");
  

  if (!basefile) 
   {
     cerr << "Cannot open base-file" << endl;
     return 1;
   }

  if (!pricefile) 
   {
     cerr << "Cannot open price-file" << endl;
     return 1;
   }

  if (ac < 2) 
   {
     cerr << "Usage: " << av[0] << " <n -scale> [t -ticks per day.Default=100] [d - no of days.Default=90]" << endl;
     return 1;
   }
  
  int n=0,t=100,days=90;
  
  if (ac >= 2) n = atoi(av[1]);
  if (ac >= 3) t = atoi(av[2]);
  if (ac >= 4) days = atoi(av[3]);
     

  int tps = (n*t)/28800; // ticks per second
  tps++;
  cout << "Ticks per second: " << tps << endl;

  // Generation of base info
  int nex = 5;
  char *ex[] = { "NY", "O", "AM", "LN", "TK"};

  int nsic = 10;
  char *sic[] = { "COMPUTERS", "CHEMICALS", "FINANCIAL", "INDUSTRIAL", "PHARMACEUTICALS",
                       "MEDICAL", "BANKING", "SOFTWARE", "ENTERTAINMENT", "CONSTRUCTION" };
  
  char *cu[] = { "USD", "DEM", "JPY", "FFR", "GBP"};
  int ncu = 5;
  
  char *spr[] = { "AAA", "AA", "A", "BBB", "BB", "B", "CCC", "CC", "C"};
  int nspr = 9;
  

  unsigned int rnum;
  char id[100];
  char descr[256];
  char *crdate = "3/11/1999";

  basefile << "Id | Ex | Descr | SIC | Cu" << endl;
  
  for (i=0; i<n; i++) 
   {
     sprintf(id,"Security_%d", i);
     sprintf(descr, "'Financial security number: %d'", i);
     
     basefile << id;
     basefile << " | " << ex[rg(0,nex)];
     basefile << " | " << descr;
     basefile << " | " << sic[rg(0,nsic)];
     basefile << " | " << cu[rg(0, ncu)];
     basefile << endl;
   }
  basefile.close();

  
  // generation of price info
  double tick=1.0/32.0;
  
  // 1. gen the starting price of each security
  double *bp = new double[n];
  int *seq = new int[n];
  
  for (i=0;i<n;i++)
   {
     bp[i] = rg(0,100);
     seq[i] = 0;
   }
  
  Calendar cal;
  Time tm("9:00:00");
  cal.nextWeekday();
  
  pricefile << "Id | SeqNo | TradeDate | TimeStamp | Type" << endl;
  
  for (k=0;k<days; k++)
   {
     // for each second of the business day - 8*60*60
     for(i=0;i<28800;i++) 
      {
        // generate the required ticks
        for(j=0;j<tps;j++) 
         {
           //1. select a security
           int sec = rg(0,n);
           sprintf(id, "Security_%d", sec);
           
           //2.  select  if it is a trade, ask, bid
           int tqb = rg(0,4);
           switch (tqb) 
            {
            case 0: // trade
            {
              double tp = bp[sec];
              int ts = rg(1,100) * 100;
              pricefile << id;
              pricefile << "|" << ++seq[sec];
              pricefile << "|" << cal;
              pricefile << "|" << tm;
              pricefile << "|T" << endl;
              break;
            }
            case 1: // ask
            {
              double ap = rg(0,4)*tick+bp[sec];
              int as = rg(1,100) * 100;
              pricefile << id;
              pricefile << "|" << ++seq[sec];
              pricefile << "|" << cal;
              pricefile << "|" << tm;
              pricefile << "|Q" << endl;
              break;
            }
            case 2: // bid
            {
              int dir = (rg(3,10) > 5)? +1:-1;
              bp[sec] = (dir*rg(0,3)*tick)+bp[sec];
              int bs = rg(1,100) * 100;
              pricefile << id;
              pricefile << "|" << ++seq[sec];
              pricefile << "|" << cal;
              pricefile << "|" << tm;
              pricefile << "|Q" << endl;
              break;
            }
            case 3: // cancel/correct as a trade
            {
              if (rg(0,100)  < 5) break;
              double tp = bp[sec];
              int ts = rg(1,100) * 100;
              pricefile << id;
              pricefile << "|" << ++seq[sec];
              pricefile << "|" << cal;
              pricefile << "|" << tm;
              pricefile << "|CT" << endl;
              break;
            }
            default:
              break;
            }
         }
        // Go to the next second
        tm++;
      }         
     cal.nextWeekday();
   }
  pricefile.close();
}


#endif
