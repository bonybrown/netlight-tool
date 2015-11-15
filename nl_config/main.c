#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define NETLIGHT_TCP_PORT 3601

#define MAX_LABEL_LENGTH 16
#define KEY_LENGTH 16

#define BUFFER_SIZE 1024

#define PROTOCOL_VERSION_MAJOR 1
#define PROTOCOL_VERSION_MINOR 0



#define CONFIG_ITEM_IP_ADDRESS  255
#define CONFIG_ITEM_NETMASK     254
#define CONFIG_ITEM_GATEWAY     253
#define CONFIG_ITEM_DNS_SERVER  252
#define CONFIG_ITEM_LABEL       251
#define CONFIG_ITEM_AES_KEY     250


/*
 * Program to configure a Triangulum Netlight
 */

void print_usage() {
    puts("Usage: netlight_config [options] dns_name_or_address");
    puts("Options:");
    puts("NETWORK ADDRESS");
    puts("\t-d --dhcp (use dhcp for network config)");
    puts(" OR");
    puts("\t-a --address=static ip address");
    puts("\t-m --mask=network address mask");
    puts("\t-g --gateway=network gateway address");
    puts("\t-n --nameserver=network nameserver address");
    puts("mDNS / LLMNR NAME");
    puts("\t-l --label=mDNS name (16 chars max, base name only)");
    puts("ENCRYPTION KEY");
    puts("\t-k --key=AES key (16 hex bytes, eg: 00ffabcdef01233400ffabcdef012334 )");
    puts("\t\t Set key to all zeros to disable encryption (default)");
}


void print_error(const char *msg)
{
  if( errno ){
    perror(msg);
  }
  else{
    fputs( msg, stderr);
  }
}

void error(const char *msg)
{
  print_error(msg);
  exit(EXIT_FAILURE);
}


int tcp_connect_to( const char *address , const uint16_t port){
  int sockfd = -1;
  struct sockaddr_in sin;
  struct hostent *server;
  
  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd < 0){
    print_error("ERROR opening socket");
    return -1;
  }
  server = gethostbyname(address);
  if (server == NULL)
  {
    print_error( hstrerror( h_errno ) );
    return -1;
  }
  memset((char *) &sin,0, sizeof(sin));
  sin.sin_family = AF_INET;
  memcpy(&sin.sin_addr,
   server->h_addr,
   server->h_length);
  sin.sin_port = htons(port);
  if (connect(sockfd,(struct sockaddr *)&sin,sizeof(sin)) < 0){
    error("ERROR connecting");
  }
  return sockfd;
}

bool device_check( const int sockfd, const char *device_id, const int protocol_major, const int protocol_minor ){
  /* On connection, the netlight sends:
   * a nul-terminated string of it's model number
   * a one-byte protocol id
   * 16 random bytes. These can be used as an IV for encrypted comms
   */
  uint8_t buffer[BUFFER_SIZE];
  int n;
  size_t id_length =  strlen(device_id);
  
  n = read(sockfd,buffer,BUFFER_SIZE);
  if( n <= id_length ){
    //not enough data read to succeed
    return false;
  }
  
  if( strncmp(device_id, (char*)buffer, id_length) != 0 ){
    //  Did not find NL5
    return false;
  }
  
  /* protocol check */
  if( buffer[id_length + 1] != protocol_major ){
    //error("Cannot communicate with NL5; wrong protocol id");
    return false;
  }
  /* protocol check - minor */
  if( buffer[id_length + 2] != protocol_minor ){
    //error("Cannot communicate with NL5; wrong protocol id");
    return false;
  }
  return true;
}

int config_send( const int sockfd, const uint8_t ip_address[4], 
                 const uint8_t netmask[4], const uint8_t gateway[4], 
                 const uint8_t nameserver[4], const char *label, 
                 const uint8_t aes_key[KEY_LENGTH] ){
  ssize_t n = 0;
  uint8_t config_function_code;
  uint8_t buffer[BUFFER_SIZE];
  int i;
  
  memset( buffer, 0, BUFFER_SIZE );
  buffer[n++] = 0x17;
  /*version*/
  buffer[n++] = PROTOCOL_VERSION_MAJOR;
  buffer[n++] = PROTOCOL_VERSION_MINOR;
  
  /*size - offsets 3 &4 , small endian*/
  buffer[n++] = 0x00;
  buffer[n++] = 0x00;
  
  config_function_code = CONFIG_ITEM_IP_ADDRESS;
  memcpy( buffer+n, &config_function_code, 1 );
  n++;
  memcpy( buffer+n, ip_address, 4 );
  n+=4;
  
  config_function_code = CONFIG_ITEM_NETMASK;
  memcpy( buffer+n, &config_function_code, 1 );
  n++;
  memcpy( buffer+n, netmask, 4 );
  n+=4;
  
  config_function_code = CONFIG_ITEM_GATEWAY;
  memcpy( buffer+n, &config_function_code, 1 );
  n++;
  memcpy( buffer+n, gateway, 4 );
  n+=4;
  
  config_function_code = CONFIG_ITEM_DNS_SERVER;
  memcpy( buffer+n, &config_function_code, 1 );
  n++;
  memcpy( buffer+n, nameserver, 4 );
  n+=4;
  
  if( NULL != label ){
    config_function_code = CONFIG_ITEM_LABEL;
    memcpy( buffer+n, &config_function_code, 1 );
    n++;
    memcpy( buffer+n, label, strlen( label ) );
    n+=strlen( label ) + 1;
  }
  
  for( i = 0; i < KEY_LENGTH; i++){
    if( aes_key[i] != 0 ){
      break;
    }
  }
  //if exiting loop and i == KEY_LENGTH, then key is all zeroes
  if( i != KEY_LENGTH ){
    config_function_code = CONFIG_ITEM_AES_KEY;
    memcpy( buffer+n, &config_function_code, 1 );
    n++;
    memcpy( buffer+n, aes_key, KEY_LENGTH );
    n+= KEY_LENGTH;
  }
  
  size_t packet_length = n - 5; /* for packet_type, prot_maj/min, size */
  buffer[3] = packet_length & 0xff;
  buffer[4] = packet_length >> 8;
  write( sockfd, buffer, n );
  
  return n;
}


int parse_ip_address( const char *string, uint8_t address[4] ){
  int parsed[4] = {0};
  int matched = sscanf( string, "%d.%d.%d.%d", &parsed[0], &parsed[1], &parsed[2], &parsed[3] );
  int i;
  for(  i = 0; i < 4 ; i++ ){
    if( parsed[i] < 0 || parsed[i] > 255 ){
      return -i - 1;
    }
    address[i] = parsed[i];
  }
  //int matched = sscanf(string, "%hhd.%hhd.%hhd.%hhd", address, address+1, address+2, address+3);
  return matched == 4;
}

int parse_key( const char *string, uint8_t key[KEY_LENGTH] ){
  int i;
  char temp[3];
  for (i = 0; i < KEY_LENGTH; i++){
    strncpy(temp, string + (i*2), 2);
    temp[2] = '\0';
    if( isxdigit(temp[0]) && isxdigit(temp[1]) ){
      key[i] = strtoul( temp, NULL, 16 );
    }
    else{
      return 0;
    }
  }
  return 1;
}

int main(int argc, char *argv[]) {
    int option = 0;
    int dhcp = 0;
    
    int manual_param_count = 0;
    uint8_t address[4]={0};
    uint8_t mask[4]={0};
    uint8_t gateway[4]={0};
    uint8_t nameserver[4]={0};
    
    uint8_t key[KEY_LENGTH] = {0};
    
    char *label = NULL;
    char *netlight_address = NULL;
    
    int sockfd = -1;

    struct option long_options[]={
      {"dhcp", no_argument, &dhcp, 1},
      {"address", required_argument, 0, 'a'},
      {"mask", required_argument, 0, 'm'},
      {"gateway", required_argument, 0, 'g'},
      {"nameserver", required_argument, 0, 'n'},
      {"label", required_argument, 0, 'l'},
      {"key", required_argument, 0, 'k'},
      {0,0,0,0}
    };
    //Specifying the expected options
    int option_index = 0;
    int result = 0;
    while ( (option = getopt_long(argc, argv,"da:m:g:n:l:k:", long_options, &option_index )) != -1) {
      switch (option) {
        case 0:
          //flag set
          break;
        case 'd':
          dhcp = 1;
          break;
        case 'a' :
          result = parse_ip_address( optarg, address );
          if( result != 1 ){
            error("Error parsing address parameter");
          }
          manual_param_count+=1;
          break;
        case 'm' :
          result = parse_ip_address( optarg, mask );
          if( result != 1 ){
            error("Error parsing mask parameter");
          }
          manual_param_count+=1;
          break;
        case 'g' :
          result = parse_ip_address( optarg, gateway );
          if( result != 1 ){
            error("Error parsing gateway parameter");
          }
          manual_param_count+=1;
          break;
        case 'n' :
          result = parse_ip_address( optarg, nameserver );
          if( result != 1 ){
            error("Error parsing nameserver parameter");
          }
          manual_param_count+=1;
          break;
        case 'l' :
          label = optarg;
          if( strlen(label) > MAX_LABEL_LENGTH ){
            error("Label exceeds maximum length");
          }
          break;
        case 'k' :
          if( strlen( optarg ) != (KEY_LENGTH * 2) ){
            error("Key must be 32 hex digits (16 bytes) long");
          }
          result = parse_key( optarg, key );
          if( result != 1 ){
            error("Error parsing encryption key");
          }
          break;
        default: 
          print_usage(); 
          exit(EXIT_FAILURE);
      }
    }
    if( label != NULL ){
      printf("LABEL: %s\n", label );
    }
    printf("DHCP: %d\n", dhcp );
    
    if (optind >= argc || argv[optind] == NULL ) {
      puts("ERROR: Target NetLight network address required");
      print_usage();
      exit(EXIT_FAILURE);
    }
    else
    {
      netlight_address = argv[optind];
    }
    
    if( ( dhcp == 1 && manual_param_count > 0 ) ||
        ( dhcp == 0 && manual_param_count == 0 ) ){
      error("Specify either --dhcp or the manual network configuration parameters, but not both");
    }
    if( dhcp == 0 && manual_param_count != 4 ){
      error("All the manual network parameters must be specified");
    }
    
    
    sockfd = tcp_connect_to( netlight_address, NETLIGHT_TCP_PORT );
    if( sockfd < 0 ){
      exit(-1);
    }
    
    if( ! device_check(sockfd, "NL5", PROTOCOL_VERSION_MAJOR, PROTOCOL_VERSION_MINOR) ){
      close(sockfd);
      error("Did not find NL5 at host address");
    }
    
    config_send( sockfd, address, mask, gateway, nameserver, label, key );
    
    close(sockfd);
    
    
    return 0;
}

