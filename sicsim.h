#define TRUE 1
#define FALSE 0
#define TOKLEN 80
#define _CONT -1
#define _QUIT 0
#define _HELP 1
#define _DIR 2
#define _HISTORY 3
#define _DUMP 4
#define _EDIT 5
#define _FILL 6
#define _RESET 7
#define _OPtoMN 8
#define _MNtoOP 9
#define _OPLIST 10
#define _MNLIST 11
#define _TYPE 13
#define _ASM 12
#define _SYM 14
#define _dASM 15
#define _ADRS 16
#define _LOAD 17
#define _RUN 18
#define _BRP 19

int getArgm(int);

unsigned char M[65536][16]={{0}};	//SICSIM virtual memory space
char str[TOKLEN];	//raw user input
char _str[TOKLEN]; 	//backup of str
char *tok;	//string tokens of command, indicating arguments
int argm[3];	//store argument(s)

typedef struct _symtab{
	char label[TOKLEN];
	int loc;
	int flag;
	struct _symtab *link;
}SYMTAB;

SYMTAB* _SYMTAB;

typedef struct _littab{
	char name[TOKLEN];
	char hex[TOKLEN];
	int len;
	int loc;
	struct _littab *link;
}LITTAB;

LITTAB* _LITTAB;

typedef struct _estab{
	char name[TOKLEN];
	int loc;
	struct _estab *link;
}ESTAB;

ESTAB* _ESTAB;

/*file and dir pointers, file info structure*/
DIR *dir;
struct dirent *entry;
struct stat fileStat;

int tmp_adr=0;	//store default start address of dump

/*linked queue for history*/
typedef struct _node{
	char cmd[TOKLEN];
	struct _node* link;
}node;

node* front;
node* rear;

/*add an element in the linked queue*/
void addq(){
	node* temp;

	temp = (node*)malloc(sizeof(node));
	strcpy(temp->cmd, _str);
	temp->link = NULL;
	if(front)
		rear->link=temp;
	else
		front=temp;
	rear=temp;
}

/*hash node*/
typedef struct _hash{
	char MN[5];			//mnemonic string
	int OP;				//opcode
	int key;			//hash key
	int fmt;		//operation format
	struct _hash* link;	//next hash node
}hash;

hash mnTabl[20];	//array of index nodes(mnemonic table)
hash opTabl[20];	//array of index nodes(opcode table)

int HexToDec(char* hexArr){ /* decimalize hexadecimal number*/

	int i;
	int dec = 0;    //dest. decimal value
	int exp;        //hexadecimal exponent

	for(i=0;i<strlen(hexArr);i++){
		if(hexArr[i]>=97 && hexArr[i]<=102)
			hexArr[i] -= 32;
		exp = strlen(hexArr)-i-1;
		if(hexArr[i]<=57)       //Case : hexadecimal number < 10
			dec += (hexArr[i]-48) * pow(16,exp);    //adjust ASCII value to the integer number
		else                    //Case : hexadecimal number >= 10
			dec += (hexArr[i]-55) * pow(16,exp);    //adjust ASCII value to the integer number
	}

	return dec;
}

/*generate string of hexadecimal number(s)*/
char* DecToHex(int len, int dec){

	//len : required length of the format
	//dec : source
	char* hexArr;	//string of hexadecimal #
	int e;			//hexadecimal exponent
	int den;		//denominator
	int quot;		//quotient

	hexArr = (char*)calloc(len,sizeof(char));	//memory allocation 
	/*start from the biggest cipher, find quotient and repeat with remainder*/
	for(e=len-1;e>=0;e--){
		den=pow(16,e);
		quot=dec/den;
		dec -= quot*den;

		if(quot<10)
			hexArr[len-1-e] = quot +48;	//case : 0~9
		else
			hexArr[len-1-e] = quot +55;	//case : A~F
	}

	return hexArr;
}

/*hash function of opcode*/
int hashFunc_OP(int op){
	int key;
	key=(op+3)/13;

	return key;
}

/*hash function of mnemonic*/
int hashFunc_MN(char* mn){
	int key=0, i;
	
	for(i=0;i<strlen(mn);i++)
		key += mn[i]+'L';

	return key%20;
}

/*initialize hash table by using hash function*/
void initHash(){

	FILE* fp;
	hash* ptr;
	hash *mntemp, *optemp;
	char input[TOKLEN];	//temporary string storage for input
	char hex[2];		//temporary opcode string for input
	char str[5];		//temporary mnemonic string for input
	int fmt;
	//char ord[TOKLEN];	//temporary priority string for input
	int dec,key,i;

	/*set the keys of index hash nodes and initialize*/
	for(i=0;i<20;i++){
		mnTabl[i].key=i;
		mnTabl[i].link=NULL;
		opTabl[i].key=i;
		opTabl[i].link=NULL;		
	}
	
	fp = fopen("opcode.txt", "r");
	
	while(fgets(input, sizeof(input), fp)){
		sscanf(input,"%[^ \t]	%[^ \t]	%d",hex,str,&fmt);	//read a line by line from opcode.txt until EOF

		dec=HexToDec(hex);	//store opcode in decimal number

		/*opcode key*/
		key=hashFunc_OP(dec);	//get the hash key of opcode
		optemp = (hash*)malloc(sizeof(hash));	//make new hash node
		ptr=&opTabl[key];	//make ptr indicate the first node
		while(ptr->link!=NULL)	//search the last node
			ptr = ptr->link;
		optemp->OP = dec;	//set&locate optemp
		strcpy(optemp->MN,str);
		optemp->fmt=fmt;
		optemp->link = NULL;
		ptr->link = optemp;

		/*mnemonic key*/		
		key=hashFunc_MN(str);	//get the hash key of mnemonic
		ptr=&mnTabl[key];	//make ptr indicate the first node
		mntemp = (hash*)malloc(sizeof(hash));	//make new hash node
		while(ptr->link!=NULL)	//search the last node
			ptr = ptr->link;
		mntemp->OP = dec;	//set&locate mntemp
		strcpy(mntemp->MN,str);
		mntemp->fmt=fmt;
		mntemp->link = NULL;
		ptr->link = mntemp;
	}
	fclose(fp);		
}

/*print generated hash tables*/
void showTabl(int cmd){
	
	hash* ptr;
	char sym_loc[TOKLEN];
	FILE* fp;
	int i;
	
	if(getArgm(cmd)==-1){
		printf("invalid argument\n");
		return;
	}
	
	if(cmd==_SYM){
		fp = fopen("symtab.txt","r");
		if(fp==NULL){
			printf("no SYMTAB exists\n");
			return;
		}
		while(fgets(sym_loc,sizeof(sym_loc),fp))
			printf("%s",sym_loc);
		addq();
		fclose(fp);
		return;
	}

	for(i=0;i<20;i++){
		//make ptr indicate index(i) hash node
		if(cmd==_OPLIST)
			ptr=opTabl[i].link;
		else if(cmd==_MNLIST)
			ptr=mnTabl[i].link;
	
		printf("%2d : ",i);
		while(ptr){
			printf("[%s, %s]",ptr->MN, DecToHex(2,ptr->OP));
			ptr=ptr->link;	//search next node
			if(ptr)
				printf(" -> ");
		}
		printf("\n");
	}
	
	addq();
	
	
}

/*check arguments if they are hexadecimal number. return value is TRUE or FALSE*/
int isHex(char* hexArr, int maxlen){

	int i;
	int flag=TRUE;

	if(strlen(hexArr)>maxlen)	//check upper boundary
		flag=FALSE;
	else{
		for(i=0;i<strlen(hexArr);i++){
			if(hexArr[i]>=97 && hexArr[i]<=102)	//adjust lowercase to uppercase
				hexArr[i] -= 32;
			if(hexArr[i]<48 || (hexArr[i]>57 && hexArr[i]<65) || hexArr[i]>70){	//not a hexadecimal expression
				flag=FALSE;
				break;
			}
		}
	}

	return flag;
}

/*get the input from user and categorize it*/
int getCmd(){

	int cmd=-2;	//set default

	fgets(str,TOKLEN,stdin);	//user input
	
	if(str[0]!='\n' || strlen(str)>1)	//set the string boundary
		str[strlen(str)-1]='\0';
	else
		return _CONT;	//case : input is '\n'

	strcpy(_str,str);	//backup the primitive input
	tok = strtok(str," ");	//tokenize the input by blank character
	
	if(strcmp(tok,"q")==0 || strcmp(tok,"quit")==0)
		cmd=_QUIT;
	else if(strcmp(tok,"h")==0 || strcmp(tok,"help")==0)
		cmd=_HELP;
	else if(strcmp(tok,"d")==0 || strcmp(tok,"dir")==0)
		cmd=_DIR;
	else if(strcmp(tok,"hi")==0 || strcmp(tok,"history")==0)
		cmd=_HISTORY;
	else if(strcmp(tok,"du")==0 || strcmp(tok,"dump")==0)
		cmd=_DUMP;
	else if(strcmp(tok,"e")==0 || strcmp(tok,"edit")==0)
		cmd=_EDIT;
	else if(strcmp(tok,"f")==0 || strcmp(tok,"fill")==0)
		cmd=_FILL;
	else if(strcmp(tok,"reset")==0)
		cmd=_RESET;
	else if(strcmp(tok,"opcode")==0)
		cmd=_MNtoOP;
	else if(strcmp(tok,"mnemonic")==0)
		cmd=_OPtoMN;
	else if(strcmp(tok,"opcodelist")==0)
		cmd=_OPLIST;
	else if(strcmp(tok,"mnemoniclist")==0)
		cmd=_MNLIST;
	else if(strcmp(tok,"type")==0)
		cmd=_TYPE;
	else if(strcmp(tok,"assemble")==0)
		cmd=_ASM;
	else if(strcmp(tok,"symbol")==0)
		cmd=_SYM;
	else if(strcmp(tok,"disassemble")==0)
		cmd=_dASM;
	else if(strcmp(tok,"progaddr")==0)
		cmd=_ADRS;
	else if(strcmp(tok,"loader")==0)
		cmd=_LOAD;
	else if(strcmp(tok,"run")==0)
		cmd=_RUN;
	else if(strcmp(tok,"bp")==0)
		cmd=_BRP;

	tok = strtok(NULL, " ,");

	return cmd;
}

/*check argument format violation by ',' and ' '*/
int checkArgm(){

	int check=TRUE, aflag=0, qflag=0, i,j;
	//aflag points argument continued
	//qflag indicates (# of argm(s))-(# of ',')
	
	/*extracting argument part */
	for(i=0;i<strlen(_str);i++){

		if(_str[i]==' '){
			for(j=i; j<strlen(_str);j++){
				if(_str[j]!=' ')
					break;
			}
			break;
		}
		else
			continue;
	}
	/*start checking argument format violation		*/
	/*violation occurs when							*/
	/*1. the time new argument starts but qflag!=1	*/
	/*2. qflag!=0 at the end of the checking		*/
	aflag=1;
	for(i=j; i<strlen(_str); i++){

		if(_str[i]==','){
			qflag++;
			if(aflag==1)
				aflag--;
		}
		else if(_str[i]==' '){
			if(aflag==1)
				aflag--;
		}			
		else if(aflag==0){
			if(qflag!=1){
				check=FALSE;
				break;
			}
			qflag--;
			aflag++;
		}
	}
	if(qflag!=0)
		check=FALSE;

	return check;
}

/*determine format(int fmt) of command(# of arguments) and get the arguments*/
int getArgm(int cmd){

	int fmt=0;
	int i;

	if(!(checkArgm()))
		fmt=-1;	//argument format violation(invalid)
	else{
		switch(cmd){
			case _DUMP :
				//format 0 : 'dump'
				//format 1 : 'dump (start)'
				//format 2 : 'dump (start),(end)'
				while(tok){
					if(!(isHex(tok,5)) || fmt>1){	//case :too big or too many or not a hexadecimal argument(s) (invalid)
						fmt=-1;
						break;
					}
					else{
						argm[fmt++] = HexToDec(tok);	//decimalize & store the argument(s)
						tok = strtok(NULL, " ,");
					}
				}
				if(fmt==2 && argm[0]>argm[1])	//case : start address > end address(invalid)
					fmt=-1;
				break;

			case _EDIT :
				//format 2 : 'edit (address),(value)'
				fmt=2;
				if(tok==NULL || !(isHex(tok,5))){	//case : no argument || (too big or not a hexadecimal argument) (invalid)
					fmt=-1;
					break;
				}
				else{
					argm[0]=HexToDec(tok);	//decimalize & store first argument(address)
					tok = strtok(NULL, " ,");
				}

				if(tok==NULL || !(isHex(tok,2)) || (strtok(NULL, "")!=NULL)){	//check if there are two arguments valid
					fmt=-1;
					break;
				}
				else
					argm[1]=HexToDec(tok);	//decimalize & store second argument(value)
				break;

			case _FILL :
				//format 3 : 'fill (start),(end),(value)'
				fmt=3;
				/*check if there are two arguments(start,end address) valid*/
				for(i=0;i<2;i++){
					if(tok==NULL || !(isHex(tok,5))){
						fmt=-1;
						break;
					}
					else{
						argm[i]=HexToDec(tok);	//decimalize & store address
						tok = strtok(NULL, " ,");
					}
				}
				if(tok==NULL || !(isHex(tok,2)) || (strtok(NULL, "")!=NULL) || argm[0]>argm[1])	//invalid conditions
					fmt=-1;
				else
					argm[2]=HexToDec(tok);	//decimalize & store value
				break;
			case _TYPE :
			case _OPtoMN :								
			case _MNtoOP :
			case _ADRS :
			case _BRP :
			case _LOAD :
				fmt=1;
				if(tok==NULL)
					fmt=-1;
				break;
			case _RUN :
			case _HISTORY:
			case _HELP :
			case _RESET :
			case _SYM :
			case _OPLIST :
			case _MNLIST :
				fmt=0;
				if(tok!=NULL)
					fmt=-1;
				break;
		}
	}

	return fmt;
}

/*print help messages*/
void showHelp(){
	if(getArgm(_HELP)==-1){
		printf("invalid argument\n");
		return;
	}
	printf("\nh[elp]\nd[ir]\nq[uit]\nhi[story]\ndu[mp]\ne[dit] (address), (value)\nf[ill] (start), (end), (value)\nreset\nopcode (mnemonic)\nmnemonic (opcode)\nopcodelist\nmnemoniclist\nassemble (filename)\ntype (filename)\nsymbol\ndisassemble (filename)\nloader (filename(s,))\nrun\nbp\nbp clear\nbp (address)\n");
	addq();
	return;
	
}

/*print directories and files under current directory*/
void showDir(){	

	int cnt=0;	//used for print format

	if(getArgm(_DIR)==-1){
		printf("invalid argument\n");
		return;
	}
	dir = opendir(".");	//load current directory

	/*check directory pointer valid*/
	if(dir==NULL){
		printf("directory open error\n");
		return;
	}
	
	/*load files in order in the directory*/
	while((entry = readdir(dir))!=NULL){

		lstat(entry->d_name, &fileStat);	//load status of the file

		if(S_ISDIR(fileStat.st_mode))	//case : directory
			printf("%s/\t",entry->d_name);
		else if((fileStat.st_mode&S_IXUSR) || (fileStat.st_mode&S_IXGRP) || (fileStat.st_mode&S_IXOTH) )	//case : executable file
			printf("%s*\t",entry->d_name);
		else	//case : default
			printf("%s\t",entry->d_name);

		/*print a newline every 3 files*/
		cnt++;
		if(cnt==3){
			printf("\n");
			cnt=0;
		}
	}
	if(cnt!=0)
		printf("\n");
	closedir(dir);	//close current directory
	addq();
	return;
}

/*print the legacy of the valid commands by searching linked queue*/
void showHist(){

	node* temp;	//start searching from the first node
	int cnt=0;	//index

	if(getArgm(_HISTORY)==-1){
		printf("invalid argument\n");
		return;
	}
	addq();
	temp=front;
	printf("\n");
	while(temp){
		cnt++;
		printf("%d\t%s\n",cnt,temp->cmd);
		temp=temp->link;
	}
	printf("\n");
}

/*dump memory field ranged from (start) to (end)*/
void dumpLine(int start, int end){

	int s_row, e_row, s_col, e_col, row, col, i;

	s_row = start / 16;	//start row
	s_col = start % 16;	//start column
	e_row = end / 16;	//end row
	e_col = end % 16;	//end column

	printf("        ");
	for(i=0;i<0x10;i++)
		printf(" %X ",i);
	printf("\n");
	for(row=s_row; row<=e_row; row++){

		/*address*/
		printf("%s | ", DecToHex(5,row*16));

		/*value*/
		for(col=0;col<16;col++){
			if((row==s_row && col<s_col) || (row==e_row && col>e_col))	//case : out of the range required
				printf("   ");
			else
				printf("%s ",DecToHex(2,(int)M[row][col]));
		}

		printf("; ");
		/*ASCII expression*/
		for(col=0;col<16;col++){
			if(M[row][col]<32 || M[row][col]>126 || (row==s_row && col<s_col) || (row==e_row && col>e_col))
				printf(".");
			else
				printf("%c",M[row][col]);
		}
		printf("\n");
	}

	tmp_adr = end+1;	//store the last address +1 for the command 'dump'
}

/*classify dump command format and pass proper arguments*/
void Dump(){

	switch(getArgm(_DUMP)){

		case -1 :
			printf("invalid argument(s)\n");
			return;
		case 0 :
			//command : 'dump'
			if(tmp_adr <= 0xFFFFF-159){
				dumpLine(tmp_adr, tmp_adr+159);
				addq();
			}
			else
				printf("requested range exceeds memory boundary\n");
			break;
		case 1 :
			//command : 'dump (start)'
			if(argm[0] <= 0xFFFFF-159){
				dumpLine(argm[0],argm[0]+159);
				addq();
			}
			else
				printf("requested range exceeds memory boundary\n");
			break;
		case 2 :
			//command : 'dump (start),(end)'
			dumpLine(argm[0],argm[1]);
			addq();
			break;
		default :
			printf("unknown error\n");
			break;
	}
	return;
}

/*edit the value at the specific address*/
void Edit(){

	switch(getArgm(_EDIT)){
	
		case -1 :
			printf("invalid argument(s)\n");
			break;
		case 2 :
			M[argm[0]/16][argm[0]%16] = argm[1];
			addq();
			break;
		default :
			printf("unknown error\n");
	}
	return;
}

/*fill the values at the range of address*/
void Fill(){

	int i;

	switch(getArgm(_FILL)){

		case -1 :
			printf("invalid argument(s)\n");
			break;
		case 3 :
			for(i=argm[0];i<=argm[1];i++)
				M[i/16][i%16]=argm[2];
			addq();
			break;
		default :
			printf("unknown error\n");
	}
	return;
}

/*set all value in the memory zero*/
void Reset(){

	int i;
	if(getArgm(_RESET)==-1){
		printf("invalid argument\n");
		return;
	}
	
	for(i=0; i<=0xFFFFF; i++)
		M[i/16][i%16]=0;
	addq();
	
	return;
}

/*show the mnemonic of the opcode*/
void OPtoMN(){
	
	hash* ptr;
	int key, dec, flag=0;

	/*get the hash key of the argument*/
	if(getArgm(_OPtoMN)==-1){
		printf("invalid argument\n");
		return;
	}

	dec = HexToDec(tok);
	key = hashFunc_OP(dec);
	if(key<0 || key>19){
		printf("invalid argument\n");
		return;
	}
	/*start searching the opcodelist*/
	ptr=opTabl[key].link;
	while(ptr){
		if(ptr->OP == dec){
			printf("mnemonic is %s\n",ptr->MN);
			flag=1;
			addq();
			break;
		}
		else
			ptr = ptr->link;
	}
	if(!flag)
		printf("invalid argument\n");
	
	return;
}

/*show the opcode of the mnemonic*/
void MNtoOP(){
	
	hash* ptr;
	int key, flag=0;

	/*get the hash key of the argument*/
	if(getArgm(_MNtoOP)==-1){
		printf("invalid argument\n");
        return;
	}
	key = hashFunc_MN(tok);
	if(key<0 || key>19){
		printf("invalid argument\n");
		return;
	}
	/*start searching the mnemoniclist*/
	ptr = mnTabl[key].link;
	while(ptr){
		if(strcmp(ptr->MN, tok)==0){
			printf("opcode is %s\n",DecToHex(2,ptr->OP));
			flag=1;
			addq();
			break;
		}
		else
			ptr = ptr->link;
	}
	if(!flag)
		printf("invalid argument\n");
}

/*print target file to stdout*/
void View(){

	FILE *fp;
	char line[TOKLEN];

	if(getArgm(_TYPE)==-1){
		printf("invalid argument\n");
		return;
	}
	fp=fopen(tok,"r");
	if(fp==NULL){
		printf("file open error\n");
		return;
	}
	while(fgets(line,sizeof(line),fp))
		printf("%s",line);

	addq();
	fclose(fp);
	return;
}

void refresh(){

	hash* ptr;
	int i;

	for(i=0;i<20;i++){
		ptr=opTabl[i].link;
		while(ptr){
			DecToHex(2,ptr->OP);
			ptr=ptr->link;  //search next node
		}
	}
}
