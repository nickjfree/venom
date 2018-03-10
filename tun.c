#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>
#include <pthread.h>



struct sockaddr_in nodes[256];



int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  if( (fd = open(clonedev , O_RDWR)) < 0 ) {
    perror("Opening /dev/net/tun");
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = flags;

  if (*dev) {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }

  if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
    perror("ioctl(TUNSETIFF)");
    close(fd);
    return err;
  }

  strcpy(dev, ifr.ifr_name);

  return fd;
}


int setup_socket(int flag) {

   int s;
   struct sockaddr_in si_me;
   s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if (s == -1) {
      perror("socket()"); 
   }

   if (!flag) {
      si_me.sin_family = AF_INET;
      si_me.sin_port = htons(55555);
      si_me.sin_addr.s_addr = htonl(INADDR_ANY);
      if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me))==-1) {
         perror("bind()");
      }
   }
   return s;
}


int main(int argc, char **argv) {

   char ifname[1024];
   int tun_fd, net_fd;
   int maxfd;
   strcpy(ifname, "tun1");
   tun_fd = tun_alloc(ifname, IFF_TUN|IFF_NO_PI);
   printf("name: %s\n", ifname);

   // set up a dup socket
   net_fd = setup_socket(0);
   maxfd = (net_fd > tun_fd)?net_fd:tun_fd;

   // remote client
   struct sockaddr_in remote;
   int remote_len = sizeof(remote);

   if(argc > 1) {
       remote.sin_family = AF_INET;
       remote.sin_port = htons(55555);
   
       if (inet_aton(argv[1], &remote.sin_addr) == 0){
          perror("inet_aton() failed\n");
       }
   }

   while (1) {
      
      int ret;
      fd_set rd_set;

      FD_ZERO(&rd_set);
      FD_SET(tun_fd, &rd_set);
      FD_SET(net_fd, &rd_set);
      
          
      ret = select(maxfd+1, &rd_set, NULL, NULL, NULL);
      if (ret < 0) {
         perror("select");
         exit(1);
      } 
//      printf("select return %d\n", ret);
      if (FD_ISSET(tun_fd, &rd_set)) {
         int nread = 0, index;
         char buff[2048];
         nread = read(tun_fd, buff, 2048);
         unsigned int dst_ip =  *(unsigned int*)(buff + 16);
         printf("read %d btyes data, dst ip %d\n", nread, dst_ip);
         index = ntohl(dst_ip) & 0xff;
         struct sockaddr_in * addr = &nodes[index];
         if (argc == 2) {
             addr = &remote;
         }
         nread = sendto(net_fd, buff, nread, 0, (struct sockaddr *)addr, sizeof(struct sockaddr_in));
         printf("send %d btye to %s index %d\n", nread, inet_ntoa(addr->sin_addr), index);
      }
      if (FD_ISSET(net_fd, &rd_set)) {
        int nread = 0, index;
        char buff[2048];
        //struct sockaddr_in remote;
        nread = recvfrom(net_fd, buff, 2048, 0, (struct sockaddr *)&remote, &remote_len);
        // make sure it is a valid packet
        if (nread > 1) {
            unsigned int src_ip = *(unsigned int*)(buff + 12);
            index = ntohl(src_ip) & 0xff;
            struct sockaddr_in * addr = &nodes[index];
            memcpy(addr, &remote, sizeof(struct sockaddr_in));
            printf("recv %d bytes from %s, with src ip %d index %d\n", nread, inet_ntoa(addr->sin_addr), src_ip, index);
            nread = write(tun_fd, buff, nread);
            printf("tun %d bytes\n", nread);
        }
      }  
      
   }

   //reader(tun_fd, 0);
   return 0;
}
