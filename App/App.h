#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>  
#include <sys/stat.h>   
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/time.h> 
#include <time.h> 
#include <signal.h>
#include <malloc.h>

#define DRAW_PIC 7
#define DRAW_LINE 00
#define PAGE_NUM  2
#define OPTION_NUM 8
////////////////////////////////////////////////////
#define DRAW_RECT     1
#define FILL_RECT     2
#define DRAW_ELLIPSE  3
#define FILL_ELLIPSE  4
#define DRAW_CIRCLE   5
#define FILL_CIRCLE   6
#define DRAW_PIC      7
#define GRAPHIC_FONT  8
#define SET_PIXEL     9
#define CLEAR_LCD    10
#define FILL_CNB     11
#define LCDFILL      20
//#define SAVE_RECT    11
#define PAINT_RECT   12
#define SET_ICON     13
#define SET_CONTRAST    14
#define LCD_CLOSE	15
#define LCD_WRITE_AF	   16

#define Hanzk_addr        0x0
#define Hanzk_one_sec     0x69C
#define Hanzk_sec6_offset Hanzk_addr - 2*Hanzk_one_sec //Hanzk_addr-2*Hanzk_one_sec
#define Hanzk_sec9_offset Hanzk_addr - 4*Hanzk_one_sec  //Hanzk_addr-4*Hanzk_one_sec 
#define Hanzk_han_offset  Hanzk_addr - 10*Hanzk_one_sec  //Hanzk_addr-10*Hanzk_one_sec 
#define Hanzk_space       Hanzk_addr + 0x750      //Hanzk_addr+(a2,ab) (0x750)
#define TRUE  1
#define FALSE 0

#define ASII_addr  0x0
#define Lcd_width  131
#define Lcd_height 64

#define PAGE_ONE	1
#define PAGE_TWO	2

#define MENU_FLAG	1

/****************************************************************************************************************/
										//系统配置参数	
#define CONTRAST	0
#define LIGHT		1
#define LANGUAGE	2
/****************************************************************************************************************/

/****************************************************************************************************************/
										//	键值预定义
#define KEY_UP		15
#define KEY_DOWN	10
#define KEY_F1		23
#define KEY_F2		22
#define KEY_F3		21
#define KEY_F4		20
#define KEY_CANCEL	5
#define KEY_ENTER	1

#define KEY_NUM0	3
#define KEY_NUM1	19
#define KEY_NUM2	17
#define KEY_NUM3	16
#define KEY_NUM4	14
#define KEY_NUM5	13
#define KEY_NUM6	11
#define KEY_NUM7	8
//#define KEY_NUM7	6
#define KEY_NUM8	9
//#define KEY_NUM8	8
#define KEY_NUM9	7
#define KEY_DELETE  4

/****************************************************************************************************************/
/*set_gpio*/

#define	HIGH				1
#define LOW					0

#define GPRS_PORT		   'C'
#define GPRS_PORT_NUM		7
#define PWM_PORT		   'G'
#define PWM_PORT_NUM	   10
#define MGCARD_PORT		   'G'
#define MGCARD_PORT_NUM		8
#define RC531_PORT	 	   'G'
#define RC531_PORT_NUM		7
#define USB_PORT		   'C'
#define USB_PORT_NUM	    5	   
#define PRINTTER_PORT	   'C'
#define PRINTER_PORT_NUM0	0
#define PRINTER_PORT_NUM1	1
/*************gprs function*****************/
#define LOOP		0
#define DEFAULT		4
#define NORMAL		1
#define TIMEOUT		2
#define END		3


/****************************************************************************************************************/

/****************************************************************************************************************/
/*pwm.c*/
/*
typedef struct pwm_config
{
	unsigned long  pwm_ctrl;
	unsigned long  pwm_div;
	unsigned long  pwm_period;
	unsigned long  pwm_data;
};
*/
/****************************************************************************************************************/
extern char *pos_chnmenu1[20],*pos_chnmenu2[60],*pos_ukmenu1[20],*pos_ukmenu2[60],*pos_chnopen[50],*pos_ukopen[50],*pos_chnedit[35],*pos_ukedit[35];
static char **pos_menu1,**pos_menu2,**pos_menu3,**pos_open;


typedef struct _paint_struct
{
	int arg1;
	int arg2;
	int arg3;
	int arg4;
	unsigned long ptr_data;
}paint_struct;


void set_backlignt(int state);

struct DRIVER
{
	char driver_name[13];
	int  fd;
}Drivers[20];

enum driver
{
	GRAYLCD,KEY,GPIO,ASYNCIO,POWER,RC531,TLC549,MGCARD,TTYS3,SYSCONF,Light,PWM,PSAM0,PSAM1,TTYS2,
};
