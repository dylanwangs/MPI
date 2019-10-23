#include "mpi.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
FILE *fp;
int row_col[1];
int **Mat;
int **output;
int **final;

//read matrix from binary format
void fileread(char *file){
  int rownum;
  int colnum;
  int a = 0;
  int token[1];

  if((fp  = fopen(file, "rb")) == NULL){
    printf("couldn't find input matrix file %s\n", file);
    exit(1);
  }

  fread(row_col,sizeof(row_col),1,fp);

  Mat = (int **)malloc(row_col[0] * sizeof(int*));
  for (a=0; a<row_col[0]; a++){
      Mat[a] = (int *)malloc( row_col[0] * sizeof(int));
  }

  for(rownum=0; rownum<row_col[0];rownum++){
    for(colnum=0; colnum<row_col[0]; colnum++){
      fread(token,sizeof(token),1,fp);
      Mat[rownum][colnum] = token[0];
    }
  }
}

//implemented Floyd Warshal algorism finding the shortest path
void getshortpath(){
  int row;
  int col;
  int a;
  int my_rank;
  int size;
  int point;
  MPI_Request request;
  MPI_Status status;

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  // allocate memory for 2D array
  output = (int **)malloc(row_col[0] * sizeof(int*));
  for (a=0; a<row_col[0]; a++){
      output[a] = (int *)malloc( row_col[0] * sizeof(int));
  }

 //allocate memory for result 2D array
  final =  (int **)malloc(row_col[0] * sizeof(int*));
  for (a=0; a<row_col[0]; a++){
      final[a] = (int *)malloc( row_col[0] * sizeof(int));
  }
    //get chunk size based on rows
    int chunk_size = row_col[0]/(size);
    int remind; //chunk size for reminder node (last node)
    int previous; //each node's first row number
    int until; //each node's first row number

      //divide up tasks
      if(my_rank == (size-1)){
         previous = my_rank * chunk_size;
         until = row_col[0];
         remind = until - previous;
      }
      else{
       previous = my_rank * chunk_size;
       until = previous + chunk_size;
      }
      //algorism based on each node
      for(row=previous; row<until;row++){
        for(col=0;col<row_col[0];col++){
          if(row==col){
            output[row][col] = 0;
          }
          else{
            int Min = Mat[row][col];
            int intercount;
            for(intercount=0; intercount<row_col[0]; intercount++){
              if( (Mat[row][intercount] != 0) && (Mat[intercount][col] != 0) ){
                int token = Mat[row][intercount] + Mat[intercount][col];
                if((Min==0) || ((Min>token) && (Min!=0))){
                  Min = token;
                }
              }
            }
            output[row][col] = Min;
          }
        }
      }

    //send back results to last node row by row
    for(point=0; point<chunk_size;point++){
    if(my_rank==(size-1)){
      int r;
      for(r=(size-2); r>=0; r--){
      MPI_Recv(output[r*chunk_size+point], row_col[0], MPI_INT, r, 0,MPI_COMM_WORLD,&status);
    }
    }
    else{
      MPI_Send(output[previous+point],row_col[0], MPI_INT, size-1, 0, MPI_COMM_WORLD);
    }
    }
        //testing result
        /*if(my_rank==(size-1)){
          int b;
        for(a=0;a<row_col[0];a++){
          for(b=0;b<row_col[0];b++){
            printf("%d ", output[a][b]);
          }
          printf("\n");
        }
      }*/
}

//write file function
void filewrite(char *f){
  char *out;
  char outname[1000];
  FILE *fw;
  int r;
  int c;
  int tokenw[1];

  out = strtok(f, ".");
  sprintf(outname, "%s.out", out);
  fw = fopen(outname, "wb");

  for(r=0;r<row_col[0];r++){
    for(c=0;c<row_col[0];c++){
      tokenw[0] = final[r][c];
      fwrite(tokenw,sizeof(tokenw),1,fw);
    }
  }

}


int main(int argc, char** argv) {
        int rank;
        int sizea;
        int opt;
        int fflag = 0, errflag =0;
        int **out;
        char filename[1000];
        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &sizea);
        struct timeval start, end;
        //check how many argument
        if(argc > 3){
          printf("too many arguments were taken, please check again\n");
          return -1;
        }
        if(argc<2){
          printf("Too few arguments were taken, please use -f filename or filename to compile\n");
          return -1;
        }
        //getopt to assign flag
        while ((opt = getopt (argc, argv, "f:")) != -1){
           switch (opt){
             case 'f':
                  fflag++;
                  strcpy(filename, optarg);
                  break;
           }
        }

        if(errflag){
          printf("Can not recognize your command option, please use -f\n");
          exit(1);
        }
        if(!fflag){
          strcpy(filename, argv[1]);
        }


        fileread(filename);
        gettimeofday(&start, NULL);
        getshortpath();
        gettimeofday(&end, NULL);
        //get time taken
        double delta = ((end.tv_sec - start.tv_sec) * 1000000u + end.tv_usec - start.tv_usec) / 1.e6;

        if(rank==(sizea-1)){
        filewrite(filename);
        printf("time elapsed = %12.10f on rank %d\n", delta, rank);
        //only print the time taken for last node(as it's the longgest)
        }
        MPI_Finalize();
        return 0;
}
