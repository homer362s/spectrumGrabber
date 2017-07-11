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
#define  MAINPANEL_GRAPH                  2       /* control type: graph, callback function: (none) */
#define  MAINPANEL_VGSTARTBOX             3       /* control type: numeric, callback function: editVgBox_CB */
#define  MAINPANEL_VGSTOPBOX              4       /* control type: numeric, callback function: editVgBox_CB */
#define  MAINPANEL_NUMVGSTEPBOX           5       /* control type: numeric, callback function: editVgBox_CB */
#define  MAINPANEL_VDSTARTBOX             6       /* control type: numeric, callback function: editVdBox_CB */
#define  MAINPANEL_VDSTOPBOX              7       /* control type: numeric, callback function: editVdBox_CB */
#define  MAINPANEL_NUMVDSTEPBOX           8       /* control type: numeric, callback function: editVdBox_CB */
#define  MAINPANEL_BUILDTABLEBUTTON       9       /* control type: command, callback function: buildtableButton_CB */
#define  MAINPANEL_VGCOEFFBOX             10      /* control type: numeric, callback function: (none) */
#define  MAINPANEL_VDCOEFFBOX             11      /* control type: numeric, callback function: (none) */
#define  MAINPANEL_AVGBOX                 12      /* control type: numeric, callback function: (none) */
#define  MAINPANEL_TABLE                  13      /* control type: table, callback function: (none) */
#define  MAINPANEL_STOPBUTTON             14      /* control type: command, callback function: stopButton_CB */
#define  MAINPANEL_NEXTBUTTON             15      /* control type: command, callback function: nextButton_CB */
#define  MAINPANEL_BINSRING               16      /* control type: ring, callback function: binsRing_CB */
#define  MAINPANEL_FILEPREFIX             17      /* control type: string, callback function: (none) */
#define  MAINPANEL_LOADBUTTON             18      /* control type: command, callback function: loadButton_CB */
#define  MAINPANEL_RUNBUTTON              19      /* control type: command, callback function: runButton_CB */
#define  MAINPANEL_CLEARBUTTON            20      /* control type: command, callback function: clearButton_CB */
#define  MAINPANEL_DELROWBUTTON           21      /* control type: command, callback function: (none) */
#define  MAINPANEL_ADDROWBUTTON           22      /* control type: command, callback function: addrowButton_CB */
#define  MAINPANEL_VDSTEPSIZEBOX          23      /* control type: numeric, callback function: (none) */
#define  MAINPANEL_VGSTEPSIZEBOX          24      /* control type: numeric, callback function: (none) */
#define  MAINPANEL_MEASCOUNTDISP          25      /* control type: textMsg, callback function: (none) */
#define  MAINPANEL_DECORATION             26      /* control type: deco, callback function: (none) */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK addrowButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK binsRing_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK buildtableButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clearButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK editVdBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK editVgBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK loadButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK mainpanel_CB(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK nextButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK runButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK stopButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif