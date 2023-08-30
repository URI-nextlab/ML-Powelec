#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv){
    FILE* fp;
    float temp;
    int i,j;
    fp = fopen(argv[1],"r");
    for (i = 0; i < 13;i++){
        for (j = 0;j < 13;j++){
            fscanf(fp,"%f",&temp);
            printf("[%d,%d]%.20f\n",i,j,temp);
        }
    }
    fclose(fp);
    return 0;
}
