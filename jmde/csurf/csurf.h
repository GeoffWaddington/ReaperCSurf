#ifndef _CSURF_H_
#define _CSURF_H_

#include "../reaper_plugin.h"
#include "../../WDL/db2val.h"
#include "../../WDL/wdlstring.h"
#include "../../WDL/wdlcstring.h"
#include <stdio.h>
#include "resource.h"


enum
{
  HZOOM_EDITPLAYCUR=0,
  HZOOM_EDITCUR=1,
  HZOOM_VIEWCTR=2,
  HZOOM_MOUSECUR=3
};

enum
{
  VZOOM_VIEWCTR=0,
  VZOOM_TOPVIS=1,
  VZOOM_LASTSEL=2,
  VZOOM_MOUSECUR=3
};


extern REAPER_PLUGIN_HINSTANCE g_hInst; // used for dialogs
extern HWND g_hwnd;
/* 
** Calls back to REAPER (all validated on load)
*/
extern double (*DB2SLIDER)(double x);
extern double (*SLIDER2DB)(double y);
extern int (*GetNumMIDIInputs)(); 
extern int (*GetNumMIDIOutputs)();
extern midi_Input *(*CreateMIDIInput)(int dev);
extern midi_Output *(*CreateMIDIOutput)(int dev, bool streamMode, int *msoffset100); 
extern bool (*GetMIDIOutputName)(int dev, char *nameout, int nameoutlen);
extern bool (*GetMIDIInputName)(int dev, char *nameout, int nameoutlen);


extern int (*CSurf_TrackToID)(MediaTrack *track, bool mcpView);
extern MediaTrack *(*CSurf_TrackFromID)(int idx, bool mcpView);
extern int (*CSurf_NumTracks)(bool mcpView);

    // these will be called from app when something changes
extern void (*CSurf_SetTrackListChange)();
extern void (*CSurf_SetSurfaceVolume)(MediaTrack *trackid, double volume, IReaperControlSurface *ignoresurf);
extern void (*CSurf_SetSurfacePan)(MediaTrack *trackid, double pan, IReaperControlSurface *ignoresurf);
extern void (*CSurf_SetSurfaceMute)(MediaTrack *trackid, bool mute, IReaperControlSurface *ignoresurf);
extern void (*CSurf_SetSurfaceSelected)(MediaTrack *trackid, bool selected, IReaperControlSurface *ignoresurf);
extern void (*CSurf_SetSurfaceSolo)(MediaTrack *trackid, bool solo, IReaperControlSurface *ignoresurf);
extern void (*CSurf_SetSurfaceRecArm)(MediaTrack *trackid, bool recarm, IReaperControlSurface *ignoresurf);
extern bool (*CSurf_GetTouchState)(MediaTrack *trackid, int isPan);
extern void (*CSurf_SetAutoMode)(int mode, IReaperControlSurface *ignoresurf);

extern void (*CSurf_SetPlayState)(bool play, bool pause, bool rec, IReaperControlSurface *ignoresurf);
extern void (*CSurf_SetRepeatState)(bool rep, IReaperControlSurface *ignoresurf);

// these are called by our surfaces, and actually update the project
extern double (*CSurf_OnVolumeChange)(MediaTrack *trackid, double volume, bool relative);
extern double (*CSurf_OnPanChange)(MediaTrack *trackid, double pan, bool relative);
extern double (*CSurf_OnVolumeChangeEx)(MediaTrack *trackid, double volume, bool relative, bool allowgang);
extern double (*CSurf_OnPanChangeEx)(MediaTrack *trackid, double pan, bool relative, bool allowgang);
extern double (*CSurf_OnWidthChangeEx)(MediaTrack* trackid, double width, bool relative, bool allowgang);
extern bool (*CSurf_OnMuteChange)(MediaTrack *trackid, int mute);
extern bool (*CSurf_OnMuteChangeEx)(MediaTrack *trackid, int mute, bool allowgang);
extern bool (*CSurf_OnSelectedChange)(MediaTrack *trackid, int selected);
extern bool (*CSurf_OnSoloChange)(MediaTrack *trackid, int solo);
extern bool (*CSurf_OnSoloChangeEx)(MediaTrack *trackid, int solo, bool allowgang);
extern bool (*CSurf_OnFXChange)(MediaTrack *trackid, int en);
extern bool (*CSurf_OnRecArmChange)(MediaTrack *trackid, int recarm);
extern bool (*CSurf_OnRecArmChangeEx)(MediaTrack *trackid, int recarm, bool allowgang);
extern int (*CSurf_OnInputMonitorChange)(MediaTrack* trackid, int monitor);
extern int (*CSurf_OnInputMonitorChangeEx)(MediaTrack* trackid, int monitor, bool allowgang);

extern void (*CSurf_OnPlay)();
extern void (*CSurf_OnStop)();
extern void (*CSurf_OnPause)();
extern void (*CSurf_OnFwd)(int seekplay);
extern void (*CSurf_OnRew)(int seekplay);
extern void (*CSurf_OnRecord)();
extern void (*CSurf_GoStart)();
extern void (*CSurf_GoEnd)();
extern void (*CSurf_OnArrow)(int whichdir, bool wantzoom);
extern void (*CSurf_OnScroll)(int xdir, int ydir);
extern void (*CSurf_OnZoom)(int xdir, int ydir);
extern void (*CSurf_OnTrackSelection)(MediaTrack *trackid);
extern void (*CSurf_ResetAllCachedVolPanStates)();
extern void (*CSurf_ScrubAmt)(double amt);

extern double (*CSurf_OnSendVolumeChange)(MediaTrack* trackid, int sendidx, double volume, bool relative);
extern double (*CSurf_OnSendPanChange)(MediaTrack* trackid, int sendidx, double pan, bool relative);
extern double (*CSurf_OnRecvVolumeChange)(MediaTrack* trackid, int recvidx, double volume, bool relative);
extern double (*CSurf_OnRecvPanChange)(MediaTrack* trackid, int recvidx, double pan, bool relative);

extern void (*CSurf_OnPlayRateChange)(double playrate);
extern void (*CSurf_OnTempoChange)(double bpm);


// when the user enables an osc surface for the kbd/control system,
// this func is called when a message is received from that surface
extern void (*CSurf_OnOscControlMessage)(const char* msg, const float* arg);

extern double (*Master_GetPlayRate)(ReaProject*);
extern double (*Master_NormalizePlayRate)(double playrate, bool isnormalized);
extern double (*Master_GetTempo)();
extern double (*Master_NormalizeTempo)(double bpm, bool isnormalized);

extern void (*kbd_OnMidiEvent)(MIDI_event_t *evt, int dev_index);

extern const char *(*GetTrackInfo)(INT_PTR track, int *flags); 
extern void* (*GetSetMediaTrackInfo)(INT_PTR track, const char *parmname, void *setNewValue);
extern int (*GetMasterTrackVisibility)();
extern MediaTrack* (*SetMixerScroll)(MediaTrack* leftmosttrack);
extern MediaTrack* (*GetMixerScroll)();

extern int (*GetMasterMuteSoloFlags)();
extern void (*TrackList_UpdateAllExternalSurfaces)();

extern void (*MoveEditCursor)(double adjamt, bool dosel);
extern void (*adjustZoom)(double amt, int forceset, bool doupd, int centermode); // 0,true,-1 are defaults
extern double (*GetHZoomLevel)(); // returns pixels/second


extern void (*ClearAllRecArmed)();
extern void (*SetTrackAutomationMode)(MediaTrack *tr, int mode);
extern int (*GetTrackAutomationMode)(MediaTrack *tr);
extern bool (*AnyTrackSolo)(ReaProject*);
extern void (*SoloAllTracks)(int solo); // solo=2 for SIP
extern void (*MuteAllTracks)(bool mute);
extern void (*BypassFxAllTracks)(int bypass); // -1 = bypass all if not all bypassed, otherwise unbypass all
extern bool (*IsTrackSelected)(MediaTrack* tr);
extern void (*SetTrackSelected)(MediaTrack *tr, bool sel);
extern void (*SetOnlyTrackSelected)(MediaTrack*);
extern int (*GetPlayState)();
extern double (*GetPlayPosition)();
extern double (*GetCursorPosition)();
extern void (*format_timestr_pos)(double tpos, char *buf, int buflen, int modeoverride); // modeoverride=-1 for proj
extern void (*UpdateTimeline)(void);

extern int (*GetSetRepeat)(int val);

extern void (*SetAutomationMode)(int mode, bool onlySel);
extern void (*Main_UpdateLoopInfo)(int ignoremask);
extern bool (*Loop_OnArrow)(ReaProject*,int dir);

extern void (*Main_OnCommand)(int command, int flag);
extern void (*KBD_OnMainActionEx)(int cmd, int val, int valhw, int relmode, HWND hwnd, ReaProject *__proj);
extern HWND (*GetMainHwnd)();

extern int (*GetToggleCommandState)(int command);
extern bool (*MIDIEditor_LastFocused_OnCommand)(int cmd, bool islistview);

extern double (*TimeMap2_timeToBeats)(void *proj, double tpos, int *measures, int *cml, double *fullbeats, int *cdenom);

extern void * (*projectconfig_var_addr)(void *proj, int idx);

extern double (*Track_GetPeakInfo)(MediaTrack *tr, int chidx);
extern double (*Track_GetPeakHoldDB)(MediaTrack* tr, int chidx, bool clear);
extern bool (*GetTrackUIVolPan)(MediaTrack *tr, double *vol, double *pan);
extern bool (*GetTrackUIPan)(MediaTrack* tr, double* pan1, double* pan2, int* mode);
extern bool (*GetTrackSendUIVolPan)(MediaTrack* tr, int sendidx, double* vol, double* pan);
extern bool (*GetTrackSendName)(MediaTrack* tr, int sendidx, char* buf, int buflen);
extern bool (*GetTrackReceiveUIVolPan)(MediaTrack* tr, int recvidx, double* vol, double* pan);
extern bool (*GetTrackReceiveName)(MediaTrack* tr, int recvidx, char* buf, int buflen);

extern void (*mkvolpanstr)(char *str, double vol, double pan);
extern void (*mkvolstr)(char *str, double vol);
extern void (*mkpanstr)(char *str, double pan);

extern int (*TrackFX_GetCount)(MediaTrack *tr);
extern int (*TrackFX_GetInstrument)(MediaTrack* tr);
extern int (*TrackFX_GetEQ)(MediaTrack* tr, bool instantiate);
extern int (*TrackFX_SetEQParam)(MediaTrack* tr, int fxidx, int bandtype, int bandidx, int paramtype, double val, bool isnorm);
extern bool (*TrackFX_GetEQParam)(MediaTrack* tr, int fxidx, int paramidx, int* bandtype, int* bandidx, int* parmtype, double* normval);
extern bool (*TrackFX_SetEQBandEnabled)(MediaTrack* tr, int fxidx, int bandtype, int bandidx, bool enable);
extern bool (*TrackFX_GetEQBandEnabled)(MediaTrack* tr, int fxidx, int bandtype, int bandidx);
extern int (*TrackFX_GetNumParams)(MediaTrack *tr, int fx);
extern double (*TrackFX_GetParam)(MediaTrack *tr, int fx, int param, double *minval, double *maxval);
extern double (*TrackFX_GetParamEx)(MediaTrack *tr, int fx, int param, double *minval, double *maxval, double* midval);
extern bool (*TrackFX_SetParam)(MediaTrack *tr, int fx, int param, double val);
extern bool (*TrackFX_GetParamName)(MediaTrack *tr, int fx, int param, char *buf, int buflen);
extern bool (*TrackFX_GetFormattedParamValue)(MediaTrack* tr, int fx, int param, char* buf, int buflen);
extern bool (*TrackFX_GetFXName)(MediaTrack *tr, int fx, char *buf, int buflen);
extern bool (*TrackFX_NavigatePresets)(MediaTrack* tr, int fx, int chg);
extern bool (*TrackFX_GetPreset)(MediaTrack* tr, int fx, char* buf, int buflen);
extern bool (*TrackFX_SetPreset)(MediaTrack* tr, int fx, const char* buf);

extern double (*TrackFX_GetParamNormalized)(MediaTrack* track, int fx, int param);
extern bool (*TrackFX_SetParamNormalized)(MediaTrack* track, int fx, int param, double value);

extern int (*TrackFX_GetChainVisible)(MediaTrack*);
extern HWND (*TrackFX_GetFloatingWindow)(MediaTrack*, int);
extern void (*TrackFX_Show)(MediaTrack*, int, int);
extern bool (*TrackFX_GetEnabled)(MediaTrack*,int);
extern void (*TrackFX_SetEnabled)(MediaTrack*,int,bool);

extern bool (*TrackFX_GetOpen)(MediaTrack* tr, int fx);
extern void (*TrackFX_SetOpen)(MediaTrack* tr, int fx, bool open);

extern MediaTrack* (*GetLastTouchedTrack)();
extern bool (*GetLastTouchedFX)(int* trackidx, int* fxidx, int* parmidx);
extern bool (*GetFocusedFX)(int* trackidx, int* itemidx, int* fxidx);

extern double (*TimeMap2_beatsToTime)(ReaProject* proj, double tpos, int* measures);
extern double (*parse_timestr_pos)(const char* buf, int mode);
extern void (*SetEditCurPos)(double time, bool moveview, bool seekplay);

extern GUID *(*GetTrackGUID)(MediaTrack *tr);

extern const char* (*GetOscCfgDir)();
extern const char* (*get_ini_file)();
extern const char* (*GetAppVersion)();
extern int (*RecursiveCreateDirectory)(const char*, void*);


extern char* (*WDL_ChooseFileForOpen)(HWND, const char*, const char*, const char*, const char*, const char*, 
                                     bool, bool, const char*, void*, 
#ifdef _WIN32
                                     HINSTANCE hInstance
#else
                                     struct SWELL_DialogResourceIndex*
#endif
                                    );


extern int *g_config_csurf_rate;
extern int *g_config_zoommode;

extern int* g_vu_minvol;
extern int* g_vu_maxvol;
extern int* g_config_vudecay;

extern int __g_projectconfig_timemode2;
extern int __g_projectconfig_timemode;
extern int __g_projectconfig_timeoffs;
extern int __g_projectconfig_measoffs;
extern int __g_projectconfig_showgrid;
extern int __g_projectconfig_autoxfade;

/*
** REAPER command message defines
*/

#define IDC_REPEAT                      1068
#define ID_FILE_SAVEAS                  40022
#define ID_FILE_NEWPROJECT              40023
#define ID_FILE_OPENPROJECT             40025
#define ID_FILE_SAVEPROJECT             40026
#define IDC_EDIT_UNDO                   40029
#define IDC_EDIT_REDO                   40030
#define ID_MARKER_PREV                  40172
#define ID_MARKER_NEXT                  40173
#define ID_INSERT_MARKERRGN             40174
#define ID_INSERT_MARKER                40157
#define ID_LOOP_SETSTART                40222
#define ID_LOOP_SETEND                  40223
#define ID_METRONOME                    40364
#define ID_GOTO_MARKER1                 40161
#define ID_SET_MARKER1                  40657

// Reaper track automation modes
enum AutoMode {
  AUTO_MODE_TRIM,
  AUTO_MODE_READ,
  AUTO_MODE_TOUCH,
  AUTO_MODE_WRITE,
  AUTO_MODE_LATCH,
};

midi_Output *CreateThreadedMIDIOutput(midi_Output *output); // returns null on null

#ifdef _WIN32
#define PREF_DIRCH '\\'
#define PREF_DIRSTR "\\"
#define IS_DIR_CHAR(x) ((x)=='\\' || (x) == '/')
#else
#define PREF_DIRCH '/'
#define PREF_DIRSTR "/"
#define IS_DIR_CHAR(x) ((x) == '/')
#endif


#endif