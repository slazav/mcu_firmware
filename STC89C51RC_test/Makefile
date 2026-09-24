all: main.ihx

XRAM_SIZE = 1024
CODE_SIZE = 8192

%.inc: %.txt font_cnv
	./font_cnv $< > $@

main.ihx: main.c command.c timer.c uart.c i2c.c display.c font_ascii7.inc font_koi8_r.inc
	sdcc -mmcs51 --xram-size ${XRAM_SIZE} --code-size ${CODE_SIZE} --model-large $<
#

upload:
	~/py_env/bin/stcgal -b 4800 main.ihx

clean:
	rm -f *.asm *.lk *.lst *.map *.mem *.rel *.rst *.sym *.inc *.ihx

clean_dist: clean
	rm -f *.ihx
