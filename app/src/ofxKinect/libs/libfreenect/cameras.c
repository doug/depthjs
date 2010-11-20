/*  libfreenect - an open source Kinect driver

Copyright (C) 2010  Hector Martin "marcan" <hector@marcansoft.com>

This code is licensed to you under the terms of the GNU GPL, version 2 or version 3;
see:
 http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 http://www.gnu.org/licenses/gpl-3.0.txt
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb.h>
#include "libfreenect.h"
#include "cameras.h"
#include <unistd.h>

#if defined(__APPLE__)
/* OS X likes things on powers of 2 boundaries, and needs WAY larger
   packets/xfers to deal with what it's getting. Feel free to play
   with these numbers, but keep them in multiples of 8, otherwise you
   may get weirdness only seen with USB Prober on level 4 or
   higher. */
#define DEPTH_LEN 2048
#define RGB_LEN 2048
#define PKTS_PER_XFER 512
#define NUM_XFERS 64
#else
#define DEPTH_LEN 1760
#define RGB_LEN 1920
#define PKTS_PER_XFER 16
#define NUM_XFERS 32
#endif

static struct libusb_transfer *depth_xfers[NUM_XFERS];
static struct libusb_transfer *rgb_xfers[NUM_XFERS];

static void *rgb_bufs[NUM_XFERS];
static void *depth_bufs[NUM_XFERS];

static depthcb depth_cb;
static rgbcb rgb_cb;

static libusb_device_handle *dev;

struct frame_hdr {
	uint8_t magic[2];
	uint8_t pad;
	uint8_t flag;
	uint8_t unk1;
	uint8_t seq;
	uint8_t unk2;
	uint8_t unk3;
	uint32_t timestamp;
};

uint8_t depth_buf[2*422400];
uint16_t depth_frame[640*480];
int depth_pos = 0;

uint8_t rgb_buf[2*307200];
#if defined(__APPLE__)
uint8_t rgb_frame[640*480*4];
#else
uint8_t rgb_frame[640*480*3];
#endif

int rgb_pos = 0;

extern const struct caminit inits[];
extern const int num_inits;
/********************************* COPYRIGHT NOTICE *******************************\
  Original code for Bayer->BGR/RGB conversion is provided by Dirk Schaefer
  from MD-Mathematische Dienste GmbH. Below is the copyright notice:

    IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
    By downloading, copying, installing or using the software you agree
    to this license. If you do not agree to this license, do not download,
    install, copy or use the software.

    Contributors License Agreement:

      Copyright (c) 2002,
      MD-Mathematische Dienste GmbH
      Im Defdahl 5-10
      44141 Dortmund
      Germany
      www.md-it.de

    Redistribution and use in source and binary forms,
    with or without modification, are permitted provided
    that the following conditions are met:

    Redistributions of source code must retain
    the above copyright notice, this list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    The name of Contributor may not be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
    THE POSSIBILITY OF SUCH DAMAGE.
\**********************************************************************************/

void Bayer2BGR( const uint8_t *bayer0, int bayer_step,
                       uint8_t *dst0, int dst_step,
                       unsigned int width, unsigned int height)
{
    int blue = 1;
    int start_with_green = 1;

    memset( dst0, 0, width*3*sizeof(dst0[0]) );
    memset( dst0 + (height - 1)*dst_step, 0, width*3*sizeof(dst0[0]) );
    dst0 += dst_step + 3 + 1;
    height -= 2;
    width -= 2;

    for( ; height-- > 0; bayer0 += bayer_step, dst0 += dst_step )
    {
        int t0, t1;
        const uint8_t* bayer = bayer0;
        uint8_t* dst = dst0;
        const uint8_t* bayer_end = bayer + width;

        dst[-4] = dst[-3] = dst[-2] = dst[width*3-1] =
            dst[width*3] = dst[width*3+1] = 0;

        if( width <= 0 )
            continue;

        if( start_with_green )
        {
            t0 = (bayer[1] + bayer[bayer_step*2+1] + 1) >> 1;
            t1 = (bayer[bayer_step] + bayer[bayer_step+2] + 1) >> 1;
            dst[-blue] = (uint8_t)t0;
            dst[0] = bayer[bayer_step+1];
            dst[blue] = (uint8_t)t1;
            bayer++;
            dst += 3;
        }

        if( blue > 0 )
        {
            for( ; bayer <= bayer_end - 2; bayer += 2, dst += 6 )
            {
                t0 = (bayer[0] + bayer[2] + bayer[bayer_step*2] +
                      bayer[bayer_step*2+2] + 2) >> 2;
                t1 = (bayer[1] + bayer[bayer_step] +
                      bayer[bayer_step+2] + bayer[bayer_step*2+1]+2) >> 2;
                dst[-1] = (uint8_t)t0;
                dst[0] = (uint8_t)t1;
                dst[1] = bayer[bayer_step+1];

                t0 = (bayer[2] + bayer[bayer_step*2+2] + 1) >> 1;
                t1 = (bayer[bayer_step+1] + bayer[bayer_step+3] + 1) >> 1;
                dst[2] = (uint8_t)t0;
                dst[3] = bayer[bayer_step+2];
                dst[4] = (uint8_t)t1;
            }
        }
        else
        {
            for( ; bayer <= bayer_end - 2; bayer += 2, dst += 6 )
            {
                t0 = (bayer[0] + bayer[2] + bayer[bayer_step*2] +
                      bayer[bayer_step*2+2] + 2) >> 2;
                t1 = (bayer[1] + bayer[bayer_step] +
                      bayer[bayer_step+2] + bayer[bayer_step*2+1]+2) >> 2;
                dst[1] = (uint8_t)t0;
                dst[0] = (uint8_t)t1;
                dst[-1] = bayer[bayer_step+1];

                t0 = (bayer[2] + bayer[bayer_step*2+2] + 1) >> 1;
                t1 = (bayer[bayer_step+1] + bayer[bayer_step+3] + 1) >> 1;
                dst[4] = (uint8_t)t0;
                dst[3] = bayer[bayer_step+2];
                dst[2] = (uint8_t)t1;
            }
        }

        if( bayer < bayer_end )
        {
            t0 = (bayer[0] + bayer[2] + bayer[bayer_step*2] +
                  bayer[bayer_step*2+2] + 2) >> 2;
            t1 = (bayer[1] + bayer[bayer_step] +
                  bayer[bayer_step+2] + bayer[bayer_step*2+1]+2) >> 2;
            dst[-blue] = (uint8_t)t0;
            dst[0] = (uint8_t)t1;
            dst[blue] = bayer[bayer_step+1];
            bayer++;
            dst += 3;
        }

        blue = -blue;
        start_with_green = !start_with_green;
    }
}


static void rgb_process(uint8_t *buf, size_t len)
{
	int x,y,i;
	if (len == 0)
		return;

	struct frame_hdr *hdr = (void*)buf;
	uint8_t *data = buf + sizeof(*hdr);
	int datalen = len - sizeof(*hdr);

	//printf("%02x %x\n", hdr->flag, depth_pos);
	switch (hdr->flag) {
		case 0x81:
			rgb_pos = 0;
		case 0x82:
		case 0x85:
			memcpy(&rgb_buf[rgb_pos], data, datalen);
			rgb_pos += datalen;
			break;
	}

	if (hdr->flag != 0x85)
		return;

//#if defined(__APPLE__)
	/* For some reason, we're getting two end (0x75/0x85) packets on OS
	   X. This is causing the frame rendering function to fire twice,
	   which makes things render glitchy. This needs to be debugged,
	   but for the moment this patch fixes it.
	*/
	if (rgb_pos != 307200)
	{
		printf("DROPPED RGB FRAME %d bytes\n", depth_pos);
		return;
	}
//#endif

	printf("GOT RGB FRAME, %d bytes\n", rgb_pos);
	Bayer2BGR( rgb_buf, 640, rgb_frame, 640*3, 640, 480);
	rgb_cb(rgb_frame, 640, 480);
}

static void depth_process(uint8_t *buf, size_t len)
{
	int i;
	if (len == 0)
		return;

	struct frame_hdr *hdr = (void*)buf;
	uint8_t *data = buf + sizeof(*hdr);
	int datalen = len - sizeof(*hdr);

	//printf("%02x %x\n", hdr->flag, depth_pos);
	switch (hdr->flag) {
		case 0x71:
			depth_pos = 0;
		case 0x72:
		case 0x75:
			memcpy(&depth_buf[depth_pos], data, datalen);
			depth_pos += datalen;
			break;
	}

	if (hdr->flag != 0x75)
		return;

	if (depth_pos != 422400){
		printf("DROPPED DEPTH FRAME %d bytes\n", depth_pos);
		return;
	}
	int bitshift = 0;
	for (i=0; i<(640*480); i++) {
		int idx = (i*11)/8;
		uint32_t word = (depth_buf[idx]<<16) | (depth_buf[idx+1]<<8) | depth_buf[idx+2];
		depth_frame[i] = ((word >> (13-bitshift)) & 0x7ff);
		bitshift = (bitshift + 11) % 8;
	}

	depth_cb(depth_frame, 640, 480);
}


static void depth_callback(struct libusb_transfer *xfer)
{
	int i;
	if(xfer->status == LIBUSB_TRANSFER_COMPLETED) {
		uint8_t *buf = (void*)xfer->buffer;
		for (i=0; i<PKTS_PER_XFER; i++) {
			//printf("DCB %p %d\n", buf, xfer->iso_packet_desc[i].actual_length);
			depth_process(buf, xfer->iso_packet_desc[i].actual_length);
			buf += DEPTH_LEN;
		}
		libusb_submit_transfer(xfer);
	} else {
		printf("Xfer error: %d\n", xfer->status);
	}
}

static void rgb_callback(struct libusb_transfer *xfer)
{
	int i;
	if(xfer->status == LIBUSB_TRANSFER_COMPLETED) {
		uint8_t *buf = (void*)xfer->buffer;
		for (i=0; i<PKTS_PER_XFER; i++) {
			rgb_process(buf, xfer->iso_packet_desc[i].actual_length);
			buf += RGB_LEN;
		}
		libusb_submit_transfer(xfer);
	} else {
		printf("Xfer error: %d\n", xfer->status);
	}
}

struct cam_hdr {
	uint8_t magic[2];
	uint16_t len;
	uint16_t cmd;
	uint16_t tag;
};

void send_init(void)
{
	int i, j, ret;
	uint8_t obuf[0x2000];
	uint8_t ibuf[0x2000];
	struct cam_hdr *chdr = (void*)obuf;
	struct cam_hdr *rhdr = (void*)ibuf;

	ret = libusb_control_transfer(dev, 0x80, 0x06, 0x3ee, 0, ibuf, 0x12, 0);
	printf("First xfer: %d\n", ret);

	chdr->magic[0] = 0x47;
	chdr->magic[1] = 0x4d;
	
	for (i=0; i<num_inits; i++) {
		const struct caminit *ip = &inits[i];
		chdr->cmd = ip->command;
		chdr->tag = ip->tag;
		chdr->len = ip->cmdlen / 2;
		memcpy(obuf+sizeof(*chdr), ip->cmddata, ip->cmdlen);
		ret = libusb_control_transfer(dev, 0x40, 0, 0, 0, obuf, ip->cmdlen + sizeof(*chdr), 0);
		printf("CTL CMD %04x %04x = %d\n", chdr->cmd, chdr->tag, ret);
		do {
			ret = libusb_control_transfer(dev, 0xc0, 0, 0, 0, ibuf, 0x200, 0);
		} while (ret == 0);
		printf("CTL RES = %d\n", ret);
		if (rhdr->magic[0] != 0x52 || rhdr->magic[1] != 0x42) {
			printf("Bad magic %02x %02x\n", rhdr->magic[0], rhdr->magic[1]);
			continue;
		}
		if (rhdr->cmd != chdr->cmd) {
			printf("Bad cmd %02x != %02x\n", rhdr->cmd, chdr->cmd);
			continue;
		}
		if (rhdr->tag != chdr->tag) {
			printf("Bad tag %04x != %04x\n", rhdr->tag, chdr->tag);
			continue;
		}
		if (rhdr->len != (ret-sizeof(*rhdr))/2) {
			printf("Bad len %04x != %04x\n", rhdr->len, (int)(ret-sizeof(*rhdr))/2);
			continue;
		}
		if (rhdr->len != (ip->replylen/2) || memcmp(ibuf+sizeof(*rhdr), ip->replydata, ip->replylen)) {
			printf("Expected: ");
			for (j=0; j<ip->replylen; j++) {
				printf("%02x ", ip->replydata[j]);
			}
			printf("\nGot:      ");
			for (j=0; j<(rhdr->len*2); j++) {
				printf("%02x ", ibuf[j+sizeof(*rhdr)]);
			}
			printf("\n");
		}
	}
}

void cams_init(libusb_device_handle *d, depthcb dcb, rgbcb rcb)
{
	int i, ret;
	dev = d;

	depth_cb = dcb;
	rgb_cb = rcb;
	
	for (i=0; i<NUM_XFERS; i++) {
		printf("Creating RGB and depth transfers #%d\n", i);
		rgb_bufs[i] = malloc(RGB_LEN*PKTS_PER_XFER);
		depth_bufs[i] = malloc(DEPTH_LEN*PKTS_PER_XFER);

		rgb_xfers[i] = libusb_alloc_transfer(PKTS_PER_XFER);
		depth_xfers[i] = libusb_alloc_transfer(PKTS_PER_XFER);

		libusb_fill_iso_transfer(rgb_xfers[i], dev, 0x81, rgb_bufs[i], PKTS_PER_XFER*RGB_LEN, PKTS_PER_XFER, rgb_callback, NULL, 0);
		libusb_fill_iso_transfer(depth_xfers[i], dev, 0x82, depth_bufs[i], PKTS_PER_XFER*DEPTH_LEN, PKTS_PER_XFER, depth_callback, NULL, 0);

		libusb_set_iso_packet_lengths(rgb_xfers[i], RGB_LEN);
		libusb_set_iso_packet_lengths(depth_xfers[i], DEPTH_LEN);

		ret = libusb_submit_transfer(rgb_xfers[i]);
		if (ret)
			printf("Failed to submit RGB xfer %d: %d\n", i, ret);
		ret = libusb_submit_transfer(depth_xfers[i]);
		if (ret)
			printf("Failed to submit Depth xfer %d: %d\n", i, ret);
	}

	send_init();

}

