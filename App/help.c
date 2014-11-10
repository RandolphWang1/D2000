#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/time.h> 
#include <signal.h>


extern char **pos_menu1,**pos_menu2,**pos_menu3,**pos_open;

char *pos_chnopen[50]= {"        ��ˢ�� ",\
	" ˢ�������ʱ",\
		" �������Ϊ��5    ",\
		" ����Ϊ33,33..33��",\
		" ��ˢ����",\
		" �������Ϊ��5",\
		" ��ˢ����",\
		" ���ڳ�ʼ����",\
		" д��ʧ��",\
		" д���ɹ�",\
		" ����ʧ��",\
		" �����ɹ�",\
		" ��ȡ���ݳɹ���",\
		" PPP�����У����Ժ�..",\
		" GPRSģ�����",\
		" GSM������",\
		" ",\
		" PPP���ųɹ�,��ȡ����:",\
		" ��ȡ����ʧ��!",\
		" �ɹ��ر�GPRS",\
		" ��ˢ��:",\
		" ˢ��ʧ��,����ˢ",\
		" ˢ��ʧ��,����ˢ",\
		" ��ص�����Ӧ��ADֵ��",\
		" ��ص�����Ӧ��ADֵ��",\
		" ��ϵ�����PSAM���ٲ���!",\
		" ��ϵ�����PSAM���ٲ���!",\
		" ��λ����ֵ:",\
		" ���������:",\
		" ���ȸ�λ����",\
		" ���سɹ�",\
		" ж�سɹ�",\
		" ���óɹ�",\
		" �����뿪������",\
		" �������",\
		" �����뿪������",\
		" ������������",\
		" ������ԭʼ����",\
		" �����޸ĳɹ�",\
		" ������������",\
		" �������",\
		" ������ԭʼ����",\
		"      �Աȶȵ���",\
		"      �Աȶȵ���",\
		"       ���ȵ���",\
		"       ���ȵ���",\
}; //���ı�����Ϣ

char *pos_ukopen[50]={" Swip your card: ",\
	" Timeout",\
		" Block number:5",\
		" Data:33.33.33",\
		" Rwip your card",\
		"Block number:5",\
		"Swipe your card",\
		" Initialing:",\
		" Write failed",\
		" Writting succeed",\
		" Read failed",\
		" Reading succeed",\
		" Received:",\
		" Initialing:",\
		" GPRS error!",\
		" GSM card error!",\
		" GPRS ready..",\
		" Received:",\
		" Receive failed!",\
		" Shutdown GPRS",\
		" Swip your card:",\
		" Swipe too fast try again",\
		" Timeout",\
		" Transformed value of AD power",\
		" Transformed value of AD power",\
		" Check your PSAM card and try again!",\
		" Check your PSAM card and try again!",\
		" Retrun value :",\
		" Sending numbers:",\
		" Please reset first",\
		" Mount succeed",\
		" Umount succeed",\
		" Set up complete",\
		" Please enter your   password",\
		" Fault password",\
		" Please enter your   password",\
		" Please enter a new    password",\
		" Please enter your   password",\
		" Password modefied   succeed",\
		" Please enter a new password",\
		" Fault password",\
		" Please enter your   password",\
		" ajuset contrast",\
		" ajuset contrast",\
		" ajuset brightness",\
		" ajuset brightness",\
};//Ӣ�ı�����Ϣ

char *pos_chnmenu1[20]={"      �� �� �� ��",\
	"  1.����     2.����",\
		"  3.IC��     4.GPRS",\
		"  5.��ӡ��   6.������",\
		"      �� �� �� ��",\
		"  7.��Ƶ��   8.˯��",\
		"  9.ϵͳ����",\
		" ",\
		" 1.PSAMС������",\
		" 2.PSAM�󿨲���",\
		" 6.����������",\
		"  9.USB��������",\
		" 10.˯�߲���",\
		//												" 11.����ѡ��",
		" 11.ϵͳ���ò˵�",\
		" 12.SD������",\
		" 4.����ѡ��",
		" 5.������ʾ",\
			" 6.ʱ������",\
			"     IC�����Բ˵�",\
			" 3.�˳�"
};



char *pos_ukmenu1[20]={   "     Function menu",\
	" 1.Sound   2.Power",\
		" 3.IC card 4.GPRS",\
		" 5.Printer 6.Brushcard",\
		"    Function menu",\
		" 7.RFID    8.Sleep",\
		" 9.Update",\
		" ",\
		" 1.MiniPSAM test",\
		" 2.LargePSAM test",\
		" 6.����������",\
		"  9.USB��������",\
		" 10.˯��",\
		//												" 11.����ѡ��",
		" 11.ϵͳ���ò˵�",\
		" 12.SD������",\
		" 4.����ѡ��",
		" 5.������ʾ",\
			" 6.ʱ������",\
			"    PSAM menu",\
			" 3.Quit"
};

char *pos_chnmenu2[60]={"      ��Ƶ���˵�",\
	" 1.��ȡ����",\
		" 2.ע�Ῠ��",\
		" 3.��ֵ����",\
		"     GPRS��ʾ�˵�",\
		" 1.���Ͷ��� ",\
		" 2.����绰",\
		" ",\
		"      �������Բ˵�",\
		" 1.��������",\
		" 2.�˳�",\
		"",\
		"   ��ص������˵�",\
		" 1.�������",\
		" 2.�˳�",\
		"",\
		"     ��ӡ���Բ˵�",\
		" 1.��ӡ����",\
		" 2.�˳�",\
		"",\
		"     ���������Բ˵�",\
		" 1.ˢ������",\
		" 2.�˳�",\
		"",\
		"      PSAMС���˵�",\
		" 1.�临λ����",\
		" 2.��ȡ�����",\
		"",\
		"    PSAM�󿨲˵�",\
		" 1.�临λ����",\
		" 2.��ȡ�����",\
		"",\
		"    USB������ʾ�˵�",\
		" 1.�����̷�",\
		" 2.ж���̷�",\
		" 3.�˳�",\
		"      ϵ ͳ �� ��",\
		" 1.TF������",\
		" 2.�˳�",\
		" ",\
		"     ����ѡ��˵�",\
		" 1.����",\
		" 2.Ӣ��",\
		"",\
		"      ϵ ͳ �� ��",\
		" 1.��������2.����ѡ��",\
		" 3.��������4.������ʾ",\
		" 5.ʱ������6.�Աȶ�"  

};

char *pos_ukmenu2[60]={"     RFID menu",\
	" 1.Get card NO.",\
		" 2.Write card",\
		" 3.Read card",\
		"   GPRS display menu",\
		" 1.Get data test",\
		" 2.Close socket",\
		" 3.Quit",\
		"    PWM display menu",\
		" 1.PWM test",\
		" 2.Quit",\
		"",\
		"     Vol check test",\
		" 1.Voltage check",\
		" 2.Quit",\
		"",\
		"   Printer menu",\
		" 1.Print test",\
		" 2.Quit",\
		"",\
		"     Brushcard menu",\
		" 1.Brush card",\
		" 2.Quit",\
		"",\
		"   MiniPSAM display",\
		" 1.Cold reset",\
		" 2.Get random NO.",\
		" 3.Quit",\
		"   BigPSAM display",\
		" 1.Cold reset",\
		" 2.Get random NO.",\
		" 3.Quit",\
		"       USB updata",\
		" 1.Mount disk",\
		" 2.Umount disk",\
		" 3.Quit",\
		"     System update",\
		" 1.USB update",\
		" 2.TF update",\
		" 3.Quit",\
		" Language selection",\
		" 1.Chinese",\
		" 2.English",\
		"",\
		"     Configuration",\
		" 1.Key     2.Language",\
		" 3.Light   4.Calendar",\
		" 5.Time    6.Contrast",\
};

char *pos_chnedit[50] = {
	" ����ʱ���޸�",\
		"ʱ�����޸�",\
		" �͹���ģʽ����",\
		"��Ƶ���˵�",\
		" 4.���Ѳ���",\

		" 5.�˳�",\
		" ppp����ʧ��",\
		" ģ������10����ٲ�",\
		"",\
		" ��ȡ��������Ϣ",\

		" ����:",\
		" ˢ����ʱ,����ˢ",\
		" ��ȷ������",\
		" ������������",\
		" �������޸�",\

		"LCD�Աȶȵ��ڳɹ�",\
		"����ϵͳ��...",\
		"��ʼ��GPRS...",\
		"GPRS��ʼ�����",\
		"GPRS��ʼ��ʧ��",\

		"���SIM��...",\
		"GSM����״̬���" ,\
		"GSM��ע��" ,\
		"GSMδע��" ,\
		"GPRS����״̬���",\

		"GPRS��ע��",\
		"GPRSδע��",\
		"TCP���Ӳ���",\
		"TCP����ʧ��" ,\
		"TCP���ӳɹ�" ,\

		"GPRS����������",\
		"������δ��Ӧ",\
		" ��ӡȱֽ",\
		"SIM�����ɹ�",\
		"SIM�����ʧ��",\

	    " ��������շ����룺",\
		" ",\
		" ",\
		" ���",\
		" ��ֵ",\

		" ����",\
};

char *pos_ukedit[50] = {
	" change date",\
		" date had been changed",\
		" low power mode error",\
		" mgcard menu",\
		" 4.Consume test",\
		
		" 5.Quit",\
		" PPP call error",\
		" recall after 10s",\
		" ppp call succeed",\
		" waitting server info",\

		" card no:",\
		" charge error",\
		" please ensure your passwd",\
		" enter a new passwd",\
		" passwd had been changed",\

		" contrast changed successfully",\
		" system waking up...",\
		" Gprs initial...",\
		" Gprs initial complete",\
		" Gprs initial failed",\

		" check sim card",\
		" check GSM state",\
		" GSM registed",\
		" GSM had not been registed",\
		" check gprs state",\

		" GPRS had been registed",\
		" GPRS had not been registed",\
		" check for tcp connection",\
		" TCP connection failed",\
		" TCP connection succeed",\

		" activating GPRS stage",\
		" no response from server",\
		" no paper",\
		" sim card check ok",\
		" sim card check failed",\

		" sig:",\
		" succeed:",\
		" failed:",\
		" remain",\
		" recharge",\

		" consume",\
};
