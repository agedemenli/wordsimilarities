#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


const int MAX_LINE_LEN = 6000;
const int MAX_WORD_LEN = 20;
const int NUM_WORDS = 1000;
const int EMBEDDING_DIMENSION= 300;
const char DELIMITER[2] = "\t";

const int COMMAND_EXIT = 0;
const int COMMAND_QUERY = 1;
const int COMMAND_CALCULATE_SIMILARITY = 2;

void distributeEmbeddings(char *filename, int world_size){

    // I used the source code of the sample application that we covered during PS.
    // I modified it for this project.

    //printf( "Reading embedding file\n");
    char line[MAX_LINE_LEN];
    FILE *file = fopen (filename, "r" );
    int wordIndex = 0;
    int p = world_size;   // Master included

    for (int i = 1; i < p; i++) { // Iterates for each processor

        float* embeddings_matrix = (float*)malloc(sizeof(float) * NUM_WORDS/(p-1)*EMBEDDING_DIMENSION);
        char* words = (char*)malloc(sizeof(char) * NUM_WORDS/(p-1)*MAX_WORD_LEN);

        for(int j = 0; j<NUM_WORDS/(p-1); j++){

            fgets(line, MAX_LINE_LEN, file);
            char *word;
            word = strtok(line, DELIMITER);

            // I TRIED TO DO THIS PART USING STRCPY LIKE THE EXAMPLE SOURCE CODE GIVEN IN PS
            // I DON'T KNOW WHY BUT STRINGS CAME WRONG THIS WAY, MAYBE BECAUSE OF THE TURKISH CHARACTERS.

            // *********************************
            /*
            printf("%s%d=",word,strlen(word));
            *(word+strlen(word)) = NULL;
            strcpy(words+j*MAX_WORD_LEN, word);
            */
            // *********************************

            // SO I CREATED MY OWN STRCPY MANUALLY:)
            // strcpy
            int len = 0;
            while (*(word+len) != '\0'){
                *(words+j*MAX_WORD_LEN+len) = *(word+len);
                len++;
            }

            for (int k=len;k<MAX_WORD_LEN;k++)
                *(words+j*MAX_WORD_LEN+k) = NULL;
            // end of strcpy


            // The same as the source code of the sample application that we covered during PS
            // Fills the embeddings matrix
            for(int embIndex = 0; embIndex<EMBEDDING_DIMENSION; embIndex++){

                char *field = strtok(NULL, DELIMITER);
                float emb = strtof(field,NULL);
                *(embeddings_matrix+j*EMBEDDING_DIMENSION+embIndex) = emb;
            }
        }

        // Sending words to process i

        MPI_Send(
                /* data         = */ words,
                /* count        = */ NUM_WORDS/(p-1)*MAX_WORD_LEN,
                /* datatype     = */ MPI_CHAR,
                /* destination  = */ i,
                /* tag          = */ 0,
                /* communicator = */ MPI_COMM_WORLD);

        // Sending embeddings to process i

        MPI_Send(
                /* data         = */ embeddings_matrix,
                /* count        = */ NUM_WORDS/(p-1)*EMBEDDING_DIMENSION,
                /* datatype     = */ MPI_FLOAT,
                /* destination  = */ i,
                /* tag          = */ 0,
                /* communicator = */ MPI_COMM_WORLD);

        free(words);
        free(embeddings_matrix);
    }

    // Embedding file.. has been distributed
}

int findWordIndex(char *words, char *query_word){

    // I used this function as same as the source code of the sample application that we covered during PS
    // Finds and returns the index of the query word if it exists, -1 otherwise

    for(int wordIndex = 0; wordIndex<NUM_WORDS; wordIndex++){

        if(strcmp((words+wordIndex*MAX_WORD_LEN), query_word)==0){

            return wordIndex;
        }
    }
    return -1;
}


void runMasterNode(int world_rank,int world_size){

    // input file
    distributeEmbeddings("./word_embeddings_1000.txt",world_size);

    // I used the source code of the sample application that we covered during PS, with some modifications.

    while(1==1) {

        // Getting the input, query word.
        printf("Please type a query word:\n");
        char queryWord[256];
        scanf("%s", queryWord);
        printf("Query word:%s\n",queryWord);
        int p = world_size;
        int command = COMMAND_QUERY;

        // Checking if EXIT given.
        if (queryWord[0] == 'E' && queryWord[1] == 'X' && queryWord[2] == 'I' && queryWord[3] == 'T')
            command = COMMAND_EXIT;

        // Sending commands to the slave nodes.
        for (int i=1;i<p;i++) {

            // Command is being sent to process i

            MPI_Send(
                    /* data         = */ (void *) &command,
                    /* count        = */ 1,
                    /* datatype     = */ MPI_INT,
                    /* destination  = */ i,
                    /* tag          = */ 0,
                    /* communicator = */ MPI_COMM_WORLD);
        }

        // If EXIT is given, after sending this command to the slaves, MASTER terminates itself.
        if(command == COMMAND_EXIT){
            printf("EXIT given. Terminating.\n");
            return;
        }

        // Sending queries to the slaves.
        for (int i=1;i<p;i++) {

            // QueryWord is being sent to process i
            MPI_Send(
                    /* data         = */ queryWord,
                    /* count        = */ MAX_WORD_LEN,
                    /* datatype     = */ MPI_CHAR,
                    /* destination  = */ i,
                    /* tag          = */ 0,
                    /* communicator = */ MPI_COMM_WORLD);
        }

        int* indexes = (int*)calloc(p-1, sizeof(int));
        int index;
        float embedding_matrix[300];
        for (int i=1;i<p;i++) {

            // Collecting the word indexes from the slaves.
            MPI_Recv(
                    /* data         = */ &index,
                    /* count        = */ 1,
                    /* datatype     = */ MPI_INT,
                    /* source       = */ i,
                    /* tag          = */ 0,
                    /* communicator = */ MPI_COMM_WORLD,
                    /* status       = */ MPI_STATUS_IGNORE);
            indexes[i-1] = index;

            // Means we need to calculate similarity scores.
            // Our word is found by one of the slaves.
            if(index != -1){
                command = COMMAND_CALCULATE_SIMILARITY;
                MPI_Recv(
                        /* data         = */ embedding_matrix,
                        /* count        = */ 300,
                        /* datatype     = */ MPI_FLOAT,
                        /* source       = */ i,
                        /* tag          = */ 0,
                        /* communicator = */ MPI_COMM_WORLD,
                        /* status       = */ MPI_STATUS_IGNORE);

            }
        }

        // Sending commands to the slaves.
        for (int i=1;i<p;i++) {
            MPI_Send(
                    /* data         = */ &command,
                    /* count        = */ 1,
                    /* datatype     = */ MPI_INT,
                    /* destination  = */ i,
                    /* tag          = */ 0,
                    /* communicator = */ MPI_COMM_WORLD);
        }

        // If we need to calculate the similarity;
        if(command == COMMAND_CALCULATE_SIMILARITY){

            // Sending embedding matrix to the slaves for calculation.
            for (int i=1;i<p;i++) {
                MPI_Send(
                        /* data         = */ embedding_matrix,
                        /* count        = */ 300,
                        /* datatype     = */ MPI_FLOAT,
                        /* destination  = */ i,
                        /* tag          = */ 0,
                        /* communicator = */ MPI_COMM_WORLD);
            }

            float* similarityScores = (float*)calloc(p-1, sizeof(float));
            char* word = (char*)calloc(MAX_WORD_LEN, sizeof(char));
            char* words = (char*)calloc((p-1)*MAX_WORD_LEN, sizeof(char));
            float similarityScore;

            for (int i=1;i<p;i++) {

                // Recieving the word from each slave.
                MPI_Recv(
                        /* data         = */ word,
                        /* count        = */ MAX_WORD_LEN,
                        /* datatype     = */ MPI_CHAR,
                        /* source       = */ i,
                        /* tag          = */ 0,
                        /* communicator = */ MPI_COMM_WORLD,
                        /* status       = */ MPI_STATUS_IGNORE);

                // Recieving the similarity score for the corresponding word.
                MPI_Recv(
                        /* data         = */ &similarityScore,
                        /* count        = */ 1,
                        /* datatype     = */ MPI_FLOAT,
                        /* source       = */ i,
                        /* tag          = */ 0,
                        /* communicator = */ MPI_COMM_WORLD,
                        /* status       = */ MPI_STATUS_IGNORE);

                for (int j=0;j<MAX_WORD_LEN;j++) {

                    words[(i-1)*MAX_WORD_LEN+j] = word[j];
                }

                similarityScores[i-1] = similarityScore;
            }

            // Printing the words with their scores.

            printf("=======Query results: ========\n");
            for (int k=0;k<p-1;k++) {
                for (int i=0;i<MAX_WORD_LEN;i++) {
                    if(words[k*MAX_WORD_LEN+i]!=NULL)
                        printf("%c",words[k*MAX_WORD_LEN+i]);
                    else printf(" ");
                }
                printf("\tfound with the similarity score of %f\n",similarityScores[k]);
            }
            //printf("==============================\n");
        }
    }
}

void runSlaveNode(int world_rank,int p){

    // I used the same function of the source code of the sample application that we covered during PS, with some modifications.
    char* words = (char*)malloc(sizeof(char) * NUM_WORDS/(p-1)*MAX_WORD_LEN);

    // Recieving the words.
    MPI_Recv(words, NUM_WORDS/(p-1)*MAX_WORD_LEN, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Words received in the process

    float* embeddings_matrix = (float*)malloc(sizeof(float) * NUM_WORDS/(p-1)*EMBEDDING_DIMENSION);

    // Recieving the embeddings matrix.
    MPI_Recv(embeddings_matrix, NUM_WORDS/(p-1)*EMBEDDING_DIMENSION, MPI_FLOAT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);

    // Process received its embedding part
    while(1==1){

        // Slave is waiting for a command
        // Recieving the command from the MASTER.
        int command;
        MPI_Recv(
                /* data         = */ &command,
                /* count        = */ 1,
                /* datatype     = */ MPI_INT,
                /* source       = */ 0,
                /* tag          = */ 0,
                /* communicator = */ MPI_COMM_WORLD,
                /* status       = */ MPI_STATUS_IGNORE);

        // Command received by the process from the MASTER

        // If the EXIT COMMAND is recieved, free the memory we got, terminate the slave and return.
        if(command == COMMAND_EXIT){

            free(words);
            free(embeddings_matrix);
            return;
        }

        // If the QUERY COMMAND is recieved;
        else if(command == COMMAND_QUERY){

            char* query_word = (char*)malloc(sizeof(char) * MAX_WORD_LEN);

            // Recieving the query word from the MASTER
            MPI_Recv(
                    /* data         = */ query_word,
                    /* count        = */ MAX_WORD_LEN,
                    /* datatype     = */ MPI_CHAR,
                    /* source       = */ 0,
                    /* tag          = */ 0,
                    /* communicator = */ MPI_COMM_WORLD,
                    /* status       = */ MPI_STATUS_IGNORE);

            // QueryWord received by the process

            int indexfound = findWordIndex(words, query_word);

            // Sending the index of the query word if it is found, -1 otherwise.
            MPI_Send(
                        /* data         = */ &indexfound,
                        /* count        = */ 1,
                        /* datatype     = */ MPI_INT,
                        /* destination  = */ 0,
                        /* tag          = */ 0,
                        /* communicator = */ MPI_COMM_WORLD);

            // If the word is not found, sends emmeddings matrix to the MASTER.
            // If calculating the similarity score is necessary, the slave will receive it back.
            if(indexfound != -1){
                MPI_Send(
                    /* data         = */ embeddings_matrix+EMBEDDING_DIMENSION*indexfound,
                    /* count        = */ 300,
                    /* datatype     = */ MPI_FLOAT,
                    /* destination  = */ 0,
                    /* tag          = */ 0,
                    /* communicator = */ MPI_COMM_WORLD);
                }
            int the_new_command;
            // Again, recieving a command from the MASTER, if it is CALCULATE, the slave will calculate the similarity.
            MPI_Recv(
                    /* data         = */ &the_new_command,
                    /* count        = */ 1,
                    /* datatype     = */ MPI_INT,
                    /* source       = */ 0,
                    /* tag          = */ 0,
                    /* communicator = */ MPI_COMM_WORLD,
                    /* status       = */ MPI_STATUS_IGNORE);

            // If the new command is CALCULATE;
            if(the_new_command == COMMAND_CALCULATE_SIMILARITY){

                float taken_matrix[300];
                MPI_Recv(
                    /* data         = */ taken_matrix,
                    /* count        = */ 300,
                    /* datatype     = */ MPI_FLOAT,
                    /* source       = */ 0,
                    /* tag          = */ 0,
                    /* communicator = */ MPI_COMM_WORLD,
                    /* status       = */ MPI_STATUS_IGNORE);


                int mostSimilarWordIndex = -1;
                float maxScore = 0;

                // Calculateing the score by Cosine Similarity.

                for(int wordIndex = 0; wordIndex<NUM_WORDS/(p-1); wordIndex++){
                    float similarity = 0.0;

                    for(int embIndex = 0; embIndex<EMBEDDING_DIMENSION; embIndex++)
                        similarity +=(taken_matrix[embIndex]*(*(embeddings_matrix + wordIndex*EMBEDDING_DIMENSION + embIndex)));

                    // Updating the MAX if it is necessary.
                    if(similarity > maxScore){
                        mostSimilarWordIndex = wordIndex;
                        maxScore = similarity;
                    }
                }

                // Similarities was calculated and being sent to the MASTER with the words

                // Sending the most similar word to the MASTER back.
                MPI_Send(
                    /* data         = */ words+mostSimilarWordIndex*MAX_WORD_LEN,
                    /* count        = */ MAX_WORD_LEN,
                    /* datatype     = */ MPI_CHAR,
                    /* destination  = */ 0,
                    /* tag          = */ 0,
                    /* communicator = */ MPI_COMM_WORLD);

                // Sending the maximum similarity score to the MASTER.
                MPI_Send(
                    /* data         = */ &maxScore,
                    /* count        = */ 1,
                    /* datatype     = */ MPI_FLOAT,
                    /* destination  = */ 0,
                    /* tag          = */ 0,
                    /* communicator = */ MPI_COMM_WORLD);
            }
        }
    }
}

int main(int argc, char** argv) {

    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    // Print off a hello world message

    // We are assuming at least 2 processes for this task
    if (world_size < 2) {
        fprintf(stderr, "World size must be greater than 1 for %s\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int wordIndex;

    if (world_rank == 0) {
        runMasterNode(world_rank,world_size);


    } else {

        runSlaveNode(world_rank,world_size);

    }

    MPI_Finalize();


}
