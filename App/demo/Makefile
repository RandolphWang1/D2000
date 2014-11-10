CC = /usr/local/arm/3.4.1/bin/arm-linux-gcc
LINK = $(CC)

OBJ = adc key printer psam0 psam1 pwm gprs mgcard App rc531
all:$(OBJ)
$(OBJ):%:demo_%.c
	$(LINK) -L../ -l_function -o   $@ $<
	mv $@ ../demo_App
clean:
	rm -rf *~ App *.o demo_adc demo_key
 
