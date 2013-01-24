#include <ctype.h>
#include "csurf.h"
#include "osc.h"
#include "../../WDL/ptrlist.h"
#include "../../WDL/assocarray.h"
#include "../../WDL/projectcontext.cpp"
#include "../../WDL/dirscan.h"

#ifdef _DEBUG
#include <assert.h>
#endif


#define OSC_EXT ".ReaperOSC"

#define MAX_OSC_WC 16
#define MAX_OSC_RPTCNT 16

#define CSURF_EXT_IMPL_ADD 0x00010000
#define CSURF_EXT_SETPAN_EX_IMPL (CSURF_EXT_SETPAN_EX+CSURF_EXT_IMPL_ADD)
#define CSURF_EXT_SETINPUTMONITOR_IMPL (CSURF_EXT_SETINPUTMONITOR+CSURF_EXT_IMPL_ADD)
#define CSURF_EXT_SETSENDVOLUME_IMPL (CSURF_EXT_SETSENDVOLUME+CSURF_EXT_IMPL_ADD)
#define CSURF_EXT_SETSENDPAN_IMPL (CSURF_EXT_SETSENDPAN+CSURF_EXT_IMPL_ADD)
#define CSURF_EXT_SETRECVVOLUME_IMPL (CSURF_EXT_SETRECVVOLUME+CSURF_EXT_IMPL_ADD)
#define CSURF_EXT_SETRECVPAN_IMPL (CSURF_EXT_SETRECVPAN+CSURF_EXT_IMPL_ADD)
#define CSURF_EXT_SETFXENABLED_IMPL (CSURF_EXT_SETFXENABLED+CSURF_EXT_IMPL_ADD)
#define CSURF_EXT_SETFXOPEN_IMPL (CSURF_EXT_SETFXOPEN+CSURF_EXT_IMPL_ADD)


#define MAX_LASTTOUCHED_TRACK 512
#define ROTARY_STEP (1.0/1024.0)


enum 
{
  PAN_MODE_CLASSIC=0,
  PAN_MODE_NEW_BALANCE=3,
  PAN_MODE_STEREO_PAN=5,
  PAN_MODE_DUAL_PAN=6
};


// awk ' $1 && $1 !~ /^#/ { print "  \"" $0 "\"," }' jmde/OSC/Default.ReaperOSC
static const char* defcfg[] = 
{
  "DEVICE_TRACK_COUNT 8",
  "DEVICE_SEND_COUNT 4",
  "DEVICE_RECEIVE_COUNT 4",
  "DEVICE_FX_COUNT 8",
  "DEVICE_FX_PARAM_COUNT 16",
  "DEVICE_FX_INST_PARAM_COUNT 16",
  "REAPER_TRACK_FOLLOWS REAPER",
  "DEVICE_TRACK_FOLLOWS DEVICE",
  "DEVICE_TRACK_BANK_FOLLOWS DEVICE",
  "DEVICE_FX_FOLLOWS DEVICE",
  "DEVICE_EQ INSERT",
  "DEVICE_ROTARY_CENTER 0",
  "SCROLL_X- b/scroll/x/- r/scroll/x",
  "SCROLL_X+ b/scroll/x/+ r/scroll/x",
  "SCROLL_Y- b/scroll/y/- r/scroll/y",
  "SCROLL_Y+ b/scroll/y/+ r/scroll/y",
  "ZOOM_X- b/zoom/x/- r/zoom/x",
  "ZOOM_X+ b/zoom/x/+ r/zoom/x",
  "ZOOM_Y- b/zoom/y/- r/zoom/y",
  "ZOOM_Y+ b/zoom/y/+ r/zoom/y",
  "TIME f/time s/time/str",
  "BEAT s/beat/str",
  "SAMPLES f/samples s/samples/str",
  "FRAMES s/frames/str",
  "METRONOME t/click",
  "REPLACE t/replace",
  "REPEAT t/repeat",
  "RECORD t/record",
  "STOP t/stop",
  "PLAY t/play",
  "PAUSE t/pause",
  "AUTO_REC_ARM t/autorecarm",
  "SOLO_RESET t/soloreset",
  "ANY_SOLO b/anysolo",
  "REWIND b/rewind",
  "FORWARD b/forward",
  "REWIND_FORWARD_BYMARKER t/bymarker",
  "REWIND_FORWARD_SETLOOP t/editloop",
  "SCRUB r/scrub",
  "PLAY_RATE n/playrate f/playrate/raw r/playrate/rotary s/playrate/str",
  "TEMPO n/tempo f/tempo/raw r/tempo/rotary s/tempo/str",
  "MASTER_VOLUME n/master/volume s/master/volume/str",
  "MASTER_PAN n/master/pan s/master/pan/str",
  "MASTER_VU n/master/vu",
  "MASTER_SEND_NAME s/master/send/@/name",
  "MASTER_SEND_VOLUME n/master/send/@/volume s/master/send/@/volume/str",
  "MASTER_SEND_PAN n/master/send/@/pan s/master/send/@/pan/str",
  "TRACK_NAME s/track/name s/track/@/name",
  "TRACK_NUMBER s/track/number/str s/track/@/number/str",
  "TRACK_MUTE b/track/mute b/track/@/mute ",
  "TRACK_SOLO b/track/solo b/track/@/solo",
  "TRACK_REC_ARM b/track/recarm b/track/@/recarm ",
  "TRACK_MONITOR b/track/monitor b/track/@/monitor ",
  "TRACK_SELECT b/track/select b/track/@/select ",
  "TRACK_VU n/track/vu n/track/@/vu",
  "TRACK_VOLUME n/track/volume n/track/@/volume ",
  "TRACK_VOLUME s/track/volume/str s/track/@/volume/str",
  "TRACK_VOLUME f/track/volume/db f/track/@/volume/db",
  "TRACK_PAN n/track/pan n/track/@/pan s/track/pan/str s/track/@/pan/str",
  "TRACK_PAN2 n/track/pan2 n/track/@/pan2 s/track/pan2/str s/track/@/pan2/str",
  "TRACK_PAN_MODE s/track/panmode s/track/@/panmode",
  "TRACK_SEND_NAME s/track/send/@/name s/track/@/send/@/name",
  "TRACK_SEND_VOLUME n/track/send/@/volume n/track/@/send/@/volume",
  "TRACK_SEND_VOLUME s/track/send/@/volume/str s/track/@/send/@/volume/str",
  "TRACK_SEND_PAN n/track/send/@/pan n/track/@/send/@/pan",
  "TRACK_SEND_PAN s/track/send/@/pan/str s/track/@/send/@/pan/str",
  "TRACK_RECV_NAME s/track/recv/@/name s/track/@/recv/@/name",
  "TRACK_RECV_VOLUME n/track/recv/@/volume n/track/@/recv/@/volume",
  "TRACK_RECV_VOLUME s/track/recv/@/volume/str s/track/@/recv/@/volume/str",
  "TRACK_RECV_PAN n/track/recv/@/pan n/track/@/recv/@/pan",
  "TRACK_RECV_PAN s/track/recv/@/pan/str s/track/@/recv/@/pan/str",
  "TRACK_AUTO s/track/auto",
  "TRACK_AUTO_TRIM t/track/autotrim t/track/@/autotrim",
  "TRACK_AUTO_READ t/track/autoread t/track/@/autoread",
  "TRACK_AUTO_LATCH t/track/autolatch t/track/@/autolatch",
  "TRACK_AUTO_TOUCH t/track/autotouch t/track/@/autotouch",
  "TRACK_AUTO_WRITE t/track/autowrite t/track/@/autowrite",
  "TRACK_VOLUME_TOUCH b/track/volume/touch b/track/@/volume/touch",
  "TRACK_PAN_TOUCH b/track/pan/touch b/track/@/pan/touch",
  "FX_NAME s/fx/name s/fx/@/name s/track/@/fx/@/name",
  "FX_NUMBER s/fx/number/str s/fx/@/number/str s/track/@/fx/@/number/str",
  "FX_BYPASS b/fx/bypass b/fx/@/bypass b/track/@/fx/@/bypass ",
  "FX_OPEN_UI b/fx/openui b/fx/@/openui b/track/@/fx/@/openui",
  "FX_PRESET s/fx/preset s/fx/@/preset s/track/@/fx/@/preset",
  "FX_PREV_PRESET t/fx/preset- t/fx/@/preset- t/track/@/fx/@/preset-",
  "FX_NEXT_PRESET t/fx/preset+ t/fx/@/preset+ t/track/@/fx/@/preset+",
  "FX_PARAM_NAME s/fxparam/@/name s/fx/@/fxparam/@/name",
  "FX_WETDRY n/fx/wetdry n/fx/@/wetdry n/track/@/fx/@/wetdry",
  "FX_WETDRY s/fx/wetdry/str s/fx/@/wetdry/str s/track/@/fx/@/wetdry/str",
  "FX_PARAM_VALUE n/fxparam/@/value n/fx/@/fxparam/@/value n/track/@/fx/@/fxparam/@/value",
  "FX_PARAM_VALUE s/fxparam/@/value/str s/fx/@/fxparam/@/value/str",
  "FX_EQ_BYPASS b/fxeq/bypass b/track/@/fxeq/bypass",
  "FX_EQ_OPEN_UI b/fxeq/openui b/track/@/fxeq/openui",
  "FX_EQ_PRESET s/fxeq/preset s/track/@/fxeq/preset",
  "FX_EQ_PREV_PRESET s/fxeq/preset- s/track/@/fxeq/preset-",
  "FX_EQ_NEXT_PRESET s/fxeq/preset+ s/track/@/fxeq/preset+",
  "FX_EQ_MASTER_GAIN n/fxeq/gain n/track/@/fxeq/gain ",
  "FX_EQ_MASTER_GAIN f/fxeq/gain/db f/track/@/fxeq/gain/db s/fxeq/gain/str",
  "FX_EQ_WETDRY n/fxeq/wetdry n/track/@/fxeq/wetdry",
  "FX_EQ_WETDRY s/fxeq/wetdry/str s/track/@/fxeq/wetdry/str",
  "FX_EQ_HIPASS_NAME s/fxeq/hipass/str",
  "FX_EQ_HIPASS_BYPASS b/fxeq/hipass/bypass",
  "FX_EQ_HIPASS_FREQ n/fxeq/hipass/freq n/track/@/fxeq/hipass/freq",
  "FX_EQ_HIPASS_FREQ f/fxeq/hipass/freq/hz f/track/@/fxeq/hipass/freq/hz",
  "FX_EQ_HIPASS_FREQ s/fxeq/hipass/freq/str s/track/@/fxeq/hipass/freq/str",
  "FX_EQ_HIPASS_Q n/fxeq/hipass/q n/track/@/fxeq/hipass/q",
  "FX_EQ_HIPASS_Q f/fxeq/hipass/q/oct f/track/@/fxeq/hipass/q/oct",
  "FX_EQ_HIPASS_Q s/fxeq/hipass/q/str s/track/@/fxeq/hipass/q/str",
  "FX_EQ_LOSHELF_NAME s/fxeq/loshelf/str",
  "FX_EQ_LOSHELF_BYPASS b/fxeq/loshelf/bypass",
  "FX_EQ_LOSHELF_FREQ n/fxeq/loshelf/freq n/track/@/fxeq/loshelf/freq",
  "FX_EQ_LOSHELF_FREQ f/fxeq/loshelf/freq/hz f/track/@/fxeq/loshelf/freq/hz",
  "FX_EQ_LOSHELF_FREQ s/fxeq/loshelf/freq/str s/track/@/fxeq/loshelf/freq/str",
  "FX_EQ_LOSHELF_GAIN n/fxeq/loshelf/gain n/track/@/fxeq/loshelf/gain",
  "FX_EQ_LOSHELF_GAIN f/fxeq/loshelf/gain/db f/track/@/fxeq/loshelf/gain/db",
  "FX_EQ_LOSHELF_GAIN s/fxeq/loshelf/gain/str s/track/@/fxeq/loshelf/gain/str",
  "FX_EQ_LOSHELF_Q n/fxeq/loshelf/q n/track/@/fxeq/loshelf/q",
  "FX_EQ_LOSHELF_Q f/fxeq/loshelf/q/oct f/track/@/fxeq/loshelf/q/oct",
  "FX_EQ_LOSHELF_Q s/fxeq/loshelf/q/str s/track/@/fxeq/loshelf/q/str",
  "FX_EQ_BAND_NAME s/fxeq/band/str",
  "FX_EQ_BAND_BYPASS b/fxeq/band/@/bypass",
  "FX_EQ_BAND_FREQ n/fxeq/band/@/freq n/track/@/fxeq/band/@/freq",
  "FX_EQ_BAND_FREQ f/fxeq/band/@/freq/hz f/track/@/fxeq/band/@/freq/hz",
  "FX_EQ_BAND_FREQ s/fxeq/band/@/freq/str s/track/@/fxeq/band/@/freq/str",
  "FX_EQ_BAND_GAIN n/fxeq/band/@/gain n/track/@/fxeq/band/@/gain",
  "FX_EQ_BAND_GAIN f/fxeq/band/@/gain/db f/track/@/fxeq/band/@/gain/db",
  "FX_EQ_BAND_GAIN s/fxeq/band/@/gain/str s/track/@/fxeq/band/@/gain/str",
  "FX_EQ_BAND_Q n/fxeq/band/@/q n/track/@/fxeq/band/@/q",
  "FX_EQ_BAND_Q f/fxeq/band/@/q/oct f/track/@/fxeq/band/@/q/oct",
  "FX_EQ_BAND_Q s/fxeq/band/@/q/str s/track/@/fxeq/band/@/q/str",
  "FX_EQ_NOTCH_NAME s/fxeq/notch/str",
  "FX_EQ_NOTCH_BYPASS b/fxeq/notch/bypass",
  "FX_EQ_NOTCH_FREQ n/fxeq/notch/freq n/track/@/fxeq/notch/freq",
  "FX_EQ_NOTCH_FREQ f/fxeq/notch/freq/hz f/track/@/fxeq/notch/freq/hz",
  "FX_EQ_NOTCH_FREQ s/fxeq/notch/freq/str s/track/@/fxeq/notch/freq/str",
  "FX_EQ_NOTCH_GAIN n/fxeq/notch/gain n/track/@/fxeq/notch/gain",
  "FX_EQ_NOTCH_GAIN f/fxeq/notch/gain/db f/track/@/fxeq/notch/gain/db",
  "FX_EQ_NOTCH_GAIN s/fxeq/notch/gain/str s/track/@/fxeq/notch/gain/str",
  "FX_EQ_NOTCH_Q n/fxeq/notch/q n/track/@/fxeq/notch/q",
  "FX_EQ_NOTCH_Q f/fxeq/notch/q/oct f/track/@/fxeq/notch/q/oct",
  "FX_EQ_NOTCH_Q s/fxeq/notch/q/str s/track/@/fxeq/notch/q/str",
  "FX_EQ_HISHELF_NAME s/fxeq/hishelf/str",
  "FX_EQ_HISHELF_BYPASS b/fxeq/hishelf/bypass",
  "FX_EQ_HISHELF_FREQ n/fxeq/hishelf/freq n/track/@/fxeq/hishelf/freq",
  "FX_EQ_HISHELF_FREQ f/fxeq/hishelf/freq/hz f/track/@/fxeq/hishelf/freq/hz",
  "FX_EQ_HISHELF_FREQ s/fxeq/hishelf/freq/str s/track/@/fxeq/hishelf/freq/str",
  "FX_EQ_HISHELF_GAIN n/fxeq/hishelf/gain n/track/@/fxeq/hishelf/gain",
  "FX_EQ_HISHELF_GAIN f/fxeq/hishelf/gain/sb f/track/@/fxeq/hishelf/gain/db",
  "FX_EQ_HISHELF_GAIN s/fxeq/hishelf/gain/str s/track/@/fxeq/hishelf/gain/str",
  "FX_EQ_HISHELF_Q n/fxeq/hishelf/q n/track/@/fxeq/hishelf/q",
  "FX_EQ_HISHELF_Q f/fxeq/hishelf/q/oct f/track/@/fxeq/hishelf/q/oct",
  "FX_EQ_HISHELF_Q s/fxeq/hishelf/q/str s/track/@/fxeq/hishelf/q/str",
  "FX_EQ_LOPASS_NAME s/fxeq/lopass/str",
  "FX_EQ_LOPASS_BYPASS b/fxeq/lopass/bypass",
  "FX_EQ_LOPASS_FREQ n/fxeq/lopass/freq n/track/@/fxeq/lopass/freq",
  "FX_EQ_LOPASS_FREQ f/fxeq/lopass/freq/hz f/track/@/fxeq/lopass/freq/hz",
  "FX_EQ_LOPASS_FREQ s/fxeq/lopass/freq/str s/track/@/fxeq/lopass/freq/str",
  "FX_EQ_LOPASS_Q n/fxeq/lopass/q n/track/@/fxeq/lopass/q",
  "FX_EQ_LOPASS_Q f/fxeq/lopass/q/oct f/track/@/fxeq/lopass/q/oct",
  "FX_EQ_LOPASS_Q s/fxeq/lopass/q/str s/track/@/fxeq/lopass/q/str",
  "FX_INST_NAME s/fxinst/name s/track/@/fxinst/name",
  "FX_INST_BYPASS b/fxinst/bypass b/track/@/fxinst/bypass",
  "FX_INST_OPEN_UI b/fxinst/openui b/track/@/fxinst/openui",
  "FX_INST_PRESET s/fxinst/preset s/track/@/fxinst/preset",
  "FX_INST_PREV_PRESET t/fxinst/preset- t/track/@/fxinst/preset-",
  "FX_INST_NEXT_PRESET t/fxinst/preset+ t/track/@/fxinst/preset+",
  "FX_INST_PARAM_NAME s/fxinstparam/@/name",
  "FX_INST_PARAM_VALUE n/fxinstparam/@/value n/track/@/fxinstparam/@/value",
  "FX_INST_PARAM_VALUE s/fxinstparam/@/value/str",
  "LAST_TOUCHED_FX_TRACK_NAME s/fx/last_touched/track/name",
  "LAST_TOUCHED_FX_TRACK_NUMBER s/fx/last_touched/track/number/str",
  "LAST_TOUCHED_FX_NAME s/fx/last_touched/name",
  "LAST_TOUCHED_FX_NUMBER s/fx/last_touched/number/str",
  "LAST_TOUCHED_FX_PARAM_NAME s/fxparam/last_touched/name",
  "LAST_TOUCHED_FX_PARAM_VALUE n/fxparam/last_touched/value s/fxparam/last_touched/value/str",
  "ACTION i/action t/action/@",
  "MIDIACTION i/midiaction t/midiaction/@",
  "MIDILISTACTION i/midilistaction t/midilistaction/@",
  "DEVICE_TRACK_COUNT i/device/track/count t/device/track/count/@",
  "DEVICE_SEND_COUNT i/device/send/count t/device/send/count/@",
  "DEVICE_RECEIVE_COUNT i/device/receive/count t/device/receive/count/@",
  "DEVICE_FX_COUNT i/device/fx/count t/device/fx/count/@",
  "DEVICE_FX_PARAM_COUNT i/device/fxparam/count t/device/fxparam/count/@",
  "DEVICE_FX_INST_PARAM_COUNT i/device/fxinstparam/count t/device/fxinstparam/count/@",
  "REAPER_TRACK_FOLLOWS s/reaper/track/follows",
  "REAPER_TRACK_FOLLOWS_REAPER t/reaper/track/follows/reaper",
  "REAPER_TRACK_FOLLOWS_DEVICE t/reaper/track/follows/device",
  "DEVICE_TRACK_FOLLOWS s/device/track/follows",
  "DEVICE_TRACK_FOLLOWS_DEVICE t/device/track/follows/device",
  "DEVICE_TRACK_FOLLOWS_LAST_TOUCHED t/device/track/follows/last_touched",
  "DEVICE_TRACK_BANK_FOLLOWS s/device/track/bank/follows",
  "DEVICE_TRACK_BANK_FOLLOWS_DEVICE t/device/track/bank/follows/device",
  "DEVICE_TRACK_BANK_FOLLOWS_MIXER t/device/track/bank/follows/mixer",
  "DEVICE_FX_FOLLOWS s/device/fx/follows",
  "DEVICE_FX_FOLLOWS_DEVICE t/device/fx/follows/device",
  "DEVICE_FX_FOLLOWS_LAST_TOUCHED t/device/fx/follows/last_touched",
  "DEVICE_FX_FOLLOWS_FOCUSED t/device/fx/follows/focused",
  "DEVICE_TRACK_SELECT i/device/track/select t/device/track/select/@ ",
  "DEVICE_PREV_TRACK t/device/track/-",
  "DEVICE_NEXT_TRACK t/device/track/+",
  "DEVICE_TRACK_BANK_SELECT i/device/track/bank/select t/device/track/bank/select/@",
  "DEVICE_PREV_TRACK_BANK t/device/track/bank/-",
  "DEVICE_NEXT_TRACK_BANK t/device/track/bank/+",
  "DEVICE_FX_SELECT i/device/fx/select t/device/fx/select/@",
  "DEVICE_PREV_FX t/device/fx/-",
  "DEVICE_NEXT_FX t/device/fx/+",
  "DEVICE_FX_PARAM_BANK_SELECT i/device/fxparam/bank/select t/device/fxparam/bank/select/@ ",
  "DEVICE_FX_PARAM_BANK_SELECT s/device/fxparam/bank/str",
  "DEVICE_PREV_FX_PARAM_BANK t/device/fxparam/bank/-",
  "DEVICE_NEXT_FX_PARAM_BANK t/device/fxparam/bank/+",
  "DEVICE_FX_INST_PARAM_BANK_SELECT i/device/fxinstparam/bank/select t/device/fxinstparam/bank/select/@",
  "DEVICE_FX_INST_PARAM_BANK_SELECT s/device/fxinstparam/bank/str",
  "DEVICE_PREV_FX_INST_PARAM_BANK t/device/fxinstparam/bank/-",
  "DEVICE_NEXT_FX_INST_PARAM_BANK t/device/fxinstparam/bank/+",
};


template <class T> void _swap(T& a, T& b) 
{
  T t=a;
  a=b;
  b=t; 
}

template <class T> void _reverse(T* vec, int len)
{
  int i;
  for (i=0; i < len/2; ++i)
  {
    _swap(vec[i], vec[len-i-1]);
  }
}

bool strstarts(const char* p, const char* q)
{
  return !strncmp(p, q, strlen(q));
}

bool strends(const char* p, const char* q)
{
  const char* s=strstr(p, q);
  return (s && strlen(s) == strlen(q));
}

bool isint(const char* p)
{
  if (!*p) return false;
  while (*p)
  {
    if (!isdigit(*p)) return false;
    ++p;
  }
  return true;
}

bool isfloat(const char* p)
{
  if (!*p) return false;
  if (*p == '-') ++p;
  bool hasdec=false;
  while (*p)
  {
    if (*p == '.')
    {   
      if (hasdec) return false;
      hasdec=true;
    }
    else if (!isdigit(*p))
    {
      return false;
    }
    ++p;
  }
  return true;
}


bool LoadCfgFile(const char* fn, WDL_PtrList<char>* cfg, char* errbuf)
{
  if (errbuf) errbuf[0]=0;
  if (!fn || !fn[0]) return false;

  const char* p=fn+strlen(fn)-1;
  while (p >= fn && *p != '/' && *p != '\\') --p;
  ++p;

  WDL_String fnbuf;
  fnbuf.Set(GetOscCfgDir());
  fnbuf.Append(p);
  if (stricmp(fn+strlen(fn)-strlen(OSC_EXT), OSC_EXT))
  {
    fnbuf.Append(OSC_EXT);
  }

  ProjectStateContext* ctx=ProjectCreateFileRead(fnbuf.Get());
  if (!ctx) return false;

  bool comment_state=false;

  // load up the valid desc key strings
  WDL_StringKeyedArray<char> keys;
  int i;
  for (i=0; i < sizeof(defcfg)/sizeof(defcfg[0]); ++i)
  {
    LineParser lp(comment_state);
    if (lp.parse(defcfg[i]) || lp.getnumtokens() < 1) continue;    
      
    const char* key=lp.gettoken_str(0);
    if (key[0] == '#') continue;      
        
    keys.Insert(key, 1);    
  }

  bool haderr=false;
  int line=0;
  char buf[4096];
  while (!ctx->GetLine(buf, sizeof(buf)))
  {
    ++line;
    LineParser lp(comment_state);
    if (lp.parse(buf) || lp.getnumtokens() < 2) continue;

    const char* key=lp.gettoken_str(0);
    if (key[0] == '#') continue;

    if (!keys.Exists(key))
    {
      if (errbuf && !errbuf[0]) 
      {
        sprintf(errbuf, "Unknown action \"%s\" on line %d", key, line);
      }
      haderr=true;
      continue;
    }

    for (i=1; i < lp.getnumtokens(); ++i)
    {
      const char* pattern=lp.gettoken_str(i);

      if (i == 1)
      {
        if (strstarts(key, "DEVICE_") || strstarts(key, "REAPER_"))
        {
          if (strends(key, "_COUNT") && isint(pattern)) 
          {
            continue;
          }
          if (strends(key, "_ROTARY_CENTER") && isfloat(pattern))
          {
            continue;
          }
          if (strends(key, "_FOLLOWS") && 
            (!strcmp(pattern, "REAPER") ||
             !strcmp(pattern, "DEVICE") ||
             !strcmp(pattern, "LAST_TOUCHED") ||
             !strcmp(pattern, "FOCUSED") ||
             !strcmp(pattern, "MIXER")))
          {
            continue;
          }
          if (strends(key, "_EQ") && !strcmp(pattern, "INSERT"))
          {
            continue;
          }
        }
      }
 
      if (!strchr("nfbtrsi", pattern[0]))
      {
        if (errbuf && !errbuf[0])
        {
          sprintf(errbuf, "Pattern \"%s\" starts with unknown flag '%c' on line %d "
            "(allowed: [nfbtrsi])", 
            pattern, pattern[0], line);
        }
        haderr=true;
        break;
      }
      if (pattern[1] != '/')
      {
        if (errbuf && !errbuf[0])
        {
          sprintf(errbuf, "Pattern \"%s\" does not start with '/' on line %d", pattern, line);
        }
        haderr=true;
        break;
      }
      if (strlen(pattern) > 512)
      {
        if (errbuf && !errbuf[0])
        {
          sprintf(errbuf, "Pattern is too long on line %d", line);
        }
        haderr=true;
        break;
      }
    }
    if (haderr) break;
    if (cfg) cfg->Add(strdup(buf));
  }

  delete ctx;
  if (haderr && cfg) cfg->Empty(true, free);

  return !haderr;
}


// '*' matches anything
// '?' matches any character
// '@' matches any integer and matches are returned in wcmatches
// a pattern like /track/1/fx/2,3/fxparam/5,7 has 5 wildcards in 3 slots
// in processing, this will be expanded to track/1/fx/2/fxparam/5 and track/1/fx/3/fxparam/7
// numwc=5, numslots=3, slotcnt=[1,2,2], rptcnt=2

static int OscPatternMatch(const char* a, const char* b, 
                           int* wcmatches, int* numwc, 
                           int* numslots, int* slotcnt, int* rptcnt,
                           char* flag)
{
  if (!a) return -1; 
  if (!b) return 1;

  if (*a != '/')
  {
    if (flag) *flag=*a;
    ++a;
  }
  if (*b != '/')
  {
    if (flag) *flag=*b;
    ++b;
  }
    
  int wantwc = (wcmatches && numwc && numslots && slotcnt && rptcnt);

  while (*a && *b)
  {
    if (*a == *b || *a == '?' || *b == '?')
    {
      ++a;
      ++b;
    }
    else if (*a == '*' || *b == '*')
    {
      bool wca = (*a == '*');
      int ai=0, bi=0;
      while (a[ai] && a[ai] != '/') ++ai;
      while (b[bi] && b[bi] != '/') ++bi;
      int cmp=0;
      if (wca) cmp=strncmp(a, b+bi-ai+1, ai-1);
      else cmp=strncmp(a+ai-bi+1, b, bi-1);
      if (cmp) return cmp;
      a += ai;
      b += bi;
    }
    else if (*a == '@' || *b == '@')
    {
      bool wca = (*a == '@');
      if (!wca) _swap(a, b);

      ++a;
      while (*a == '@') // multiple @ in a row, match one digit to all but the last
      {
        if (isdigit(*b))
        {
          if (wantwc && *numwc < MAX_OSC_WC*MAX_OSC_RPTCNT) 
          {
            wcmatches[(*numwc)++]=*b-'0';
            slotcnt[(*numslots)++]=1;
          }
          ++b;
        }
        ++a;
      }

      if (wantwc && *numwc < MAX_OSC_WC*MAX_OSC_RPTCNT) 
      {
        wcmatches[(*numwc)++]=atoi(b);
        slotcnt[*numslots]=1;
      }
      while (isdigit(*b)) ++b;

      while (*b == ',' && isdigit(*(b+1)))
      {
        ++b;
        if (wantwc && *numwc < MAX_OSC_WC*MAX_OSC_RPTCNT) 
        {
          wcmatches[(*numwc)++]=atoi(b);
          int sc=++slotcnt[*numslots];
          if (sc > *rptcnt) *rptcnt=sc;
        }
        while (isdigit(*b)) ++b;
      }
      if (wantwc) (*numslots)++;

      if (!wca) _swap(a, b);
    }
    else
    {
      return *a-*b;
    }
  }

  return *a-*b;
}


static bool OscWildcardSub(char* msg, char wc, const char* wcsub)
{  
  char* p=strchr(msg, wc);
  if (p)
  {
    int sublen=strlen(wcsub);
    memmove(p+sublen, p+1, strlen(p));
    memcpy(p, wcsub, sublen);
    return true;
  }
  return false;
}

static int CountWildcards(const char* msg)
{
  int cnt=0;
  while (*msg)
  {
    if (*msg == '@') ++cnt;
    ++msg;
  }
  return cnt;
}

static void PackCfg(WDL_String* str,
                    const char* name, int flags,
                    int recvport, const char* sendip, int sendport,
                    int maxpacketsz, int sendsleep,
                    const char* cfgfn)
{
  if (!name) name="";
  if (!sendip) sendip="";
  if (!cfgfn) cfgfn="";
  str->SetFormatted(1024, "\"%s\" %d %d \"%s\" %d %d %d \"%s\"", 
    name, flags, recvport, sendip, sendport, maxpacketsz, sendsleep, cfgfn);
}

static void ParseCfg(const char* cfgstr, 
                     char* name, int namelen, int* flags,
                     int* recvport, char* sendip, int* sendport,
                     int* maxpacketsz, int* sendsleep, 
                     char* cfgfn, int cfgfnlen)
{
  name[0]=0;
  *flags=0;
  *recvport=0;
  sendip[0]=0;
  *sendport=0;
  *maxpacketsz=DEF_MAXPACKETSZ;
  *sendsleep=DEF_SENDSLEEP;
  cfgfn[0]=0;

  if (strstr(cfgstr, "@@@")) return;  // reject config str from pre13

  bool comment_state=false;
  LineParser lp(comment_state);
  if (!lp.parse(cfgstr))
  {
    if (lp.getnumtokens() > 0) lstrcpyn(name, lp.gettoken_str(0), namelen);
    if (lp.getnumtokens() > 1) *flags=lp.gettoken_int(1);
    if (lp.getnumtokens() > 2) *recvport=lp.gettoken_int(2);
    if (lp.getnumtokens() > 3) lstrcpyn(sendip, lp.gettoken_str(3), 64);
    if (lp.getnumtokens() > 4) *sendport=lp.gettoken_int(4);
    if (lp.getnumtokens() > 5) *maxpacketsz=lp.gettoken_int(5);
    if (lp.getnumtokens() > 6) *sendsleep=lp.gettoken_int(6);
    if (lp.getnumtokens() > 7) lstrcpyn(cfgfn, lp.gettoken_str(7), cfgfnlen);
  }
}


struct OscVal 
{
  OscVal(int* i, double* f, const char* s) 
  {
    memset(&val, 0, sizeof(val));
    if (i) val.ival=*i;
    else if (f) val.fval=*f;
    else if (s) lstrcpyn(val.sval, s, sizeof(val.sval));
    cnt=0;
  }

  bool Update(int* i, double* f, const char* s)
  {
    if (i)
    {
      if (*i != val.ival)
      {
        val.ival=*i;
        cnt=0;
      }
    }
    else if (f)
    {
      if (*f != val.fval) 
      {
        val.fval=*f;
        cnt=0;
      }
    }
    else if (s)
    {
      if (strcmp(s, val.sval))
      {
        lstrcpyn(val.sval, s, sizeof(val.sval));
        cnt=0;
      }
    }
    return (cnt++ <= 1);
  }

  void Invalidate()
  {
    cnt=0;
  }

  union
  {
    int ival;
    double fval;
    char sval[128];
  } val;
  int cnt;
};

static int _strcmp_p(const char** a, const char** b)
{
  return strcmp(*a, *b);
}

static int _osccmp_p(const char** a, const char** b)
{
  return OscPatternMatch(*a, *b, 0, 0, 0, 0, 0, 0);
}

static void _oscvaldispose(OscVal* oscval)
{
  delete oscval; 
}


#define SETSURFNORM(pattern,nval) SetSurfaceVal(pattern,0,0,0,0,&nval,0)
#define SETSURFNORMWC(pattern,wc,numwc,nval) SetSurfaceVal(pattern,wc,numwc,0,0,&nval,0)
#define SETSURFSTR(pattern,str) SetSurfaceVal(pattern,0,0,0,0,0,str)
#define SETSURFSTRWC(pattern,wc,numwc,str) SetSurfaceVal(pattern,wc,numwc,0,0,0,str)
#define SETSURFBOOL(pattern,bval) { double v=(double)bval; SetSurfaceVal(pattern,0,0,0,0,&v,0); }
#define SETSURFBOOLWC(pattern,wc,numwc,bval) { double v=(double)bval; SetSurfaceVal(pattern,wc,numwc,0,0,&v,0); }

#define SETSURFTRACKNORM(pattern,tidx,nval) SetSurfaceTrackVal(pattern,tidx,0,0,&nval,0)
#define SETSURFTRACKSTR(pattern,tidx,str) SetSurfaceTrackVal(pattern,tidx,0,0,0,str)
#define SETSURFTRACKBOOL(pattern,tidx,bval) { double v=(double)bval; SetSurfaceTrackVal(pattern,tidx,0,0,&v,0); }


typedef bool (*OscLocalCallbackFunc)(void* obj, const char* msg, int msglen);

struct OscLocalHandler
{
  void* m_obj;
  OscLocalCallbackFunc m_callback;
};


class CSurf_Osc : public IReaperControlSurface
{
public:

  OscHandler* m_osc; // network I/O
  OscLocalHandler* m_osc_local; // local I/O

  WDL_String m_desc;
  WDL_String m_cfg;

  int m_curtrack;  // 0=master, 1=track 0, etc
  int m_curbankstart; // 0=track 0, etc
  int m_curfx;     // 0=first fx on m_curtrack, etc
  int m_curfxparmbankstart;
  int m_curfxinstparmbankstart;

  int m_trackbanksize;
  int m_sendbanksize;
  int m_recvbanksize;
  int m_fxbanksize;
  int m_fxparmbanksize;
  int m_fxinstparmbanksize;

  double m_rotarylo;
  double m_rotarycenter;
  double m_rotaryhi;

  int m_followflag;  // &1=track follows last touched, &2=FX follows last touched, &4=FX follows focused FX, &8=track bank follows mixer, &16=reaper track sel follows device, &32=insert reaeq on any FX_EQ message
  int m_flags; // &1=enable receive, &2=enable send, &4=bind to actions/fxlearn

  int m_nav_active; // if m_altnav != 1, -1 or 1 while rewind/forward button is held down
  int m_altnav; // makes rewind/forward buttons 1=navigate markers, 2=edit loop pts

  int m_scrollx;
  int m_scrolly;
  int m_zoomx;
  int m_zoomy;

  // pattern table is simplest as a list, 
  // because we need to look up by both key and value
  WDL_PtrList<char> m_msgtab;

  WDL_AssocArray<const char*, int> m_msgkeyidx; // [key] => index of key
  WDL_AssocArray<const char*, int> m_msgvalidx; // [value] => index of value

  // project state .. keep this to a minimum
  DWORD m_lastupd;
  double m_lastpos;
  bool m_anysolo; 
  bool m_surfinit;
  WDL_StringKeyedArray<OscVal*> m_lastvals;

  int m_wantfx;   // &1=want fx parm feedback, &2=want last touched fx feedback, &4=want fx inst feedback, &8=want fx parm feedback for inactive tracks, &16=want fx inst feedback for inactive tracks, &32=want fxeq feedback, &64=want fxeq feedback for inactive tracks
  int m_wantpos;  // &1=time, &2=beats, &4=samples, &8=frames
  int m_wantvu; // &1=master, &2=other tracks

  const char* m_curedit; // the message that is currently being edited, to avoid feedback
  char m_curflag;

  char m_supports_touch;  // &1=vol, &2=pan
  char m_hastouch[MAX_LASTTOUCHED_TRACK]; // &1=vol, &2=pan
  
  DWORD m_vol_lasttouch[MAX_LASTTOUCHED_TRACK]; // only used if !(m_supports_touch&1)
  DWORD m_pan_lasttouch[MAX_LASTTOUCHED_TRACK]; // only used if !(m_supports_touch&2)

  CSurf_Osc(const char* name, int flags, 
            int recvport, const char* sendip, int sendport,
            int maxpacketsz, int sendsleep, 
            OscLocalHandler* osc_local,
            const char* cfgfn)
  : m_msgkeyidx(_strcmp_p),  m_msgvalidx(_osccmp_p), m_lastvals(true, _oscvaldispose)
  {
    m_osc=0;  
    m_osc_local=osc_local;

    if (!name) name="";
    if (!sendip) sendip="";

    m_flags=flags;
    bool rcven=!!(flags&1);
    bool senden=!!(flags&2);

    m_desc.Set("OSC");
    if (name[0]) 
    {
      m_desc.AppendFormatted(1024, ": %s", name);
    }
    else
    {
      if (rcven || senden) m_desc.Append(" (");
      if (rcven) m_desc.AppendFormatted(512, "recv %d", recvport);
      if (rcven && senden) m_desc.Append(", ");
      if (senden) m_desc.AppendFormatted(512, "send %s:%d", sendip, sendport);
      if (rcven || senden) m_desc.Append(")");
    }

    PackCfg(&m_cfg, name, flags, recvport, sendip, sendport, maxpacketsz, sendsleep, cfgfn);

    m_curtrack=0;
    m_curbankstart=0;
    m_curfx=0;
    m_curfxparmbankstart=0;
    m_curfxinstparmbankstart=0;

    m_nav_active=0;
    m_altnav=0;
    m_scrollx=0;
    m_scrolly=0;
    m_zoomx=0;
    m_zoomy=0;

    m_surfinit=false;
    m_lastupd=0;
    m_lastpos=0.0;
    m_anysolo=false;

    m_curedit=0;
    m_curflag=0;

    m_trackbanksize=8;
    m_sendbanksize=4;
    m_recvbanksize=4;
    m_fxbanksize=8;
    m_fxparmbanksize=16;
    m_fxinstparmbanksize=16;

    m_rotarylo=-1.0;
    m_rotarycenter=0.0;
    m_rotaryhi=1.0;

    m_followflag=0;
    m_flags=flags;

    m_wantfx=0;
    m_wantpos=0;
    m_wantvu=0;

    m_supports_touch=0;
    memset(m_hastouch, 0, sizeof(m_hastouch));
    memset(m_vol_lasttouch, 0, sizeof(m_vol_lasttouch));
    memset(m_pan_lasttouch, 0, sizeof(m_pan_lasttouch));

    const char** cfg=defcfg;
    int cfglen=sizeof(defcfg)/sizeof(defcfg[0]);
    
    WDL_PtrList<char> customcfg;
    if (LoadCfgFile(cfgfn, &customcfg, 0) && customcfg.GetSize())
    {
      cfg=(const char**)customcfg.GetList();
      cfglen=customcfg.GetSize();
    }

    bool comment_state=false;
    int i;
    for (i=0; i < cfglen; ++i)
    {  
      LineParser lp(comment_state);
      if (lp.parse(cfg[i]) || lp.getnumtokens() < 2) continue;

      const char* key=lp.gettoken_str(0);
      if (key[0] == '#') continue;

      int fx_feedback_wc=0;
      bool skipdef=false;
      const char* pattern=lp.gettoken_str(1);

      if (strstarts(key, "DEVICE_") || strstarts(key, "REAPER_"))
      {   
        if (strends(key, "_COUNT"))
        {
          if (isint(pattern)) 
          {
            int a=atoi(pattern);
            if (a > 0)
            {
              if (strends(key, "_TRACK_COUNT")) m_trackbanksize=a;
              else if (strends(key, "_SEND_COUNT")) m_sendbanksize=a;
              else if (strends(key, "_RECEIVE_COUNT")) m_recvbanksize=a;
              else if (strends(key, "_FX_COUNT")) m_fxbanksize=a;
              else if (strends(key, "_FX_PARAM_COUNT")) m_fxparmbanksize=a;
              else if (strends(key, "_FX_INST_PARAM_COUNT")) m_fxinstparmbanksize=a;
            }      
            skipdef=true;
          }
        }
        else if (strends(key, "_ROTARY_CENTER"))
        {
          if (isfloat(pattern))
          {
            double center=atof(pattern);
            if (center == 0.0f)
            {
              m_rotarylo=-1.0;
              m_rotarycenter=0.0;
              m_rotaryhi=1.0;
            }
            else if (center > 0.0f)
            {
              m_rotarylo=0.0;
              m_rotarycenter=center;
              m_rotaryhi=2.0*center;
            }
            skipdef=true;
          }
        }
        else if (strends(key, "_FOLLOWS"))
        {      
          if (!strcmp(pattern, "DEVICE") ||
              !strcmp(pattern, "REAPER") ||
              !strcmp(pattern, "MIXER") ||
              !strcmp(pattern, "LAST_TOUCHED") ||
              !strcmp(pattern, "FOCUSED"))
          {
            if (!strcmp(key, "REAPER_TRACK_FOLLOWS"))
            {
              if (!strcmp(pattern, "DEVICE")) m_followflag |= 16;
            }
            else if (!strcmp(key, "DEVICE_TRACK_FOLLOWS"))
            {
              if (!strcmp(pattern, "LAST_TOUCHED")) m_followflag |= 1;
            }
            else if (!strcmp(key, "DEVICE_FX_FOLLOWS"))
            {
              if (!strcmp(pattern, "LAST_TOUCHED")) m_followflag |= 2;
              else if (!strcmp(pattern, "FOCUSED")) m_followflag |= 4;
            }
            else if (!strcmp(key, "DEVICE_TRACK_BANK_FOLLOWS"))
            {
              if (!strcmp(pattern, "MIXER")) m_followflag |= 8;
            }
            skipdef=true;
          }
        }
        else if (strends(key, "_EQ"))
        {
          if (!strcmp(pattern, "INSERT")) m_followflag |= 32;
          skipdef=true;
        }
      }
      else
      {
        if (!strcmp(key, "TIME")) m_wantpos |= 1;      
        else if (!strcmp(key, "BEAT")) m_wantpos |= 2;      
        else if (!strcmp(key, "SAMPLES")) m_wantpos |= 4;
        else if (!strcmp(key, "FRAMES")) m_wantpos |= 8;
        else if (!strcmp(key, "MASTER_VU")) m_wantvu |= 1;      
        else if (!strcmp(key, "TRACK_VU")) m_wantvu |= 2;
        else if (strstarts(key, "FX_PARAM_VALUE") || !strcmp(key, "FX_WETDRY")) m_wantfx |= 1;
        else if (strstarts(key, "LAST_TOUCHED_FX_")) m_wantfx |= 2;
        else if (strstarts(key, "FX_INST_")) m_wantfx |= 4;
        else if (strstarts(key, "FX_EQ_")) m_wantfx |= 32;
        else if (!strcmp(key, "TRACK_VOLUME_TOUCH")) m_supports_touch |= 1;
        else if (!strcmp(key, "TRACK_PAN_TOUCH")) m_supports_touch |= 2;

        // want inactive track FX feedback only if we have this many wildcards
        if (strstarts(key, "FX_PARAM_VALUE")) fx_feedback_wc=3;
        else if (!strcmp(key, "FX_WETDRY")) fx_feedback_wc=2;
        else if (strstarts(key, "FX_INST_")) fx_feedback_wc=2;
        else if (strstarts(key, "FX_EQ_")) fx_feedback_wc=2;
      }
      
      if (skipdef && lp.getnumtokens() < 3) continue;

      // check if we have already seen this key
      int pos;
      for (pos=0; pos < m_msgtab.GetSize(); ++pos)
      {
        if ((!pos || !m_msgtab.Get(pos-1)) && !strcmp(key, m_msgtab.Get(pos))) break;
      }
      if (pos < m_msgtab.GetSize()) ++pos;
      else m_msgtab.Insert(pos++, strdup(key));    
      
      int j;
      for (j = (skipdef ? 2 : 1); j < lp.getnumtokens(); ++j)
      {
        pattern=lp.gettoken_str(j);
        
#ifdef _DEBUG
        assert(strchr("nfbtrsi", pattern[0]));
        assert(pattern[1] == '/');
#endif

        m_msgtab.Insert(pos++, strdup(pattern));

        if (fx_feedback_wc && CountWildcards(pattern) >= fx_feedback_wc)
        {
          if (strstarts(key, "FX_INST_PARAM_VALUE")) m_wantfx |= 16;
          else if (strstarts(key, "FX_EQ_")) m_wantfx |= 32;
          else m_wantfx |= 8;
        }
      }

      if (pos >= m_msgtab.GetSize()) m_msgtab.Add(0);
    }

    for (i=0; i < m_msgtab.GetSize(); ++i)
    {
      const char* p=m_msgtab.Get(i);
      if (!p) continue;
      if (!i || !m_msgtab.Get(i-1)) m_msgkeyidx.Insert(p, i);
      else m_msgvalidx.Insert(p, i);
    }

    customcfg.Empty(true, free);
    
    if (!senden)
    {
      m_wantfx=m_wantpos=m_wantvu=0;
    }

    if (rcven || senden)
    {
      m_osc=new OscHandler;

      m_osc->m_recv_enable=rcven;
      if (m_flags&4) m_osc->m_recv_enable |= 2;
      m_osc->m_recvsock=-1;
      if (rcven)
      {
        m_osc->m_recvaddr.sin_family=AF_INET;
        m_osc->m_recvaddr.sin_addr.s_addr=htonl(INADDR_ANY);
        m_osc->m_recvaddr.sin_port=htons(recvport);
      }

      m_osc->m_send_enable=senden;
      m_osc->m_sendsock=-1;
      if (senden)
      {
        m_osc->m_sendaddr.sin_family=AF_INET;
        m_osc->m_sendaddr.sin_addr.s_addr=inet_addr(sendip);
        m_osc->m_sendaddr.sin_port=htons(sendport);
      }

      m_osc->m_maxpacketsz=maxpacketsz;
      m_osc->m_sendsleep=sendsleep;

      m_osc->m_obj=this;
      m_osc->m_handler=ProcessOscMessage;

      OscInit(m_osc);
    }
  }

  ~CSurf_Osc()
  {
    if (m_osc)
    {
      OscQuit(m_osc);
      delete m_osc;
      m_osc=0;
    }
    if (m_osc_local)
    {
      delete m_osc_local;
      m_osc_local=0;
    }
    m_msgtab.Empty(true, free);
  }

  const char* GetTypeString()
  {
    return "OSC";
  }

  const char* GetDescString()
  {
    return m_desc.Get();
  }

  const char* GetConfigString()
  {
    return m_cfg.Get();
  }

  void Run()
  {
    if (m_osc) OscGetInput(m_osc);

    if (m_nav_active)
    {
      if (!m_altnav)
      {
        if (m_nav_active < 0) CSurf_OnRew(1);
        else if (m_nav_active > 0) CSurf_OnFwd(1);
      }
      else if (m_altnav == 2)
      {
        Loop_OnArrow(0, m_nav_active);
      }
      else
      {
        m_nav_active=0;
      }
    }

    if (m_scrollx || m_scrolly)
    {
      CSurf_OnScroll(m_scrollx, m_scrolly);
    }
    if (m_zoomx || m_zoomy)
    {
      CSurf_OnZoom(m_zoomx, m_zoomy);
    }

    if (!m_wantpos && !m_wantvu) return;

    DWORD now=timeGetTime();
    if (now >= m_lastupd+(1000/max((*g_config_csurf_rate),1)) || now < m_lastupd-250)
    {
      int dt=now-m_lastupd;
      m_lastupd=now;

      if (m_wantpos)
      {
        int ps=GetPlayState();
        double pos=((ps&1) ? GetPlayPosition() : GetCursorPosition());
        if (pos != m_lastpos)
        {
          m_lastpos=pos;
          char buf[512];
          if (m_wantpos&1)
          {
            format_timestr_pos(pos, buf, sizeof(buf), 0);
            SetSurfaceVal("TIME", 0, 0, 0, &pos, 0, buf);
          }
          if (m_wantpos&2)
          {
            format_timestr_pos(pos, buf, sizeof(buf), 1);
            SETSURFSTR("BEAT", buf);
          }
          if (m_wantpos&4)
          {
            format_timestr_pos(pos, buf, sizeof(buf), 4);
            double spos=atof(buf);
            SetSurfaceVal("SAMPLES", 0, 0, 0, &spos, 0, buf);
          }
          if (m_wantpos&8)
          {
            format_timestr_pos(pos, buf, sizeof(buf), 5);
            SETSURFSTR("FRAMES", buf);
          }
        }
      }

      if (m_wantvu)
      { 
        double minvu=(double)*g_vu_minvol;
        double maxvu=(double)*g_vu_maxvol;      

        int starti = ((m_wantvu&1) ? -1 : 0);
        int endi = ((m_wantvu&2) ? m_trackbanksize : -1);
        int i;
        for (i=starti; i <= endi; ++i)
        {
          int tidx;
          if (i == -1) tidx=0; // master
          else if (!i) tidx=m_curtrack;
          else tidx=m_curbankstart+i;
          MediaTrack* tr=CSurf_TrackFromID(tidx, !!(m_followflag&8));

          double vu=0.0;
          if (tr) 
          {
            vu=VAL2DB((Track_GetPeakInfo(tr,0)+Track_GetPeakInfo(tr,1))*0.5);
            if (vu < minvu) vu=0.0;
            else if (vu > maxvu) vu=1.0;
            else vu=(vu-minvu)/(maxvu-minvu);
          }
          
          if (i < 0) 
          {
            SETSURFNORM("MASTER_VU", vu);
          }
          else if (!i)
          {
            SETSURFNORM("TRACK_VU", vu);
          }
          else
          {
            int wc[1] = { i };
            SETSURFNORMWC("TRACK_VU", wc, 1, vu);
          }
        }
      }
    }          
  }



  static bool ProcessOscMessage(void* _this, OscMessageRead* rmsg)
  {
    return ((CSurf_Osc*)_this)->ProcessMessage(rmsg);
  }

  const char* FindOscMatch(const char* msg, int* wc, int* numwc, int* rptcnt, char* flag)
  {
    int* validx=m_msgvalidx.GetPtr(msg);
    if (!validx) return 0; // message didn't match anything
    
    if (!wc || !numwc || !rptcnt) return 0; // assert

    const char* p=m_msgtab.Get(*validx);

    // call patterncmp function again to fill in wildcards  
    int numslots=0;
    int slotcnt[MAX_OSC_WC] = { 0 }; // for each slot, the count of comma-separated wildcards
    if (OscPatternMatch(msg, p, wc, numwc, &numslots, slotcnt, rptcnt, flag)) return 0; // assert
    
    if (*rptcnt > 1)
    {
      if (*rptcnt > MAX_OSC_RPTCNT) return 0;
      int wc2[MAX_OSC_WC*MAX_OSC_RPTCNT] = { 0 };
      
      int i, j;
      int pos=0;   
      for (i=0; i < numslots; ++i)
      {             
        // every slot must have either 1 or rptcnt wildcards
        if (slotcnt[i] != 1 && slotcnt[i] != *rptcnt) return 0;
        for (j=0; j < *rptcnt; ++j)
        {
          int pos2=i+numslots*j;
          wc2[pos2]=wc[pos];
          if (slotcnt[i] == *rptcnt) ++pos;
        }
        if (slotcnt[i] == 1) ++pos;
      }
      *numwc=*rptcnt*numslots;
      memcpy(wc, wc2, *numwc*sizeof(int));
    }

    if (numwc && *numwc > 1)
    {    
      const char* q=strchr(p, '@');
      if (q && *(q+1) != '@') // nonconsecutive wildcards
      {
        _reverse(wc, *numwc);
      }
    }   

    // back up until we find the key
    int i;
    for (i=*validx-1; i >= 0; --i)
    {
      p=m_msgtab.Get(i-1);
      if (!p) break;
    }
    
    return m_msgtab.Get(i);    
  }

  static double GetFloatArg(OscMessageRead* rmsg, char flag, bool* hasarg=0)
  {  
    if (hasarg) *hasarg=true;
    if (strchr("nfbtr", flag)) 
    {
      const float* f=rmsg->PopFloatArg(false);
      if (f) return (double)*f;   
      const int* i=rmsg->PopIntArg(false);
      if (i) return (double)*i;
    }
    else if (flag == 'i')
    {
      const int* i=rmsg->PopIntArg(false);
      if (i) return (double)*i;
      const float* f=rmsg->PopFloatArg(false);
      if (f) return (double)*f;      
    }    
    else if (flag == 's')
    {
      const char* s=rmsg->PopStringArg(false);
      if (s) return atof(s);
    }
    if (hasarg) *hasarg=false;
    if (flag == 't') return 1.0;  // trigger messages do not need an argument
    return 0.0;
  }

  int TriggerMessage(OscMessageRead* rmsg, char flag)
  {
    if (strchr("ntbr", flag))
    {           
      bool hasarg=false;
      double v=GetFloatArg(rmsg, flag, &hasarg); 
      if (flag == 'b' && hasarg && v == 0.0) return -1;
      if (flag == 'b' && hasarg && v == 1.0) return 1;  
      if (flag == 't' && (!hasarg || v == 1.0)) return 1; 
      if (flag == 'r' && hasarg && v < m_rotarycenter) return -1;
      if (flag == 'r' && hasarg && v > m_rotarycenter) return 1; 
    }
    return 0;
  }

  bool ProcessGlobalAction(OscMessageRead* rmsg, const char* pattern, char flag)
  {
    // trigger messages do not latch for scroll/zoom
    bool scroll=strstarts(pattern, "SCROLL_");
    bool zoom=strstarts(pattern, "ZOOM_");
    if (scroll || zoom)
    {     
      int len=strlen(scroll ? "SCROLL_" : "ZOOM_");
      char axis=pattern[len];
      char dir=pattern[len+1];
      int iv=0;
      int t=TriggerMessage(rmsg, flag);
      if (t)
      {
        if (flag == 'r') iv=t;
        else if (dir == '-' && t > 0) iv=-1;
        else if (dir == '+' && t > 0) iv=1;
        int dx = (axis == 'X' ? iv : 0);
        int dy = (axis == 'Y' ? iv : 0);
        if (scroll) 
        {
          if (flag == 'b')
          {
            dx = m_scrollx = (m_scrollx ? 0 : dx);
            dy = m_scrolly = (m_scrolly ? 0 : dy);
          }
          if (dx || dy) CSurf_OnScroll(dx, dy);
        }
        else if (zoom)
        {
          if (flag == 'b')
          {
            dx = m_zoomx = (m_zoomx ? 0 : dx);
            dy = m_zoomy = (m_zoomy ? 0 : dy);
          }
          if (dx || dy) CSurf_OnZoom(dx, dy);
        }
      }
      return true;
    }

    // untested
    bool time=!strcmp(pattern, "TIME");
    bool beat=!strcmp(pattern, "BEAT");
    bool samples=!strcmp(pattern, "SAMPLES");
    bool frames=!strcmp(pattern, "FRAMES");
    if (time || beat || samples || frames)
    {
      double v=0.0;
      if (flag == 'f')
      {
        v=GetFloatArg(rmsg, flag);
        if (time) // time in seconds
        {
        }
        else if (beat)
        {
          v=TimeMap2_beatsToTime(0, v, 0);
        }
        else if (samples)
        {
          char buf[128];
          sprintf(buf, "%.0f", v);
          v=parse_timestr_pos(buf, 4);
        }
        else // can't parse frames
        {
          return true;
        }
      }
      else if (flag == 's')
      {
        const char* s=rmsg->PopStringArg(false);
        if (s)
        {
          int mode;
          if (time) mode=0;
          if (beat) mode=1;
          else if (samples) mode=4;
          else if (frames) mode=5;
          else return true;
          v=parse_timestr_pos(s, mode);
        }
      }
      SetEditCurPos(v, true, true);
      return true;
    }

    if (!strcmp(pattern, "METRONOME"))
    {
      int t=TriggerMessage(rmsg, flag);
      if (t)
      {
        int click=GetToggleCommandState(40364); // toggle metronome
        if (flag == 't' || click != (t > 0))
        {     
          Main_OnCommand(40364, 0);
          Extended(CSURF_EXT_SETMETRONOME, (void*)(!click), 0, 0);
        }
      }
      return true;
    }

    if (!strcmp(pattern, "REPLACE"))
    {
      int t=TriggerMessage(rmsg, flag);
      if (t)
      {
        int repl=GetToggleCommandState(41186); // replace mode
        if (flag == 't' || repl != (t > 0))
        {
          if (repl) Main_OnCommand(41330, 0); // autosplit
          else Main_OnCommand(41186, 0);
        }
      }
      return true;
    }

    if (!strcmp(pattern, "REPEAT"))
    {
      int t=TriggerMessage(rmsg, flag);
      if (t)
      {
        int rpt=GetSetRepeat(-1);
        if (flag == 't' || rpt != (t > 0))
        {
          GetSetRepeat(2);
        }
      }
      return true;
    }

    bool rec=!strcmp(pattern, "RECORD");
    bool stop=!strcmp(pattern, "STOP");
    bool play=!strcmp(pattern, "PLAY");
    bool pause=!strcmp(pattern, "PAUSE");
    if (rec || stop || play || pause)
    {
      if (TriggerMessage(rmsg, flag))
      {
        if (rec) CSurf_OnRecord();
        else if (stop) CSurf_OnStop();
        else if (play) CSurf_OnPlay();           
        else if (pause) CSurf_OnPause();
        int ps=GetPlayState();
        SetPlayState(!!(ps&1), !!(ps&2), !!(ps&4));
      }
      return true;
    }

    if (!strcmp(pattern, "AUTO_REC_ARM"))
    {
      int t=TriggerMessage(rmsg, flag);
      if (t)
      {
        int recarm=GetToggleCommandState(40740); // auto-recarm all tracks
        if (flag == 't' || recarm != (t > 0))
        {
          Main_OnCommand(40740, 0);
        }
      }
      return true;
    }

    if (!strcmp(pattern, "SOLO_RESET"))
    {
      if (TriggerMessage(rmsg, flag) > 0)
      {
        SoloAllTracks(0);
      }
      return true;
    }

    // maybe remove this
    if (!strcmp(pattern, "ANY_SOLO"))
    {
      if (TriggerMessage(rmsg, flag) > 0)
      {
        SoloAllTracks(0);
      }
      return true;
    }    

    // trigger messages latch for rew/fwd
    bool rew=!strcmp(pattern, "REWIND");
    bool fwd=!strcmp(pattern, "FORWARD");
    if (rew || fwd)
    {
      if (!m_altnav)
      {
        int t=TriggerMessage(rmsg, flag);
        if (t)
        {              
          if (rew)
          {
            if (flag == 't' || (m_nav_active < 0) != (t > 0))
            {
              m_nav_active = (m_nav_active ? 0 : -1);
            }
            if (m_nav_active) CSurf_OnRew(1);
          }
          else if (fwd)
          {
            if (flag == 't' || (m_nav_active > 0) != (t > 0))
            {
              m_nav_active = (m_nav_active ? 0 : 1);
            }
            if (m_nav_active) CSurf_OnFwd(1); 
          }
        }
        bool en = ((rew && m_nav_active < 0) || (fwd && m_nav_active > 0));
        SETSURFBOOL(pattern, en);
      }
      else if (m_altnav == 1) // nav by marker
      {   
        if (TriggerMessage(rmsg, flag) > 0)
        {
          m_nav_active=0;
          if (rew) Main_OnCommand(40172, 0); // go to previous marker
          else if (fwd) Main_OnCommand(40173, 0); // go to next marker
        }
      }
      else if (m_altnav == 2) // move loop points
      {
        bool snaploop=Loop_OnArrow(0, 0);
        int t=TriggerMessage(rmsg, flag);
        if (t)
        {
          if (snaploop && t > 0)
          {
            if (rew) Loop_OnArrow(0, -1);
            else if (fwd) Loop_OnArrow(0, 1);
          }        
          else if (!snaploop)
          {
            if (rew)
            {
              if (flag == 't' || (m_nav_active < 0) != (t > 0))
              {
                m_nav_active = (m_nav_active ? 0 : -1);
              }       
              if (m_nav_active) Loop_OnArrow(0, -1);          
            }          
            else if (fwd)
            {
              if (flag == 't' || (m_nav_active > 0) != (t > 0))
              {
                m_nav_active = (m_nav_active ? 0 : 1);
              }       
              if (m_nav_active) Loop_OnArrow(0, 1);      
            }
          }
        }
        if (!snaploop)
        {
          bool en = ((rew && m_nav_active < 0) || (fwd && m_nav_active > 0));
          SETSURFBOOL(pattern, en);
        }
      }
      return true;
    }

    bool bymarker=!strcmp(pattern, "REWIND_FORWARD_BYMARKER");
    bool setloop=!strcmp(pattern, "REWIND_FORWARD_SETLOOP");
    if (bymarker || setloop)
    {
      m_nav_active=0;
      int t=TriggerMessage(rmsg, flag);
      if (t)
      {
        if (bymarker)
        {     
          if (flag == 't' || (m_altnav == 1) != (t > 0))
          {
            m_altnav = (m_altnav == 1 ? 0 : 1);
          }
        }
        else if (setloop)
        {
          if (flag == 't' || (m_altnav == 2) != (t > 0))
          {
            m_altnav = (m_altnav == 2 ? 0 : 2);
          }
        }
        SETSURFBOOL("REWIND_FORWARD_BYMARKER", (m_altnav == 1));
        SETSURFBOOL("REWIND_FORWARD_SETLOOP", (m_altnav == 2));
      }
      return true;
    }

    if (!strcmp(pattern, "SCRUB"))
    {   
      m_nav_active=0;
      if (flag == 'r')
      {
        int t=TriggerMessage(rmsg, flag);
        if (t < 0) CSurf_OnRew(1);
        else if (t > 0) CSurf_OnFwd(1);
      }
      return true;
    }

    if (!strcmp(pattern, "PLAY_RATE"))
    {
      if (strchr("nfr", flag))
      {
        double pr=GetFloatArg(rmsg, flag); 
        if (flag == 'r')
        {
          double opr=Master_GetPlayRate(0);
          opr=Master_NormalizePlayRate(opr, false);
          pr=opr+ROTARY_STEP*2.0*(pr-m_rotarycenter)/(m_rotaryhi-m_rotarylo);
        }
        if (flag == 'n' || flag == 'r')
        {
          pr=Master_NormalizePlayRate(pr, true);
        }
        CSurf_OnPlayRateChange(pr);
      }
      return true;
    }

    if (!strcmp(pattern, "TEMPO"))
    {
      if (strchr("nfr", flag))
      {
        double bpm=GetFloatArg(rmsg, flag);         
        if (flag == 'r')
        {
          double obpm=Master_GetTempo();
          obpm=Master_NormalizeTempo(obpm, false);
          bpm=obpm+ROTARY_STEP*2.0*(bpm-m_rotarycenter)/(m_rotaryhi-m_rotarylo);
        }
        if (flag == 'n' || flag == 'r')
        {
          bpm=Master_NormalizeTempo(bpm, true);
        }
        CSurf_OnTempoChange(bpm);
      }
      return true;
    }

    return false;
  }

  bool ProcessStateAction(OscMessageRead* rmsg, const char* pattern, char flag, int* wc, int numwc)
  {
    if (!strstarts(pattern, "DEVICE_") && !strstarts(pattern, "REAPER_")) return false;

    if (strends(pattern, "_COUNT"))
    {
      int ival=-1;
      if (flag == 'i')
      {
        const int* i=rmsg->PopIntArg(false);
        if (i) ival=*i;
      }
      else if (numwc > 0 && TriggerMessage(rmsg, flag) > 0)
      {
        ival=wc[0];
      }
      if (ival > 0)
      {              
        if (strends(pattern, "_TRACK_COUNT"))
        {
          m_trackbanksize=ival;
          SetTrackListChange();
        }
        else if (strends(pattern, "_SEND_COUNT"))
        {
          m_sendbanksize=ival;
          SetTrackListChange();
        }
        else if (strends(pattern, "_RECEIVE_COUNT"))
        {
          m_recvbanksize=ival;
          SetTrackListChange();
        }
        else if (strends(pattern, "_FX_COUNT"))
        {
          m_fxbanksize=ival;
          SetTrackListChange();
        }
        else if (strends(pattern, "_FX_PARAM_COUNT"))
        {
          m_fxparmbanksize=ival;
          SetActiveFXChange();
        }
        else if (strends(pattern, "_FX_INST_PARAM_COUNT"))
        {
          m_fxinstparmbanksize=ival;
          SetActiveFXInstChange();
        }
      }
      return true;
    }

    if (strstr(pattern, "_FOLLOWS"))
    {
      if (flag == 's' || TriggerMessage(rmsg, flag) > 0)
      {
        if (flag == 's')
        {
          const char* s=rmsg->PopStringArg(false);
          if (s)
          {      
            if (!strcmp(pattern, "REAPER_TRACK_FOLLOWS"))
            {
              if (!strcmp(s, "DEVICE")) pattern="REAPER_TRACK_FOLLOWS_DEVICE";
              else pattern="REAPER_TRACK_FOLLOWS_REAPER";
            }
            else if (!strcmp(pattern, "DEVICE_TRACK_FOLLOWS"))
            {
              if (!strcmp(s, "LAST_TOUCHED")) pattern="DEVICE_TRACK_FOLLOWS_LAST_TOUCHED";
              else pattern="DEVICE_TRACK_FOLLOWS_DEVICE";
            }
            else if (!strcmp(pattern, "DEVICE_TRACK_BANK_FOLLOWS"))
            {
              if (!strcmp(s, "MIXER")) pattern="DEVICE_TRACK_BANK_FOLLOWS_MIXER";
              else pattern="DEVICE_TRACK_BANK_FOLLOWS_DEVICE";
            }
            else if (!strcmp(pattern, "DEVICE_FX_FOLLOWS"))
            {
              if (!strcmp(s, "LAST_TOUCHED")) pattern="DEVICE_FX_FOLLOWS_LAST_TOUCHED";
              else if (!strcmp(s, "FOCUSED")) pattern="DEVICE_FX_FOLLOWS_FOCUSED";
              else pattern="DEVICE_FX_FOLLOWS_DEVICE";
            }
          }
        }
        
        if (strstarts(pattern, "REAPER_TRACK_FOLLOWS_"))
        {
          m_followflag &= ~16;
          if (!strcmp(pattern, "REAPER_TRACK_FOLLOWS_DEVICE"))
          {
            m_followflag |= 16;
            MediaTrack* tr=CSurf_TrackFromID(m_curtrack, !!(m_followflag&8));
            if (tr) SetOnlyTrackSelected(tr);
          }
        }
        else if (strstarts(pattern, "DEVICE_TRACK_FOLLOWS_"))
        {
          m_followflag &= ~1;
          if (!strcmp(pattern, "DEVICE_TRACK_FOLLOWS_LAST_TOUCHED")) 
          {
            m_followflag |= 1;
            MediaTrack* tr=GetLastTouchedTrack();
            if (tr) Extended(CSURF_EXT_SETLASTTOUCHEDTRACK, tr, 0, 0);
          }
        }

        else if (strstarts(pattern, "DEVICE_TRACK_BANK_FOLLOWS_"))
        {
          m_followflag &= ~8;
          if (!strcmp(pattern, "DEVICE_TRACK_BANK_FOLLOWS_MIXER"))
          {
            m_followflag |= 8;
            MediaTrack* tr=GetMixerScroll();
            if (tr) Extended(CSURF_EXT_SETMIXERSCROLL, tr, 0, (void*)(INT_PTR)1);
          }
        }
        else if (strstarts(pattern, "DEVICE_FX_FOLLOWS_"))
        {
          m_followflag &= ~(2|4);
          int tidx=-1;
          int fxidx=-1;
          if (!strcmp(pattern, "DEVICE_FX_FOLLOWS_LAST_TOUCHED"))
          {
            m_followflag |= 2;
            if (GetLastTouchedFX(&tidx, &fxidx, 0))
            {
              MediaTrack* tr=CSurf_TrackFromID(tidx, false);
              if (tr) Extended(CSURF_EXT_SETLASTTOUCHEDFX, tr, 0, &fxidx);
            }
          }
          else if (!strcmp(pattern, "DEVICE_FX_FOLLOWS_FOCUSED"))
          {
            m_followflag |= 4;
            if (GetFocusedFX(&tidx, 0, &fxidx))
            {
              MediaTrack* tr=CSurf_TrackFromID(tidx, false);
              if (tr) Extended(CSURF_EXT_SETFOCUSEDFX, tr, 0, &fxidx);
            }
          }
        }
      }
      return true;
    }

    if (strends(pattern, "_SELECT"))
    {
      int ival=-1;
      if (flag == 'i')
      {
        const int* i=rmsg->PopIntArg(false);
        if (i) ival=*i;
      }
      else if (numwc > 0 && TriggerMessage(rmsg, flag) > 0)
      {
        ival=wc[0];
      }
      if (ival >= 0)
      {   
        if (strends(pattern, "_TRACK_SELECT"))
        {
          int numtracks=CSurf_NumTracks(!!(m_followflag&8));  
          m_curtrack=m_curbankstart+ival;
          if (m_curtrack < 0) m_curtrack=0;
          else if (m_curtrack > numtracks) m_curtrack=numtracks;
          if (m_followflag&16)
          {
            MediaTrack* tr=CSurf_TrackFromID(m_curtrack, !!(m_followflag&8));
            if (tr) SetOnlyTrackSelected(tr);
          }
          SetActiveTrackChange();
        }
        else if (strends(pattern, "_TRACK_BANK_SELECT"))
        {
          int numtracks=CSurf_NumTracks(!!(m_followflag&8));
          m_curbankstart=(ival-1)*m_trackbanksize;
          if (m_curbankstart < 0) m_curbankstart=0;
          else if (m_curbankstart >= numtracks-1) m_curbankstart=max(0,numtracks-2);
          m_curbankstart -= m_curbankstart%m_trackbanksize;
          if (m_followflag&8)
          {
            MediaTrack* tr=CSurf_TrackFromID(m_curbankstart+1, true);
            if (tr) 
            {
              tr=SetMixerScroll(tr);
              Extended(CSURF_EXT_SETMIXERSCROLL, tr, 0, (void*)(INT_PTR)1);
            }
          }
          else
          {
            SetTrackListChange();
          }
        }
        else if (strends(pattern, "_FX_SELECT"))
        {
          int numfx=0;
          MediaTrack* tr=CSurf_TrackFromID(m_curtrack, !!(m_followflag&8));
          if (tr) numfx=TrackFX_GetCount(tr);
          m_curfx=ival-1;
          if (m_curfx < 0) m_curfx=0;
          else if (m_curfx >= numfx) m_curfx=max(0,numfx);
          SetActiveFXChange();
        }
        else if (strends(pattern, "FX_PARAM_BANK_SELECT"))
        {
          int numparms=0;
          MediaTrack* tr=CSurf_TrackFromID(m_curtrack, !!(m_followflag&8));
          if (tr) numparms=TrackFX_GetNumParams(tr, m_curfx);
          m_curfxparmbankstart=(ival-1)*m_fxparmbanksize;
          if (m_curfxparmbankstart < 0) m_curfxparmbankstart=0;
          else if (m_curfxparmbankstart >= numparms) m_curfxparmbankstart=max(0,numparms-1);
          m_curfxparmbankstart -= m_curfxparmbankstart%m_fxparmbanksize;
          SetActiveFXChange();
        }
        else if (strends(pattern, "FX_INST_PARAM_BANK_SELECT"))
        {
          int numparms=0;
          MediaTrack* tr=CSurf_TrackFromID(m_curtrack, !!(m_followflag&8));
          int fxidx=-1;
          if (tr) fxidx=TrackFX_GetInstrument(tr);
          if (fxidx >= 0) numparms=TrackFX_GetNumParams(tr, fxidx);
          m_curfxinstparmbankstart=(ival-1)*m_fxinstparmbanksize;
          if (m_curfxinstparmbankstart < 0) m_curfxinstparmbankstart=0;
          else if (m_curfxinstparmbankstart >= numparms) m_curfxinstparmbankstart=max(0,numparms-1);
          m_curfxinstparmbankstart -= m_curfxinstparmbankstart%m_fxinstparmbanksize;
          SetActiveFXInstChange();
        }
      }
      return true;
    }

    bool prevtr=strends(pattern, "_PREV_TRACK");
    bool nexttr=strends(pattern, "_NEXT_TRACK");
    if (prevtr || nexttr)
    {
      if (TriggerMessage(rmsg, flag) > 0)
      {
        int numtracks=CSurf_NumTracks(!!(m_followflag&8));      
        int dir = (prevtr ? -1 : 1);
        m_curtrack += dir;
        if (m_curtrack < 0) m_curtrack=numtracks;
        else if (m_curtrack > numtracks) m_curtrack=0;
        if (!m_curtrack && numtracks)
        {
          int vis=GetMasterTrackVisibility();
          if (!(vis&1)) m_curtrack=(dir > 0 ? 1 : numtracks);
        }
        if (m_followflag&16)
        {
          MediaTrack* tr=CSurf_TrackFromID(m_curtrack, !!(m_followflag&8));
          if (tr) SetOnlyTrackSelected(tr);
        }
        SetActiveTrackChange();
      }
      return true;
    }


    bool prevbank=strends(pattern, "_PREV_TRACK_BANK");
    bool nextbank=strends(pattern, "_NEXT_TRACK_BANK");
    if (prevbank || nextbank)
    {
      if (TriggerMessage(rmsg, flag) > 0)
      {
        int obank=m_curbankstart;
        int numtracks=CSurf_NumTracks(!!(m_followflag&8));
        if (prevbank) m_curbankstart -= m_trackbanksize;
        else m_curbankstart += m_trackbanksize;
        if (m_curbankstart < 0)
        {
          if (m_followflag&8) m_curbankstart=0;
          else m_curbankstart=max(0,numtracks-1);
        }
        else if (m_curbankstart >= numtracks) 
        {
          if (m_followflag&8) m_curbankstart=max(0,numtracks-1);
          else m_curbankstart=0;
        }
        m_curbankstart -= m_curbankstart%m_trackbanksize;
        if (m_followflag&8)
        {
          MediaTrack* tr=CSurf_TrackFromID(m_curbankstart+1, true);
          if (tr) 
          {
            SetMixerScroll(tr);
            Extended(CSURF_EXT_SETMIXERSCROLL, tr, 0, (void*)(INT_PTR)1);
          }
        }
        else
        {
          SetTrackListChange();
        }
      }
      return true;
    }

    bool prevfx=strends(pattern, "_PREV_FX");
    bool nextfx=strends(pattern, "_NEXT_FX");
    if (prevfx || nextfx)
    {
      if (TriggerMessage(rmsg, flag) > 0)
      {
        int numfx=0;
        MediaTrack* tr=CSurf_TrackFromID(m_curtrack, !!(m_followflag&8));
        if (tr) numfx=TrackFX_GetCount(tr);
        m_curfx += (prevfx ? -1 : 1);
        if (m_curfx < 0) m_curfx=max(0,numfx-1);
        else if (m_curfx >= numfx) m_curfx=0;
        SetActiveFXChange();
      }
      return true;
    }

    bool prevfxparmbank=strends(pattern, "_PREV_FX_PARAM_BANK");
    bool nextfxparmbank=strends(pattern, "_NEXT_FX_PARAM_BANK");
    if (prevfxparmbank || nextfxparmbank)
    {
      if (TriggerMessage(rmsg, flag) > 0)
      {
        int numparms=0;
        MediaTrack* tr=CSurf_TrackFromID(m_curtrack, !!(m_followflag&8));
        if (tr) numparms=TrackFX_GetNumParams(tr, m_curfx);
        if (prevfxparmbank) m_curfxparmbankstart -= m_fxparmbanksize;
        else m_curfxparmbankstart += m_fxparmbanksize;     
        if (m_curfxparmbankstart < 0) m_curfxparmbankstart=max(0,numparms-1);
        else if (m_curfxparmbankstart >= numparms) m_curfxparmbankstart=0;
        m_curfxparmbankstart -= m_curfxparmbankstart%m_fxparmbanksize;
        SetActiveFXChange();
      }
      return true;
    }

    bool prevfxinstparmbank=strends(pattern, "_PREV_FX_INST_PARAM_BANK");
    bool nextfxinstparmbank=strends(pattern, "_NEXT_FX_INST_PARAM_BANK");
    if (prevfxinstparmbank || nextfxinstparmbank)
    {
      if (TriggerMessage(rmsg, flag) > 0)
      {
        int numparms=0;
        MediaTrack* tr=CSurf_TrackFromID(m_curtrack, !!(m_followflag&8));
        int fxidx=-1;
        if (tr) fxidx=TrackFX_GetInstrument(tr);
        if (fxidx >= 0) numparms=TrackFX_GetNumParams(tr, fxidx);
        if (prevfxinstparmbank) m_curfxinstparmbankstart -= m_fxinstparmbanksize;
        else m_curfxinstparmbankstart += m_fxinstparmbanksize;     
        if (m_curfxinstparmbankstart < 0) m_curfxinstparmbankstart=max(0,numparms-1);
        else if (m_curfxinstparmbankstart >= numparms) m_curfxinstparmbankstart=0;
        m_curfxinstparmbankstart -= m_curfxinstparmbankstart%m_fxinstparmbanksize;
        SetActiveFXInstChange();
      }
      return true;
    }

    return true;
  }

  bool ProcessTrackAction(OscMessageRead* rmsg, const char* pattern, char flag, int* wc, int numwc)
  {
    if (!strstarts(pattern, "TRACK_") && !strstarts(pattern, "MASTER_"))
    {
      return false;
    }
    if (strstr(pattern, "_SEND_") || strstr(pattern, "_RECV_")) 
    {
      return false;
    }

    int tidx=m_curtrack;
    if (strstarts(pattern, "MASTER_")) 
    {
      tidx=0;
    }
    else if (numwc) 
    {
      tidx=m_curbankstart+wc[0];
    }
    MediaTrack* tr=CSurf_TrackFromID(tidx, !!(m_followflag&8));
    if (!tr) return true;  // badly formed pattern
    
    if (strends(pattern, "_VOLUME"))
    {
      bool hasarg=false;
      double v=GetFloatArg(rmsg, flag, &hasarg);
      if (hasarg)
      {
        if (strchr("nbtr", flag)) v=SLIDER2DB(v*1000);
        v=DB2VAL(v);
        CSurf_OnVolumeChangeEx(tr, v, false, true); 
        SetSurfaceVolume(tr, v);

        if (!(m_supports_touch&1) && tidx >= 0 && tidx < MAX_LASTTOUCHED_TRACK)
        {
          m_vol_lasttouch[tidx]=GetTickCount();
        }
      }
      return true;
    }

    bool pan=strends(pattern, "_PAN");
    bool pan2=strends(pattern, "_PAN2");
    if (pan || pan2)
    {
      bool hasarg=false;
      double v=GetFloatArg(rmsg, flag, &hasarg);
      if (hasarg)
      {
        if (strchr("nbtr", flag)) v=v*2.0-1.0;
        if (pan) CSurf_OnPanChangeEx(tr, v, false, true);
        else if (pan2) CSurf_OnWidthChangeEx(tr, v, false, true);
        int panmode=PAN_MODE_NEW_BALANCE;
        double tpan[2] = { 0.0, 0.0 };
        GetTrackUIPan(tr, &tpan[0], &tpan[1], &panmode);
        Extended(CSURF_EXT_SETPAN_EX_IMPL, &tidx, tpan, &panmode);

        if (!(m_supports_touch&2) && tidx >= 0 && tidx < MAX_LASTTOUCHED_TRACK)
        {
          m_pan_lasttouch[tidx]=GetTickCount();
        }
      }
      return true;
    }

    if (strends(pattern, "_SELECT"))
    {
      int t=TriggerMessage(rmsg, flag);
      if (t)
      {
        bool sel=IsTrackSelected(tr);
        if (flag == 't' || sel != (t > 0))
        {  
          CSurf_OnSelectedChange(tr, !sel);
          SetSurfaceSelected(tr, !sel);
        }
      }
      return true;
    }

    bool mute=strends(pattern, "_MUTE");
    bool solo=strends(pattern, "_SOLO");
    bool recarm=strends(pattern, "_REC_ARM");
    bool monitor=strends(pattern, "_MONITOR");
    if (mute || solo || recarm || monitor)
    {
      if (!tidx && !mute && !solo) return true;
      int t=TriggerMessage(rmsg, flag);
      if (t)
      {      
        int flags=0;
        GetTrackInfo((INT_PTR)tr, &flags);
     
        int a=-1;
        if (mute && (flag == 't' || (flags&8) != (t > 0)))
        {
          a=!(flags&8);
        }
        else if (solo && (flag == 't' || (flags&16) != (t > 0)))
        {
          a=!(flags&16);
        }
        else if (recarm && (flag == 't' || (flags&64) != (t > 0)))
        {
          a=!(flags&64);
        }
        else if (monitor)
        {
          a=0;
          if (flags&128) a=1;
          else if (flags&256) a=2;
          if (flag == 't' || !!a != (t > 0))
          {
            if (++a > 2) a=0;
          }
          else
          {
            a=-1;
          }
        }

        if (a >= 0)
        {
          if (mute) CSurf_OnMuteChangeEx(tr, a, true);                  
          else if (solo) CSurf_OnSoloChangeEx(tr, a, true);
          else if (recarm) CSurf_OnRecArmChangeEx(tr, a, true); // triggers UpdateAllExternalSurfaces
          else if (monitor) CSurf_OnInputMonitorChangeEx(tr, a, true);      

          bool sel=IsTrackSelected(tr);
          int n=CSurf_NumTracks(!!(m_followflag&8));
          int i;
          for (i=0; i <= n; ++i)
          {
            MediaTrack* ttr=CSurf_TrackFromID(i, !!(m_followflag&8));
            if (tr == ttr || (sel && IsTrackSelected(ttr)))
            {
              if (mute) SetSurfaceMute(ttr, !!a);
              else if (solo) SetSurfaceSolo(ttr, !!a);
              else if (recarm) SetSurfaceRecArm(ttr, !!a);
              else if (monitor) Extended(CSURF_EXT_SETINPUTMONITOR, ttr, &a, 0);
            }
          }
        }
      }
      return true;
    }

    int automode=-1;
    if (!strcmp(pattern, "TRACK_AUTO_TRIM")) automode=AUTO_MODE_TRIM;
    else if (!strcmp(pattern, "TRACK_AUTO_READ")) automode=AUTO_MODE_READ;
    else if (!strcmp(pattern, "TRACK_AUTO_LATCH")) automode=AUTO_MODE_LATCH;
    else if (!strcmp(pattern, "TRACK_AUTO_TOUCH")) automode=AUTO_MODE_TOUCH;
    else if (!strcmp(pattern, "TRACK_AUTO_WRITE")) automode=AUTO_MODE_WRITE;
    if (automode >= 0)
    {
      int t=TriggerMessage(rmsg, flag);
      if (t)
      {
        if (t < 0) automode=AUTO_MODE_TRIM;   
        
        bool sel=IsTrackSelected(tr);

        int i;
        int n=CSurf_NumTracks(!!(m_followflag&8));
        for (i=-1; i < n; ++i)
        {         
          if (i >= 0 && !sel) break;
          if (i == tidx) continue;
          MediaTrack* ttr = (i < 0 ? tr : CSurf_TrackFromID(i, !!(m_followflag&8)));
          if (!ttr || (i >= 0 && !IsTrackSelected(ttr))) continue;
          
          SetTrackAutomationMode(ttr, automode);
          SetSurfaceAutoMode(ttr, automode);
        }
      }
      return true;
    }

    bool voltouch=!strcmp(pattern, "TRACK_VOLUME_TOUCH");
    bool pantouch=!strcmp(pattern, "TRACK_PAN_TOUCH");
    if (voltouch || pantouch)
    {
      int t=TriggerMessage(rmsg, flag);
      if (t && tidx >= 0 && tidx < MAX_LASTTOUCHED_TRACK)
      {
        char mask = (voltouch ? 1 : 2);      
        if (t > 0) m_hastouch[tidx] |= mask;
        else m_hastouch[tidx] &= ~mask;
      }
      return true;
    }
  
    return true;
  }

  bool ProcessFXAction(OscMessageRead* rmsg, const char* pattern, char flag, int* wc, int numwc)
  {
    if (!strstarts(pattern, "FX_") && !strstarts(pattern, "LAST_TOUCHED_FX_"))
    {
      return false;
    }

    MediaTrack* tr=0;
    int tidx=m_curtrack;
    int fxidx=m_curfx;

    // continuous actions

    bool fxparm=!strcmp(pattern, "FX_PARAM_VALUE");
    bool fxwet=!strcmp(pattern, "FX_WETDRY");
    bool fxeqwet=!strcmp(pattern, "FX_EQ_WETDRY");
    bool instparm=!strcmp(pattern, "FX_INST_PARAM_VALUE");
    bool lasttouchedparm=!strcmp(pattern, "LAST_TOUCHED_FX_PARAM_VALUE");
    if (fxparm || fxwet || fxeqwet || instparm || lasttouchedparm)
    {      
      const float* f=rmsg->PopFloatArg(false);
      if (f)
      {
        int parmidx=-1;

        if (fxparm)
        {
          if (!numwc) return true; // need at least parmidx
          parmidx=m_curfxparmbankstart+wc[0]-1;
          if (numwc > 1) fxidx=wc[1]-1;          
          if (numwc > 2) tidx=m_curbankstart+wc[2];
          tr=CSurf_TrackFromID(tidx, !!(m_followflag&8));
        }
        else if (fxwet)
        {
          if (numwc) fxidx=wc[0]-1;
          if (numwc > 1) tidx=m_curbankstart+wc[1];
          tr=CSurf_TrackFromID(tidx, !!(m_followflag&8));
          int numparms=0;
          if (tr) numparms=TrackFX_GetNumParams(tr, fxidx);
          parmidx=numparms-1;
        }
        else if (fxeqwet)
        {
          if (numwc) tidx=m_curbankstart+wc[0];
          tr=CSurf_TrackFromID(tidx, !!(m_followflag&8));
          if (tr) fxidx=TrackFX_GetEQ(tr, !!(m_followflag&32));
          int numparms=0;
          if (tr) numparms=TrackFX_GetNumParams(tr, fxidx);
          parmidx=numparms-1;
        }
        else if (instparm)
        {
          if (!numwc) return true; // need at least parmidx
          parmidx=m_curfxinstparmbankstart+wc[0]-1;
          if (numwc > 1) tidx=m_curbankstart+wc[1];
          tr=CSurf_TrackFromID(tidx, !!(m_followflag&8));         
          if (tr) fxidx=TrackFX_GetInstrument(tr);          
        }
        else if (lasttouchedparm)
        {
          if (!GetLastTouchedFX(&tidx, &fxidx, &parmidx)) return true;
          tr=CSurf_TrackFromID(tidx, false);
        }

        if (tr && fxidx >= 0 && parmidx >= 0)
        {
          TrackFX_SetParamNormalized(tr, fxidx, parmidx, *f);
        }
      }
      return true;          
    }

    int bandidx=0;
    bool isinst=strstarts(pattern, "FX_INST_");
    bool iseq=strstarts(pattern, "FX_EQ_");
    if (isinst || iseq)
    {
      if (iseq && strstr(pattern, "_BAND_"))
      {
        if (numwc) bandidx=wc[0];
        if (numwc > 0) tidx=m_curbankstart+wc[1];
      }
      else if (numwc) 
      {
        tidx=m_curbankstart+wc[0];
      }
      tr=CSurf_TrackFromID(tidx, !!(m_followflag&8));
      if (!tr) return true;
      if (isinst) fxidx=TrackFX_GetInstrument(tr);
      else if (iseq) fxidx=TrackFX_GetEQ(tr, !!(m_followflag&32));
      if (fxidx < 0) return true;    

      if (strends(pattern, "_BYPASS")) pattern="FX_BYPASS";
      else if (strends(pattern, "_OPEN_UI")) pattern="FX_OPEN_UI";
      else if (strends(pattern, "_PREV_PRESET")) pattern="FX_PREV_PRESET";
      else if (strends(pattern, "_NEXT_PRESET")) pattern="FX_NEXT_PRESET";
      else if (strends(pattern, "_PRESET")) pattern="FX_PRESET";
    }
    else
    {
      if (numwc) fxidx=wc[0]-1;
      if (numwc > 1) tidx=m_curbankstart+wc[1];
      tr=CSurf_TrackFromID(tidx, !!(m_followflag&8));
    }

    if (!strcmp(pattern, "FX_BYPASS"))
    {
      int t=TriggerMessage(rmsg, flag);
      if (tr && t)
      {
        bool en=TrackFX_GetEnabled(tr, fxidx);
        if (flag == 't' || en != (t > 0))
        {
          TrackFX_SetEnabled(tr, fxidx, !en);
        }
      }
      return true;
    }

    if (!strcmp(pattern, "FX_OPEN_UI"))
    {
      int t=TriggerMessage(rmsg, flag);
      if (tr && t)
      {
        bool open=TrackFX_GetOpen(tr, fxidx);
        if (flag == 't' || open != (t > 0))
        {
          TrackFX_SetOpen(tr, fxidx, !open);
        }
      }
      return true;
    }

    bool prevpreset=!strcmp(pattern, "FX_PREV_PRESET");
    bool nextpreset=!strcmp(pattern, "FX_NEXT_PRESET");
    if (prevpreset || nextpreset)
    {
      if (TriggerMessage(rmsg, flag) > 0 && tr)
      {
        int dir = (prevpreset ? -1 : 1);
        TrackFX_NavigatePresets(tr, fxidx, dir);
        if (isinst) SetActiveFXInstChange();
        else SetActiveFXChange();
      }
      return true;
    }

    if (!strcmp(pattern, "FX_PRESET"))
    {
      const char* s=rmsg->PopStringArg(false);
      if (tr && s && s[0])
      {
        TrackFX_SetPreset(tr, fxidx, s);
      }     
      return true;
    }

    if (strstarts(pattern, "FX_EQ_"))
    {
      // we know tr, fxidx, bandidx are valid

      int bandtype=-2;
      int parmtype=-2;       

      if (strends(pattern, "_MASTER_GAIN"))
      {
        bandtype=parmtype=-1;
      }
      else
      {
        // bandtype: -1=master gain, 0=hipass, 1=loshelf, 2=band, 3=notch, 4=hishelf, 5=lopass
        if (strstr(pattern, "_HIPASS_")) bandtype=0;
        else if (strstr(pattern, "_LOSHELF_")) bandtype=1;
        else if (strstr(pattern, "_BAND_")) bandtype=2;
        else if (strstr(pattern, "_NOTCH_")) bandtype=3;
        else if (strstr(pattern, "_HISHELF_")) bandtype=4;
        else if (strstr(pattern, "_LOPASS_")) bandtype=5;
      
        if (strends(pattern, "_FREQ")) parmtype=0;
        else if (strends(pattern, "_GAIN")) parmtype=1;
        else if (strends(pattern, "_Q")) parmtype=2;
        else if (strends(pattern, "_BYPASS")) parmtype=-1;
      }

      if (bandtype >= -1 && parmtype >= -1)
      {       
        if (bandtype >= 0 && parmtype == -1)
        {
          int t=TriggerMessage(rmsg, flag);
          if (t)
          {
            bool en=TrackFX_GetEQBandEnabled(tr, fxidx, bandtype, bandidx);
            if (flag == 't' || en != (t > 0))
            {
              TrackFX_SetEQBandEnabled(tr, fxidx, bandtype, bandidx, !en);
            }
          }
        }
        else
        {
          bool hasarg=false;
          double v=GetFloatArg(rmsg, flag, &hasarg);
          if (hasarg)
          {
            bool isnorm=!strchr("fi", flag);
            TrackFX_SetEQParam(tr, fxidx, bandtype, bandidx, parmtype, v, isnorm);
          }
        }
      }
      return true;
    }

    return true;
  }

  bool ProcessSendAction(OscMessageRead* rmsg, const char* pattern, char flag, int* wc, int numwc)
  {
    if (!strstr(pattern, "_SEND_") && !strstr(pattern, "_RECV_")) return false;

    if (!numwc) return true;  // badly formed pattern
    int sidx=wc[0];
    if (sidx) --sidx;

    int tidx=m_curtrack;
    if (strstarts(pattern, "MASTER_") && strstr(pattern, "_SEND_"))
    {
      tidx=0;
    }
    else if (numwc > 1) 
    {
      tidx=m_curbankstart+wc[1];
    }
    MediaTrack* tr=CSurf_TrackFromID(tidx, !!(m_followflag&8));
    if (!tr) return true; // badly formed pattern

    bool sendvol=strends(pattern, "_SEND_VOLUME");
    bool recvvol=strends(pattern, "_RECV_VOLUME");
    if (sendvol || recvvol)
    {
      bool hasarg=false;
      double v=GetFloatArg(rmsg, flag, &hasarg);
      if (hasarg)
      {
        if (strchr("nbtr", flag)) v=SLIDER2DB(v*1000.0);
        v=DB2VAL(v);
        if (sendvol) 
        {
          CSurf_OnSendVolumeChange(tr, sidx, v, false);
          Extended(CSURF_EXT_SETSENDVOLUME, tr, &sidx, &v);
        }
        else if (recvvol)
        {
          CSurf_OnRecvVolumeChange(tr, sidx, v, false);
          Extended(CSURF_EXT_SETRECVVOLUME, tr, &sidx, &v);
        }      
      }
      return true;
    }

    bool sendpan=strends(pattern, "_SEND_PAN");
    bool recvpan=strends(pattern, "_RECV_PAN");
    if (sendpan || recvpan)
    {
      bool hasarg=false;
      double v=GetFloatArg(rmsg, flag, &hasarg);
      if (hasarg)
      {
        if (strchr("nbtr", flag)) v=v*2.0-1.0;
        if (sendpan) 
        {
          CSurf_OnSendPanChange(tr, sidx, v, false);
          Extended(CSURF_EXT_SETSENDPAN, tr, &sidx, &v);
        }
        else
        {
          CSurf_OnRecvPanChange(tr, sidx, v, false);
          Extended(CSURF_EXT_SETRECVPAN, tr, &sidx, &v);
        }
      }
      return true;
    }

    return true;
  }

  static void DoOSCAction(const char* pattern, int cmd, const float* val)
  {
    if (!strcmp(pattern, "ACTION"))
    {
      if (val) 
      {
        int ival=(int)(*val*16383.0);   
        KBD_OnMainActionEx(cmd, (ival>>7)&0x7F, ival&0x7F, 0, GetMainHwnd(), 0);
      }
      else
      {
        Main_OnCommand(cmd, 0);
      }
    }
    else if (!strcmp(pattern, "MIDIACTION"))
    {
      MIDIEditor_LastFocused_OnCommand(cmd, false);
    }
    else if (!strcmp(pattern, "MIDILISTACTION"))
    {
      MIDIEditor_LastFocused_OnCommand(cmd, true);
    }
  }

  bool ProcessAction(OscMessageRead* rmsg, const char* pattern, char flag, int* wc, int numwc)
  {
    if (strstr(pattern, "ACTION"))
    {
      if (numwc && TriggerMessage(rmsg, flag) > 0)
      {
        DoOSCAction(pattern, wc[0], 0);
      }
      else
      {
        const int* i=rmsg->PopIntArg(false);
        while (i && *i > 0)
        {
          const float* f=rmsg->PopFloatArg(false);
          DoOSCAction(pattern, *i, f);
          i=rmsg->PopIntArg(false);
        }
      }
      return true;
    }

    return false;
  }

  bool ProcessMessage(OscMessageRead* rmsg)
  { 
    char flag='n';
    int wc[MAX_OSC_WC*MAX_OSC_RPTCNT]; 
    int numwc=0;
    int rptcnt=1;
    const char* msg=rmsg->GetMessage();
    const char* pattern=FindOscMatch(msg, wc, &numwc, &rptcnt, &flag);
    if (!pattern) return false;

    OscVal** lastval=m_lastvals.GetPtr(msg);
    if (lastval) (*lastval)->Invalidate();

    m_curedit=msg;
    m_curflag=flag;
  
    bool ok=false;
    if (rptcnt < 1) rptcnt=1;
    numwc /= rptcnt;
    int i;
    for (i=rptcnt-1; i >= 0; --i)
    {
      int* twc=wc+i*numwc;
      if (!numwc && ProcessGlobalAction(rmsg, pattern, flag)) ok=true;
      else if (ProcessStateAction(rmsg, pattern, flag, twc, numwc)) ok=true;
      else if (ProcessTrackAction(rmsg, pattern, flag, twc, numwc)) ok=true;
      else if (ProcessFXAction(rmsg, pattern, flag, twc, numwc)) ok=true;
      else if (ProcessSendAction(rmsg, pattern, flag, twc, numwc)) ok=true;
      else if (ProcessAction(rmsg, pattern, flag, twc, numwc)) ok=true;
    }

    m_curedit=0;
    m_curflag=0;

    return ok;
  }

  // core implementation for sending data to the csurf
  // nval=normalized
  void SetSurfaceVal(const char* pattern, int* wcval, int numwcval,
    int* ival, double* fval, double* nval, const char* sval)
  {
    if ((!m_osc || !m_osc->m_send_enable) && !m_osc_local) return;
    if (!pattern) return;

    int* keyidx=m_msgkeyidx.GetPtr(pattern);
    if (!keyidx) return;  // code error, asking to send an undefined key string

    int i, j;
    for (i=*keyidx+1; i < m_msgtab.GetSize(); ++i)
    {
      const char* p=m_msgtab.Get(i);
      if (!p) break;

      char flag=*p;
      ++p;

#ifdef _DEBUG
      assert(*p == '/');
#endif

      int* tival=0;
      double* tfval=0;
      const char* tsval=0;
      if (flag == 'i') tival=ival;
      else if (flag == 'f') tfval=fval;
      else if (strchr("nbt", flag)) tfval=nval;
      else if (flag == 's') tsval=sval;
      else return; // don't send 'r' messages to the device

      char msg[512];
      strcpy(msg, p);

      bool rev=false;
      if (numwcval > 1)
      {
        const char* q=strchr(p, '@');
        if (q && *(q+1) != '@') rev=true; // nonconsecutive wildcards
      }
    
      bool wcok=true;
      for (j=0; j < numwcval; ++j)
      {
        char tmp[128];
        int k = (rev ? numwcval-j-1 : j);
        sprintf(tmp, "%d", wcval[k]);
        if (!OscWildcardSub(msg, '@', tmp))
        {
          wcok=false;
          break;
        }
      }
      if (!wcok || strchr(msg, '@')) continue; // need to fill in all wildcards

      if (m_curedit && !strchr("tb", flag) && !strcmp(m_curedit, msg) && m_curflag == flag) 
      {
        continue; // antifeedback
      }

      OscVal** lastval=m_lastvals.GetPtr(msg);
      if (lastval && !(*lastval)->Update(tival, tfval, tsval)) continue;
      if (!lastval) m_lastvals.Insert(msg, new OscVal(tival, tfval, tsval));
 
      OscMessageWrite wmsg;
      wmsg.PushWord(msg);

      if (tival) wmsg.PushIntArg(*tival);
      else if (tfval) wmsg.PushFloatArg((float)*tfval);
      else if (tsval) wmsg.PushStringArg(tsval);

      if (m_osc) OscSendOutput(m_osc, &wmsg);

      if (m_osc_local && m_osc_local->m_callback) 
      {
        int len=0;
        const char* p=wmsg.GetBuffer(&len);
        if (p && len)
        {
          m_osc_local->m_callback(m_osc_local->m_obj, p, len);
        }
      }
    }
  }

  // validate the track, if it could be visible in the csurf send the message
  void SetSurfaceTrackVal(const char* pattern, int tidx,
    int* ival, double* fval, double* nval, const char* sval)
  {
    if (tidx == m_curtrack)
    {
      SetSurfaceVal(pattern, 0, 0, ival, fval, nval, sval);
    }
    if (tidx > m_curbankstart && tidx <= m_curbankstart+m_trackbanksize)
    {
      int wcval[1] = { tidx-m_curbankstart };
      SetSurfaceVal(pattern, wcval, 1, ival, fval, nval, sval);
    }
  }


  static void GetFXName(MediaTrack* tr, int fxidx, char* buf, int buflen)
  {
    buf[0]=0;
    if (TrackFX_GetFXName(tr, fxidx, buf, buflen) && buf[0])
    {
      const char* prefix[] = { "VST: ", "VSTi: ", "JS: ", "DX: ", "DXi: ", "AU: ", "AUi: " };
      int i;
      for (i=0; i < sizeof(prefix)/sizeof(prefix[0]); ++i)
      {
        if (strstarts(buf, prefix[i]))
        {
          memmove(buf, buf+strlen(prefix[i]), strlen(buf)-strlen(prefix[i])+1);
          return;
        }
      }
    }  
  }

  void UpdateSurfaceTrack(int tidx)
  {
    if ((!m_osc || !m_osc->m_send_enable) && !m_osc_local) return;

    MediaTrack* tr=CSurf_TrackFromID(tidx, !!(m_followflag&8));
   
    char buf[512];
    char buf2[512];
    char ibuf[128];

    buf[0]=0;           
    sprintf(ibuf, "%d", tidx);
    const char* p=0;
    if (tr) p=GetTrackInfo((INT_PTR)tr, 0);
    if (p) lstrcpyn(buf, p, sizeof(buf));              
    if (!buf[0] || !strcmp(buf, ibuf))
    {
      if (!tidx) strcpy(buf, "MASTER");
      else sprintf(buf, "Track %d", tidx);
    }   
    SetSurfaceTrackVal("TRACK_NAME", tidx, 0, 0, 0, buf);
    SetSurfaceTrackVal("TRACK_NUMBER", tidx, 0, 0, 0, ibuf);

    bool sel=false;
    if (tr) sel=IsTrackSelected(tr);
    SetSurfaceSelectedImpl(tidx, sel);
    
    double vol=0.0;
    if (tr) GetTrackUIVolPan(tr, &vol, 0);
    SetSurfaceVolumeImpl(tidx, vol);

    double pan[2] = { 0.0, 0.0 };
    int panmode=PAN_MODE_NEW_BALANCE;
    if (tr) GetTrackUIPan(tr, &pan[0], &pan[1], &panmode);
    Extended(CSURF_EXT_SETPAN_EX_IMPL, &tidx, pan, &panmode);

    int flags=0;
    if (tr) GetTrackInfo((INT_PTR)tr, &flags);
    SetSurfaceMuteImpl(tidx, !!(flags&8));
    SetSurfaceSoloImpl(tidx, !!(flags&16));
    if (tidx)
    {
      SetSurfaceRecArmImpl(tidx, !!(flags&64));
      int a=0;
      if (flags&128) a=1;
      else if (flags&256) a=2;
      Extended(CSURF_EXT_SETINPUTMONITOR_IMPL, &tidx, &a, 0);
    }

    int mode=0;
    if (tr) mode=GetTrackAutomationMode(tr);
    SetSurfaceAutoModeImpl(tidx, mode);

    bool iscur = (tidx == m_curtrack);
    bool isbank = (tidx > m_curbankstart && tidx <= m_curbankstart+m_trackbanksize);

    int i;
    for (i=0; i < m_sendbanksize; ++i)
    {
      buf[0]=0;
      if (tr) GetTrackSendName(tr, i, buf, sizeof(buf));
      if (!buf[0]) sprintf(buf, (!tidx ? "Aux %d" : "Send %d"), i+1);
      int wc[2] = { i+1, tidx-m_curbankstart };     
      if (!tidx) SETSURFSTRWC("MASTER_SEND_NAME", wc, 1, buf);
      if (iscur) SETSURFSTRWC("TRACK_SEND_NAME", wc, 1, buf);
      if (isbank) SETSURFSTRWC("TRACK_SEND_NAME", wc, 2, buf);
    
      double vol=0.0;
      double pan=0.0;
      if (tr) GetTrackSendUIVolPan(tr, i, &vol, &pan);
      Extended(CSURF_EXT_SETSENDVOLUME_IMPL, &tidx, &i, &vol);
      Extended(CSURF_EXT_SETSENDPAN_IMPL, &tidx, &i, &pan);
    }

    if (tidx)
    {
      for (i=0; i < m_recvbanksize; ++i)
      {
        buf[0]=0;
        if (tr) GetTrackReceiveName(tr, i, buf, sizeof(buf));
        if (!buf[0]) sprintf(buf, "Recv %d", i+1);
        int wc[2] = { i+1, tidx-m_curbankstart };     
        if (iscur) SETSURFSTRWC("TRACK_RECV_NAME", wc, 1, buf);
        if (isbank) SETSURFSTRWC("TRACK_RECV_NAME", wc, 2, buf);

        double vol=0.0;
        double pan=0.0;
        if (tr) GetTrackReceiveUIVolPan(tr, i, &vol, &pan);
        Extended(CSURF_EXT_SETRECVVOLUME_IMPL, &tidx, &i, &vol);
        Extended(CSURF_EXT_SETRECVPAN_IMPL, &tidx, &i, &pan);
      }
    }
 
    for (i=0; i < m_fxbanksize; ++i)
    {
      bool en=false;
      bool open=false;
      buf[0]=buf2[0]=ibuf[0]=0;
      if (tr) 
      {
        int numfx=TrackFX_GetCount(tr);
        if (i < numfx)
        {      
          en=TrackFX_GetEnabled(tr, i);
          open=TrackFX_GetOpen(tr, i);
          GetFXName(tr, i, buf, sizeof(buf));
          sprintf(ibuf, "%d", i+1);
          TrackFX_GetPreset(tr, i, buf2, sizeof(buf2));
        }
      }

      bool iscurfx = (iscur && i == m_curfx);
      {
        int wc[2] = { i+1, tidx-m_curbankstart };
        int k;
        for (k=0; k <= 2; ++k)
        {
          if ((!k && iscurfx) || (k == 1 && iscur) || (k == 2 && isbank))
          {
            SETSURFSTRWC("FX_NAME", wc, k, buf);
            SETSURFSTRWC("FX_NUMBER", wc, k, ibuf); 
            SETSURFSTRWC("FX_PRESET", wc, k, buf2);
            SETSURFBOOLWC("FX_BYPASS", wc, k, en);
            SETSURFBOOLWC("FX_OPEN_UI", wc, k, open);
          }
        }
      }

      int fxidx=-1;
      if (tr) fxidx=TrackFX_GetInstrument(tr);

      en=false;
      open=false;
      buf[0]=buf2[0]=0;
      if (tr && fxidx >= 0)
      {
        en=TrackFX_GetEnabled(tr, fxidx);
        open=TrackFX_GetOpen(tr, fxidx);
        GetFXName(tr, fxidx, buf, sizeof(buf));
        TrackFX_GetPreset(tr, fxidx, buf2, sizeof(buf2));       
      }
  
      {
        int wc[1] = { tidx-m_curbankstart };
        int k;
        for (k=0; k <= 1; ++k)
        {
          if ((iscur && k == 0) || (isbank && k == 1))
          {
            SETSURFSTRWC("FX_INST_NAME", wc, k, buf);         
            SETSURFSTRWC("FX_INST_PRESET", wc, k, buf2);
            SETSURFBOOLWC("FX_INST_BYPASS", wc, k, en);
            SETSURFBOOLWC("FX_INST_OPEN_UI", wc, k, open);
          }
        }
      }
    }
  }

  // track order or current bank changed
  void SetTrackListChange()
  {
    if ((!m_osc || !m_osc->m_send_enable) && !m_osc_local) return;

    if (!m_surfinit)
    {
      m_surfinit=true;

      SETSURFBOOL("REWIND_FORWARD_BYMARKER", (double)(m_altnav == 1));
      SETSURFBOOL("REWIND_FORWARD_SETLOOP", (double)(m_altnav == 2));
      m_nav_active=0;

      m_scrollx=0;
      m_scrolly=0;
      m_zoomx=0;
      m_zoomy=0;
      SETSURFBOOL("SCROLL_X-", false);
      SETSURFBOOL("SCROLL_X+", false);
      SETSURFBOOL("SCROLL_Y-", false);
      SETSURFBOOL("SCROLL_Y+", false);
      SETSURFBOOL("ZOOM_X-", false);
      SETSURFBOOL("ZOOM_X+", false);
      SETSURFBOOL("ZOOM_Y-", false);
      SETSURFBOOL("ZOOM_X+", false);

      int recmode=!!GetToggleCommandState(41186); // rec mode replace (tape)
      Extended(CSURF_EXT_SETRECMODE, &recmode, 0, 0);

      int click=!!GetToggleCommandState(40364); // metronome enable
      Extended(CSURF_EXT_SETMETRONOME, (void*)(INT_PTR)click, 0, 0);

      int rpt=GetSetRepeat(-1);  
      SetRepeatState(!!rpt);

      int ps=GetPlayState();
      SetPlayState(!!(ps&1), !!(ps&2), !!(ps&4));

      double pr=Master_GetPlayRate(0); 
      double bpm=Master_GetTempo();
      Extended(CSURF_EXT_SETBPMANDPLAYRATE, &bpm, &pr, 0);

      int autorecarm=GetToggleCommandState(40740); // auto-recarm all
      Extended(CSURF_EXT_SETAUTORECARM, (void*)(INT_PTR)(!!autorecarm), 0, 0);

      if (m_wantfx&32)
      {
        SETSURFSTR("FX_EQ_HIPASS_NAME", "HPF");
        SETSURFSTR("FX_EQ_LOSHELF_NAME", "Lo Shlf");
        SETSURFSTR("FX_EQ_BAND_NAME", "Band");
        SETSURFSTR("FX_EQ_NOTCH_NAME", "Notch");
        SETSURFSTR("FX_EQ_HISHELF_NAME", "Hi Shlf");
        SETSURFSTR("FX_EQ_LOPASS_NAME", "LPF");
      }

      SetActiveTrackChange();
    }

    int i;
    for (i=0; i <= m_trackbanksize; ++i)
    {
      int tidx = (!i ? 0 : m_curbankstart+i); // !i == master
      UpdateSurfaceTrack(tidx);
    }
  }

  // m_curtrack changed, not track bank
  void SetActiveTrackChange()
  {
    if ((!m_osc || !m_osc->m_send_enable) && !m_osc_local) return;

    int i;
    for (i=1; i <= m_trackbanksize; ++i)
    {
      int tidx=m_curbankstart+i;
      bool iscur = (tidx == m_curtrack);
      int wc[1] = { i }; 
      SETSURFBOOLWC("DEVICE_TRACK_SELECT", wc, 1, iscur);
    }
    
    UpdateSurfaceTrack(m_curtrack);
    SetActiveFXChange();
    SetActiveFXInstChange();
    SetActiveFXEQChange();
  }

  void SetActiveFXEQChange()
  {
    if ((!m_osc || !m_osc->m_send_enable) && !m_osc_local) return;

    int fxidx=-1;

    MediaTrack* tr=CSurf_TrackFromID(m_curtrack, !!(m_followflag&8));
    if (tr) fxidx=TrackFX_GetEQ(tr, false);

    char buf[512];
    buf[0]=0;
    bool en=false;
    bool open=false;
    if (tr && fxidx >= 0)
    {
      en=TrackFX_GetEnabled(tr, fxidx);
      open=TrackFX_GetOpen(tr, fxidx);
      TrackFX_GetPreset(tr, fxidx, buf, sizeof(buf));
    }
    SETSURFSTR("FX_EQ_PRESET", buf);
    SETSURFBOOL("FX_EQ_BYPASS", en);
    SETSURFBOOL("FX_EQ_OPEN_UI", open);

    static const char* bandstr[6] = { "_HIPASS", "_LOSHELF", "_BAND", "_NOTCH", "_HISHELF", "_LOPASS" };
    static const char* parmtypestr[3] = { "_FREQ", "_GAIN", "_Q" };
    static double defval[3] = { 0.0, 0.5, 0.5 };
    static const char* defstr[3] = { "0Hz", "0dB", "1.0" };

    int i, j, k;
    for (i=0; i < 6; ++i)
    {
      bool isband = (i == 2);

      for (j=0; j < 3; ++j)
      {
        char pattern[512];
        strcpy(pattern, "FX_EQ");
        strcat(pattern, bandstr[i]);
        strcat(pattern, parmtypestr[j]);
        for (k=0; k < (isband ? 8 : 1); ++k)
        {
          int wc[1] = { k };
          SetSurfaceVal(pattern, (isband ? wc : 0), (isband ? 1 : 0), 0, 0, &defval[j], defstr[j]);
        }
      }

      for (k=0; k < (isband ? 8 : 1); ++k)
      {
        bool banden=false;
        if (tr && fxidx >= 0)
        {
          bool band_en=TrackFX_GetEQBandEnabled(tr, fxidx, i, k);
          
          char pattern[512];
          strcpy(pattern, "FX_EQ");
          strcat(pattern, bandstr[i]);
          strcat(pattern, "_BYPASS");
          
          int wc[1] = { k };
          if (isband) 
          {
            SETSURFBOOLWC(pattern, wc, 1, band_en);
          }
          else 
          {
            SETSURFBOOL(pattern, band_en);
          }
        }
      }
    }

    if (tr && fxidx >= 0)
    {
      int numparms=TrackFX_GetNumParams(tr, fxidx);           
      int i;
      for (i=0; i < numparms; ++i)
      {
        double val=TrackFX_GetParamNormalized(tr, fxidx, i);
        int f=(fxidx<<16)|i;
        Extended(CSURF_EXT_SETFXPARAM, tr, &f, &val);
      }
    }
  }

  void SetActiveFXChange()
  {
    if ((!m_osc || !m_osc->m_send_enable) && !m_osc_local) return;

    int numfx=0;
    int numparms=0;
    MediaTrack* tr=CSurf_TrackFromID(m_curtrack, !!(m_followflag&8));
    if (tr) 
    {
      numfx=TrackFX_GetCount(tr);
      numparms=TrackFX_GetNumParams(tr, m_curfx);
    }
    if (m_curfx >= numfx) m_curfx=0;
    if (m_curfxparmbankstart >= numparms) m_curfxparmbankstart=0;

    int i;
    for (i=0; i < m_fxbanksize; ++i)
    {
      bool iscur = (numfx && i == m_curfx);
      int wc[1] = { i+1 }; 
      SETSURFBOOLWC("DEVICE_FX_SELECT", wc, 1, iscur);
    }

    char buf[512];
    char buf2[512];
    char ibuf[128];

    bool en=false;
    bool open=false;
    buf[0]=buf2[0]=ibuf[0]=0;
    if (tr && numfx)
    {
      en=TrackFX_GetEnabled(tr, m_curfx);
      open=TrackFX_GetOpen(tr, m_curfx);
      GetFXName(tr, m_curfx, buf, sizeof(buf));
      sprintf(ibuf, "%d", m_curfx+1);
      TrackFX_GetPreset(tr, m_curfx, buf2, sizeof(buf2));
    }

    SETSURFSTR("FX_NAME", buf);
    SETSURFSTR("FX_NUMBER", ibuf); 
    SETSURFSTR("FX_PRESET", buf2);
    SETSURFBOOL("FX_BYPASS", en);
    SETSURFBOOL("FX_OPEN_UI", open);

    sprintf(buf, "%d", m_curfxparmbankstart/m_fxparmbanksize+1);
    SETSURFSTR("DEVICE_FX_PARAM_BANK_SELECT", buf);

    for (i=0; i < m_fxparmbanksize; ++i)
    {
      int parmidx=m_curfxparmbankstart+i;
      buf[0]=buf2[0]=0;
      double val=0.0;

      if (tr && m_curfx < numfx && parmidx < numparms)
      {
        TrackFX_GetParamName(tr, m_curfx, parmidx, buf, sizeof(buf));
        val=TrackFX_GetParamNormalized(tr, m_curfx, parmidx);
        TrackFX_GetFormattedParamValue(tr, m_curfx, parmidx, buf2, sizeof(buf2));
      }

      int wc[1] = { i+1 };
      SETSURFSTRWC("FX_PARAM_NAME", wc, 1, buf);
      SetSurfaceVal("FX_PARAM_VALUE", wc, 1, 0, 0, &val, buf2);
      if (parmidx == numparms-1)
      {      
        SetSurfaceVal("FX_WETDRY", 0, 0, 0, 0, &val, buf2);
      }
    }
  }

  void SetActiveFXInstChange()
  {
    if ((!m_osc || !m_osc->m_send_enable) && !m_osc_local) return;

    int fxidx=-1;
    MediaTrack* tr=CSurf_TrackFromID(m_curtrack, !!(m_followflag&8));
    if (tr) fxidx=TrackFX_GetInstrument(tr);    

    int numparms=0;
    if (fxidx >= 0) numparms=TrackFX_GetNumParams(tr, fxidx);
    if (m_curfxinstparmbankstart >= numparms) m_curfxinstparmbankstart=0;

    char buf[512];
    char buf2[512];

    buf[0]=buf2[0]=0;
    if (fxidx >= 0)
    {
      GetFXName(tr, fxidx, buf, sizeof(buf));
      TrackFX_GetPreset(tr, fxidx, buf2, sizeof(buf2));
    }
    SETSURFSTR("FX_INST_NAME", buf);
    SETSURFSTR("FX_INST_PRESET", buf2);

    sprintf(buf, "%d", m_curfxinstparmbankstart/m_fxinstparmbanksize+1);
    SETSURFSTR("DEVICE_FX_INST_PARAM_BANK_SELECT", buf);

    int i;
    for (i=0; i < m_fxinstparmbanksize; ++i)
    {
      int parmidx=m_curfxinstparmbankstart+i;
      buf[0]=buf2[0]=0;         
      double val=0.0;

      if (tr && fxidx >= 0 && parmidx < numparms)
      {
        TrackFX_GetParamName(tr, fxidx, parmidx, buf, sizeof(buf));
        val=TrackFX_GetParamNormalized(tr, fxidx, parmidx);
        TrackFX_GetFormattedParamValue(tr, fxidx, parmidx, buf2, sizeof(buf2));   
      }

      int wc[1] = { i+1 };
      SETSURFSTRWC("FX_INST_PARAM_NAME", wc, 1, buf);
      SetSurfaceVal("FX_INST_PARAM_VALUE", wc, 1, 0, 0, &val, buf2);
    }
  }

  int GetSurfaceTrackIdx(MediaTrack* tr) // returns -1 if the track is not visible in the surface
  {
    int tidx=CSurf_TrackToID(tr, !!(m_followflag&8));
    if (tidx == m_curtrack) return tidx;
    if (tidx > m_curbankstart && tidx <= m_curbankstart+m_trackbanksize) return tidx;
    return -1;
  }

  void SetTrackTitle(MediaTrack* tr, const char* title)
  {   
    char buf[128];
    int tidx=GetSurfaceTrackIdx(tr);
    if (tidx >= 0) 
    {
      char ibuf[128];   
      sprintf(ibuf, "%d", tidx);
      if (!title[0] || !strcmp(ibuf, title))
      {
        sprintf(buf, "Track %d", tidx);
        title=buf;
      }
    }
    SETSURFTRACKSTR("TRACK_NAME", tidx, title);
  }

  void SetSurfaceSelectedImpl(int tidx, bool selected)
  {
    SETSURFTRACKBOOL("TRACK_SELECT", tidx, selected);
  }

  void SetSurfaceSelected(MediaTrack *tr, bool selected)
  {
    int tidx=GetSurfaceTrackIdx(tr);
    if (tidx >= 0) SetSurfaceSelectedImpl(tidx, selected);
  }

  void SetSurfaceVolumeImpl(int tidx, double volume)
  {
    double db=VAL2DB(volume);
    double v=DB2SLIDER(db)/1000.0;
    char buf[128];
    mkvolstr(buf, volume);
    
    if (!tidx) SetSurfaceVal("MASTER_VOLUME", 0, 0, 0, &db, &v, buf);
    SetSurfaceTrackVal("TRACK_VOLUME", tidx, 0, &db, &v, buf); 
  }

  void SetSurfaceVolume(MediaTrack *tr, double volume)
  {
    int tidx=GetSurfaceTrackIdx(tr);
    if (tidx >= 0) SetSurfaceVolumeImpl(tidx, volume);
  }

  void SetSurfacePan(MediaTrack* tr, double pan)
  {
    // ignore because we handle CSURF_EXT_SETPAN_EX
  }

  void SetSurfaceMuteImpl(int tidx, bool mute)
  {
    SETSURFTRACKBOOL("TRACK_MUTE", tidx, mute);
  }

  void SetSurfaceMute(MediaTrack* tr, bool mute)
  {
    int tidx=GetSurfaceTrackIdx(tr);
    if (tidx >= 0) SetSurfaceMuteImpl(tidx, mute);
  }

  void SetSurfaceSoloImpl(int tidx, bool solo)
  {
    SETSURFTRACKBOOL("TRACK_SOLO", tidx, solo);
    if (m_anysolo != AnyTrackSolo(0))
    {
      m_anysolo=!m_anysolo;
      SETSURFBOOL("ANY_SOLO", m_anysolo);
    }
  }

  void SetSurfaceSolo(MediaTrack *tr, bool solo)
  {
    int tidx=GetSurfaceTrackIdx(tr);
    if (tidx >= 0) SetSurfaceSoloImpl(tidx, solo);
  }

  void SetSurfaceRecArmImpl(int tidx, bool recarm)
  {
    SETSURFTRACKBOOL("TRACK_REC_ARM", tidx, recarm);
  }

  void SetSurfaceRecArm(MediaTrack *tr, bool recarm)
  {
    int tidx=GetSurfaceTrackIdx(tr);
    if (tidx >= 0) SetSurfaceRecArmImpl(tidx, recarm);
  }

  bool GetTouchState(MediaTrack* tr, int ispan)
  {
    int tidx=GetSurfaceTrackIdx(tr);
    if (tidx < 0 || tidx >= MAX_LASTTOUCHED_TRACK) return false;

    if ((!ispan && (m_supports_touch&1)) || (ispan && (m_supports_touch&2)))
    {
      return (!ispan ? !!(m_hastouch[tidx]&1) : !!(m_hastouch[tidx]&2));
    }

    DWORD lastt = (!ispan ? m_vol_lasttouch[tidx] : m_pan_lasttouch[tidx]);
    if (!lastt) return false;
    DWORD curt=GetTickCount();
    return (curt < lastt+3000 && (lastt < 1000 || curt >= lastt-1000));
  }

  void SetAutoMode(int mode) // the passed-in mode is ignored
  {
    int i;
    for (i=0; i <= m_trackbanksize; ++i)
    {
      int tidx;
      if (!i) tidx=m_curtrack;
      else tidx=m_curbankstart+i;
      MediaTrack* tr=CSurf_TrackFromID(tidx, !!(m_followflag&8));
      int mode=0;
      if (tr) mode=GetTrackAutomationMode(tr);
      SetSurfaceAutoModeImpl(tidx, mode);
    }
  }

  void SetSurfaceAutoModeImpl(int tidx, int mode)
  {
    SETSURFTRACKBOOL("TRACK_AUTO_TRIM", tidx, (mode == AUTO_MODE_TRIM));
    SETSURFTRACKBOOL("TRACK_AUTO_READ", tidx, (mode == AUTO_MODE_READ));
    SETSURFTRACKBOOL("TRACK_AUTO_LATCH", tidx, (mode == AUTO_MODE_LATCH));
    SETSURFTRACKBOOL("TRACK_AUTO_TOUCH", tidx, (mode == AUTO_MODE_TOUCH));
    SETSURFTRACKBOOL("TRACK_AUTO_WRITE", tidx, (mode == AUTO_MODE_WRITE));
  }

  void SetSurfaceAutoMode(MediaTrack* tr, int mode)
  {
    int tidx=GetSurfaceTrackIdx(tr);
    if (tidx >= 0) SetSurfaceAutoModeImpl(tidx, mode);
  }

  void SetRepeatState(bool rpt)
  {
    SETSURFBOOL("REPEAT", (double)rpt);
  }

  void SetPlayState(bool play, bool pause, bool rec)
  {
    SETSURFBOOL("RECORD", rec);
    SETSURFBOOL("STOP", pause);
    SETSURFBOOL("PAUSE",  pause);
    SETSURFBOOL("PLAY", (play || pause));
  }

  int Extended(int call, void* parm1, void* parm2, void* parm3)
  {
    int tidx=-1;

    if (call == CSURF_EXT_SETPAN_EX ||
        call == CSURF_EXT_SETINPUTMONITOR ||
        call == CSURF_EXT_SETSENDVOLUME ||
        call == CSURF_EXT_SETSENDPAN ||
        call == CSURF_EXT_SETRECVVOLUME ||
        call == CSURF_EXT_SETRECVPAN ||
        call == CSURF_EXT_SETFXENABLED ||
        call == CSURF_EXT_SETFXOPEN)
    {
      if (parm1) tidx=CSurf_TrackToID((MediaTrack*)parm1, !!(m_followflag&8)); 
      if (tidx < 0) return 1;
      parm1=&tidx;
      call += CSURF_EXT_IMPL_ADD;
    }

    if (call == CSURF_EXT_SETPAN_EX_IMPL)
    {
      if (parm1 && parm2 && parm3)
      {
        int tidx=*(int*)parm1;
        double* pan=(double*)parm2;
        int mode=*(int*)parm3;

        char panstr[256];
        mkpanstr(panstr, *pan);
        if (!strcmp(panstr, "center")) strcpy(panstr, "C");
        double tpan=0.5*(*pan+1.0);

        if (!tidx) 
        {
          SetSurfaceVal("MASTER_PAN", 0,0, 0, 0, &tpan, panstr);
        }
        else
        {     
          const char* panmode="";    
          char panstr2[256];
          panstr2[0]=0;
          double tpan2=0.5;
          if (mode == PAN_MODE_CLASSIC)
          {
            panmode="Balance (classic)";
          }
          else if (mode == PAN_MODE_NEW_BALANCE)
          {
            panmode="Balance";
          }
          else if (mode == PAN_MODE_STEREO_PAN)
          {
            panmode="Stereo pan";
            sprintf(panstr2, "%.0fW", 100.0*pan[1]);
            tpan2=0.5*(pan[1]+1.0);
          }
          else if (mode == PAN_MODE_DUAL_PAN)
          {
            panmode="Dual pan";    
            mkpanstr(panstr2, pan[1]);
            if (!strcmp(panstr2, "center")) strcpy(panstr2, "C");
            tpan2=0.5*(pan[1]+1.0);
          }
        
          SETSURFTRACKSTR("TRACK_PAN_MODE", tidx, panmode);
          SetSurfaceTrackVal("TRACK_PAN2", tidx, 0, 0, &tpan2, panstr2);
        }
          
        SetSurfaceTrackVal("TRACK_PAN", tidx, 0, 0, &tpan, panstr);
      }
    }

    if (call == CSURF_EXT_SETSENDVOLUME_IMPL)
    {
      if (parm1 && parm2 && parm3)
      {
        int tidx=*(int*)parm1;
        int sidx=*(int*)parm2;
        double volume=*(double*)parm3;

        double v=DB2SLIDER(VAL2DB(volume))/1000.0;
        char buf[128];
        mkvolstr(buf, volume);

        int wc[2] = { sidx+1, tidx-m_curbankstart };
        if (tidx == m_curtrack)
        {         
          SetSurfaceVal("TRACK_SEND_VOLUME", wc, 1, 0, 0, &v, buf);
        }
        if (wc[1] > 0 && wc[1] <= m_trackbanksize)
        {
          SetSurfaceVal("TRACK_SEND_VOLUME", wc, 2, 0, 0, &v, buf);
        }
        if (!tidx)
        {
          SetSurfaceVal("MASTER_SEND_VOLUME", wc, 1, 0, 0, &v, buf);
        }
      }
      return 1;
    }

    if (call == CSURF_EXT_SETRECVVOLUME_IMPL)
    {
      if (parm1 && parm2 && parm3)
      {
        int tidx=*(int*)parm1;
        int sidx=*(int*)parm2;
        double volume=*(double*)parm3;

        double v=DB2SLIDER(VAL2DB(volume))/1000.0;
        char buf[128];
        mkvolstr(buf, volume);

        int wc[2] = { sidx+1, tidx-m_curbankstart };
        if (tidx == m_curtrack)
        {         
          SetSurfaceVal("TRACK_RECV_VOLUME", wc, 1, 0, 0, &v, buf);
        }
        if (wc[1] > 0 && wc[1] <= m_trackbanksize)
        {
          SetSurfaceVal("TRACK_RECV_VOLUME", wc, 2, 0, 0, &v, buf);
        }
      }
      return 1;
    }

    if (call == CSURF_EXT_SETSENDPAN_IMPL)
    {
      if (parm1 && parm2 && parm3) // parm1 can be index 0
      {
        int tidx=*(int*)parm1;
        int sidx=*(int*)parm2;
        double pan=*(double*)parm3;
        double p=0.5*(pan+1.0);
        char buf[128];
        mkpanstr(buf, pan);

        int wc[2] = { sidx+1, tidx-m_curbankstart };
        if (tidx == m_curtrack)
        {
          SetSurfaceVal("TRACK_SEND_PAN", wc, 1, 0, 0, &p, buf);
        }
        if (tidx >= 0 && wc[1] > 0 && wc[1] <= m_trackbanksize)
        {
          SetSurfaceVal("TRACK_SEND_PAN", wc, 2, 0, 0, &p, buf);
        }
        if (!tidx) 
        {      
          SetSurfaceVal("MASTER_SEND_PAN", wc, 1, 0, 0, &p, buf);
        }
      }
      return 1;
    }

    if (call == CSURF_EXT_SETRECVPAN_IMPL)
    {
      if (parm1 && parm2 && parm3) // parm1 can be index 0
      {
        int tidx=*(int*)parm1;
        int sidx=*(int*)parm2;
        double pan=*(double*)parm3;
        double p=0.5*(pan+1.0);
        char buf[128];
        mkpanstr(buf, pan);

        int wc[2] = { sidx+1, tidx-m_curbankstart };
        if (tidx == m_curtrack)
        {
          SetSurfaceVal("TRACK_RECV_PAN", wc, 1, 0, 0, &p, buf);
        }
        if (tidx >= 0 && wc[1] > 0 && wc[1] <= m_trackbanksize)
        {
          SetSurfaceVal("TRACK_RECV_PAN", wc, 2, 0, 0, &p, buf);
        }
      }
      return 1;
    }

    if (call == CSURF_EXT_SETINPUTMONITOR_IMPL)
    {
      if (parm1 && parm2)
      {
        int tidx=*(int*)parm1;
        bool mon=!!(*(int*)parm2);
        SETSURFTRACKBOOL("TRACK_MONITOR", tidx, mon);
      }
      return 1;
    }

    if (call == CSURF_EXT_SETMETRONOME)
    {       
      SETSURFBOOL("METRONOME", !!parm1);
      return 1;
    }

    if (call == CSURF_EXT_SETAUTORECARM)
    {
      SETSURFBOOL("AUTO_REC_ARM", !!parm1);
      return 1;
    }

    if (call == CSURF_EXT_SETRECMODE)
    {
      if (parm1)
      {
        SETSURFBOOL("REPLACE", (*(int*)parm1 == 1));
      }
      return 1;
    }

    if (call == CSURF_EXT_SETLASTTOUCHEDTRACK)
    {
      if (parm1)
      {
        if (m_followflag&1)
        {
          MediaTrack* tr=(MediaTrack*)parm1;
          int tidx=CSurf_TrackToID(tr, !!(m_followflag&8));
          if (tidx >= 0 && tidx != m_curtrack)
          {
            m_curtrack=tidx;
            SetActiveTrackChange();         
          }
        }
      }
      return 1;
    }

    if (call == CSURF_EXT_SETFXENABLED_IMPL ||
        call == CSURF_EXT_SETFXOPEN_IMPL)
    {
      if (parm1 && parm2) // parm3 can be index 0
      {
        int tidx=*(int*)parm1;
        int fxidx=*(int*)parm2;
        bool en=!!parm3;

        bool isinst=false;
        bool iseq=false;
        if (m_wantfx&(4|32))
        {
          MediaTrack* tr=CSurf_TrackFromID(tidx, !!(m_followflag&8));
          isinst = ((m_wantfx&4) && fxidx == TrackFX_GetInstrument(tr));
          iseq = ((m_wantfx&32) && fxidx == TrackFX_GetEQ(tr, false));
        }

        int wc[2] = { fxidx+1, tidx };
        if (tidx == m_curtrack)
        {
          if (call == CSURF_EXT_SETFXENABLED_IMPL) 
          {
            SETSURFBOOLWC("FX_BYPASS", wc, 1, en);
            if (isinst) SETSURFBOOL("FX_INST_BYPASS", en);
            if (iseq) SETSURFBOOL("FX_EQ_BYPASS", en);
          }
          else if (call == CSURF_EXT_SETFXOPEN_IMPL)
          {
            SETSURFBOOLWC("FX_OPEN_UI", wc, 1, en);
            if (isinst) SETSURFBOOL("FX_INST_OPEN_UI", en);
            if (iseq) SETSURFBOOL("FX_EQ_OPEN_UI", en);
          }
        }
        if (wc[1] > 0 && wc[1] <= m_trackbanksize) 
        {
          if (call == CSURF_EXT_SETFXENABLED_IMPL)
          {
            SETSURFBOOLWC("FX_BYPASS", wc, 2, en);
            if (isinst) SETSURFBOOLWC("FX_INST_BYPASS", wc+1, 1, en);          
            if (iseq) SETSURFBOOLWC("FX_EQ_BYPASS", wc+1, 1, en);
          }
          else if (call == CSURF_EXT_SETFXOPEN_IMPL)
          {
            SETSURFBOOLWC("FX_OPEN_UI", wc, 2, en);
            if (isinst) SETSURFBOOLWC("FX_INST_OPEN_UI", wc+1, 1, en);
            if (iseq) SETSURFBOOLWC("FX_EQ_OPEN_UI", wc+1, 1, en);
          }
        }        
      }
      return 1;
    }

    if (call == CSURF_EXT_SETFXPARAM)
    {
      if (parm1 && parm2 && parm3) 
      {
        MediaTrack* tr=(MediaTrack*)parm1;
        int f=*(int*)parm2;
        int fxidx=(f>>16)&0xFFFF;
        int parmidx=f&0xFFFF;
        double val=*(double*)parm3;

        int tidx=CSurf_TrackToID(tr, !!(m_followflag&8));

        bool isinst=false;
        bool iseq=false;

        bool iscurfx=false;
        bool iscurinst=false;
        bool iscureq=false;
        bool islasttouched=false;
        bool iscurtrackfx=false;
        bool isbanktrackfx=false;
        bool isbanktrackfxinst=false;
        bool isbanktrackeq=false;
        
        if ((m_wantfx&4) && fxidx == TrackFX_GetInstrument(tr)) isinst=true;
        else if ((m_wantfx&32) && fxidx == TrackFX_GetEQ(tr, false)) iseq=true;

        if (tidx == m_curtrack)
        {
          if ((m_wantfx&1) && fxidx == m_curfx && 
            parmidx >= m_curfxparmbankstart &&
            parmidx < m_curfxparmbankstart+m_fxparmbanksize)
          {
            iscurfx=true;
          }
  
          if (isinst &&           
              parmidx >= m_curfxinstparmbankstart &&
              parmidx < m_curfxinstparmbankstart+m_fxinstparmbanksize)
          {
            iscurinst=true;
          }

          if (iseq)
          {
            iscureq=true;
          }
        }

        if (tidx > m_curbankstart && tidx < m_curbankstart+m_trackbanksize)
        {
          if ((m_wantfx&8) &&
              parmidx >= m_curfxparmbankstart &&
              parmidx < m_curfxparmbankstart+m_fxparmbanksize)
          {
            isbanktrackfx=true;
          }

          if (isinst && (m_wantfx&16) && 
              parmidx >= m_curfxinstparmbankstart &&
              parmidx < m_curfxinstparmbankstart+m_fxinstparmbanksize)
          {
            isbanktrackfxinst=true;
          }

          if (iseq && (m_wantfx&64))
          {
            isbanktrackeq=true;
          }
        }

        if (m_wantfx&2)
        {
          int ltidx=-1;
          int lfxidx=-1;
          int lparmidx=-1;
          if (GetLastTouchedFX(&ltidx, &lfxidx, &lparmidx) &&
              tidx == ltidx && fxidx == lfxidx && parmidx == lparmidx)
          {
            islasttouched=true;
          }
        }

        if (iscurfx || iscurinst || iscureq || islasttouched || 
            iscurtrackfx || isbanktrackfx || isbanktrackfxinst || isbanktrackeq)
        {
          char buf[512];
          buf[0]=0;
          TrackFX_GetFormattedParamValue(tr, fxidx, parmidx, buf, sizeof(buf));
          if (!buf[0]) sprintf(buf, "%.3f", val);

          if (iscureq || isbanktrackeq)
          {
            int bandtype=-1;
            int bandidx=0;
            int parmtype=-1;
            if (TrackFX_GetEQParam(tr, fxidx, parmidx, &bandtype, &bandidx, &parmtype, 0))
            {
              char pattern[512];
              strcpy(pattern, "FX_EQ");

              if (bandtype < 0 || bandtype >= 6) bandtype=2;
              static const char* bandstr[6] = { "_HIPASS", "_LOSHELF", "_BAND", "_NOTCH", "_HISHELF", "_LOPASS" };
              strcat(pattern, bandstr[bandtype]);
              bool isband = (bandtype == 2);

              if (parmtype < 0 || parmtype >= 3) parmtype=0;
              static const char* parmtypestr[3] = { "_FREQ", "_GAIN", "_Q" };
              strcat(pattern, parmtypestr[parmtype]);

              if (parmtype == 0) strcat(buf, "Hz");
              else if (parmtype == 1) strcat(buf, "dB");

              int wc[2] = { bandidx, tidx-m_curbankstart };              
              if (iscureq) SetSurfaceVal(pattern, (isband ? wc : 0), (isband ? 1 : 0), 0, 0, &val, buf);
              if (isbanktrackeq) SetSurfaceVal(pattern, (isband ? wc : wc+1), (isband ? 2 : 1), 0, 0, &val, buf);
            }
          }

          if (iscurfx || isbanktrackfx)
          {
            int numparms=TrackFX_GetNumParams(tr, fxidx);
            bool iswet = (parmidx == numparms-1);

            int wc[3] = { parmidx-m_curfxparmbankstart+1, fxidx+1, tidx-m_curbankstart };
            int k;
            for (k=1; k <= 3; ++k)
            {
              if ((iscurfx && k == 1) || (iscurtrackfx && k == 2) || (isbanktrackfx && k == 3))
              {
                SetSurfaceVal("FX_PARAM_VALUE", wc, k, 0, 0, &val, buf);
                if (iswet)
                {
                  SetSurfaceVal("FX_WETDRY", wc+1, k-1, 0, 0, &val, buf);
                }
              }
            }
          } 
    
          if (iscurinst || isbanktrackfxinst)
          {
            int wc[2] = { parmidx-m_curfxinstparmbankstart+1, tidx-m_curbankstart };
            if (iscurinst) SetSurfaceVal("FX_INST_PARAM_VALUE", wc, 1, 0, 0, &val, buf);
            if (isbanktrackfxinst) SetSurfaceVal("FX_INST_PARAM_VALUE", wc, 2, 0, 0, &val, buf);
          }

          if (islasttouched)
          {
            SetSurfaceVal("LAST_TOUCHED_FX_PARAM_VALUE", 0, 0, 0, 0, &val, buf);

            buf[0]=0;
            char ibuf[128];    
            sprintf(ibuf, "%d", tidx);
            const char* p=GetTrackInfo((INT_PTR)tr, 0);
            if (p) lstrcpyn(buf, p, sizeof(buf));              
            if (!buf[0] || !strcmp(buf, ibuf))
            {
              if (!tidx) strcpy(buf, "MASTER");
              else sprintf(buf, "Track %d", tidx);
            }         
            SetSurfaceVal("LAST_TOUCHED_FX_TRACK_NAME", 0, 0, 0, 0, 0, buf);
            SetSurfaceVal("LAST_TOUCHED_FX_TRACK_NUMBER", 0, 0, 0, 0, 0, ibuf);

            buf[0]=0;
            if (tr) GetFXName(tr, fxidx, buf, sizeof(buf));
            SetSurfaceVal("LAST_TOUCHED_FX_NAME", 0, 0, 0, 0, 0, buf);
            buf[0]=0;
            if (tr) sprintf(buf, "%d", fxidx+1);
            SetSurfaceVal("LAST_TOUCHED_FX_NUMBER", 0, 0, 0, 0, 0, buf);         
            buf[0]=0;
            if (tr) TrackFX_GetParamName(tr, fxidx, parmidx, buf, sizeof(buf));
            SetSurfaceVal("LAST_TOUCHED_FX_PARAM_NAME", 0, 0, 0, 0, 0, buf);
          }
        }
      }
      return 1;
    }

    if (call == CSURF_EXT_SETFOCUSEDFX ||
        call == CSURF_EXT_SETLASTTOUCHEDFX)
    {
      if (parm1 && !parm2 && parm3)
      {
        if (((m_followflag&2) && call == CSURF_EXT_SETLASTTOUCHEDFX) ||
            ((m_followflag&4) && call == CSURF_EXT_SETFOCUSEDFX))
        {
          MediaTrack* tr=(MediaTrack*)parm1;
          int tidx=CSurf_TrackToID(tr, !!(m_followflag&8));
          int fxidx=*(int*)parm3;
          if (tidx >= 0 && fxidx >= 0 && fxidx < TrackFX_GetCount(tr))
          {
            if (tidx != m_curtrack) 
            {
              m_curtrack=tidx;
              m_curfx=fxidx;
              SetActiveTrackChange();
            }
            else if (fxidx != m_curfx)
            {
              m_curfx=fxidx;
              SetActiveFXChange();
            }
          }
        }
      }
      return 1;
    }

    if (call == CSURF_EXT_SETBPMANDPLAYRATE)
    {
      char buf[512];
      buf[0]=0;
      if (parm1)
      {
        double bpm=*(double*)parm1;
        double nbpm=Master_NormalizeTempo(bpm, false);
        sprintf(buf, "%g", bpm);
        SetSurfaceVal("TEMPO", 0, 0, 0, &bpm, &nbpm, buf);
      }
      if (parm2)
      {
        double pr=*(double*)parm2;
        double npr=Master_NormalizePlayRate(pr, false);
        sprintf(buf, "%g", pr);
        SetSurfaceVal("PLAY_RATE", 0, 0, 0, &pr, &npr, buf);
      }
      return 1;
    }

    if (call == CSURF_EXT_SETMIXERSCROLL)
    {
      if (parm1 && (m_followflag&8))
      {
        MediaTrack* tr=(MediaTrack*)parm1;
        int tidx=CSurf_TrackToID(tr, true);
        tidx=max(0, tidx-1);             
        if (m_curbankstart != tidx || parm3)
        {
          m_curbankstart=tidx;
          SetTrackListChange();
        }
      }
      return 1;
    }

    if (call == CSURF_EXT_SETFXCHANGE)
    {
      SetTrackListChange();
      return 1;
    }

    if (call == CSURF_EXT_RESET)
    {
      m_lastvals.DeleteAll();
      m_surfinit=false;
      m_lastupd=0;
      m_lastpos=0.0;
      m_anysolo=false;
      SetTrackListChange();  
      SetActiveTrackChange();
      return 1;
    }

    return 0;
  }
};


// simple 1-way stateless message sending (like from reascript)
void OscLocalMessageToHost(const char* msg, double* value)
{
  OscMessageWrite wmsg;
  wmsg.PushWord(msg);
  if (value) wmsg.PushFloatArg((float)*value);

  int len=0;
  char* p=(char*)wmsg.GetBuffer(&len); // OK to write on this
  OscMessageRead rmsg(p, len);

  CSurf_Osc tmposc(0, 0, 0, 0, 0, 0, 0, 0, 0);
  tmposc.ProcessMessage(&rmsg);
}


static IReaperControlSurface *createFunc(const char* typestr, const char* cfgstr, int* err)
{
  char name[512];
  int flags=0;
  int recvport=0; 
  char sendip[128];
  int sendport=0;
  int maxpacketsz=DEF_MAXPACKETSZ;
  int sendsleep=DEF_SENDSLEEP;
  char cfgfn[2048];
  ParseCfg(cfgstr, name, sizeof(name), &flags, &recvport, sendip, &sendport, &maxpacketsz, &sendsleep, cfgfn, sizeof(cfgfn));
  return new CSurf_Osc(name, flags, recvport, sendip, sendport, maxpacketsz, sendsleep, 0, cfgfn);
}


void* CreateLocalOscHandler(void* obj, OscLocalCallbackFunc callback)
{
  OscLocalHandler* osc_local=new OscLocalHandler;
  osc_local->m_obj=obj;
  osc_local->m_callback=callback;
  return new CSurf_Osc(0, 0, 0, 0, 0, 0, 0, osc_local, 0);
}

void SendLocalOscMessage(void* csurf_osc, const char* msg, int msglen)
{
  if (!csurf_osc || !msg || !msg[0] || msglen > MAX_OSC_MSG_LEN) return;

  char buf[MAX_OSC_MSG_LEN];
  memcpy(buf, msg, msglen);
  OscMessageRead rmsg(buf, msglen);
  CSurf_Osc::ProcessOscMessage(csurf_osc, &rmsg);
}

void DestroyLocalOscHandler(void* csurf_osc)
{
  delete (CSurf_Osc*)csurf_osc;
}


bool OscListener(void* obj, OscMessageRead* rmsg)
{
  char dump[MAX_OSC_MSG_LEN*2];
  dump[0]=0;
  rmsg->DebugDump(0, dump, sizeof(dump));
  if (dump[0])
  {
    strcat(dump, "\r\n");    
    SendMessage((HWND)obj, WM_USER+100, 0, (LPARAM)dump);
  }
  return true;
}

static WDL_DLGRET OscListenProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static OscHandler* s_osc;

  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      s_osc=0;
      if (lParam)
      {
        char buf[512];
        sprintf(buf, "OSC: listening to port %d", (int)lParam);
        SetWindowText(hwndDlg, buf);
        
        s_osc=OscAddLocalListener(OscListener, hwndDlg, (int)lParam);
        if (s_osc) SetTimer(hwndDlg, 1, 50, 0);
        else SetDlgItemText(hwndDlg, IDC_EDIT1, "Error: can't create OSC listener");
      }
    }
    return 0;

    case WM_TIMER:
      if (wParam == 1)
      {
        OscGetInput(s_osc); // will call back to OscListener
      }
    return 0;

    case WM_USER+100:
    {
      if (lParam)
      {
        const char* msg=(const char*)lParam;
        if (msg[0])
        {                 
          char buf[8192];
          int nlen=strlen(msg);
          if (nlen < sizeof(buf)-1)
          {
            GetDlgItemText(hwndDlg, IDC_EDIT1, buf, sizeof(buf));
            int olen=strlen(buf);
            if (olen+nlen > sizeof(buf)-1) buf[0]=0;
            strcat(buf, msg);
            SetDlgItemText(hwndDlg, IDC_EDIT1, buf);
            SendDlgItemMessage(hwndDlg, IDC_EDIT1, EM_SCROLL, SB_BOTTOM, 0);
          }
        }
      }
    }
    return 0;

    case WM_DESTROY:
      OscRemoveLocalListener(s_osc);
      delete s_osc;
      s_osc=0;
    return 0;

    case WM_COMMAND:
      if (LOWORD(wParam) == IDCANCEL)
      {
        EndDialog(hwndDlg, 0);
      }
    return 0;
  }

  return 0;
}


static WDL_DLGRET dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static char s_cfgfn[2048];
  static WDL_PtrList<char> s_patterncfgs;

  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      char name[512];
      name[0]=0;
      int flags=0;
      int recvport=0;    
      char sendip[128];
      sendip[0]=0;
      int sendport=0;
      int maxpacketsz=DEF_MAXPACKETSZ;
      int sendsleep=DEF_SENDSLEEP;
      s_cfgfn[0]=0;

      if (lParam) 
      {
        ParseCfg((const char*)lParam, name, sizeof(name), &flags, &recvport, sendip, &sendport, &maxpacketsz, &sendsleep, s_cfgfn, sizeof(s_cfgfn));
      }
      if (!recvport) recvport=DEF_RECVPORT;
      if (!sendip[0]) strcpy(sendip, "0.0.0.0");
      if (!sendport) sendport=DEF_SENDPORT;

      SetDlgItemText(hwndDlg, IDC_EDIT4, name);

      if (flags&1) CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
      char buf[256];
      sprintf(buf, "%d", recvport);
      SetDlgItemText(hwndDlg, IDC_EDIT1, buf);

      if (flags&2) CheckDlgButton(hwndDlg, IDC_CHECK2, BST_CHECKED);
      SetDlgItemText(hwndDlg, IDC_EDIT2, sendip);
      sprintf(buf, "%d", sendport);
      SetDlgItemText(hwndDlg, IDC_EDIT3, buf);
 
      GetLocalIP(buf, sizeof(buf));
      if (!buf[0]) strcpy(buf, "0.0.0.0");
      SetDlgItemText(hwndDlg, IDC_EDIT6, buf);

      if (flags&4) CheckDlgButton(hwndDlg, IDC_CHECK3, BST_CHECKED);

      sprintf(buf, "%d", maxpacketsz);
      SetDlgItemText(hwndDlg, IDC_EDIT7, buf);

      sprintf(buf, "%d", sendsleep);
      SetDlgItemText(hwndDlg, IDC_EDIT8, buf);

      SendMessage(hwndDlg, WM_USER+100, 0, 0);
    }
    // fall through

    case WM_COMMAND:
      if (uMsg == WM_INITDIALOG ||
          LOWORD(wParam) == IDC_CHECK1 || 
          LOWORD(wParam) == IDC_CHECK2)
      {
        bool en=!!IsDlgButtonChecked(hwndDlg, IDC_CHECK1);
        EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT1), en);
        EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON2), en);
        EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK3), en);

        en=!!IsDlgButtonChecked(hwndDlg, IDC_CHECK2);
        EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT2), en);
        EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT3), en);
        EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT7), en);
        EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT8), en);
      }  
      else if (LOWORD(wParam) == IDC_BUTTON2)
      {
        char buf[128];
        GetDlgItemText(hwndDlg, IDC_EDIT1, buf, sizeof(buf));
        int recvport=atoi(buf);
        if (recvport)
        {
          DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_OSC_LISTEN), hwndDlg, OscListenProc, (LPARAM)recvport);
        }
      }
      else if (LOWORD(wParam) == IDC_COMBO1 && HIWORD(wParam) == CBN_SELCHANGE)
      {
        int a=SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
        if (!a)
        {
          s_cfgfn[0]=0;
        }
        else if (a < s_patterncfgs.GetSize()+1) 
        {
          lstrcpyn(s_cfgfn, s_patterncfgs.Get(a-1), sizeof(s_cfgfn));
          
          // validate the cfg file
          char errbuf[512];
          if (s_cfgfn[0] && !LoadCfgFile(s_cfgfn, 0, errbuf))
          {
            if (!errbuf[0]) sprintf(errbuf, "Warning: possible error parsing config file \"%s\"", s_cfgfn);
            MessageBox(hwndDlg, errbuf, "OSC Config File Warning", MB_OK);
          }
        }
        else if (a == s_patterncfgs.GetSize()+1)
        {
          SendMessage(hwndDlg, WM_USER+100, 0, 0);
        }
        else if (a == s_patterncfgs.GetSize()+2)
        {
          const char* dir=GetOscCfgDir();
          ShellExecute(hwndDlg, "open", dir, "", dir, SW_SHOW);
          SendMessage(hwndDlg, WM_USER+100, 0, 0);
        }
      }
    return 0;

    case WM_DESTROY:
      s_patterncfgs.Empty(true, free);
    return 0;

    case WM_USER+100: // refresh pattern config file list
    {
      s_patterncfgs.Empty(true, free);
      
      const char* dir=GetOscCfgDir();
      WDL_DirScan ds;
      if (!ds.First(dir))
      {
        do
        {
          const char* fn=ds.GetCurrentFN();
          int len=strlen(fn);
          int extlen=strlen(OSC_EXT);
          if (len > extlen && !stricmp(fn+len-extlen, OSC_EXT) && strnicmp(fn, "default.", strlen("default.")))
          {
            char* p=s_patterncfgs.Add(strdup(fn));
            p += len-extlen;
            *p=0;
          }
        } while (!ds.Next());
      }

      SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_RESETCONTENT, 0, 0);
      SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)"Default");

      int cursel=0;
      WDL_String tmp;
      int i;
      for (i=0; i < s_patterncfgs.GetSize(); ++i)
      {
        const char* p=s_patterncfgs.Get(i);
        int a=SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)p);
        if (!stricmp(s_cfgfn, p)) cursel=a;
      }

      SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)"(refresh list)");
      SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)"(open config directory)");
      SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_SETCURSEL, cursel, 0);
    }
    return 0;

    case WM_USER+1024:
      if (wParam > 1 && lParam)
      {
        char name[512];
        GetDlgItemText(hwndDlg, IDC_EDIT4, name, sizeof(name));

        int flags=0;
        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK1)) flags |= 1;
        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK2)) flags |= 2;
        if (IsDlgButtonChecked(hwndDlg, IDC_CHECK3)) flags |= 4;

        char buf[512];
        GetDlgItemText(hwndDlg, IDC_EDIT1, buf, sizeof(buf));
        int recvport=atoi(buf);
        
        char sendip[512];
        GetDlgItemText(hwndDlg, IDC_EDIT2, sendip, sizeof(sendip));
        GetDlgItemText(hwndDlg, IDC_EDIT3, buf, sizeof(buf));
        int sendport=atoi(buf);

        GetDlgItemText(hwndDlg, IDC_EDIT7, buf, sizeof(buf));
        int maxpacketsz=atoi(buf);
        GetDlgItemText(hwndDlg, IDC_EDIT8, buf, sizeof(buf));
        int sendsleep=atoi(buf);

        WDL_String str;
        PackCfg(&str, name, flags, recvport, sendip, sendport, maxpacketsz, sendsleep, s_cfgfn);
        lstrcpyn((char*)lParam, str.Get(), wParam);
      }
    return 0;
  }

  return 0;
}

static HWND configFunc(const char* typestr, HWND parent, const char* cfgstr)
{
  return CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SURFACEEDIT_OSC), parent, dlgProc,(LPARAM)cfgstr);
}

reaper_csurf_reg_t csurf_osc_reg = 
{
  "OSC",
  "OSC (Open Sound Control)",
  createFunc,
  configFunc,
};
