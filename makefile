NAME=fcgame.nes

$(NAME): nes.cfg map1 crt0.o main.o runtime.lib 
	ld65 -C nes.cfg -o $(NAME) crt0.o main.o runtime.lib nes.lib

main.s: main.c 
	cc65 -t nes -Oirs --add-source $<
# Consider adding -Cl 

%.o: %.s
	ca65 $<

map1: 
	node csv2header.js newmap

clean:
	rm *.o main.s
