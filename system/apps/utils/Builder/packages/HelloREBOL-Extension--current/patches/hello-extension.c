#include "reb-host.h"

const char *init_block =
	"REBOL [\n"
		"Title: {Hello REBOL Extension}\n"
		"Name: hello-extension\n"
		"Type: extension\n"
	"]\n"
	"export hello: command [{Prints hello from a REBOL extension.}]\n"
	"export goodbye: command [{Prints goodbye from a REBOL extension.}]\n"
;

RL_LIB *RL;

RXIEXT const char *RX_Init(int options, RL_LIB *library) {
	RL = library;
	if (!CHECK_STRUCT_ALIGN) return 0;
	return init_block;
}

RXIEXT int RX_Quit(int options) {
	return 0;
}

RXIEXT int RX_Call(int command, RXIFRM *arguments, void *data) {
	switch (command) {
	case 0: {
		RL_PRINT("%s\n", "Hello REBOL extension!");
		break;
	}
	case 1: {
		RL_PRINT("%s\n", "I say hello!");
		break;
	}
	default:
		return RXR_NO_COMMAND;
	}
	return RXR_UNSET;
}
