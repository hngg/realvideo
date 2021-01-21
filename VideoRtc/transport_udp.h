#ifndef __PJMEDIA_TRANSPORT_UDP_H__
#define __PJMEDIA_TRANSPORT_UDP_H__

#include <pthread.h>
#include "types.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "packets_list.h"
#include "os.h"


#ifndef PJMEDIA_MAX_MRU
#define PJMEDIA_MAX_MRU                  2000
#endif

#define RTP_LEN            PJMEDIA_MAX_MRU
/* Maximum size of incoming RTCP packet */
#define RTCP_LEN            600

#define RESEND_BUFF_NUMBER  1024

#ifndef PJ_SOCKADDR_IN_SIN_ZERO_LEN
#   define PJ_SOCKADDR_IN_SIN_ZERO_LEN	8
#endif

#define PJ_INVALID_SOCKET   (-1)

typedef struct pj_in_addr
{
    pj_uint32_t	s_addr;		/**< The 32bit IP address.	    */
} pj_in_addr;

typedef union pj_in6_addr
{
    /* This is the main entry */
    //pj_uint8_t  s6_addr[16];   /**< 8-bit array */

    /* While these are used for proper alignment */
    pj_uint32_t	u6_addr32[4];

    /* Do not use this with Winsock2, as this will align pj_sockaddr_in6
     * to 64-bit boundary and Winsock2 doesn't like it!
     * Update 26/04/2010:
     *  This is now disabled, see http://trac.pjsip.org/repos/ticket/1058
     */
#if 0 && defined(PJ_HAS_INT64) && PJ_HAS_INT64!=0 && \
    (!defined(PJ_WIN32) || PJ_WIN32==0)
    pj_int64_t	u6_addr64[2];
#endif

} pj_in6_addr;

struct pj_sockaddr_in
{
#if defined(PJ_SOCKADDR_HAS_LEN) && PJ_SOCKADDR_HAS_LEN!=0
    pj_uint8_t  sin_zero_len;	/**< Just ignore this.		    */
    pj_uint8_t  sin_family;	/**< Address family.		    */
#else
    pj_uint16_t	sin_family;	/**< Address family.		    */
#endif
    pj_uint16_t	sin_port;	/**< Transport layer port number.   */
    pj_in_addr	sin_addr;	/**< IP address.		    */
    char	sin_zero[PJ_SOCKADDR_IN_SIN_ZERO_LEN]; /**< Padding.*/
};

typedef struct pj_sockaddr_in6
{
#if defined(PJ_SOCKADDR_HAS_LEN) && PJ_SOCKADDR_HAS_LEN!=0
    pj_uint8_t  sin6_zero_len;	    /**< Just ignore this.	   */
    pj_uint8_t  sin6_family;	    /**< Address family.	   */
#else
    pj_uint16_t	sin6_family;	    /**< Address family		    */
#endif
    pj_uint16_t	sin6_port;	    /**< Transport layer port number. */
    pj_uint32_t	sin6_flowinfo;	    /**< IPv6 flow information	    */
    pj_in6_addr sin6_addr;	    /**< IPv6 address.		    */
    pj_uint32_t sin6_scope_id;	    /**< Set of interfaces for a scope	*/
} pj_sockaddr_in6;

typedef struct pj_addr_hdr
{
#if defined(PJ_SOCKADDR_HAS_LEN) && PJ_SOCKADDR_HAS_LEN!=0
    pj_uint8_t  sa_zero_len;
    pj_uint8_t  sa_family;
#else
    pj_uint16_t	sa_family;	/**< Common data: address family.   */
#endif
} pj_addr_hdr;


/**
 * This union describes a generic socket address.
 */
typedef union pj_sockaddr
{
    pj_addr_hdr	    addr;	/**< Generic transport address.	    */
    pj_sockaddr_in  ipv4;	/**< IPv4 transport address.	    */
    pj_sockaddr_in6 ipv6;	/**< IPv6 transport address.	    */
} pj_sockaddr;


typedef struct pjmedia_vid_buf
{
		void		   *buf; 	        /**< resend output buffer.			*/
		pj_uint16_t	   *pkt_len;        /*resend output pkt len except the head*/
		pj_uint16_t	   *resend_times;   /*resend output pkt len except the head*/
		pj_uint16_t	   buf_size;        /*buffer size*/
} pjmedia_vid_buf;

typedef struct trans_channel
{
    void *user_udp;
    pj_bool_t is_rtcp;
    int sockfd;             //socket id
    struct sockaddr_in rem_addr; //remote socket addr to sendto
    struct sockaddr_in src_addr; //source socket addr of recvfrom
    pj_thread_t recv_tid;       //recv thread id
    char recv_buf[RTP_LEN];     //recv buffer
    on_recv_data recv_cb; //recv data callback fuction
    struct rtp_sendto_thread_list_header send_list; //save packet to list when sendto buffer is full
    
} trans_channel;

struct transport_udp
{
    //pjmedia_transport	base;		/**< Base transport.		    */

    pj_pool_t	    *pool;		// Memory pool
    unsigned		options;	//Transport options.
    unsigned		media_options;	// Transport media options.

    void	        *user_stream;	//struct pjmedia_vid_stream point address
    pj_bool_t		attached;

    
    //socklen_t       addr_len;    /**< Length of addresses.        */
    pjmedia_vid_buf *resend;
    
    //local
    struct sockaddr_in        local_rtp_addr;    /**< Published RTP address.        */
    struct sockaddr_in        local_rtcp_addr;    /**< Published RTCP address.        */
    
    //memory_list *mem_list;
    pthread_mutex_t  rtp_cache_mutex;
    pthread_mutex_t  udp_socket_mutex;  /* add by j33783 20190509 */
    
    
    struct trans_channel rtp_chanel;    //rtp send and recv channel
    struct trans_channel rtcp_chanel;   //rtcp send and recv channel
    
};

typedef struct transport_udp transport_udp;

//create and destroy
pj_status_t transport_udp_create(struct transport_udp** udpout, const char *addr, unsigned short port,
                                    void (*rtp_cb)(void*, void*, pj_ssize_t),
				                    void (*rtcp_cb)(void*, void*, pj_ssize_t));
pj_status_t transport_udp_destroy(struct transport_udp* udp);

pj_status_t transport_udp_start(struct transport_udp* udp, const char*remoteAddr, unsigned short remoteRtpPort);
pj_status_t transport_udp_stop(struct transport_udp* udp);
// pj_status_t transport_udp_start_send( struct transport_udp* tp, const char*remoteAddr, unsigned short remoteRtpPort);
// pj_status_t transport_udp_stop_send( struct transport_udp* tp);
// pj_status_t transport_udp_start_recv( struct transport_udp* tp, const char*remoteAddr, unsigned short remoteRtpPort);
// pj_status_t transport_udp_stop_recv( struct transport_udp* tp);


//send
pj_status_t transport_priority_send_rtp( transport_udp* udp, const void *rtpPacket, pj_uint32_t size);
pj_status_t transport_send_rtcp(struct transport_udp* udp, const void *rtpPacket, pj_uint32_t size);

pj_status_t resend_losted_package( struct transport_udp* udp, unsigned begin_seq, unsigned count);


//pj_status_t transport_save_packet(struct transport_udp* udp, const void *rtpPacket, pj_uint32_t size);
//pj_status_t transport_send_rtp_seq(struct transport_udp* udp, const void *rtpPacket, pj_size_t size, unsigned short extSeq);


//pj_status_t transport_reset_socket(struct transport_udp* udp);
//pj_status_t transport_reset_rtp_socket(struct transport_udp*  udp);

#endif

