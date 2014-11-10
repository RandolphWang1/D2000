//#include "head.h"

int main()
{
    configure_init();   //初始化屏幕对比度，亮度
    security_server();  
    lcd_clear();
    time_show();
    system_show(); 
    return 0;
}				
