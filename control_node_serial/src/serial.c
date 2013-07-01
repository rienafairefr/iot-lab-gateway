// cfmakeraw needs
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include <stdio.h>
#include <string.h>

#include <termios.h>
#include <unistd.h> // tcgetattr
#include <fcntl.h>  // open

#include <errno.h>
#include <sys/param.h> // MIN

#include "common.h"
#include "constants.h"


#include "serial.h"

static void parse_rx_data(unsigned char *rx_buff, unsigned int len,
                void (*handle_pkt)(struct pkt*));
static const unsigned char sync_byte = (unsigned char) SYNC_BYTE;


int configure_tty(char *tty_path)
{
        /*
         * TTY configuration inspired by:
         *   - Contiki/tunslip code
         *   - http://www.unixwiz.net/techtips/termios-vmin-vtime.html
         *   - termios manpage
         *   - http://www.tldp.org/HOWTO/Serial-Programming-HOWTO/x115.html
         */
        int serial_fd;
        struct termios tty;
        memset(&tty, 0, sizeof(tty));

        serial_fd = open(tty_path, O_RDWR | O_NOCTTY | O_SYNC);
        if (serial_fd == -1) {
                fprintf(LOG, "ERROR: Could not open %s\n", tty_path);
                return -1;
        }
        if (tcflush(serial_fd, TCIOFLUSH) == -1) {
                perror("Error in tcflush");
                return -1;
        }
        if (tcgetattr(serial_fd, &tty)) {
                perror("Error in tcgetattr");
                return -1;
        }

        /*
         * Configure TTY
         */
        cfmakeraw(&tty);
        // blocking mode, should read at least 1 char and then can return
        tty.c_cc[VMIN]  = 1;
        tty.c_cc[VTIME] = 0;
        // Disable control characters and signals and all
        tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS (hardware) flow control
        tty.c_cflag &= ~HUPCL;   // No "hanging up" when closing
        tty.c_cflag |=  CLOCAL;  // ignore modem status line
        if (cfsetspeed(&tty, B500000)) {
                perror("Error while setting terminal speed");
                return -1;
        }

        // Apply and discard characters that may have arrived
        if (tcsetattr(serial_fd, TCSAFLUSH, &tty) == -1) {
                perror("Could not set attribute to tty");
                return -1;
        }
        return serial_fd;
}



void start_listening(int fd, void (*handle_pkt)(struct pkt*))
{
        unsigned char rx_buff[2048];
        int n_chars = 0;

        while (1) {
                do {
                        n_chars = read(fd, rx_buff, sizeof(rx_buff));

                        DEBUG_PRINT("n_chars %d\n", n_chars);
                        if (n_chars == -1)
                                fprintf(LOG, "Error %d: %s\n", errno,
                                                strerror(errno));
                } while (n_chars <= 0);

#if DEBUG
                for (int i=0; i < n_chars; i++)
                        DEBUG_PRINT(" %02X", rx_buff[i]);
                DEBUG_PRINT("\n");
#endif

                parse_rx_data(rx_buff, n_chars, handle_pkt);
        }


}


enum state_t {
        STATE_IDLE = 0,
        STATE_WAIT_LEN = 1,
        STATE_GET_PAYLOAD = 2,
        STATE_FULL = 3
};

static void parse_rx_data(unsigned char *rx_buff, unsigned len,
                void (*handle_pkt)(struct pkt*))
{
        // Current packet state
        static struct {
                struct pkt p;
                unsigned int cur_idx;
        } pkt;
        // Current parser state
        static enum state_t state = STATE_IDLE;
        unsigned int cur_idx   = 0;
        // tmp variables
        unsigned char *sync_byte_addr;
        unsigned int n_bytes;

        while (len > cur_idx || state == STATE_FULL) {
                switch (state) {
                        case STATE_IDLE:
                                /*
                                 * Search for 'SYNC_BYTE' byte
                                 */
                                sync_byte_addr = (unsigned char *) memchr(
                                                &rx_buff[cur_idx], sync_byte,
                                                (len - cur_idx));
                                if (sync_byte_addr == NULL)
                                        return; // SYNC not found: drop data

                                cur_idx = (sync_byte_addr - rx_buff);
                                cur_idx++; // consume 'SYNC' byte
                                state   = STATE_WAIT_LEN;
                                break;
                        case STATE_WAIT_LEN:
                                /*
                                 * Read 'length' byte and initialize packet
                                 */
                                pkt.p.len   = rx_buff[cur_idx];
                                pkt.cur_idx = 0;

                                cur_idx++; // process 'len' byte
                                state = STATE_GET_PAYLOAD;
                                break;
                        case STATE_GET_PAYLOAD:
                                /*
                                 * Get missing bytes in payload,
                                 * or as much as available
                                 */
                                n_bytes = MIN((pkt.p.len - pkt.cur_idx),
                                                (len - cur_idx));
                                // copy data in 'pkt'
                                memcpy(&pkt.p.data[pkt.cur_idx],
                                                &rx_buff[cur_idx], n_bytes);
                                pkt.cur_idx += n_bytes;
                                cur_idx     += n_bytes;

                                // Packet full
                                if (pkt.p.len == pkt.cur_idx)
                                        state = STATE_FULL;
                                break;
                        case STATE_FULL:
                                /*
                                 * Handle packet and get back to idle
                                 */
                                DEBUG_PRINT("Got pkt: len = %d\n", pkt.p.len);
                                handle_pkt(&pkt.p);
                                state = STATE_IDLE;
                                break;
                        default:
                                break;
                }
        }
}
