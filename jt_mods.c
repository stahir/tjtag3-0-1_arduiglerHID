/*Modified by S. Tahir Ali for use with Arduino HID (tahir00ali@gmail.com)
 *Done by J. Trimble.
 *Modified by David Teran Garrido (davidtgbe@hotmail.com)
 *Little reminder: This mod has been programmed for personal use
 *to debrick my WRT54G and is not fully tested so I'm not responsible of possible bugs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <windows.h>
#include <conio.h>
#include "jt_mods.h"

#include "hid.h"

// Set to non-zero to enable debug.
int jtMod_debug = 0;


/***********************
 * Copied from tjtag.h:
 ***********************/
// --- Xilinx Type Cable ---

#define TDI     0
#define TCK     1
#define TMS     2
#define TDO     4

// ---Wiggler Type Cable ---

#define WTDI      3
#define WTCK      2
#define WTMS      1
#define WTDO      7
#define WTRST_N   4
#define WSRST_N   0
/***********************/

#define sleep(n) Sleep(1000 * n)

const unsigned char CMD_RESET  = 0x74; //ASCII for t
const unsigned char CMD_SEND   = 0x73; //ASCII for s
const unsigned char CMD_READ   = 0x72; //ASCII for r

const unsigned char STATUS_OK   = 0x6B; //ASCII for ok
const unsigned char STATUS_ERR = 0x65; //ASCII for e1



#define ARDUOP_BITS (0x60) // 0110 0000
#define ARDUOP_LSHIFT (5)

#define ASSERT(expr) ASSERT_WFL(expr, __FILE__, __LINE__)

#define ASSERT_WFL(expr, file, line)                \
if ( !(expr) )                                      \
{                                                   \
    fprintf(stderr,                                 \
            "jtMod assert failed (%s:%d): '%s'\n",  \
            (file), (line),                         \
            (#expr) );                              \
                                                    \
    exit(42);                                       \
}

int jtMod_init(void)
{

	int i, r, num;
	unsigned int buf[16];

	// C-based example is 16C0:0480:FFAB:0200
	r = rawhid_open(1, 0x16C0, 0x0480, 0xFFAB, 0x0200);
	if (r <= 0) {
		// Arduino-based example is 16C0:0486:FFAB:0200
		r = rawhid_open(1, 0x16C0, 0x0486, 0xFFAB, 0x0200);
		}
	
	 
   if (r<=0)
    {
        printf("JTMOD: Failed to open Arduino HID.\n");
        exit(42);
    }

    printf("JTMOD: Opened Arduino HID.\n");

    printf("JTMOD: Resetting Arduino HID.\n");

    // Send reset signal to Arduino,
    {
        buf[0] = CMD_RESET;
			for (i=1; i<16; i++) {
				buf[i] = 0;
			}
			rawhid_send(0, buf, 16, 100);
        
    }

    {
        int tries = 0;
		int    	bytesRead;
        while (1)
        {
            // wait for response before continuing.
                        
            num = rawhid_recv(0, buf, 16, 220);
			bytesRead = num;

            if (jtMod_debug && bytesRead > 0)
                printf("JTMOD:    Waiting...read %d bytes, 0x%X\n", num, buf[0]);

            if ( bytesRead > 0 &&  (buf[0] & 0xFF) == STATUS_OK )
                break;

            if (++tries > 10)
            {
                fprintf(stderr, "JTMOD:  Hmm.  Looks like the Arduino HID isn't responding.  Did you program it with the correct hex file?\n");
                exit(42);
            }
            else
            {
                sleep(1);
            }
        }

    }

    printf("JTMOD: Ready.\n");
    return 0;
}

int jtMod_inp (unsigned char * p_byteToFill)
{
    // Read byte from port.
    char	buf[16];
    int    	bytesRead;
    int    	tries;
	buf[0] = CMD_READ;

    if (jtMod_debug)
        printf("\nJTMOD:  Entering jtMod_inp\n");

    rawhid_send(0, buf, 16, 100);
	
    tries = 0;
    bytesRead = -1;
    while (bytesRead <= 0 && tries < 2)
    {
    	bytesRead = rawhid_recv(0, buf, 16, 220);;
        if (jtMod_debug)
        	printf("JTMOD:  Received 0x%X\n",buf[1] & 0xFF);

        tries++;
    }

    if (bytesRead < 0)
    {
        printf("read from Arduino returned %d:\n", bytesRead);
        exit(42);
    }

     //None of bits 6-0 should be set.
    if ( (buf[1] & 0xFF) & 0x7F )
    {
        fprintf(stderr,"Invalid data (%X) on read from Arduino!\n", buf[1] & 0xFF);
        exit(42);
    }

    *p_byteToFill = buf[1] & 0xFF;
    *p_byteToFill ^= 0x80;      // it took me hours to find this

    if (jtMod_debug)
        printf("JTMOD:  Leaving jtMod_inp\n");

    return 0;
}

int jtMod_outp(unsigned char p_byteToSend)
{
    // Pack bits into port.
    char buf[16];
    int bytesRead;

    if (jtMod_debug)
        printf("\nJTMOD:  Entering jtMod_outp\n");

    // Populate bits 4,3,2,1,0
    buf[1] = p_byteToSend & 0xFF;

    // Clear bits 7,6,5
    buf[1] &= 0x1F;        // B0001 1111

    // Set bits 6,5 to '01'
    buf[0] = CMD_SEND;        // B0010 0000

    // Send to Arduino
    if (jtMod_debug)
        printf("JTMOD: Sent... 0x%X\n",buf[0]);

    
	rawhid_send(0, buf, 16, 100);

    // Wait for response before continuing.
    bytesRead = -1;
    while (bytesRead <= 0)
    {
    	bytesRead = rawhid_recv(0, buf, 16, 220);;
    }
    if (jtMod_debug)
    	printf("JTMOD:  Received 0x%X\n",buf[0] & 0xFF);

    if ( bytesRead < 1 || (buf[0] & 0xFF) != STATUS_OK )
    {
        fprintf(stderr, "Invalid data (0x%X) while waiting for send to complete!\n", buf[0]);
        exit(42);
    }

    if (jtMod_debug)
        printf("JTMOD:  Leaving jtMod_outp\n");

    return 0;
}

int jtMod_done(void)
{
    printf("JTMOD: Cleaning up.\n");

    // Send reset signal to Arduino,
    {
        char buf[16];
		buf[0] = CMD_RESET;
		rawhid_send(0, buf, 16, 100);
        
    }

    // Reset and close port.

    rawhid_close(0);
    printf("JTMOD: Done.\n");
    return 0;
}
