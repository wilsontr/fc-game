NAME=fcgame.nes

$(NAME): nes.cfg crt0.o main.o runtime.lib
	ld65 -C nes.cfg -o $(NAME) crt0.o main.o runtime.lib

main.s: main.c
	cc65 -Oirs --add-source $<
# Consider adding -Cl and -Oirs

%.o: %.s
	ca65 $<

clean:
	rm *.o main.s
