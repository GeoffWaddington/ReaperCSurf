/*
** reaper_csurf
** EuCon support
** Copyright (C) 2013 Cockos Incorporated
*/

#include "csurf.h"

class CSurf_EuCon : public IReaperControlSurface
{
  WDL_String descspace;
  char configtmp[1024];

public:
  CSurf_EuCon()
  {
  }

  ~CSurf_EuCon()
  {
  }

  const char *GetTypeString() 
  { 
	  return "EUCON"; 
  }

  const char *GetDescString()
  {
    descspace.Set("EuCon");
    return descspace.Get();     
  }

  const char *GetConfigString() // string of configuration data
  {
    sprintf(configtmp,"0 0");      
    return configtmp;
  }
};

static void parseParms(const char *str, int parms[4])
{
  parms[0]=0;
  parms[1]=9;
  parms[2]=parms[3]=-1;

  const char *p=str;
  if (p)
  {
    int x=0;
    while (x<4)
    {
      while (*p == ' ') p++;
      if ((*p < '0' || *p > '9') && *p != '-') break;
      parms[x++]=atoi(p);
      while (*p && *p != ' ') p++;
    }
  }  
}

static IReaperControlSurface *createFunc(const char *type_string, const char *configString, int *errStats)
{
  int parms[4];
  parseParms(configString,parms);

  return new CSurf_EuCon();
}

static HWND configFunc(const char *type_string, HWND parent, const char *initConfigString)
{
  return 0;
}

reaper_csurf_reg_t csurf_eucon_reg = 
{
  "EUCON",
  "EuCon",
  createFunc,
  configFunc,
};