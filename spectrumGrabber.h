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
#define  MAINPANEL_TABLE                  3       /* control type: table, callback function: (none) */
#define  MAINPANEL_RUNBUTTON              4       /* control type: command, callback function: runButton_CB */
#define  MAINPANEL_RING                   5       /* control type: ring, callback function: (none) */
#define  MAINPANEL_BINSRING               6       /* control type: ring, callback function: binsRing_CB */
#define  MAINPANEL_AVGBOX                 7       /* control type: numeric, callback function: (none) */
#define  MAINPANEL_VDCOEFFBOX             8       /* control type: numeric, callback function: (none) */
#define  MAINPANEL_VGCOEFFBOX             9       /* control type: numeric, callback function: (none) */
#define  MAINPANEL_MEASCOUNTDISP          10      /* control type: textMsg, callback function: (none) */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK binsRing_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK mainpanel_CB(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK runButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
