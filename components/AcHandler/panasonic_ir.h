#ifndef _PANASONIC_IR_H_
#define _PANASONIC_IR_H_

#include "panasonic_frame.h"

void panasonic_ir_init(void (*receiver)(const struct panasonic_command *cmd, void *priv), void *priv);
void panasonic_transmit(const struct panasonic_command *cmd);

#endif /* _PANASONIC_IR_H_ */
