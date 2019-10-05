#include "common.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
int
main(int argc, char **argv)
{
	int isnum = 1;
	int isint = 0;
	float number;
	if(argc == 1){
		printf("Huh?\n");
		return 0;
	}else{

		for(unsigned i =0; i< strlen(argv[1]);i++){
			if (!isdigit(argv[1][i])){isnum=0;}
		}
		if(isnum){
			number = atof(argv[1]);	
		}else{printf("Huh?\n"); return 0;}	

		if (number == (int)number){
			isint = 1;
		}else{printf("Huh?\n");return 0;}
		if(isint == 0){
			printf("Huh?\n");
			return 0;
		}
		else{
			if (number > 12){
				printf("Overflow\n");
			}else if(number <= 0){
				printf("Huh?\n");
				return 0;
			}else{
				int product=1;
				for (int i = 1;i <= (int)number;i++){
					product = product * i;
				}
				printf("%d", product);
				printf("\n");
			}
		}
	}
	return 0;
}

//STILL INCOMPLETE
