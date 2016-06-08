#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>


#define NETLIGHT_TCP_PORT 3601

#define PROTOCOL_VERSION_MAJOR 1
#define PROTOCOL_VERSION_MINOR 0

#define BUFFER_SIZE 64

/*
 * Program to communicate with a Triangulum Netlight
 */

void print_usage() {
    puts("Usage: netlight --unit N --color RRGGBB [other options] dns_name_or_address");
    puts("\t-u --unit=N (unit number; first unit is 0)");
    puts("\t-c --color=RRGGBB (the hex color code to set on the light unit)");
    puts("Other options:");
    puts("\t-s --sound=none|silence|up|down|short|long|chirp|rise|siren|xmas");
    puts("\t-r --sound-repeat=N (number of times to repeat the sound)");
    puts("\t-T --future-timeout=N (number of seconds until the future. Required to specify any of the following)");
    puts("\t-S --future-sound=none|silence|up|down|short|long|chirp|rise|siren|xmas");
    puts("\t-R --future-sound-repeat=N (as for sound-repeat)");
    puts("\t-C --future-color=RRGGBB");
}

void error(char *msg)
{
  perror(msg);
  exit(EXIT_FAILURE);
}


void netlight_send(char *netlight_address, int unit, uint32_t color, int sound, int sound_repeat, uint32_t timeout, uint32_t future_color, int future_sound, int future_sound_repeat  ){
  int sockfd, n;
  struct sockaddr_in sin;
  struct hostent *server;
  uint8_t buffer[BUFFER_SIZE];
  
  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd < 0){
    error("ERROR opening socket");
  }
  server = gethostbyname(netlight_address);
  if (server == NULL)
  {
    error("No such host");
  }
  memset((char *) &sin,0, sizeof(sin));
  sin.sin_family = AF_INET;
  memcpy(&sin.sin_addr,
	 server->h_addr,
	 server->h_length);
  sin.sin_port = htons(NETLIGHT_TCP_PORT);
  if (connect(sockfd,(struct sockaddr *)&sin,sizeof(sin)) < 0){
    error("ERROR connecting");
  }
  memset(buffer,0,BUFFER_SIZE);
  
  /* On connection, the netlight sends:
   * a nul-terminated string of it's model number
   * a one-byte protocol id
   * 16 random bytes. These can be used as an IV for encrypted comms
   */
  n = read(sockfd,buffer,BUFFER_SIZE);
  if( strncmp("NL5", (char*)buffer, 10) != 0 )
  {
    error("Did not find NL5");
  }
  
  /* protocol check */
  if( buffer[4] != 1 ){
    error("Cannot communicate with NL5; wrong protocol id");
  }
  
  /* reply back */
  memset(buffer,0,BUFFER_SIZE);
  n=0;
  buffer[n++] = 0x17; //frame header
  /*version*/
  buffer[n++] = PROTOCOL_VERSION_MAJOR;
  buffer[n++] = PROTOCOL_VERSION_MINOR;
  
  /*size - offsets 3 &4 , small endian*/
  buffer[n++] = 0x00;
  buffer[n++] = 0x00;
  
  buffer[n++] = 0;//function 0
  buffer[n++] = (uint8_t)unit;
  buffer[n++] = (uint8_t) ( color >> 16 );
  buffer[n++] = (uint8_t) ( color >> 8 );
  buffer[n++] = (uint8_t) ( color );
  buffer[n++] = (uint8_t) (sound | (sound_repeat << 4)); //sound
  buffer[n++] = (uint8_t) ( timeout );
  buffer[n++] = (uint8_t) ( timeout >> 8 );
  buffer[n++] = (uint8_t) ( timeout >> 16 );
  buffer[n++] = (uint8_t) ( timeout >> 24 );
  buffer[n++] = (uint8_t) ( future_color >> 16 );
  buffer[n++] = (uint8_t) ( future_color >> 8 );
  buffer[n++] = (uint8_t) ( future_color );
  buffer[n++] = (uint8_t) (future_sound | (future_sound_repeat << 4 )); //future sound
  
  size_t packet_length = n - 5; /* for packet_type, prot_maj/min, size */
  buffer[3] = packet_length & 0xff;
  buffer[4] = packet_length >> 8;
  
  n = write(sockfd,buffer,n);
  if (n < 0){
    error("ERROR writing to socket");
  }
  close(sockfd);
}

int map_sound( char * sound ){
  if( strcmp("none", sound ) == 0 ){
    return 0;
  }
  if( strcmp("silence", sound ) == 0 ){
    return 1;
  }
  if( strcmp("up", sound ) == 0 ){
    return 2;
  }
  if( strcmp("down", sound ) == 0 ){
    return 3;
  }
  if( strcmp("short", sound ) == 0 ){
    return 4;
  }
  if( strcmp("long", sound ) == 0 ){
    return 5;
  }
  if( strcmp("chirp", sound ) == 0 ){
    return 6;
  }
  if( strcmp("rise", sound ) == 0 ){
    return 7;
  }
  if( strcmp("siren", sound ) == 0 ){
    return 8;
  }
  if( strcmp("xmas", sound ) == 0 ){
    return 9;
  }
  return -1;
}


int main(int argc, char *argv[]) {
    int option = 0;
    uint32_t color = 0, future_color = 0, timeout = 0;
    int unit = -1, color_set = 0, future_color_set = 0, sound = 0, future_sound = 0;
    int sound_repeat = 0, future_sound_repeat = 0;
    char *netlight_address = NULL;

    struct option long_options[]={
	{"future-color", required_argument, 0, 'C'},
	{"future-timeout", required_argument, 0, 'T'},
	{"future-sound", required_argument, 0, 'S'},
	{"future-sound-repeat", required_argument, 0, 'R'},
	{"color", required_argument, 0, 'c'},
	{"unit", required_argument, 0, 'u'},
	{"sound", required_argument, 0, 's'},
	{"sound-repeat", required_argument, 0, 'r'},
	{0,0,0,0}
    };
    //Specifying the expected options
    //The two options l and b expect numbers as argument
    int option_index = 0;
    while ( (option = getopt_long(argc, argv,"C:T:S:R:c:u:s:r:", long_options, &option_index )) != -1) {
        switch (option) {
             case 0:
		 //flag set
		 break;
             case 'C' :
	         future_color = strtol(optarg,NULL,16); 
		 future_color_set = 1;
                 break;
             case 'T' :
		 timeout = atoi(optarg); 
                 break;
             case 'c' : 
	         color = strtol(optarg,NULL,16); 
		 color_set = 1;
                 break;
             case 'u' : 
	         unit = atoi(optarg);
                 break;
	     case 's' :
	         sound = map_sound(optarg);
		 if( sound == -1 ){
		   printf("Unknown value '%s' for parameter --sound\n", optarg);
		   exit(EXIT_FAILURE);
		 }
		 break;
	     case 'S' :
	         future_sound = map_sound(optarg);
		 if( future_sound == -1 ){
		   printf("Unknown value '%s' for parameter --future_sound\n", optarg);
		   exit(EXIT_FAILURE);
		 }
		 break;
	     case 'r' :
	         if( strcmp("continuous",optarg) == 0 ){
		   sound_repeat = -1;
		 }
		 else{
		   sound_repeat = atoi(optarg);
		 }
	         break;
	     case 'R' :
	         if( strcmp("continuous",optarg) == 0 ){
		   future_sound_repeat = -1;
		 }
		 else{
		   future_sound_repeat = atoi(optarg);
		 }
		 break;
             default: print_usage(); 
                 exit(EXIT_FAILURE);
        }
    }
    
    if (optind >= argc || argv[optind] == NULL ) {
      puts("ERROR: Target NetLight network address required");
      print_usage();
      exit(EXIT_FAILURE);
    }
    else
    {
      netlight_address = argv[optind];
    }
    
    if (unit == -1 || color_set == 0) {
        print_usage();
        exit(EXIT_FAILURE);
    }
    
    if ( timeout != 0 && future_color_set == 0 ){
      puts("ERROR: A future-color value is required if a timeout is set");
      exit(EXIT_FAILURE);
    }
    
    if ((future_color_set || future_sound || future_sound_repeat) && timeout == 0 ){
      puts("ERROR: A timeout is required if any future value is set");
      exit(EXIT_FAILURE);
    }

    
    netlight_send( netlight_address, unit, color, sound, sound_repeat, timeout, future_color , future_sound, future_sound_repeat);
        
    
    return 0;
}

