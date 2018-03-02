#ifndef __TUNTAP__
#define __TUNTAP__


//#include <stdint.h>
#include "winsock2.h"
#include "Mswsock.h"
#include "Ws2tcpip.h"
#include <Windows.h>
//#include <In6addr.h>


# if defined IFNAMSIZ && !defined IF_NAMESIZE

#  define IF_NAMESIZE IFNAMSIZ /* Historical BSD name */

# elif !defined IF_NAMESIZE

#  define IF_NAMESIZE 16

# endif



# define IF_DESCRSIZE 50 /* XXX: Tests needed on NetBSD and OpenBSD */



# if defined TUNSETDEBUG

#  define TUNSDEBUG TUNSETDEBUG

# endif



# if defined Windows

#  define TUNFD_INVALID_VALUE INVALID_HANDLE_VALUE

# else /* Unix */

#  define TUNFD_INVALID_VALUE -1

# endif



/*

* Uniformize types

* - t_tun: tun device file descriptor

* - t_tun_in_addr: struct in_addr/IN_ADDR

* - t_tun_in6_addr: struct in6_addr/IN6_ADDR

*/

# if defined Windows

typedef HANDLE t_tun;

typedef IN_ADDR t_tun_in_addr;

typedef IN6_ADDR t_tun_in6_addr;

# else /* Unix */

typedef int t_tun;

typedef struct in_addr t_tun_in_addr;

typedef struct in6_addr t_tun_in6_addr;

# endif



# define TUNTAP_ID_MAX 256

# define TUNTAP_ID_ANY 257



# define TUNTAP_MODE_ETHERNET 0x0001

# define TUNTAP_MODE_TUNNEL   0x0002

# define TUNTAP_MODE_PERSIST  0x0004



# define TUNTAP_LOG_NONE      0x0000

# define TUNTAP_LOG_DEBUG     0x0001

# define TUNTAP_LOG_INFO      0x0002

# define TUNTAP_LOG_NOTICE    0x0004

# define TUNTAP_LOG_WARN      0x0008

# define TUNTAP_LOG_ERR       0x0016



/* Versioning: 0xMMmm, with 'M' for major and 'm' for minor */

# define TUNTAP_VERSION_MAJOR 0

# define TUNTAP_VERSION_MINOR 3

# define TUNTAP_VERSION ((TUNTAP_VERSION_MAJOR<<8)|TUNTAP_VERSION_MINOR)



# define TUNTAP_GET_FD(x) (x)->tun_fd

typedef struct device {

	HANDLE			tun_fd;

	int				ctrl_sock;

	int				flags;     /* ifr.ifr_flags on Unix */

	unsigned char	hwaddr[6];

	char			if_name[IF_NAMESIZE];

}device;


typedef struct PipeOperation {

	OVERLAPPED overlapped;
	enum OpType {
		RECV_NET=1,
		SEND_NET=2,
		RECV_TUN=3,
		SEND_TUN=4
	} Type;
	char buffer[2048];
	DWORD transferred;
	char * end;
} PipeOperation;


	/* Portable "public" functions */

	  void		 tuntap_destroy(struct device *);

	  int		 tuntap_start(struct device *, int, int);

	  int		 tuntap_read(struct device *, void * buff, size_t size);

	  int		 tuntap_write(struct device *, void * buff, size_t size);

#endif