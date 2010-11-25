#include "reb-host.h"

const char *init_block =
	"REBOL [\n"
		"Title: {Hello REBOL Extension}\n"
		"Name: hello-extension\n"
		"Type: extension\n"
	"]\n"
	"export hello-once: command [{Prints hello from a REBOL extension.}]\n"
	"export hello-twice: command [{Prints hello again from a REBOL extension.}]\n"
;

RL_LIB *RL;

RXIEXT const char *RX_Init(int opts, RL_LIB *lib) {
	RL = lib;
	if (!CHECK_STRUCT_ALIGN) return 0;
	return init_block;
}

RXIEXT int RX_Quit(int opts) {
	return 0;
}

RXIEXT int RX_Call(int cmd, RXIFRM *frm, void *data) {
	switch (cmd) {
	case 0: {
		RL_Print("%s\n", "Hello REBOL extension!");
		break;
	}
	case 1: {
		RL_Print("%s\n", "Hello again!");
		break;
	}
	default:
		return RXR_NO_COMMAND;
	}
	return RXR_UNSET;
}
