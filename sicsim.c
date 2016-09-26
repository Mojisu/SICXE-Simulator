#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <math.h>
#include "sicsim.h"
#include "assembler.h"
#include "loader.h"

int main(){

	initHash();	
	initBP();
	while(1){
		printf("sicsim> ");	
		switch(getCmd()){

			case _QUIT :
				return 0;
			case _CONT :
				break;
			case _HELP :
				showHelp();
				break;
			case _DIR :
				showDir();
				break;
			case _HISTORY :
				showHist();
				break;
			case _DUMP :
				Dump();
				break;
			case _EDIT :
				Edit();
				break;
			case _FILL :
				Fill();
				break;
			case _RESET:
				Reset();
				break;
			case _OPtoMN :
				OPtoMN();
				break;
			case _MNtoOP :
				MNtoOP();
				break;
			case _OPLIST :
				showTabl(_OPLIST);
				break;
			case _MNLIST :
				showTabl(_MNLIST);
				break;
			case _TYPE :
				View();
				break;
			case _ASM :
				Assemble();
				break;
			case _SYM :
				showTabl(_SYM);
				break;
			case _dASM :
				dAssemble();
				break;
			case _ADRS :
				setADRS();
				break;
			case _LOAD :
				Load();
				break;
			case _RUN :
				Run();
				break;
			case _BRP :
				Break();
				break;
			default :
				printf("invalid command\n");
		}
	}
}
