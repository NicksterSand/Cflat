#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

enum TimeDiv{tpb, fps};

int processOperation();
int processValue();
struct list *processLocation();

//Setup for Chords
//////////////////////////////////////////////////
struct note {
	struct note *down;
	struct note *next;
	int val;
};

struct note *newNote(int in){
	struct note *out = malloc(sizeof(struct note));
	out->down = NULL;
	out->next = NULL;
	out->val  = in;
	return out;
}

int chordSize(struct note *in){
	if(in->val < 0)
		return 0;
	int i = 1;
	struct note *tmp = in;
	while(tmp->down != NULL){
		i++;
		tmp = tmp->down;
	}
	return i;
}
///////////////////////////////////////////////////

//Setup for Lists
///////////////////////////////////////////////////
struct list{
	int val;
	struct list *next;	
};

struct list *newList(){
	struct list *out = malloc(sizeof(struct list));
	out->val = 0;
	out->next = NULL;	
	return out;
}

struct list *getList(struct list *in, int num){
	struct list *tmp = in;
	for(int i = 0; i < num; i++){
		if(tmp->next != NULL){
			tmp = tmp->next;
		}else{
			tmp->next = newList();
			tmp = tmp->next;
		}
	}
	return tmp;
}
///////////////////////////////////////////////////


//Global Variables (because otherwise I would have to pass them to every function)
struct note *currentNote;
struct note *currentChord;
struct note *firstNote;
int noteNum;
struct list *lists[128];
int labels[128][5];
struct note *labelChords[128];
int DEBUG = 0;
int MIN_DT = 0;

//Programming Language Functions
///////////////////////////////////////////////////
int processValue(){
	currentChord = currentChord->next;
	currentNote = currentChord;
	noteNum++;

	int chord = chordSize(currentChord);

	if(chord == 0){
		printf("Error: rest found at note %d where there should be a designator for literal or statement.", noteNum);
		return 0;
	}

	if(chord % 2 == 0){
		return processOperation();
	}else{
		//Literals
		int sum = 0;
		do{
			currentChord = currentChord->next;
			currentNote = currentChord;
			noteNum++;
			int product = 0;
			if(currentChord->val >= 0){
				product = (currentNote->val - 60);
				while(currentNote->down != NULL){
					currentNote = currentNote->down;
					product *= (currentNote->val - 60);
				}
			}
			
			sum += product;
		}while(currentChord->val >= 0 && currentChord->next != NULL);
		return sum;
	}

	return 0;
}

struct list *processLocation(){
	currentChord = currentChord->next;
	currentNote = currentChord;
	noteNum++;
	struct list *baseList = lists[currentNote->val];
	int val = processValue();
	return getList(baseList, val);
}

int processOperation(){
	currentChord = currentChord->next;
	currentNote = currentChord;
	noteNum++;

	int output = 0;
	if(currentChord->down == NULL){
		struct list *baseList = lists[currentNote->val];
		int val = processValue();
		output = getList(baseList, val)->val;
	}else if(currentChord->down->down == NULL){
		int interval = abs(currentChord->val - currentChord->down->val);
		int val1 = processValue();
		int val2 = processValue();
		if(interval == 4 || interval == 6 || interval == 11){
			//ADD
			output = val1 + val2;
		}else if(interval == 2 || interval == 5 || interval == 8){
			//SUB
			output = val1 - val2;
		}else if(interval == 1 || interval == 7 || interval == 10){
			//MUL
			output = val1 * val2;
		}else if(interval == 3 || interval == 9){
			//DIV
			output = val1 / val2;
		}
	}
	return output;
}

int *chordToArray(int size){
	int *notes = malloc(sizeof(int) * size);
	for(int i = 0; i < size; i++){
		notes[i] = currentNote->val;
		currentNote = currentNote->down;
	}
	
	for(int i = 0; i < size - 1; i++){
		int min = notes[i];
		int mindex = i;
		for(int j = i; j < size; j++){
			if(notes[j] < min){
				min = notes[j];
				mindex = j;
			}
		}
		int tmp = notes[i];
		notes[i] = notes[mindex];
		notes[mindex] = tmp;
	}

	//printf("\n");
	return notes;
}

///////////////////////////////////////////////////

//returns 2 ^ exp
int twoPower(int exp){
	return 1 << exp;
}

//converts an array of chars to a binary string (not variable-length)
char *charsToBin(char *c, int s){
	char *out = malloc(8 * s + 1);

	for(int i = 0; i < s; i++){
		int tempC = (int)c[i];
		if(tempC < 0){
			tempC = 255 + tempC + 1;
		}
		for(int j = 7; j >= 0; j--){
			if(tempC >= twoPower(j)){
				tempC -= twoPower(j);
				out[(i * 8) + 7 - j] = '1'; 
			}else{
				out[(i * 8) + 7 - j] = '0';
			}
		}
	}
	
	out[8 * s] = '\0';
	
	return out;
}

//converts an array of chars to a variable-length int
void varLen(char *c, int *nOut, int *sOut){
	int i = 0;
	int cont = 1;
	do{
		char *tmp = charsToBin(&c[i], 1);
		if(tmp[0] == '1')
			cont = 1;
		else
			cont = 0;
		free(tmp);
		i++;
	}while(cont == 1);
	*sOut = i * 7;
	char *cOut = malloc(*sOut + 1);
	for(int j = 0; j < i; j++){
		char *tmp = charsToBin(&c[j], 1);
		strncpy(&cOut[j * 7],&tmp[1], 7);
	}
	cOut[*sOut] = '\0';
	*nOut = strtol(cOut, NULL, 2);
	*sOut = i;
}

int main(int argc, char* argv[]){
	//int midiType = 0;
	//int trackNum = 0;
	//int timeNum = 0;

	for(int i = 2; i < argc; i++){
		if(argv[i][0] == '-'){
			if(argv[i][1] == 'd'){
				DEBUG = 1;
			}
			if(argv[i][1] == 'm'){
				MIN_DT = atoi(&argv[i][2]);
			}
		}
	}

	firstNote = newNote(-1);
	currentChord = firstNote;
	currentNote = firstNote;
	
	char *buffer;
	FILE *midi = fopen(argv[1], "rb");
	if(midi != NULL){
		//Get size of file
		fseek(midi, 0, SEEK_END);
		long size = ftell(midi);
		rewind(midi);

		//Read into string
		buffer = malloc(size);
		if(buffer != NULL && size > 14){
			fread(buffer, size, 1, midi);

			//Check for correct header
			if(strncmp(buffer, "MThd", 4) != 0){
				printf("Error: probably not a midi file\n");
				free(buffer);
				return 0;
			}

			//Read rest of header data
			//midiType = buffer[9];
			//printf("midi type: %d\n", midiType);
			char *trackBin = charsToBin(&buffer[10], 2);
			//trackNum = strtol(trackBin, NULL, 2);
			free(trackBin);
			//printf("track Num: %d\n", trackNum);
			char *timeBin = charsToBin(&buffer[12], 2);
			if(timeBin[0] == '1'){
				//timeDiv = fps;
				char SMPTEbin[7];
				strncpy(SMPTEbin, &timeBin[1], 7);
				//SMPTEframes = strtol(&timeBin[8], NULL, 2);
			}else{
				//timeDiv = tpb;
				//timeNum = strtol(&timeBin[1], NULL, 2);
			}
			free(timeBin);

			//printf("Time num: %d\n", timeNum);	

			int prevOn = 0;
			int notesOn = 0;
			int addingNotes = 0;
			int minDT = 200000000;
			int i = 14;
			while(i < size){
				if(strncmp(&buffer[i], "MTrk", 4) == 0){
					char *sizeBin = charsToBin(&buffer[i + 4], 4);
					//printf("sizeBin: %s\n", sizeBin);
					int chunkSize = strtol(sizeBin, NULL, 2);
					//printf("Chunk Size: %d\n", chunkSize);
					i = i + 8;
					while(strncmp(&buffer[i], "MTrk", 4) != 0 && i < size){
						//printf("----------------------\n");
						int deltaT;
						int deltaSize;
						varLen(&buffer[i], &deltaT, &deltaSize);
						//printf("deltaT: %d, deltaSize: %d\n", deltaT, deltaSize);

						i += deltaSize;

						char *eventTypeBin = charsToBin(&buffer[i], 1);
						char eventTypeBin2[5];
						strncpy(eventTypeBin2, eventTypeBin, 4);
						eventTypeBin2[4] = '\0';	
						int eventTypeNum = strtol(eventTypeBin2, NULL, 2);
						int eventTypeNum2 = strtol(&eventTypeBin[4], NULL, 2);
						//printf("eventTypeNum: %d, eventTypeNum2: %d\n", eventTypeNum, eventTypeNum2);
						i++;

						if(eventTypeNum == 15){
							//printf("META OR EXCLUSIVE\n");
							if(eventTypeNum2 == 15){
								//META EVENT
								char *metaEventBin = charsToBin(&buffer[i], 1);
								i++;
								int metaEvent = strtol(metaEventBin, NULL, 2);
								free(metaEventBin);
								//printf("metaEvent: %d\n",metaEvent);

								int lengthSize;
								int metaLength;
								varLen(&buffer[i], &metaLength, &lengthSize);
								//printf("metaLength: %d, %d\n", metaLength, i);
								i += metaLength + 1;
							}else{
								//SYSTEM EXCLUSIVE EVENT
								int lengthSize;
								int sysLen;
								varLen(&buffer[i], &sysLen, &lengthSize);
								i += sysLen + 1;
							}
						}else{
							if(eventTypeNum == 0 && DEBUG)
								printf("Note: This midi file contains a non-standard event, if it ends up not working, try deleting an empty track or something, idk.\n");
							//printf("MIDI CHANNEL EVENT: %d  %d\n", eventTypeNum, eventTypeNum2);
							//MIDI CHANNEL EVENT
							char *param1Bin = charsToBin(&buffer[i], 1);
							int param1 = strtol(param1Bin, NULL, 2);
							char *param2Bin = charsToBin(&buffer[i + 1], 1);
							int param2 = strtol(param2Bin, NULL, 2);
							//printf("param1: %d, param2: %d\n", param1, param2);
							if(eventTypeNum == 9){
								//printf("Note event");
								//Note On
								if(addingNotes == 0){
									addingNotes = 1;
								}
								if(addingNotes != 2){
									if(notesOn == 0 && deltaT > MIN_DT){
										if(deltaT < minDT)
											minDT = deltaT;
										//Add rest
										currentChord->next = newNote(-1);
										currentChord = currentChord->next;
										currentNote = currentChord;
									}
									if(prevOn == 1 && deltaT == 0){
										//Add note to chord
										currentNote->down = newNote(param1);
										currentNote = currentNote->down;
									}else{
										//Add note not to chord
										currentChord->next = newNote(param1);
										currentChord = currentChord->next;
										currentNote = currentChord;
									}
									notesOn++;
									prevOn = 1;
								}
							}else if(eventTypeNum == 8){
								//Note Off
								notesOn--;
								if(deltaT != 0){
									prevOn = 0;
								}
							}else if(deltaT != 0 && eventTypeNum != 10){
								prevOn = 0;
							}

							if(eventTypeNum == 12 || eventTypeNum == 13){
								i++;
							}else{
								i += 2;
							}
						}
						free(eventTypeBin);
					}

					if(addingNotes == 1){
						addingNotes = 2;
					}
					free(sizeBin);
				}else{
					printf("Error: could not find header for chunk at position %d\n", i);
					break;
				}
			}
			if(DEBUG)
				printf("If there are more rests than there should be, run again with '-m%d'\n", minDT);

			free(buffer);
		}else{
			printf("Error: file is not large enough to be MIDI\n");
			return 0;
		}
	}else{
		printf("Error: file does not exist\n");
		return 0;
	}
	fclose(midi);
	if(firstNote->next == NULL){
		printf("Error: there are no notes");
	}
	
	
	//The actual programming language part of things
	currentChord = firstNote;
	noteNum = 0;
	currentNote = currentChord;
	int labelNum = 0;
	for(int i = 0; i < 128; i++){
		lists[i] = newList();
		for(int j = 0; j < 5; j++){
			labels[i][j] = 0;
		}
	}

	if(DEBUG){
		int count = 0;
		do{
			currentChord = currentChord->next;
			currentNote = currentChord;
				printf("( ");
			do{
				printf("%d ", currentNote->val);
				currentNote = currentNote->down;
			}while(currentNote != NULL);
			printf(")");
			count++;
			if(count == 10){
				printf("\n");
				count = 0;
			}
		}while(currentChord->next != NULL);
		printf("\n\n");
		currentChord = firstNote;
		noteNum = 0;
		currentNote = currentChord;
		printf("About to establish the jump labels\n");
	}

	//Establish the jump labels
	do{
		currentChord = currentChord->next;
		currentNote = currentChord;
		noteNum++;

		if(currentChord->down == NULL || (currentChord->down->down == NULL && currentChord->down->val % 12 == currentChord->val % 12)){
			processLocation();	
		}else if(currentChord->down->down == NULL){
			processLocation();
			processValue();
		}else if(currentChord->down->down->down == NULL){
			processLocation();
		}else if(currentChord->down->down->down->down == NULL){
			//LABEL
			int *notes = chordToArray(4);
			currentChord = currentChord->next;
			currentNote = currentChord;
			noteNum++;
			int chord = chordSize(currentChord);
			if(chord == 0 || chord > 3){
				//It's definitely a label
				labels[labelNum][0] = noteNum + 1;
				for(int i = 0; i < 4; i++){
					labels[labelNum][i + 1] = notes[i];
				}
				labelChords[labelNum] = currentChord;
				labelNum++;
			}else{
				processValue();
				processValue();
			}
		}else{
			printf("Error at note %d, expecting statement declaration, but the chord has more than 4 notes.", noteNum);
		}
	}while(currentChord->next != NULL);

	noteNum = 0;
	currentNote = firstNote;
	currentChord = currentNote;

	//The main loop
	do{
		currentChord = currentChord->next;
		currentNote = currentChord;
		noteNum++;
		if(DEBUG)
			printf("STARTING MAIN LOOP\n");
		if(currentChord->down == NULL || (currentChord->down->down == NULL && currentChord->down->val % 12 == currentChord->val % 12)){
			if(DEBUG){
				printf("Waiting for Input at %d\n", noteNum);
				printf("This is because of %d at noteNum:%d\n", currentChord->val, noteNum);
			}
			//INPUT
			int input = 0;
			scanf("%d", &input);
			struct list *l = processLocation();
			l->val = input;
			currentChord = currentChord->next;
			currentNote = currentChord;
			noteNum++;
		}else if(currentChord->down->down == NULL){
			//ASSIGN
			struct list *l = processLocation();
			int procVal = processValue();
			if(DEBUG)
				printf("Storing %d somewhere (noteNum:%d)\n", procVal, noteNum);
			l->val = procVal;
		}else if(currentChord->down->down->down == NULL){
			//OUTPUT OR PRINT
			if(DEBUG)
				printf("Going to be outputting something at %d\n\n", noteNum);
			int *notes = chordToArray(3);
			int output = processLocation()->val;
			if(notes[1] - notes[0] >= notes[2] - notes[1]){
				//OUTPUT
				printf("%d", output);
			}else{
				//PRINT
				printf("%c", output);
				if(DEBUG)
					printf("(That character is number %d)", output);
			}
			if(DEBUG){
				printf("\n\n");
			}
		}else if(currentChord->down->down->down->down == NULL){
			//JUMP LABEL
			if(DEBUG)
				printf("Jump label found at %d", noteNum);
			int *notes = chordToArray(4);
			currentChord = currentChord->next;
			currentNote = currentChord;
			noteNum++;
			int chord = chordSize(currentChord);
			int jumpBool = 0;
			int num1 = 0;
			int num2 = 0;
			int interval = 0;
			if(chord == 2){
				interval = abs(currentChord->val - currentChord->down->val);
			}
			if(chord > 0 && chord < 4){
				num1 = processValue();
				num2 = processValue();
			}
			if(chord == 1){
				//EQUALS
				if(num1 == num2){
					jumpBool = 1;
				}
			}else if(chord == 2){
				//GREATER OR LESS
				if(interval % 2 == 0){
					//GREATER
					if(num1 > num2){
						jumpBool = 1;
					}
				}else{
					//LESS
					if(num1 < num2){
						jumpBool = 1;
					}
				}
			}else if(chord == 3){
				//NOT EQUAL
				if(num1 != num2){
					jumpBool = 1;
				}
			}
			if(jumpBool == 1){
				//JUMP
				for(int i = 0; i < labelNum; i++){
					if(labels[i][1] == notes[0] && labels[i][2] == notes[1] && labels[i][3] == notes[2] && labels[i][4] == notes[3]){
						noteNum = labels[i][0];
						currentChord = labelChords[i];
						currentNote = currentChord;
						break;
					}
				}
			}
		}else{
			printf("Error: the chord designating the statement type at note %d has more than four notes\n", noteNum);
		}
	}while(currentChord->next != NULL);
	return 0;
}
