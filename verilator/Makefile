N=vex_top
D=../src
SRCS=$D/VexRiscv.v $D/vex_top.v $D/iosys2.v $D/simpleuart.v $D/usb1_host/usbh_core.sv \
	 $D/usb1_host/usbh_crc5.sv $D/usb1_host/usbh_crc16.sv $D/usb1_host/usbh_fifo.sv $D/usb1_host/usbh_sie.sv \
	 $D/usb1_host/usb_fs_phy.v $D/usb1_host/usb_transceiver.v

DEPS=
INCLUDES=-I$D -I$D/usb1_host

CFLAGS_SDL=-g -O2
LIBS_SDL=-g

.PHONY: build sim verilate clean gtkwave
	
build: ./obj_dir/V$N

verilate: ./obj_dir/V$N.cpp

./obj_dir/V$N.cpp: sim_main.cpp $(SRCS) $(DEPS)
	@echo
	@echo "### VERILATE ####"
	mkdir -p obj_dir
	verilator --top-module $N --trace-fst -Wno-PINMISSING -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -cc --exe -CFLAGS "$(CFLAGS_SDL)" -LDFLAGS "$(LIBS_SDL)" $(INCLUDES) $(SRCS) sim_main.cpp

./obj_dir/V$N: verilate
	@echo
	@echo "### BUILDING SIM ###"
	make -C obj_dir -f V$N.mk V$N

sim: ./obj_dir/V$N
	@echo
	@echo "### SIMULATION ###"
	obj_dir/V$N -t -s 100000

gtkwave:
	gtkwave obj_dir/waveform.fst

clean:
	rm -rf obj_dir
