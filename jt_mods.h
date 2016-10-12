#ifndef __JT_MODS_H_
#define __JT_MODS_H_

/*
 * All functions:
 * reuturn 0 on success, 1 otherwise
 */
int jtMod_init(void);

/* Write to serial interface. */
int jtMod_inp (unsigned char * p_byteToFill);
int jtMod_outp(unsigned char p_byteToSend);

int jtMod_done(void);

#endif
