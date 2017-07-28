/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  MAINPANEL                        1       /* callback function: mainpanel_CB */
#define  MAINPANEL_VGCOEFFBOX             2       /* control type: numeric, callback function: (none) */
#define  MAINPANEL_VDCOEFFBOX             3       /* control type: numeric, callback function: (none) */
#define  MAINPANEL_AVGBOX                 4       /* control type: numeric, callback function: avgBox_CB */
#define  MAINPANEL_TABLE                  5       /* control type: table, callback function: (none) */
#define  MAINPANEL_STOPBUTTON             6       /* control type: command, callback function: stopButton_CB */
#define  MAINPANEL_NEXTBUTTON             7       /* control type: command, callback function: nextButton_CB */
#define  MAINPANEL_BINSRING               8       /* control type: ring, callback function: binsRing_CB */
#define  MAINPANEL_FILEPATH               9       /* control type: string, callback function: (none) */
#define  MAINPANEL_GENERATEBUTTON         10      /* control type: command, callback function: generateButton_CB */
#define  MAINPANEL_LOADBUTTON             11      /* control type: command, callback function: loadButton_CB */
#define  MAINPANEL_RUNBUTTON              12      /* control type: command, callback function: runButton_CB */
#define  MAINPANEL_CLEARBUTTON            13      /* control type: command, callback function: clearButton_CB */
#define  MAINPANEL_SAVETABLEBUTTON_2      14      /* control type: command, callback function: savetableButton_CB */
#define  MAINPANEL_DELROWBUTTON           15      /* control type: command, callback function: delrowButton_CB */
#define  MAINPANEL_ADDROWBUTTON           16      /* control type: command, callback function: addrowButton_CB */
#define  MAINPANEL_VDNUM                  17      /* control type: numeric, callback function: boardNum_CB */
#define  MAINPANEL_VGNUM                  18      /* control type: numeric, callback function: boardNum_CB */
#define  MAINPANEL_BOARDNUM               19      /* control type: numeric, callback function: boardNum_CB */
#define  MAINPANEL_RATEBOX                20      /* control type: numeric, callback function: rateBox_CB */
#define  MAINPANEL_RANGED                 21      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_RANGEC                 22      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_RANGEB                 23      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_RANGEA                 24      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_DACBUTTON              25      /* control type: textButton, callback function: dacButton_CB */
#define  MAINPANEL_COUPLINGD              26      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_LEDD                   27      /* control type: LED, callback function: (none) */
#define  MAINPANEL_COEFFD                 28      /* control type: numeric, callback function: channel_CB */
#define  MAINPANEL_TIMED                  29      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_TIMEC                  30      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_TIMEB                  31      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_TIMEA                  32      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_COUPLINGC              33      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_LEDC                   34      /* control type: LED, callback function: (none) */
#define  MAINPANEL_COEFFC                 35      /* control type: numeric, callback function: channel_CB */
#define  MAINPANEL_FREQD                  36      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_COUPLINGB              37      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_FREQC                  38      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_LEDB                   39      /* control type: LED, callback function: (none) */
#define  MAINPANEL_FREQB                  40      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_COEFFB                 41      /* control type: numeric, callback function: channel_CB */
#define  MAINPANEL_FREQA                  42      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_COUPLINGA              43      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_LEDA                   44      /* control type: LED, callback function: (none) */
#define  MAINPANEL_COEFFA                 45      /* control type: numeric, callback function: channel_CB */
#define  MAINPANEL_DELAYBOX               46      /* control type: numeric, callback function: delayBox_CB */
#define  MAINPANEL_TEXTMSG_3              47      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_DECORATION_3           48      /* control type: deco, callback function: (none) */
#define  MAINPANEL_BIASCOUNTDISP          49      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_TEXTMSG                50      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_AVGCOUNTDISP           51      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_TAB                    52      /* control type: tab, callback function: (none) */
#define  MAINPANEL_DECORATION_5           53      /* control type: deco, callback function: (none) */
#define  MAINPANEL_BINSIZETEXT            54      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_CAPTIMETEXT            55      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_TOTALTIMETEXT          56      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_BIASTIMETEXT           57      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_DIRBUTTON              58      /* control type: command, callback function: dirButton_CB */
#define  MAINPANEL_BINSIZETEXTLABEL       59      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_FILEPREFIX             60      /* control type: string, callback function: (none) */
#define  MAINPANEL_OLLEDD                 61      /* control type: LED, callback function: (none) */
#define  MAINPANEL_OLLEDC                 62      /* control type: LED, callback function: (none) */
#define  MAINPANEL_OLLEDB                 63      /* control type: LED, callback function: (none) */
#define  MAINPANEL_OLLEDA                 64      /* control type: LED, callback function: (none) */
#define  MAINPANEL_CAPTIMETEXTLABEL       65      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_TOTALTIMETEXTLABEL     66      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_DECORATION             67      /* control type: deco, callback function: (none) */
#define  MAINPANEL_TEXTMSG_4              68      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_TEXTMSG_5              69      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_TEXTMSG_7              70      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_TEXTMSG_6              71      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_BIASTIMETEXTLABEL      72      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_DECORATION_4           73      /* control type: deco, callback function: (none) */
#define  MAINPANEL_TEXTMSG_2              74      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_PICOSCOPERING          75      /* control type: ring, callback function: picoscopeRing_CB */
#define  MAINPANEL_DECORATION_2           76      /* control type: deco, callback function: (none) */

#define  TGPANEL                          2       /* callback function: tgPanel_CB */
#define  TGPANEL_VGSTARTBOX               2       /* control type: numeric, callback function: editVgBox_CB */
#define  TGPANEL_VGSTOPBOX                3       /* control type: numeric, callback function: editVgBox_CB */
#define  TGPANEL_NUMVGSTEPBOX             4       /* control type: numeric, callback function: editVgBox_CB */
#define  TGPANEL_VGSTEPSIZEBOX            5       /* control type: numeric, callback function: (none) */
#define  TGPANEL_VDSTARTBOX               6       /* control type: numeric, callback function: editVdBox_CB */
#define  TGPANEL_VDSTOPBOX                7       /* control type: numeric, callback function: editVdBox_CB */
#define  TGPANEL_NUMVDSTEPBOX             8       /* control type: numeric, callback function: editVdBox_CB */
#define  TGPANEL_VDSTEPSIZEBOX            9       /* control type: numeric, callback function: (none) */
#define  TGPANEL_BUILDTABLEBUTTON         10      /* control type: command, callback function: buildtableButton_CB */
#define  TGPANEL_GENERATERING             11      /* control type: ring, callback function: generateRing_CB */
#define  TGPANEL_CLOSETGBUTTON            12      /* control type: command, callback function: closetgButton_CB */
#define  TGPANEL_TEXTMSG                  13      /* control type: textMsg, callback function: (none) */
#define  TGPANEL_TEXTMSG_2                14      /* control type: textMsg, callback function: (none) */
#define  TGPANEL_TEXTMSG_3                15      /* control type: textMsg, callback function: (none) */
#define  TGPANEL_TEXTMSG_4                16      /* control type: textMsg, callback function: (none) */
#define  TGPANEL_TEXTMSG_5                17      /* control type: textMsg, callback function: (none) */

     /* tab page panel controls */
#define  FREQTAB_FREQGRAPH                2       /* control type: graph, callback function: (none) */

     /* tab page panel controls */
#define  TIMETAB_TIMEGRAPH                2       /* control type: graph, callback function: (none) */
#define  TIMETAB_MINTIMEBOX               3       /* control type: numeric, callback function: updateTimeAxis_CB */
#define  TIMETAB_MAXTIMEBOX               4       /* control type: numeric, callback function: updateTimeAxis_CB */


     /* Control Arrays: */

#define  CTRLARRAY                        1

     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK addrowButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK avgBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK binsRing_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK boardNum_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK buildtableButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK channel_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clearButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK closetgButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK dacButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK delayBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK delrowButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK dirButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK editVdBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK editVgBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK generateButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK generateRing_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK loadButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK mainpanel_CB(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK nextButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK picoscopeRing_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK rateBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK runButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK savetableButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK stopButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK tgPanel_CB(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK updateTimeAxis_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
