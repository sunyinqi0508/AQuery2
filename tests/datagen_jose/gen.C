// cmvc_id = %Z% %W% %I% %E% %U%

#ifndef genIMPLEMENTATION
#define genIMPLEMENTATION

#include <iostream.h>
#include "RandGen.H"

int num[6];
int nelems=0;

int member(int a_)
{
  for (int i=0; i<nelems; i++) 
   {
     if (num[i] == a_) return 1;
   }
  return 0;
}

int gen(int flag_)
{
  RandNumGen rg;
  if (flag_ == 0) 
   {
     for (int i=0;i<6;i++) num[i] = 0;
     nelems=0;
     return 0;
   }

  int rn;
  while (1) 
   {
     rn = rg(1,52);
     if (member(rn) == 0) break;
   }

  num[nelems++] = rn;
  return rn;
}

int main(int ac, char *av[])
{
  if (ac < 2) 
   {
     cerr << "Usage: " << av[0] << " <number>" << endl;
     return 1;
   }
  
  int k = atoi(av[1]);
  for(int i=0; i<k; i++)
   {
     gen(0);     
     for (int j=0;j<6;j++) cout << gen(1) << " ";
     cout << endl;
   }
}
#endif
