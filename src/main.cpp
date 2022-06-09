#include "system.h"

extern int userMain(int, char*[]);

int main(int argc, char* argv[]) {
#ifndef BCC_BLOCK_IGNORE
	asm cli;
#endif
	initialize();
#ifndef BCC_BLOCK_IGNORE
	asm sti;
#endif
	int result = userMain(argc, argv);
#ifndef BCC_BLOCK_IGNORE
	asm cli;
#endif
	restore();
#ifndef BCC_BLOCK_IGNORE
	asm sti;
#endif
	cout << "main finished successfully!\n";
	return 0;
}
