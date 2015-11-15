CC=gcc

CFLAGS=-Wall 
MAIN_SOURCE=  main.c

MAIN_OBJS=	  $(MAIN_SOURCE:.c=.o)

CODE_SOURCE= 

CODE_OBJS=    $(CODE_SOURCE:.c=.o)

all:	$(CODE_OBJS) $(MAIN_OBJS)
	$(LINK.c) -o netlight_config $(MAIN_OBJS) $(CODE_OBJS) 

        

        
clean:
	$(RM) netlight_config $(CODE_OBJS) $(MAIN_OBJS)
                

