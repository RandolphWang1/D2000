CC = /usr/local/arm/3.4.1/bin/arm-linux-gcc
LINK = $(CC)
OBJ = App.c rc531.c gprs.c gprs_socket.c pwm.c mg_card.c psam.c sleep.c usb.c printer.c bios_test.c calendar.c help.c sdcard.c function.c  demo_rc531.c
#OBJ = pcnb.o
#OBJ = App.o
App : $(OBJ)
	$(LINK) -lm -pthread -o lib_function.so -fPIC -rdynamic -shared  $(OBJ)
#	$(LINK) -o pcnb -L./ -l_function $(OBJ)
#	arm-linux-gcc -lm -pthread -o lib_function.so -fPIC -rdynamic  -shared App.c rc531.c gprs.c gprs_socket.c pwm.c mg_card.c psam.c sleep.c usb.c printer.c pos_logo.c bios_test.c calendar.c help.c sdcard.c function.c
#	arm-linux-gcc -lm -lpthread -o App $(OBJ)
#lib= App.o rc531.o gprs.o gprs_socket.o pwm.o mg_card.o psam.o sleep.o usb.o printer.o bios_test.o calendar.o help.o sdcard.o function.o 
#lib: 
#	$(LINK) -lm -pthread -o lib_function.so -fPIC -rdynamic -shared $< 
clean:
	rm -rf *~ App *.o
 
