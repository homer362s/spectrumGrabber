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
#define  MAINPANEL_FREQGRAPH              2       /* control type: graph, callback function: (none) */
#define  MAINPANEL_VGCOEFFBOX             3       /* control type: numeric, callback function: (none) */
#define  MAINPANEL_VDCOEFFBOX             4       /* control type: numeric, callback function: (none) */
#define  MAINPANEL_AVGBOX                 5       /* control type: numeric, callback function: avgBox_CB */
#define  MAINPANEL_TABLE                  6       /* control type: table, callback function: (none) */
#define  MAINPANEL_STOPBUTTON             7       /* control type: command, callback function: stopButton_CB */
#define  MAINPANEL_NEXTBUTTON             8       /* control type: command, callback function: nextButton_CB */
#define  MAINPANEL_BINSRING               9       /* control type: ring, callback function: binsRing_CB */
#define  MAINPANEL_FILEPREFIX             10      /* control type: string, callback function: (none) */
#define  MAINPANEL_GENERATEBUTTON         11      /* control type: command, callback function: generateButton_CB */
#define  MAINPANEL_LOADBUTTON             12      /* control type: command, callback function: loadButton_CB */
#define  MAINPANEL_RUNBUTTON              13      /* control type: command, callback function: runButton_CB */
#define  MAINPANEL_CLEARBUTTON            14      /* control type: command, callback function: clearButton_CB */
#define  MAINPANEL_SAVETABLEBUTTON_2      15      /* control type: command, callback function: savetableButton_CB */
#define  MAINPANEL_DELROWBUTTON           16      /* control type: command, callback function: delrowButton_CB */
#define  MAINPANEL_ADDROWBUTTON           17      /* control type: command, callback function: addrowButton_CB */
#define  MAINPANEL_DECORATION_2           18      /* control type: deco, callback function: (none) */
#define  MAINPANEL_VDNUM                  19      /* control type: numeric, callback function: boardNum_CB */
#define  MAINPANEL_VGNUM                  20      /* control type: numeric, callback function: boardNum_CB */
#define  MAINPANEL_BOARDNUM               21      /* control type: numeric, callback function: boardNum_CB */
#define  MAINPANEL_RATEBOX                22      /* control type: numeric, callback function: rateBox_CB */
#define  MAINPANEL_RANGED                 23      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_RANGEC                 24      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_RANGEB                 25      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_RANGEA                 26      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_DACBUTTON              27      /* control type: textButton, callback function: dacButton_CB */
#define  MAINPANEL_COUPLINGD              28      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_LEDD                   29      /* control type: LED, callback function: (none) */
#define  MAINPANEL_COEFFD                 30      /* control type: numeric, callback function: channel_CB */
#define  MAINPANEL_TIMED                  31      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_TIMEC                  32      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_TIMEB                  33      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_TIMEA                  34      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_COUPLINGC              35      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_LEDC                   36      /* control type: LED, callback function: (none) */
#define  MAINPANEL_COEFFC                 37      /* control type: numeric, callback function: channel_CB */
#define  MAINPANEL_DECORATION_3           38      /* control type: deco, callback function: (none) */
#define  MAINPANEL_FREQD                  39      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_COUPLINGB              40      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_FREQC                  41      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_LEDB                   42      /* control type: LED, callback function: (none) */
#define  MAINPANEL_FREQB                  43      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_COEFFB                 44      /* control type: numeric, callback function: channel_CB */
#define  MAINPANEL_FREQA                  45      /* control type: radioButton, callback function: channel_CB */
#define  MAINPANEL_COUPLINGA              46      /* control type: ring, callback function: channel_CB */
#define  MAINPANEL_LEDA                   47      /* control type: LED, callback function: (none) */
#define  MAINPANEL_COEFFA                 48      /* control type: numeric, callback function: channel_CB */
#define  MAINPANEL_DELAYBOX               49      /* control type: numeric, callback function: (none) */
#define  MAINPANEL_TEXTMSG                50      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_TEXTMSG_2              51      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_DECORATION             52      /* control type: deco, callback function: (none) */
#define  MAINPANEL_AVGCOUNTDISP           53      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_BIASCOUNTDISP          54      /* control type: textMsg, callback function: (none) */

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
int  CVICALLBACK dacButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK delrowButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK editVdBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK editVgBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK generateButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK generateRing_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK loadButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK mainpanel_CB(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK nextButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK rateBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK runButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK savetableButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK stopButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK tgPanel_CB(int panel, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
