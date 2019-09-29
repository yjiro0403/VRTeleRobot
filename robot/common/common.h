#ifndef __COMMON_H__
#define	__COMMON_H__

#define	DEG2RAD	M_PI/180.0
#define	RAD2DEG	180.0/M_PI

#ifndef TRUE
#define TRUE		1
#endif   /* TRUE */

#ifndef FALSE
#define FALSE		0
#endif   /* FALSE */

#ifndef BUF_SIZE
#define	BUF_SIZE	256
#endif   /* BUF_SIZE */

#ifndef MAX_BUF
#define	MAX_BUF		256
#endif

#ifndef MS2S
#define	MS2S		1000000		/* micro sec to sec */
#endif   /* MS2S */

#ifndef S2MS
#define	S2MS		0.000001	/* sec to micro sec */
#endif   /* S2MS */

#ifndef MM2M
#define	MM2M		0.001		/* mm to m */
#endif   /* S2MS */

#ifndef MM2CM
#define	MM2CM		0.1		/* cm to m */
#endif   /* S2MS */

#ifndef TIMEOUT
#define TIMEOUT        4
#endif


#define CHECK_MALLOC(ret, msg) \
    if(ret==NULL){fprintf(stderr, msg); return FALSE;}

#define CHECK_MALLOC_VOID(ret, msg) \
    if(ret==NULL){fprintf(stderr, msg); return NULL;}


/* define device information */
/*** Camera ***/
/* maximum number of cameras that this library can use */
#define	MAX_CAM		2

/*** Microphone ***/
#define	MAX_MIC		1

/*** Microphone ***/
#define	MAX_LRF		1

/*** Common ***/
/* maximum number of devices */
#define	MAX_DEV		MAX_CAM+MAX_MIC+MAX_LRF

/* maximum number of devices */
#define	MAX_AIM		MAX_DEV

/* maximum number of internal processing threads */
#define	MAX_IN		2

/* maximum number of internal processing threads */
#define	MAX_EX		2

/* maximum number of external/internal processing threads */
#define	MAX_PROC	2

/* Kanji encoding */
/*
#define	JIS	0
#define	SJIS	1
#define	EUC	2
#define	UTF8	3
#define	UTF16	4
*/
#endif
