//#include "head.h"

int main()
{
    configure_init();   //��ʼ����Ļ�Աȶȣ�����
    security_server();  
    lcd_clear();
    time_show();
    system_show(); 
    return 0;
}				
