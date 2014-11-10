#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/time.h> 
#include <signal.h>


extern char **pos_menu1,**pos_menu2,**pos_menu3,**pos_open;

char *pos_chnopen[50]= {"        请刷卡 ",\
	" 刷卡错误或超时",\
		" 操作块号为：5    ",\
		" 数据为33,33..33：",\
		" 请刷卡：",\
		" 操作块号为：5",\
		" 请刷卡：",\
		" 正在初始化：",\
		" 写卡失败",\
		" 写卡成功",\
		" 读卡失败",\
		" 读卡成功",\
		" 获取数据成功：",\
		" PPP拨号中，请稍后..",\
		" GPRS模块错误！",\
		" GSM卡错误！",\
		" ",\
		" PPP拨号成功,获取数据:",\
		" 获取数据失败!",\
		" 成功关闭GPRS",\
		" 请刷卡:",\
		" 刷卡失败,请重刷",\
		" 刷卡失败,请重刷",\
		" 电池电量对应的AD值：",\
		" 电池电量对应的AD值：",\
		" 请断电后插上PSAM卡再测试!",\
		" 请断电后插上PSAM卡再测试!",\
		" 复位返回值:",\
		" 随机数发送:",\
		" 请先复位操作",\
		" 挂载成功",\
		" 卸载成功",\
		" 设置成功",\
		" 请输入开机密码",\
		" 密码错误",\
		" 请输入开机密码",\
		" 请输入新密码",\
		" 请输入原始密码",\
		" 密码修改成功",\
		" 请输入新密码",\
		" 密码错误",\
		" 请输入原始密码",\
		"      对比度调节",\
		"      对比度调节",\
		"       亮度调节",\
		"       亮度调节",\
}; //中文报错信息

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
};//英文报错信息

char *pos_chnmenu1[20]={"      功 能 菜 单",\
	"  1.语音     2.电量",\
		"  3.IC卡     4.GPRS",\
		"  5.打印机   6.磁条卡",\
		"      功 能 菜 单",\
		"  7.射频卡   8.睡眠",\
		"  9.系统升级",\
		" ",\
		" 1.PSAM小卡测试",\
		" 2.PSAM大卡测试",\
		" 6.磁条卡测试",\
		"  9.USB升级测试",\
		" 10.睡眠测试",\
		//												" 11.语言选择",
		" 11.系统设置菜单",\
		" 12.SD卡升级",\
		" 4.语言选择",
		" 5.日历显示",\
			" 6.时间设置",\
			"     IC卡测试菜单",\
			" 3.退出"
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
		" 6.磁条卡测试",\
		"  9.USB升级测试",\
		" 10.睡眠",\
		//												" 11.语言选择",
		" 11.系统设置菜单",\
		" 12.SD卡升级",\
		" 4.语言选择",
		" 5.日历显示",\
			" 6.时间设置",\
			"    PSAM menu",\
			" 3.Quit"
};

char *pos_chnmenu2[60]={"      射频卡菜单",\
	" 1.获取卡号",\
		" 2.注册卡号",\
		" 3.充值测试",\
		"     GPRS演示菜单",\
		" 1.发送短信 ",\
		" 2.拨打电话",\
		" ",\
		"      语音测试菜单",\
		" 1.语音播放",\
		" 2.退出",\
		"",\
		"   电池电量检测菜单",\
		" 1.电量检测",\
		" 2.退出",\
		"",\
		"     打印测试菜单",\
		" 1.打印测试",\
		" 2.退出",\
		"",\
		"     磁条卡测试菜单",\
		" 1.刷卡操作",\
		" 2.退出",\
		"",\
		"      PSAM小卡菜单",\
		" 1.冷复位操作",\
		" 2.获取随机数",\
		"",\
		"    PSAM大卡菜单",\
		" 1.冷复位操作",\
		" 2.获取随机数",\
		"",\
		"    USB升级演示菜单",\
		" 1.挂载盘符",\
		" 2.卸载盘符",\
		" 3.退出",\
		"      系 统 升 级",\
		" 1.TF卡升级",\
		" 2.退出",\
		" ",\
		"     语言选择菜单",\
		" 1.中文",\
		" 2.英文",\
		"",\
		"      系 统 菜 单",\
		" 1.密码设置2.语言选择",\
		" 3.亮度设置4.日历显示",\
		" 5.时间设置6.对比度"  

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
	" 日期时间修改",\
		"时间已修改",\
		" 低功耗模式错误",\
		"射频卡菜单",\
		" 4.消费测试",\

		" 5.退出",\
		" ppp拨号失败",\
		" 模块重启10秒后再拨",\
		"",\
		" 获取服务器信息",\

		" 卡号:",\
		" 刷卡超时,请重刷",\
		" 请确认密码",\
		" 请输入新密码",\
		" 密码已修改",\

		"LCD对比度调节成功",\
		"唤醒系统中...",\
		"初始化GPRS...",\
		"GPRS初始化完成",\
		"GPRS初始化失败",\

		"检测SIM卡...",\
		"GSM网络状态检测" ,\
		"GSM已注册" ,\
		"GSM未注册" ,\
		"GPRS网络状态检测",\

		"GPRS已注册",\
		"GPRS未注册",\
		"TCP连接测试",\
		"TCP连接失败" ,\
		"TCP连接成功" ,\

		"GPRS场景激活中",\
		"服务器未响应",\
		" 打印缺纸",\
		"SIM卡检测成功",\
		"SIM卡检测失败",\

	    " 请输入接收方号码：",\
		" ",\
		" ",\
		" 余额",\
		" 充值",\

		" 消费",\
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
