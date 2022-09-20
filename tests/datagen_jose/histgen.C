// cmvc_id = %Z% %W% %I% %E% %U%

#ifndef histgenIMPLEMENTATION
#define histgenIMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sys/time.h>

#include "RandGen.H"
#include "cal.H"
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
  ofstream basefile("../../data/hist-base-file.csv");
  ofstream pricefile("../../data/hist-price-file.csv");
  ofstream splitfile("../../data/hist-split-file.csv");
  ofstream dividendfile("../../data/hist-dividend-file.csv");

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

  if (!splitfile)
   {
     cerr << "Cannot open split-file" << endl;
     return 1;
   }
   
   if (!dividendfile)
    {
      cerr << "Cannot open dividend-file" << endl;
      return 1;
    }
   
   
  
  if (ac < 2) 
   {
     cerr << "Usage: " << av[0] << " <scale - no of elements> [depth - in days. Default = 4000days]" << endl;
     return 1;
   }

  int scale=0, ndays = 4000;
  if (ac >= 2) scale = atoi(av[1]);
  if (ac >= 3) ndays = atoi(av[2]);

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
  // the below is a pain with monetdb, changing to better date format
  //char *crdate = "3/11/1999";
  char *crdate = "1999-11-03";
  
  basefile << "Id|Ex|Descr|SIC|SPR|Cu|CreateDate" << endl;
  for (i=0; i<scale; i++) 
   {
     sprintf(id,"Security_%d", i);
     sprintf(descr, "'Financial security number: %d'", i);
     
     basefile << id;
     basefile << "|" << ex[rg(0,nex)];
     basefile << "|" << descr;
     basefile << "|" << sic[rg(0,nsic)];
     basefile << "|" << spr[rg(0,nspr)];
     basefile << "|" << cu[rg(0, ncu)];
     basefile << "|" << crdate;
     basefile << endl;
   }

  basefile.close();

  
  // generation of price info
  double *minop = new double[scale];
  for (i=0; i<scale; i++) minop[i] = 0.0;

  double *op = new double[scale];
  for (i=0; i<scale; i++) op[i] = 0.0;
  
  unsigned long *vs = new unsigned long[scale];
  for (i=0; i<scale; i++) vs[i] = 0;

  double cp, hp, lp;
  Calendar cal;
  
  pricefile << "Id|TradeDate|HighPrice|LowPrice|ClosePrice|OpenPrice|Volume" << endl;
  splitfile << "Id|SplitDate|EntryDate|SplitFactor" << endl;
  dividendfile << "Id|XdivDate|DivAmt|AnnounceDate" << endl;
  
  for (d=0;d<ndays; d++)
   {
     cal.nextWeekday();
     for (k=0;k<scale;k++)
      {
        sprintf(id,"Security_%d", k);

        if (op[k]==0.0) op[k] = rg(0,100);
        if (minop[k]==0.0) minop[k] = op[k];
        
        if (vs[k]==0) vs[k] = rg();
        else vs[k] = vs[k]*(100.0+rg(-10,+10))/100.0;

        int skew = rg(0,+2);
        double f = (100.0 + rg(-2,3+skew))/100.0;
        cp = op[k] * f;
        hp = max(op[k], cp) * (100.0+rg(0,+10))/100.0;
        lp = min(op[k], cp) * (100.0-rg(0,+10))/100.0;

        pricefile << id;
        pricefile << "|" << cal;
        pricefile << "|" << hp;
        pricefile << "|" << lp;
        pricefile << "|" << cp;
        pricefile << "|" << op[k];
        pricefile << "|" << vs[k];
        pricefile << endl;
        
        op[k] = cp;


        // check splits
        if (op[k] > 2.0*minop[k]) 
         {
           int splitfactor = rg(1,4);
           op[k] /= (double)splitfactor;
           vs[k] *= splitfactor;
           
           splitfile << id;
           splitfile << "|" << cal;
           splitfile << "|" << cal;
           splitfile << "|" << splitfactor;
           splitfile << endl;
         }
         
         // check dividends
         if (op[k] > minop[k]) 
          {
            // dividend as a fraction of current closing price
            double dividend = (rg(1, 100) / 100.0) * cp;
           
            dividendfile << id;
            dividendfile << "|" << cal;
            dividendfile << "|" << dividend;
            // assumes announced and disbursed same day, 
            // queries can be trivially modified to do away with this assumption
            dividendfile << "|" << cal; 
            dividendfile << endl;
          }
      }
      
      
   }
  splitfile.close();
  pricefile.close();
  dividendfile.close();
}


#endif
