#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <time.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>



//void memdump(void* s, size_t n) {
//  size_t i;
//  for(i = 0; i < n; ++i) {
//    printf("%3x ", *((unsigned char*) (s + i)));
//  }
//  printf("\n");
//}




int usage() {
  return 0;
}


void encode_l2addr(char *l2addr, uint8_t bytes[6]) {
  int values[6];
  int i;
  sscanf(l2addr, "%x:%x:%x:%x:%x:%x%*c",
    &values[0], &values[1], &values[2],
    &values[3], &values[4], &values[5]
  );

  for(i = 0;i < 6;i++) {
    //printf("%d ", values[i]);
    bytes[i] = (uint8_t)values[i];
  }
}


int init_interface(char *interface_name) {
  int soc;
  struct ifreq ifreq;
  struct sockaddr_ll sa;

  printf("interface: %s\n", interface_name);

  if((soc = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
    fprintf(stderr, "socket open error. errno %d\n", errno);
    return -1;
  }

  memset(&ifreq, 0, sizeof(struct ifreq));
  strncpy(ifreq.ifr_name, interface_name, sizeof(ifreq.ifr_name) - 1);
  //strncpy(ifreq.ifr_name, device, sizeof(ifreq.ifr_name) - 1);
  if(ioctl(soc, SIOCGIFINDEX, &ifreq) < 0) {
    fprintf(stderr, "ioctl error\n");
    close(soc);
    return -1;
  }

  sa.sll_family = PF_PACKET;
  sa.sll_protocol = htons(ETH_P_ALL);
  sa.sll_ifindex = ifreq.ifr_ifindex;

  if(bind(soc, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
    fprintf(stderr, "socket bind error\n");
    close(soc);
    return -1;
  }

  if(ioctl(soc, SIOCGIFFLAGS, &ifreq) < 0) {
    fprintf(stderr, "ioctl error\n");
    close(soc);
    return -1;
  }

  ifreq.ifr_flags = ifreq.ifr_flags|IFF_PROMISC;
  if(ioctl(soc, SIOCSIFFLAGS, &ifreq) < 0) {
    fprintf(stderr, "promisecas error\n");
    close(soc);
    return -1;
  }

  return soc;
}


int main(int argc, char *argv[]) {
  int soc;
  struct timespec ts;
  char *interface_name;
  uint8_t buf[9100];
  unsigned int send_size;

  int opt;
  int longindex;

  struct option longopts[] = {
    { "interface", required_argument, 0, 1 },
    { 0, 0, 0, 0 },
  };

  while((opt = getopt_long(argc, argv, "a:h", longopts, &longindex)) != -1) {
    switch(opt) {
      case 1:
        interface_name = optarg;
        //max_count = atoi(optarg);
        break;
    }
  }


  soc = init_interface(interface_name);

  send_size = 100;

  struct ether_header *eh;
  struct iphdr *v4;
  struct udphdr *udp;
  eh = (struct ether_header *)buf;
  v4 = (struct iphdr *)(buf + sizeof(struct ether_header));
  udp = (struct udphdr *)(buf + sizeof(struct ether_header) + sizeof(struct iphdr));

  eh->ether_type = htons(ETHERTYPE_IP); // 0x0800
  encode_l2addr("ff:ff:ff:ff:ff:ff", eh->ether_dhost);
  encode_l2addr("00:00:00:00:00:00", eh->ether_shost);

  v4->version = 4;
  v4->ihl = 20 / 4; // 5
  v4->ttl = 77;
  v4->protocol = 17; //UDP
  v4->tot_len = htons(send_size - 14);

  udp->source = htons(33333);
  udp->dest = htons(44444);
  udp->len = htons(send_size - 34); // ehter + ip hdr


  while(1) {
    clock_gettime(CLOCK_REALTIME, &ts);
    if(write(soc, buf, send_size) <= 0) {
      fprintf(stderr, "pkt send error\n");
      break;
    }
    usleep(10000);
    //break;
  }

  //return soc;
}



