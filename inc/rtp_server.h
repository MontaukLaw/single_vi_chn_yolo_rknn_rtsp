#ifndef RTP_SERVER_H
#define RTP_SERVER_H

typedef unsigned char uint8;

int feed_rtp(uint8 *buff, int size);

void init_rtp();

#endif
