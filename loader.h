#define REFMAX 256

int PROGADDR = 0;
int EXECADDR = 0;
int loadcnt = 0;
int A,X,L,PC,B,S,T,SW;	//registers

typedef struct _bp{
	int loc;
	struct _bp *link;
}BP;

BP *_BP;

void setADRS();
int Load();
int Run();
void initESTAB();
ESTAB* addES(char*,int);
int Write(int,int,int);
int Read(int,int,int,int);
void dumpReg();

/*set progaddr*/
void setADRS(){

	PROGADDR = HexToDec(tok);
}

/*initialize breakpoint linked list*/
void initBP(){

	_BP = (BP*)malloc(sizeof(BP));
	_BP->loc = -1;
	_BP->link = NULL;
}

/*add a breakpoint*/
int addBP(int loc){

	BP *bptr = _BP, *New;

	while((bptr->link!=NULL)&&(bptr!=NULL)){
		if(bptr->loc == loc){
			printf("[err] duplicate breakpoint %4X\n",loc);
			return -1;
		}
		else
			bptr = bptr->link;
	}

	New = (BP*)malloc(sizeof(BP));
	New->loc = loc;
	New->link = NULL;
	bptr->link = New;
	printf("[ok] create breakpoint %4X\n",loc);

	addq();
	return 0;
}

/*delete all breakpoints*/
int delBP(){

	BP *bptr, *btmp;

	if(_BP->link ==NULL)
		return -1;

	bptr=_BP->link;
	while(bptr){
		btmp = bptr;
		bptr = bptr->link;
		free(btmp);
	}
	free(_BP);
	
	initBP();
	printf("[ok] clear all breakpoints\n");
	addq();
	return 0;
}

/*printf all breakpoints*/
void showBP(){

	BP *bptr = _BP->link;

	printf("\nbreakpoint\n");
	printf("----------\n");
	while(bptr){
		printf("%4X\n",bptr->loc);
		bptr = bptr->link;
	}
	printf("\n");
	addq();
}

/*process breakpoint command*/
int Break(){

	int loc;

	if(tok==NULL)
		showBP();
	else if(strcmp(tok,"clear")==0)
		delBP();
	else{
		loc = HexToDec(tok);
		addBP(loc);
	}
	
	addq();
	return 0;
}

/*2-pass linking loader*/
int Load(){

	FILE *fpObj, *fpMap = fopen("loadmap.txt","w");
	ESTAB *eptr;
	char *Rec = calloc(TOKLEN,sizeof(char));	//record
	char *objcode = calloc(TOKLEN,sizeof(char));
	char *tmp = calloc(TOKLEN,sizeof(char));
	int *REFLOC;
	int CSADDR = PROGADDR, CSLTH, RECADDR;
	int error[4]={FALSE}, ERR=FALSE;
	int i, j, val;
	//set CSADDR to PROGADDR (for first control section)

	Reset();
	initESTAB();
	fprintf(fpMap,"Control\tSymbol\nsection\tname\tAddress\tLength\n");
	fprintf(fpMap,"_______________________________\n");
	/* PASS 1 */
	while(tok!=NULL){	//while not end of input, do
		//read next input record (Header record for control section)
		fpObj = fopen(tok, "r");
		fgets(Rec,TOKLEN,fpObj);
		//set CSLTH to control section length
		CSLTH = ObjToken(Rec,14,19,objcode);
		//search ESTAB for control section name
		ObjToken(Rec,2,7,tmp);
		eptr = _ESTAB;
		while(eptr){
			//set error flag (duplicate external symbol)
			if(strcmp(eptr->name, tmp)==0){
				printf("%s: error : duplicate external symbol '%s'\n",tok,tmp);
				error[dupSYM] = TRUE;
				ERR = TRUE;
				break;
			}
			else
				eptr = eptr->link;
		}
		//enter control section name into ESTAB with value CSADDR
		if(!error[dupSYM]){
			addES(tmp, CSADDR);
			fprintf(fpMap,"%s\t\t%04X\t%04X\n",tmp,CSADDR,CSLTH);
		}
		error[dupSYM] = FALSE;
		//while record type != E, do
		while(TRUE){
			//read next input record
			fgets(Rec,TOKLEN,fpObj);

			if(Rec[0]=='E'){
				break;
			}
			//if record type = 'D'
			if(Rec[0]=='D'){
				//search ESTAB for symbol name for each symbol in the record
				for(i=0; i<strlen(Rec)-2; i+=12){
					ObjToken(Rec,i+2,i+7,tmp);	//tokenize symbol name
					eptr = _ESTAB;
					while(eptr){
						//if found, set error flag (duplicate external symbol)
						if(strcmp(eptr->name, tmp)==0){
							printf("%s: error : duplicate external symbol '%s'\n",tok,tmp);
							error[dupSYM] = TRUE;
							ERR = TRUE;
							break;
						}
						else
							eptr = eptr->link;
					}//end while(eptr)
					//else, enter symbol into ESTAB with value (CSADDR + indicated address)
					ObjToken(Rec,i+8,i+13,objcode);
					if(!error[dupSYM]){
						addES(tmp, CSADDR+HexToDec(objcode));
						fprintf(fpMap,"\t%s\t%04X\n",tmp,CSADDR+HexToDec(objcode));
					}
					error[dupSYM] = FALSE;
				}//end for
			}//end if(D)
		}//end while(E)
		CSADDR += CSLTH;
		tok = strtok(NULL, " ");
	}//end while(EOF)
	fprintf(fpMap,"________________________________\n");
	fclose(fpMap);
	
	if(ERR)
		return -1;

	/* PASS 2 */
	strcpy(str, _str);
	tok = strtok(_str, " ");
	tok = strtok(NULL, " ");

	REFLOC = (int*)calloc(REFMAX, sizeof(int));
	for(i=0; i<REFMAX; i++)
		REFLOC[i] = -1;

	CSADDR = PROGADDR;	//set CSADDR to PROGADDR
	EXECADDR = PROGADDR;	//set EXECADDR to PROGADDR
	//while not end of input, do
	while(tok!=NULL){
		//read next input record (Header record)
		fpObj = fopen(tok, "r");
		fgets(Rec, TOKLEN, fpObj);
		REFLOC[1] = CSADDR;	//set EXTREF location to CSADDR
		//set CSLTH to control section length
		CSLTH = ObjToken(Rec,14,19,objcode);
		//while record type != 'E', do
		while(Rec[0]!='E'){
			fgets(Rec, TOKLEN, fpObj);	//read next input record

			if(Rec[0]=='R'){
				//fill the rest of R record as blank characters
				i=0;
				while((strlen(Rec)-2)%8 != 0){
					Rec[strlen(Rec)-1]=' ';
					strcat(Rec,"\n");
					i++;
				}			
				//assign external symbol location to the matched reference number
				for(i=0; i<strlen(Rec)-1; i+=8){
					ObjToken(Rec,i+4,i+9,tmp);
					eptr = _ESTAB;
					while(eptr){
						if(strcmp(tmp, eptr->name)==0){
							REFLOC[i/8+2] = eptr->loc;
							break;
						}
						else
							eptr = eptr->link;
					}
				}
			}//end if R rec
			else if(Rec[0]=='T'){
				RECADDR = ObjToken(Rec,2,7,objcode);
				//move object code from record to location (CSADDR + specified address)
				for(i=0; i<strlen(Rec)-10; i+=2)
					M[(CSADDR+RECADDR+i/2)/16][(CSADDR+RECADDR+i/2)%16] = ObjToken(Rec,i+10,i+11,objcode);
				RECADDR=-1;
			}//end if T rec
			
			else if(Rec[0]=='M'){
				//search the array of EXTREF which is indexed by reference number
				i = ObjToken(Rec,11,12,tmp);
				if(REFLOC[i]==-1){
					printf("%s: error : undefined external symbol %s\n",tok,tmp);
					ERR = TRUE;
				}
				else{
					RECADDR = ObjToken(Rec,2,7,objcode)+CSADDR;	//modification address
					strcpy(tmp, "\0");
					for(j=0; j<3; j++)
						strcat(tmp, DecToHex(2, M[(RECADDR+j)/16][(RECADDR+j)%16]));
					val = HexToDec(tmp);
					//extract value needs to be modified in case of odd digits
					if(Rec[8]=='5'){
						val %= 0x100000;
						if(val >= 0x80000)	//if negative value
							val += 0xFFF00000;
					}
					else if(val >= 0x800000)	//if negative value
						val += 0xFF000000;

					//add or subtract value
					if(Rec[9]=='+')
						val += REFLOC[i];
					else if(Rec[9]=='-')
						val -= REFLOC[i];

					if(Rec[8]=='5'){
						if(val < 0)
							val -= 0xFFF00000;
						M[RECADDR/16][RECADDR%16] /= 16;
						M[RECADDR/16][RECADDR%16] += val/0x10000;
					}
					else{
						if(val < 0)
							val -= 0xFF000000;
						M[RECADDR/16][RECADDR%16] = val/0x10000;
					}
					val %= 0x10000;
					M[(RECADDR+1)/16][(RECADDR+1)%16] = val/0x100;
					val %= 0x100;
					M[(RECADDR+2)/16][(RECADDR+2)%16] = val;
				}
			}//end if M rec
		}//end not while E rec
		if(strlen(Rec)>=7)
			EXECADDR = CSADDR + ObjToken(Rec,2,7,tmp);	//set EXECADDR
		CSADDR += CSLTH;
		tok = strtok(NULL, " ");
	}//end while(EOF)

	if(ERR){
		Reset();
		return -1;
	}
	PROGADDR = EXECADDR;	//jump to location given by EXECADDR
	//print load map
	fpMap = fopen("loadmap.txt","r");
	printf("\n");
	while(fgets(tmp, TOKLEN, fpMap))
		printf("%s",tmp);
	printf("\n");
	fclose(fpMap);
	remove("loadmap.txt");

	addq();
	strcpy(_str,str);
	return 0;
}

/*get the register value indicated by register number*/
int getRegval(int reg){

	int val;

	switch(reg){
		case 0:
			val = A;
			break;
		case 1 :
			val = X;
			break;
		case 2 :
			val = L;
			break;
		case 3 :
			val = B;
			break;
		case 4 :
			val = S;
			break;
		case 5 :
			val = T;
			break;
		case 8 :
			val = PC;
			break;
		default :
			break;
	}

	return val;
}

/*run the loaded program*/
int Run(){
	
	BP* bptr = _BP;
	int n,i,x,b,p,e;	//addressing flags
	int TA, OPC;	//target address
	hash* ptr;
	int key, j;

	PC = EXECADDR;

while(TRUE){
	n=i=x=b=p=e=0;	//initialize addressing flags
	/*extract opcode*/
	OPC = M[PC/16][PC%16] & 0xFC;
	key = hashFunc_OP(OPC);
	ptr = opTabl[key].link;
	while(ptr){
		if(OPC == ptr->OP)
			break;
		else
			ptr = ptr->link;
	}

	if(ptr->fmt == 1){
	}
	else if(ptr->fmt == 2){
		if(ptr->OP == 0xB4){	//CLEAR
			switch(M[(PC+1)/16][(PC+1)%16]){
				case 0x00 :
					A = 0;
					break;
				case 0x10 :
					X = 0;
					break;
				case 0x20 :
					L = 0;
					break;
				case 0x30 :
					B = 0;
					break;
				case 0x40 :
					S = 0;
					break;
				case 0x50 :
					T = 0;
					break;
				case 0x80 :
					PC = 0;
					break;
				default :
					break;
			}
		}
		else if(ptr->OP == 0xA0){	//COMPR
			SW = getRegval(M[(PC+1)/16][(PC+1)%16]/0x10) - getRegval(M[(PC+1)/16][(PC+1)%16]%0x10);
		}
		else if(ptr->OP == 0xB8){	//TIXR
			SW = ++X - getRegval(M[(PC+1)/16][(PC+1)%16]/0x10);
		}
		PC += 2;
	}
	else if(ptr->fmt == 3){
		//get the addressing flags
		if((M[PC/16][PC%16] & 0x02) == 0x02)
			n = 1;
		if((M[PC/16][PC%16] & 0x01) == 0x01)
			i = 1;
		if((M[(PC+1)/16][(PC+1)%16] & 0x80) == 0x80)
			x = 1;
		if((M[(PC+1)/16][(PC+1)%16] & 0x40) == 0x40)
			b = 1;
		if((M[(PC+1)/16][(PC+1)%16] & 0x20) == 0x20)
			p = 1;
		if((M[(PC+1)/16][(PC+1)%16] & 0x10) == 0x10)
			e = 1;

		TA = 0;
		for(j=1; j<3; j++){
			TA = TA << 8;
			TA += M[(PC+j)/16][(PC+j)%16];
		}
		if(e){
			TA = TA << 8;
			TA += M[(PC+j)/16][(PC+j)%16];
			TA %= 0x100000;
		}
		else{
			TA %= 0x1000;
			if(TA >= 0x800)	//sign extension
				TA += 0xFFFFF000;
		}
		
		if(x)
			TA += X;	//indexed addressing
		if(b)
			TA += B;	//base relative
		if(p)
			TA += PC+3+e*1;	//pc relative

		if(ptr->MN[0] == 'J'){
			if(n && i)
				n=0;
			else if(n && !i)
				i=1;
		}
		
		if(ptr->OP == 0x14){	//STL
			//TA = Read(TA,n,i,3);
			Write(L, TA, 3);
			PC += 3+e*1;
			//continue;
		}
		else if(ptr->OP == 0x0C){	//STA
			//TA = Read(TA,n,i,3);
			Write(A, TA, 3);
			PC += 3+e*1;
		}
		else if(ptr->OP == 0x68){	//LDB
			B = Read(TA,n,i,3);
			PC += 3+e*1;
			//continue;
		}
		else if(ptr->OP == 0x48){	//JSUB
			L = PC + 3+e*1;
			PC = Read(TA,n,i,3);
		}
		else if(ptr->OP == 0x00){	//LDA
			A = Read(TA,n,i,3);
			PC += 3+e*1;
		}
		else if(ptr->OP == 0x74){	//LDT
			T = Read(TA,n,i,3);
			PC += 3+e*1;
		}
		else if(ptr->OP == 0xE0){	//TD
			SW = TRUE;
			PC += 3+e*1;
		}
		else if(ptr->OP == 0x30){	//JEQ
			if(SW==0)
				PC = Read(TA,n,i,3);
			else
				PC += 3+e*1;
		}
		else if(ptr->OP == 0xD8){	//RD
			//A[rigtmost byte] <- data from device specified by (m)
			PC += 3+e*1;
		}
		else if(ptr->OP == 0x54){	//STCH
			//TA = Read(TA,n,i,3);
			Write(A%0x100, TA, 1);
			PC += 3+e*1;
		}
		else if(ptr->OP == 0x38){	//JLT
			if(SW < 0)
				PC = Read(TA,n,i,3);
			else
				PC += 3+e*1;
		}
		else if(ptr->OP == 0x10){	//STX
			//TA = Read(TA,n,i,3);
			Write(X, TA, 3);
			PC += 3+e*1;
		}
		else if(ptr->OP == 0x4C){	//RSUB
			PC = L;
		}
		else if(ptr->OP == 0x28){	//COMP
			SW = A - Read(TA,n,i,3);
			PC += 3+e*1;
		}
		else if(ptr->OP == 0x3C){	//J
			PC = Read(TA,n,i,3);
		}
		else if(ptr->OP == 0x50){	//LDCH
			A = (A & 0xFFFFFF00);
			A += Read(TA,n,i,1);
			PC += 3+e*1;
		}
		else if(ptr->OP == 0xDC){	//WD
			//Device specified by (m) <- (A)[rightmost byte]
			PC += 3+e*1;
		}
	}

	bptr = _BP;
	while(bptr){
		if((bptr->loc >= PC) && (bptr->loc <= PC+ptr->fmt+e*1)){
			dumpReg();
			printf("\tStop at checkpoint[%4X]\n",bptr->loc);
			EXECADDR = PC;
			return 1;
		}
		else
			bptr = bptr->link;
	}
}//end while(EOP)

	addq();
	return 0;
}

/*write value to the memory*/
int Write(int VAL, int TA, int len){

	int j;

	for(j=0; j<len; j++){
		M[(TA+j)/16][(TA+j)%16] = VAL/(pow(16,len+1-2*j));
		VAL %= (int)pow(16,len+1-2*j);
	}

	return 0;
}

/*read memory and get the value*/
int Read(int TA, int n, int i, int len){
	
	int val=0, j;

	if(n && i){         //simple addressing
		for(j=0; j<len; j++){
			val = val << 8;
			val += M[(TA+j)/16][(TA+j)%16];
		}
	}
	else if(!n && i){   //immediate addressing
		val = TA;
	}
	else if(n && !i){   //indirect addressing
		TA = Read(TA,1,1,3);
		val = Read(TA,1,1,len);
	}

	return val;
}

/*dump register values*/
void dumpReg(){

	printf("\tA  : %06X\t",A);
	printf("X  : %06X\n",X);
	printf("\tL  : %06X\t",L);
	printf("PC : %06X\n",PC);
	printf("\tB  : %06X\t",B);
	printf("S  : %06X\n",S);
	printf("\tT  : %06X\n",T);
}

/*add External Symbol to ESTAB*/
ESTAB* addES(char* str, int loc){

	ESTAB *New, *ptr;
	ptr = _ESTAB;

	New = (ESTAB*)malloc(sizeof(ESTAB));
	strcpy(New->name, str);
	New->loc = loc; 

	while((ptr->link!=NULL)&&(ptr!=NULL))
		ptr = ptr->link;

	if(ptr!=NULL)
		New->link = ptr->link;
	else 
		New->link = NULL;
	ptr->link = New; 

	return New; 
}

/*initialize or free ESTAB*/
void initESTAB(){

	ESTAB *eptr, *etmp;

	if(loadcnt){
		eptr=_ESTAB->link;
		while(eptr){
			etmp = eptr; 
			eptr = eptr->link;
			free(etmp);
		}    
		free(_ESTAB);
	}    
	loadcnt++;

	_ESTAB = (ESTAB*)malloc(sizeof(ESTAB));
	_ESTAB->name[0]='\0';
	_ESTAB->link=NULL;
}
