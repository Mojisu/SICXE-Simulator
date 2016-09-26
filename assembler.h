#define LABEL 0
#define OPCODE 1
#define OPRND1 2
#define OPRND2 3
#define dupSYM 0
#define invOPC 1
#define undSYM 2
#define outRNG 3
#define OBJMAX 60
#define _LOC 0
#define _OPC 1
#define _TA 2
#define _OBJ 3

void initSYMTAB();
void freeSYMTAB();
void write(FILE*, char*);
int checkSYM(char*);
int checkMN(char*);
int hashFunc_SYM(char*);
SYMTAB* addSYM(char*,int);
void writeLstLn(FILE*, int, int, int, char CODE[][TOKLEN], char*);
void writeDltLn(FILE*, char CODE[][TOKLEN]);
void writeSYMTAB();
int ObjToken(char *src, int s_col, int e_col, char *code);
int asmcnt=0;	//assemble count

int Tokenize(char* str, char CODE[][TOKLEN]){

	int fmt=0, i;	//fmt indicates the format(#of tokens)
	char *tmp;
	char _tmp[TOKLEN];

	tmp = calloc(TOKLEN,sizeof(char));
	strcpy(tmp,str);	//backup the line for tokenizing string

	for(i=0; i<4; i++)	//initialize tokens
		CODE[i][0]='\0';

	str = strtok(str," ,\t\n\r");
	//tokenize just by seperators and blank characters
	for(i=0; i<4 && str!=NULL; i++){
		if(str[0]=='.')
			break;
		fmt++;
		strcpy(CODE[i],str);
		str = strtok(NULL," ,\t\n\r");
	}		
	
	//detect seperators and blank characters in string and adjust them as a part of string
	if(CODE[OPRND1][0]=='C' && CODE[OPRND1][1]=='\''){
		tmp = strtok(tmp,"'");
		tmp = strtok(NULL, "'");
		strcpy(_tmp, "C'");
		strcat(_tmp, tmp);
		strcat(_tmp, "'");
		strcpy(CODE[OPRND1], _tmp);	
	}
	
	if(fmt==1){	//case : statement with an operand only
		strcpy(CODE[OPCODE],CODE[0]);
		CODE[LABEL][0]='\0';
	} 
	else if(fmt==2 || fmt==3){	//adjust tokens in case statement with no label
		if((checkMN(CODE[0])!=FALSE && checkMN(CODE[1])==FALSE) || strcmp(CODE[0],"END")==0 || strcmp(CODE[0],"START")==0 || strcmp(CODE[0],"BASE")==0 || strcmp(CODE[0],"ORG")==0){
			for(i=3; i>0; i--){
				if(CODE[i]!=NULL)
					strcpy(CODE[i],CODE[i-1]);
			}
			CODE[LABEL][0]='\0';
		}
	}
	
	return fmt;
}

/*adjust decimal string to integer number */
int StrToDec(char* str){
	
	int i;
	int dec=0;

	for(i=0; i<strlen(str); i++){
		if(str[i]=='-')
			continue;
		dec += (str[i]-48)*pow(10,(strlen(str)-i-1));
	}
	if(str[0]=='-')
		dec *= -1;

	return dec;
}

/*check if a string is mnemonic or not (returns operation format or FALSE)*/
int checkMN(char* str){

	hash *ptr;
	int key;
	int flag=FALSE;
	int extflag=FALSE;
	char *tmp;

	if(str[0]=='+'){
		tmp=(char*)calloc(TOKLEN,sizeof(char));
		strcpy(tmp,str);
		tmp=strtok(tmp,"+");
		strcpy(str,tmp);
		extflag=TRUE;
	}

	key = hashFunc_MN(str);
	ptr = &mnTabl[key];

	while(ptr != NULL){
		if(strcmp(ptr->MN,str)==0){
			if(extflag==TRUE){
			flag=4;
				strcpy(str,"+");
				strcat(str,tmp);
			}
			else
				flag=ptr->fmt;
			break;
		}
		else
			ptr = ptr->link;
	}

	return flag;		
}

/*erase or restore prefix indicators @,#,+*/
int indiCat(char* str, char* indi, int flag){

	char* tmp;

	tmp=(char*)calloc(TOKLEN,sizeof(char));

	if(flag==-1){
		strcpy(tmp,str);
		tmp=strtok(tmp,indi);
		strcpy(str,tmp);
	}
	if(flag==1){
		strcpy(tmp,indi);
		strcat(tmp,str);
		strcpy(str,tmp);
	}

	return 0;
}

/*return register number or -1 in case of invalid register name*/
int isReg(char* str){

	if(strcmp(str,"A")==0)
		return 0x0;
	else if(strcmp(str,"X")==0)
		return 0x1;
	else if(strcmp(str,"L")==0)
		return 0x2;
	else if(strcmp(str,"B")==0)
		return 0x3;
	else if(strcmp(str,"S")==0)
		return 0x4;
	else if(strcmp(str,"T")==0)
		return 0x5;
	else if(strcmp(str,"F")==0)
		return 0x6;
	else if(strcmp(str,"PC")==0)
		return 0x8;
	else if(strcmp(str,"SW")==0)
		return 0x9;
	else
		return -1;
}

/*generate error code*/
int genErrcode(int *error){
	int code=0;
	code+=error[dupSYM]+2*error[invOPC]+4*error[undSYM]+8*error[outRNG];
	return code;
}

/*add a new literal to LITTAB*/
LITTAB* addLIT(char *str, int loc){

	LITTAB *ptr, *New;
	char* tmp;
	int j;
	
	tmp = (char*)calloc(TOKLEN,sizeof(char));
	New = (LITTAB*)malloc(sizeof(LITTAB));
	ptr = _LITTAB;
		
	strcpy(New->name,str);
	New->loc = -1;	//means not located

	while((ptr->link!=NULL)&&(ptr!=NULL))
		ptr = ptr->link;

	New->link = NULL;
	ptr->link = New;

	/*get the value by cases*/
	switch(str[1]){
		case 'C' :
			strcpy(tmp,str);
			tmp = strtok(tmp,"=C'");
			New->len = strlen(tmp)*2;
			for(j=strlen(tmp)-1; j>=0; j--)
				strcat(New->hex, DecToHex(2,tmp[strlen(tmp)-1-j]));
			break;
		case 'X' :
			strcpy(tmp,str);
			tmp = strtok(tmp,"=X'");
			New->len = strlen(tmp);
			strcpy(New->hex, tmp);
			break;
		case '*' :
			New->len = 4;
			strcpy(New->hex,DecToHex(6,loc));
			strcat(New->name,New->hex);
			strcpy(str,New->name);
			break;
	}

	return New;
}

int ExpToVal(char* expr, int loc){
	
	SYMTAB* ptr = _SYMTAB;
	char* tok = (char*)calloc(TOKLEN,sizeof(char));
	int sign;
	int constFlag;
	int val=0, i, j;

	if(expr[0]=='*')
		return loc;

	for(i=0; i<strlen(expr);){
		constFlag=FALSE;

		sign = 1;
		if(expr[i]=='+')
			i++;
		else if(expr[i]=='-'){
			sign = -1;
			i++;
		}

		//tokenize an operand from expression
		for(j=0;;j++){
			if(j==0 && expr[i]>='0' && expr[i]<='9'){	//start of constant operand
				for(j=0;;j++){
					if(expr[i+j]<'0' || expr[i+j]>'9')
						break;
					else
						tok[j] = expr[i+j];
				}
				constFlag=TRUE;
				i += j;
				break;
			}

			if(expr[i]!='-' && expr[i]!='+'){
				tok[j] = expr[i];
				i++;
			}
			else
				break;

			if(i==strlen(expr))	//end of the expression
				break;
		}

		if(constFlag){
			tok[j]='\0';	//notate the end of the string
			val += sign * StrToDec(tok);		
		}
		else{
			tok[j+1]='\0';	//notate the end of the string
			ptr = _SYMTAB;
			while(ptr){
				if(strcmp(ptr->label,tok)==0){
					val += sign * ptr->loc;
					break;
				}
				else
					ptr = ptr->link;
			}
		}
		tok = (char*)realloc(tok,TOKLEN*sizeof(char));
	}
	/*
	if(val<0)
		val -= 0xFFFF0000;
	*/
	return val;
}

int Assemble(){

	FILE *fpASM, *fpINT, *fpLST, *fpOBJ;
	SYMTAB *ptr;
	hash *_ptr;
	LITTAB *lptr;
	char filename[TOKLEN];
	char lstFilename[TOKLEN];
	char objFilename[TOKLEN];
	char stm[TOKLEN], _stm[TOKLEN];
	char *tmp;
	int litFlag=FALSE, ltoFlag=FALSE, constFlag=FALSE, orgFlag=FALSE;
	char CODE[4][TOKLEN];
	int strtFlag=-1, baseFlag, j, fmt, key;
	int error[4]={FALSE}, ERR;
	int _LOCCTR=0, LOCCTR=0, ORGLOC, LINE=0, OP, DSP, LENGTH, BASE, OPfmt;
	int objCnt=0;
	int newlnFlag=FALSE;
	char tmpObj[OBJMAX];
	char *mRec;
	char* objcode;
	int n,i,x,b,p,e;
	
	/*set filename*/
	for(j=0; j<strlen(tok); j++){
		if(tok[j]=='.'){
			filename[j]='\0';
			break;
		}
		else
			filename[j]=tok[j];
	}

	/*file open*/
	strcpy(lstFilename,filename);
	strcat(lstFilename,".lst");
	strcpy(objFilename,filename);
	strcat(objFilename,".obj");
	fpINT = fopen("intfile.txt", "w");
	fpLST = fopen(lstFilename, "w");
	fpOBJ = fopen(objFilename, "w");
	fpASM = fopen(tok, "r");

	if(fpASM==NULL){
		printf("file open error\n");
		return -1;
	}

	initSYMTAB();

	/* PASS 1 */
	LINE+=5;
	fgets(stm, sizeof(stm), fpASM);
	strcpy(_stm,stm);
	fmt = Tokenize(stm,CODE);

	if(strcmp(CODE[OPCODE],"START")==0){	//case : (OPCODE)=="START"
		LOCCTR = _LOCCTR = HexToDec(CODE[OPRND1]);
		fprintf(fpINT,"%d\t%d\t%06X\n",LINE,genErrcode(error),LOCCTR);
		for(j=0;j<4;j++)
			fprintf(fpINT,"%s\t",CODE[j]);
		fprintf(fpINT,"\n");
		strtFlag = 1;
		LINE+=5;
	}
		
	while(strcmp(CODE[OPCODE],"END")!=0){	//loop while (OPCODE)!="END"
		e=0;
		for(j=0; j<4; j++)
			error[j]=FALSE;
				
		if(strtFlag){
			fgets(stm, sizeof(stm), fpASM);
			Tokenize(stm,CODE);
			strtFlag=FALSE;
			continue;
		}
		
		if(fmt!=0){	//case : not a comment line
			
			if(CODE[LABEL][0]!='\0'){	//If there is a symbol in the LABEL field
				//search SYMTAB for LABEL
				//key = hashFunc_SYM(CODE[LABEL]);	//calculate symbol key value
				ptr = _SYMTAB;	//set temporary pointer indicate proper index of the SYMTAB
				while(ptr){
					if(strcmp(ptr->label, CODE[LABEL])==0){	//search SYMTAB for LABEL
						error[dupSYM]=TRUE;	//set error flag(duplicate symbol)
						break;
					}
					else
						ptr=ptr->link;	//search next node
				}
				if(!error[dupSYM]){	//insert (LABEL, LOCCTR) into SYMTAB
					if(strcmp(CODE[OPCODE],"EQU")==0){
						j = ExpToVal(CODE[OPRND1], LOCCTR);
						addSYM(CODE[LABEL],j);
					}
					else
						addSYM(CODE[LABEL],LOCCTR);
				}
			}

			/*case : literal*/
			if(CODE[OPRND1][0]=='='){
				/*find literal to check if it exists already*/
				lptr = _LITTAB;
				litFlag = FALSE;
				
				while(lptr){
					if(strcmp(CODE[OPRND1], lptr->name)==0){
						litFlag=TRUE;
						break;
					}
					else
						lptr = lptr->link;
				}
				//add a new literal
				if(!litFlag)
					addLIT(CODE[OPRND1],LOCCTR);
			}
			
			error[invOPC]=!checkMN(CODE[OPCODE]);
			if(!error[invOPC]){	//case : valid OPCODE
				OPfmt=checkMN(CODE[OPCODE]);
				if(OPfmt==1)
					LOCCTR+=1;
				else if(OPfmt==2)
					LOCCTR+=2;
				else if(OPfmt==4)
					LOCCTR+=4;
				else
					LOCCTR+=3;
			}
			else if(strcmp(CODE[OPCODE], "BASE")==0){	//assembler directive BASE
				error[invOPC]=FALSE;
			}
			else if(strcmp(CODE[OPCODE], "WORD")==0){	//OPCODE=='WORD'
				LOCCTR += 3;
				error[invOPC]=FALSE;
			}
			else if(strcmp(CODE[OPCODE], "RESW")==0){	//OPCODE=='RESW'
				LOCCTR += 3*(StrToDec(CODE[OPRND1]));
				error[invOPC]=FALSE;
			}
			else if(strcmp(CODE[OPCODE], "RESB")==0){	//OPCODE=='RESB'
				LOCCTR += StrToDec(CODE[OPRND1]);
				error[invOPC]=FALSE;
			}
			else if(strcmp(CODE[OPCODE], "BYTE")==0){	//OPCODE=='BYTE'
				if(CODE[OPRND1][0]=='X')		//if OPERAND is hexadecimal number
					LOCCTR += 1;
				else if(CODE[OPRND1][0]=='C'){	//if OPERAND is string
					LOCCTR += strlen(CODE[OPRND1])-3;
				}
				else{}
				error[invOPC]=FALSE;
			}
			else if(strcmp(CODE[OPCODE], "ORG")==0){	//assembler directive ORG
				//set flag to printf ORG and reset for restoring location counter
				if(orgFlag){
					orgFlag=FALSE;
					if(CODE[OPRND1][0]!='\0')
						ORGLOC = ExpToVal(CODE[OPRND1], LOCCTR);	//reset location counter as expression value
					LOCCTR=ORGLOC;
				}
				else{
					orgFlag=TRUE;
					ORGLOC = LOCCTR;
					LOCCTR = ExpToVal(CODE[OPRND1], LOCCTR);	//set location counter as expression value
				}
				error[invOPC]=FALSE;
			}
			else if(strcmp(CODE[OPCODE], "EQU")==0){	//assembler directive EQU
				error[invOPC]=FALSE;	
			}			
			else if(strcmp(CODE[OPCODE], "LTORG")==0 || ltoFlag){	//assembler directive LTORG
				//set flag for locating literals
				if(!ltoFlag)
					ltoFlag=TRUE;
				else{
					ltoFlag=FALSE;
					//search for LITTAB
					lptr = _LITTAB;
					while(lptr){
						if(lptr->loc == -1){
							ltoFlag=TRUE;
							lptr->loc = LOCCTR;
							LOCCTR += lptr->len/2;
							strcpy(CODE[OPCODE],lptr->name);
							break;
						}
						else
							lptr = lptr->link;
					}
				}
				error[invOPC]=FALSE;
				if(!ltoFlag){
					if(fgets(stm, sizeof(stm), fpASM)==NULL)    //read next input line  //case : EOF
						break;
					else{
						fmt=Tokenize(stm,CODE); //tokenize a line
						LINE+=5;
					}
					continue;
				}
			}

			/*write a line to intermediate file*/
			fprintf(fpINT,"%d\t%d\t%06X\n",LINE,genErrcode(error),LOCCTR);
			for(j=0; j<4; j++)
				fprintf(fpINT,"%s\t",CODE[j]);
			fprintf(fpINT,"\n");
		}
		else{
			/*write a comment indicator '.' to intermediate file*/
			fprintf(fpINT,"%d\t%d\t%06X\n",LINE,genErrcode(error),LOCCTR);
			fprintf(fpINT,"%s\t\n",stm);
		}

		//in case of literal pool, don't read a next line
		if(ltoFlag)
			continue;

		if(fgets(stm, sizeof(stm), fpASM)==NULL)    //read next input line  //case : EOF
			break;
		else{
			fmt=Tokenize(stm,CODE);	//tokenize a line
			LINE+=5;
		}

	}
	/*write last line to intermediate file*/
	fprintf(fpINT,"%d\t%d\t%06X\n",LINE,genErrcode(error),LOCCTR);
	for(j=0; j<4; j++)
		fprintf(fpINT,"%s\t",CODE[j]);
	fprintf(fpINT,"\n");

	/*flush undefined literals*/
	lptr = _LITTAB;
	CODE[OPRND1][0]='\0';
	while(lptr){
		if(lptr->loc == -1){
			lptr->loc = LOCCTR;
			LOCCTR += lptr->len/2;
			strcpy(CODE[OPCODE],lptr->name);
			fprintf(fpINT,"%d\t%d\t%06X\n",LINE,genErrcode(error),LOCCTR);
			for(j=0; j<4; j++)
				fprintf(fpINT,"%s\t",CODE[j]);
			fprintf(fpINT,"\n");
		}
		lptr = lptr->link;
	}

	fclose(fpINT);
	
	/* PASS 2 */
	fpINT = fopen("intfile.txt","r");
	//read first input line
	baseFlag = strtFlag = FALSE;	//reset flags
	LENGTH = LOCCTR-_LOCCTR;		//calculate the program length
	mRec = (char*)calloc(OBJMAX,sizeof(char));	//allocate memory of Modification record
	fgets(stm, sizeof(stm), fpINT);
	sscanf(stm, "%d\t%d\t%06X\t",&LINE,&ERR,&LOCCTR);
	fgets(_stm, sizeof(_stm), fpINT);
	fmt=Tokenize(_stm,CODE);
	
	if(strcmp(CODE[OPCODE],"START")==0){
		writeLstLn(fpLST, fmt, LINE, LOCCTR, CODE, NULL);	//write listing line
	}
	fprintf(fpOBJ,"H%-6s%06X%06X\n",CODE[LABEL],LOCCTR,LENGTH);	//write Header record to obj program
	//read next input line
	_LOCCTR=LOCCTR;
	fgets(stm, sizeof(stm), fpINT);
	sscanf(stm, "%d\t%d\t%06X\t",&LINE,&ERR,&LOCCTR);
	fgets(_stm, sizeof(_stm), fpINT);
	fmt=Tokenize(_stm,CODE);
	
	fprintf(fpOBJ,"T%06X",_LOCCTR);

	while(TRUE){
		/*initialize flag,n,i,x,b,p,e*/
		error[undSYM]=TRUE;
		orgFlag=litFlag=constFlag=FALSE;
		n=i=p=1;
		b=x=e=0;
		//set the value of Base register to PC, to detect 'out of relative range' error
		if(!baseFlag)
			BASE = LOCCTR;

		if(fmt!=0){	//if this is not a comment line
			OPfmt = checkMN(CODE[OPCODE]);
			if(OPfmt){	//search OPTAB for OPCODE
				if(CODE[OPRND1][0]!='\0'){	//if there is a symbol in OPERAND field
					/*opcode format : 1*/
					if(OPfmt==1)
						n=i=x=b=p=e=0;
					/*opcode format : 2*/
					else if(OPfmt==2){
						DSP=n=i=x=b=p=e=0;
						for(j=OPRND1; j<=OPRND2; j++){
							if(isReg(CODE[j])>-1){
								DSP += isReg(CODE[j]);	//get the register number
								if(j==OPRND1)
									DSP *= 0x10;	//shift left the first operand
							}
							else if(j==OPRND1){
								error[undSYM]=TRUE;
							}
						}
					}
					/*opcode format : 3/4*/
					else{
						if(CODE[OPRND1][0]=='#'){	//immediate addressing
							indiCat(CODE[OPRND1],"#",-1);	//erase '#' from the operand(for searching)
							n=0;
							i=1;
						}
						else if(CODE[OPRND1][0]=='@'){	//indirect addressing
							indiCat(CODE[OPRND1],"@",-1);	//erase '@' from the operand(for searching)
							n=1;
							i=0;
						}

						/*in case of literal, search for LITTAB*/
						if(CODE[OPRND1][0]=='='){
							error[undSYM]=FALSE;
							lptr = _LITTAB;
							litFlag = TRUE;
							while(lptr){
								if(strcmp(CODE[OPRND1], lptr->name)==0){
									DSP = lptr->loc;
									break;
								}
								else
									lptr = lptr->link;	
							}													
						}
						/*in case of symbol operand, search for SYMTAB*/
						else{
							ptr = _SYMTAB;    //set temporary pointer indicate proper index of the SYMTAB
							while(ptr){
								if(CODE[OPRND1][0]>='0' && CODE[OPRND1][0]<='9'){//case : constant
									error[undSYM]=FALSE;
									constFlag=TRUE;
									DSP = StrToDec(CODE[OPRND1]);
									break;
								}			
								else if(strcmp(ptr->label, CODE[OPRND1])==0){ //search SYMTAB for OPERAND
									error[undSYM]=FALSE;
									DSP = ptr->loc;	//store symbol value as operand address
									break;
								}
								else
									ptr=ptr->link;  //search next node
							}
						}

						if(error[undSYM]){
							DSP= 0;	//store 0 as operand address	
							ERR += 4;
						}
						else if(!constFlag){
							/*adjust n,i,x,b,p,e*/
							//extended addressing
							if(CODE[OPCODE][0]=='+'){
								e=1;
								p=b=0;
								indiCat(CODE[OPCODE],"+",-1);
							}
							else{
								//BASE relative
								if(DSP - _LOCCTR > 2047 || DSP - _LOCCTR < -2048){
									p=0;
									b=1;
									DSP -= BASE;	//calculate the displacement relative to the Base
									if(DSP > 4095 || DSP < 0){	//error : out of relative range
										DSP = 0;
										p=b=0;
										error[outRNG]=TRUE;
										ERR+=8;
									}
								}
								//PC relative
								else
									DSP -= LOCCTR;	//calculate the displacement relative to the PC
							}
							if(DSP<0)
								DSP -= 0xFFFFF000;	//adjust the negative value
							if(strcmp("X", CODE[OPRND2])==0)
								x=1;
							DSP += 0x8000*x + 0x4000*b + 0x2000*p + 0x100000*e;	//adjust the flag values to the displacement
						}
					}
				}
				else
					DSP = 0;

				/*assemble the obj code instruction*/
				key=hashFunc_MN(CODE[OPCODE]);
				//get the opcode value
				_ptr = mnTabl[key].link;
				while(_ptr){
					if(strcmp(_ptr->MN, CODE[OPCODE])==0){
						OP=_ptr->OP;
						if(_ptr->fmt >2)
							OP+=2*n+i;
						break;
					}
					else
						_ptr=_ptr->link;
				}
				
				switch(OPfmt){
					case 1:	//instruction format 1
						objcode = calloc(2,sizeof(char));
						objcode[0] = '\0';
						strcat(objcode,DecToHex(2,OP));
						break;
					case 2:	//instruction format 2
						objcode = calloc(4,sizeof(char));
						objcode[0] = '\0';
						strcat(objcode,DecToHex(2,OP));
						strcat(objcode,DecToHex(2,DSP));
						break;
					case 3:
					case 4:	//instruction format 3/4
						objcode = calloc(6+2*e,sizeof(char));
						objcode[0] = '\0';
						strcat(objcode,DecToHex(2,OP));
						strcat(objcode,DecToHex(4+2*e,DSP));
						/*write M record*/
						if(e==1 && !(n==0 && i==1)){
							strcat(mRec,"M");
							strcat(mRec,DecToHex(6,_LOCCTR+1));
							strcat(mRec,"05\n");
						}							
						break;
				}
				strcat(tmpObj,objcode);
				//objCnt = strlen(tmpObj);
			}
			else if(strcmp(CODE[OPCODE],"BASE")==0){
				baseFlag = TRUE;
				ptr = _SYMTAB;
				while(ptr){
					if(strcmp(ptr->label,CODE[OPRND1])==0){
						BASE = ptr->loc;	//set the B register value
						break;
					}
					else
						ptr = ptr->link;
				}
			}
			else if(strcmp(CODE[OPCODE],"LTORG")==0){

			}
			/*literal pool*/
			else if(CODE[OPCODE][0]=='='){
				objcode = calloc(OBJMAX,sizeof(char));
				lptr = _LITTAB;
				while(lptr){
					if(lptr->loc == _LOCCTR)
						break;
					else
						lptr = lptr->link;
				}
				strcpy(objcode, lptr->hex);
				strcat(tmpObj,objcode);
			}
			/*Equation*/
			else if(strcmp(CODE[OPCODE],"EQU")==0){
				ptr = _SYMTAB;
				while(ptr){
					if(strcmp(CODE[LABEL],ptr->label)==0){
						_LOCCTR = ptr->loc;
						break;
					}
					else
						ptr = ptr->link;
				}
				newlnFlag=TRUE;
			}
			else if(strcmp(CODE[OPCODE],"ORG")==0){
				newlnFlag=TRUE;
			}
			/*if opcode is BYTE or WORD then convert constant to object code*/
			else if(strcmp(CODE[OPCODE],"BYTE")==0){
				if(CODE[OPRND1][0]=='C'){
					objcode = calloc(OBJMAX,sizeof(char));
					objcode[0] = '\0';
					tmp = calloc(TOKLEN,sizeof(char));
					strcpy(tmp,CODE[OPRND1]);
					tmp = strtok(tmp,"C'");
					for(j=0;j<strlen(tmp);j++)
						strcat(objcode,DecToHex(2,(int)tmp[j]));
					strcat(tmpObj,objcode);
				}
				else if(CODE[OPRND1][0]=='X'){
					objcode = calloc(2,sizeof(char));
					objcode[0] = '\0';
					strcpy(objcode,CODE[OPRND1]);
					objcode = strtok(objcode,"X'");
					strcat(tmpObj,objcode);
				}
			}
			else if(strcmp(CODE[OPCODE],"WORD")==0){
				objcode = calloc(6,sizeof(char));
				objcode[0] = '\0';
				DSP = StrToDec(CODE[OPRND1]);
				if(DSP<0)	//case : negative value, adjust to 2's complement
					DSP = 0x1000000+DSP;
				strcat(objcode,DecToHex(6,DSP));
				strcat(tmpObj,objcode);
			}
			/*if opcode is RESW or RESB*/
			/*write Text recore to object program*/
			else if((strcmp(CODE[OPCODE],"RESW")==0) || (strcmp(CODE[OPCODE],"RESB")==0)){
				objcode[0]='\0';
				
				if(!newlnFlag){
					fprintf(fpOBJ,"%02X%s\n",(int)strlen(tmpObj)/2,tmpObj);
					tmpObj[0]='\0';
					objCnt=0;
				}
				newlnFlag=TRUE;
			}
		}
		/*decode error code and print error mesg*/
		if(ERR!=0){
			if(ERR/8){
				printf("%s.asm:%d: error: out of relative range\n",filename,LINE);
				ERR-=8;
			}
			if(ERR/4){
				printf("%s.asm:%d: error: undefined symbol '%s'\n",filename,LINE,CODE[OPRND1]);
				ERR-=4;
			}
			if(ERR/2){
				printf("%s.asm:%d: error : invalid opcode '%s'\n",filename,LINE,CODE[0]);
				ERR-=2;
			}
			if(ERR/1)
				printf("%s.asm:%d: error : duplicate symbol '%s'\n",filename,LINE,CODE[LABEL]);
			strtFlag = TRUE;
		}

		/*write listing file*/
		//restore prefix indicators to string
		if(e==1)
			indiCat(CODE[OPCODE],"+",1);
		if(n==0 && i==1)
			indiCat(CODE[OPRND1],"#",1);
		if(n==1 && i==0)
			indiCat(CODE[OPRND1],"@",1);

		writeLstLn(fpLST,fmt,LINE,_LOCCTR,CODE,objcode);
		_LOCCTR=LOCCTR;

		/*read next input line*/
		if(fgets(stm, sizeof(stm), fpINT)==NULL)
			break;
		sscanf(stm, "%d\t%d\t%06X\t",&LINE,&ERR,&LOCCTR);
		fgets(_stm, sizeof(_stm), fpINT);
		fmt=Tokenize(_stm,CODE);

		objCnt = strlen(tmpObj);
		if(objCnt > OBJMAX-(LOCCTR-_LOCCTR) && newlnFlag==FALSE){   //initialize new Text record
			fprintf(fpOBJ,"%02X%s\nT%06X",(int)strlen(tmpObj)/2,tmpObj,LOCCTR);
			objCnt=0;
			tmpObj[0]='\0';
		}
		else if(newlnFlag && ((strcmp(CODE[OPCODE],"ORG")!=0)&&(strcmp(CODE[OPCODE],"EQU")!=0)&& (strcmp(CODE[OPCODE],"RESW")!=0) && (strcmp(CODE[OPCODE],"RESB")!=0))){
			fprintf(fpOBJ,"T%06X",_LOCCTR);
			newlnFlag=FALSE;
		}

	}
	/*write last Text record to obj program*/
	if(tmpObj[0]!='\0')
		fprintf(fpOBJ,"%02X%s\n",(int)strlen(tmpObj)/2,tmpObj);
	/*write Modification Record to obj program*/
	fprintf(fpOBJ,"%s",mRec);
	/*write End record to obj program*/
	fprintf(fpOBJ,"E%06X\n",LOCCTR-LENGTH);
	/*write last listing line*/
	//writeLstLn(fpLST,fmt,LINE,LOCCTR,CODE,NULL);

	fclose(fpINT);
	fclose(fpASM);
	fclose(fpLST);
	fclose(fpOBJ);
	
	/*print result filenames or delete files in case of failure*/
	if(strtFlag){	
		strcpy(lstFilename,filename);
		strcat(lstFilename,".lst");
		remove(lstFilename);
		remove(objFilename);
	}
	else{
		writeSYMTAB();
		printf("output file : [%s.lst], [%s.obj]\n",filename,filename);
	}
	
	addq();
	return 0;
} 

int dAssemble(){
	
	FILE *fpOBJ, *fpDLT;
	hash *ptr;
	char filename[TOKLEN], Recln[TOKLEN];
	char CODE[4][TOKLEN];
	char *str, *tmp, *_tmp;
	int LOCCTR=0, col, i, len, key, constFlag=FALSE;

	fpOBJ = fopen(tok,"r");
	tok = strtok(tok,".");
	strcpy(filename, tok);
	strcat(filename,".dlt");
	fpDLT = fopen(filename,"w");
	tmp = calloc(TOKLEN,sizeof(char));
	_tmp = calloc(TOKLEN,sizeof(char));
	if(fpOBJ==NULL){
		printf("file open error\n");
		return -1;
	}

	/*process Header record*/
	fgets(Recln,sizeof(Recln),fpOBJ);
	ObjToken(Recln,2,7,tmp);
	ObjToken(Recln,10,13,CODE[_LOC]);
	fprintf(fpDLT,"%s\t%s\tSTART\t%s\n",CODE[_LOC],tmp,CODE[_LOC]);
	fgets(Recln,sizeof(Recln),fpOBJ);
	
	/*loop while Text record*/
	str = calloc(TOKLEN,sizeof(char));

	refresh();
	while(Recln[0]!='E'){
		i=10;
		ObjToken(Recln,2,7,CODE[_LOC]);
		LOCCTR = HexToDec(CODE[_LOC]);
		//ObjToken(Recln,8,9,str);
		len = strlen(Recln);
		
		/*disassemble a line*/
		while(i<len){

			ObjToken(Recln,i,i+1,CODE[_OPC]);	//read opcode
			/*search opcode*/
			key = hashFunc_OP(HexToDec(CODE[_OPC]));					
			ptr = opTabl[key].link;
			while(ptr){
				if(HexToDec(CODE[_OPC]) == ptr->OP){
					for(col=0;col<strlen(CODE[_OPC]);col++)
						CODE[_OPC][col]='\0';
					strcpy(CODE[_OPC],ptr->MN);
					break;
				}
				else
					ptr=ptr->link;
			}

			if(ptr==NULL){	//case : constant
				strcat(str, CODE[_OPC]);
				constFlag+=2;
				i+=2;
				if(constFlag>=6){	//flush full constant
					strcpy(CODE[_LOC], DecToHex(4,LOCCTR+(i-16)/2));
					strcpy(CODE[_OPC], str);
					strcpy(CODE[_OBJ], str);
					CODE[_TA][0]='\0';
					writeDltLn(fpDLT, CODE);
					constFlag=FALSE;
					str[0]='\0';
					continue;
				}
				continue;
			}
			else if(constFlag!=FALSE){	//flush constant
				strcpy(_tmp, CODE[_OPC]);
				strcpy(CODE[_OPC], str);
				strcpy(CODE[_LOC], DecToHex(4,LOCCTR+(i-10-constFlag)/2));
				strcpy(CODE[_OBJ], str);
				CODE[_TA][0]='\0';
				writeDltLn(fpDLT, CODE);
				str[0]='\0';
				strcpy(CODE[_OPC], _tmp);
				constFlag=FALSE;
			}
			//case : default(1-word statement)		
			strcpy(CODE[_LOC], DecToHex(4, LOCCTR+(i-10)/2));
			ObjToken(Recln,i+2,i+5,CODE[_TA]);
			ObjToken(Recln,i,i+5,CODE[_OBJ]);
			writeDltLn(fpDLT,CODE);
			i+=6;
		}
		//flush constant located at the end of the Text record
		if(constFlag){
			strcpy(CODE[_LOC], DecToHex(4,LOCCTR+(i-10-constFlag)/2));
			strcpy(CODE[_OPC], str);
			strcpy(CODE[_OBJ], str);
			CODE[_TA][0]='\0';
			writeDltLn(fpDLT, CODE);
			constFlag=FALSE;
			str[0]='\0';
		}
		fgets(Recln,sizeof(Recln),fpOBJ);	//read next record
	}
	/*write last end line*/
	fprintf(fpDLT,"\t\tEND\n");

	printf("output file : [%s]\n",filename);
	fclose(fpOBJ);
	fclose(fpDLT);
	free(str);
	free(tmp);
	free(_tmp);
	addq();

	return 0;
}

/*tokenize object code from start column(s_col) to end column(e_col)*/
int ObjToken(char *src, int s_col, int e_col, char *code){
	int i;
	char *tmp;

	tmp = malloc(sizeof(code));
	for(i=0; i<=e_col-s_col; i++)
		tmp[i] = src[s_col+i-1];
	strcpy(code,tmp);
	return HexToDec(code);
}

/* insert a new symbol to the SYMTAB */
SYMTAB* addSYM(char* str, int loc){

	SYMTAB *New, *ptr;
	ptr = _SYMTAB;

	New = (SYMTAB*)malloc(sizeof(SYMTAB));
	strcpy(New->label, str);
	New->loc = loc;
	
	while((ptr->link!=NULL)&&(ptr!=NULL)){
		if(strcmp(ptr->link->label,str)>0){
			ptr=ptr->link;
			continue;
		}
		else
			break;
	}
	if(ptr!=NULL)
		New->link = ptr->link;
	else
		New->link = NULL;
	ptr->link = New;

	return New;
}

/*wrtie SYMTAB to symtab.txt in case of successful assemble*/
void writeSYMTAB(){

	FILE *fp;
	SYMTAB *ptr;
	int loc;

	fp = fopen("symtab.txt","w");
	ptr = _SYMTAB->link;
	
	while(ptr){
		loc = ptr->loc;
		if(loc<0)
			loc -= 0xFF000000;			
		fprintf(fp,"%s\t%06X\n",ptr->label,loc);
		ptr=ptr->link;
	}

	fclose(fp);
}

/*write a single disassembled statement line to file*/
void writeDltLn(FILE* fp, char CODE[][TOKLEN]){
	int i;

	for(i=0;i<4;i++){
		if(CODE[i][0]=='\0'){
			fprintf(fp,"\t");
			continue;
		}
		fprintf(fp,"%s\t",CODE[i]);
		if(i==0)
			fprintf(fp,"\t");
	}
	fprintf(fp,"\n");
}

/*write a single statement to listing file*/
void writeLstLn(FILE* fp, int fmt, int LINE, int LOCCTR, char CODE[][TOKLEN], char* objcode){

	int i;

	if(strcmp(CODE[OPCODE],"END")==0 || strcmp(CODE[OPCODE],"BASE")==0 || strcmp(CODE[OPCODE],"LTORG")==0)
		objcode=NULL;

	if(fmt==0)
		fprintf(fp,"%d\t\t.\n",LINE);
	else{
		if(LOCCTR<0)
			LOCCTR -= 0xFFFF0000;
		if(strcmp(CODE[OPCODE],"END")==0 || strcmp(CODE[OPCODE],"BASE")==0 || strcmp(CODE[OPCODE],"LTORG")==0)
			fprintf(fp,"%d\t\t",LINE);
		else
			fprintf(fp,"%d\t%04X\t",LINE,LOCCTR);

		if(isReg(CODE[OPRND2]) != -1){
			strcat(CODE[OPRND1],",");
			strcat(CODE[OPRND1],CODE[OPRND2]);
		}

		for(i=0;i<3;i++)
			fprintf(fp,"%s\t",CODE[i]);
		
		if(objcode){
			if(strlen(CODE[OPRND1]) < 8 && !(CODE[OPCODE][0]=='=' && CODE[OPCODE][1]=='*'))
				fprintf(fp,"\t");
			fprintf(fp,"%s",objcode);	
		}
		fprintf(fp,"\n");
	}
}

/*(free and) initialize SYMTAB as blank*/
void initSYMTAB(){

	SYMTAB* ptr, *tmp;
	LITTAB* lptr, *ltmp;

	if(asmcnt){
		ptr=_SYMTAB->link;
		while(ptr){
			tmp = ptr;
			ptr = ptr->link;
			free(tmp);
		}
		free(_SYMTAB);

		lptr=_LITTAB->link;
		while(lptr){
			ltmp = lptr;
			lptr = lptr->link;
			free(ltmp);
		}
		free(_LITTAB);
	}
	asmcnt++;

	_SYMTAB = (SYMTAB*)malloc(sizeof(SYMTAB));
	_SYMTAB->label[0]='\0';
	_SYMTAB->link=NULL;
	_LITTAB = (LITTAB*)malloc(sizeof(LITTAB));
	_LITTAB->name[0]='\0';
	_LITTAB->link=NULL;
}
