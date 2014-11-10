//int fd, fd_key, fd_mifare, fd_tty, fd_mg, fd_tlc549, fd_psam;
int fd_psam;
int no_big_psam = 0, no_small_psam = 0;
//, logo_state=0;
int fill_mark = 1, fill_mark_signal = 2;
int count = 0, count_printer = 0;
int page = 1;
    //int asyncio_fd = -1;
//int style=0;
int state = 0;
//, time_flag = 0;
//int set_flag, logo_flag = 0;
char getmsg[500] = { 0, };
//int k = 0, x, y;
//int light_line;			//∏ﬂ¡¡
extern char str4[50];
//extern unsigned char pos_logo[];
extern unsigned char year_code[];
//extern char *pos_chnmenu1[20],*pos_chnmenu2[60],*pos_ukmenu1[20],*pos_ukmenu2[60],*pos_chnopen[50],*pos_ukopen[50];
//char **pos_menu1,**pos_menu2,**pos_menu3,**pos_open;


struct gpio_config 
{
    int port;
    int num;
    int data;
};

struct features
{
    int size[3];
	int symbol[3];
}p;

struct itimerval tick;
struct gpio_config gpio;
static int poweroff = 0;
