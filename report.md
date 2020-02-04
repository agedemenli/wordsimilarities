# MPI Application Report 
> Created by Ahmet Gedemenli, 2014400147



<br><br><br><br>
## Introduction
In this project, first of all, I needed some research about open-mpi since I don't have any experience about parallel programming. I had to get used to `malloc` and `calloc`. At the end of the day my code works exactly the way described in the project text. It lists P similar words, each from one processor, again and again until the command **EXIT** is given as a query. When it is given, the code ends the session and terminates itself. Before I started to implement the _bonus part_, I saved my working code as `yedek.c` which you can find in my submission. 

After that, all my work is on the other code, `kod.c`. This one also works fine. It correctly calculates the similarities and lists the **most** similar P words, as described in the _bonus part_. However, unlike the other one, this one does not have an infinite session. After answering the first query, correctly finding and listing the P **most** similar words with their similarity scores, somehow, it terminates itself with an error. I couldn't solve it since I got tons of stuff to do , and unfortunately, I had to submit it as it is. Maybe the problem was about my computer, production environment or the operating system; so **please try `kod.c` on your local machine**.

The source code we covered at PS was very useful for me, it made everything a bit easier. The main structures of my, both, codes is the same of the example code given, since I worked on it. I modified and expanded it for this project. 

In this report I'm going to focus on the main modifications and implementations, which does not exist in the example code, of my source code; i.e main differences between my source code and the example one. Throughout the report, you will see a lot of parts from my code with explanations. Also, you can find **lots of comments** in the codes if you want to check out. The codes actually document and report themselves.

## `void distributeEmbeddings(char *filename, int p)`
* My distributeEmbeddings function is very similar to the one in the source code we covered at PS. I added one more parameter, `int p` because I need the processor number in the function. 

* Additionally, I put it in a larger loop scope, `for (int i = 1; i < p; i++) {` because I needed to distribute the input to the processors equally. But unlikely, the example code just sends the `words` and `embeddings_matrix` to only one. So I solved this difference with this modification.

* The problem I encountered while implementing this function was `strcpy`. Somehow, `strcpy` didn't work the way it's meant to work. So I solved this problem by implementing my own `strcpy` manually, which you can see below.

```
int len = 0;
while (*(word+len) != '\0'){
	*(words+j*MAX_WORD_LEN+len) = *(word+len);
	len++;
}
for (int k=len;k<MAX_WORD_LEN;k++) 
	*(words+j*MAX_WORD_LEN+k) = NULL;
```
## `int findWordIndex(char *words, char *query_word)`
This function is totally the same as the example code given. I modified it for only coding style. The functionality is the same as the given.
## `void runMasterNode(int world_rank, int p)`
* Differently from the example code, I took one more parameter here. `int p` is the number of processors.
* After calling `distributeEmbeddings` and entering into the loop `while(1==1)` like the example code, this function sends commands to the slave nodes. 
* Command here is, 0 if 'EXIT' is given, 1 otherwise. If the command is 1, then it sends the query to each slave node and waits for receiving the answer. 
* The answer here, is the index of the query word. If a slave node finds the query word in its word pool, it returns the index, -1 otherwise. I designed this part like this because **the master has to know, if any of the processors have the query word**. 
* If no processor finds the query word in its scope, then no output is printed. If one of the slave nodes finds the query word, then the variable `command` becomes 2, which means **CALCULATE_SIMILARITY**.
```
if(index != -1){ 
      command = COMMAND_CALCULATE_SIMILARITY;
```
* After this, MASTER sends commands each one of the slaves to calculate similarity by:
```
for (int i=1;i<p;i++) {
       MPI_Send(
           /* data         = */ &command,
           /* count        = */ 1,
           /* datatype     = */ MPI_INT,
           /* destination  = */ i,
           /* tag          = */ 0,
           /* communicator = */ MPI_COMM_WORLD);
   }
```
* After that, MASTER receives the **most similar P words and their scores** from **each of the P slaves**. For the example given in the project description, right now we have 100 words at MASTER, the most similar 10 from each of the 10 slaves.
* After receiving the words and the scores, MASTER writes them on its own arrays.
* Here I put the whole receiving part since it's a very important part of the MASTER function, for the bonus part.
```
// We need to get P-1 words with their scores from each of the P-1 processors.
for (int i=1;i<p;i++){                
    for(int k=1;k<p;k++){
	    // Recieving the words ont by one from the slave i.
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

        for (int j = 0; j < MAX_WORD_LEN; ++j) {
                words[((i-1)*(p-1)+k-1)*MAX_WORD_LEN+j] = word[j];
        }
         similarityScores[(i-1)*(p-1)+k-1] = similarityScore;   
    }
}
```
* After receiving the P*P similar words, MASTER finds the most similar P. In our example in the project description, MASTER  finds 10 out of 100 words.
* Here in order to find the P words with the biggest scores, I preferred to iterate those P*P words P times; instead of sorting them.
* For that, I needed `int* used = (int*)calloc((p-1)*(p-1), sizeof(int));` This array holds 0 if the word with the corresponding index is not used, i.e not printed as output, while it holds 1 if the word with the corresponding index is used, i.e printed as output.
* In each iteration it checks `if( (*(used+i))!=1 && *(similarityScores+i) > maxScore)` for every word that MASTER got. This way it founds **a non-used word with the maximum similarity score**. 
* Of course, after each iteration, the word found is marked as used, in order not to find it again as maximum. `*(used+maxIndex) = 1;`
* In each iteration it prints one word and after P iterations, the most similar P words are being listed in the order of descending scores.
## `void runSlaveNode(int world_rank, int p)`
* Again, differently from the example code, I took one more parameter here. `int p` is the number of processors.
* Like the same function of the example code, firstly it receives `words` and `embeddings_matrix`
* After beginning to loop `while(1==1){`, it receives a `command` from the MASTER.
* If the command is EXIT, the slave frees the arrays it took, then returns.
```
int command;
MPI_Recv(
     /* data         = */ &command,
     /* count        = */ 1,
     /* datatype     = */ MPI_INT,
     /* source       = */ 0,
     /* tag          = */ 0,
     /* communicator = */ MPI_COMM_WORLD,
     /* status       = */ MPI_STATUS_IGNORE);
//printf("Command received:%d by process %d\n",command, world_rank);
// If the EXIT COMMAND is recieved, free the memory we got, terminate the slave and return.
if(command == COMMAND_EXIT){            
      free(words);
      free(embeddings_matrix);
      return;
}
```
* If the command received is not `COMMAND_QUERY`, which is 1, then the loop finishes there. Slave starts to wait for a new command. If it is 1, then it continues, receives the query etc.
* After receiving the query word, slave tries to find it amongst the words. To do that, it calls the `findWordIndex` function as `int wordIndex = findWordIndex(words, query_word);`
* Here `wordIndex` holds the index of `query_word` in `words`. Holds -1 if it is not found. Actually it's mostly -1 because for each query, maximum 1 slave can find it.
* After finding the word index, or -1, the slave sends it to the MASTER.
* Slave starts to wait for receiving a new command from the MASTER, which will be `CALCULATE_SIMILARITY`, means 2, if calculating the similarities is necessary.
* If the new command comes as 0 or 1, the loop ends there and no calculations will be made. In the big picture, the design of the algorithm, in means **there is no such word**. If the query word is not found in our word pool, then no calculations will be made.
* If the slave receives a calculation command, it calculates the similarity for each word.
```
for(int embIndex = 0; embIndex<EMBEDDING_DIMENSION; embIndex++)
   similarity+=(taken_matrix[embIndex]*(*(embeddings_matrix+wordIndex*EMBEDDING_DIMENSION+embIndex)));
```
* If the calculated similarity is enough big to be in the **list of first P words**, it takes the place of the last element of this list. Then the slave starts to **push the new word through the front as much as possible while its score is bigger than the next word's score**.
* Since this is a very important part for the bonus part, I put the _updating the list_ part here.
```
// If the similarity score is bigger than the last(the smallest) member of the list of biggest elements,
// it means we need to remove the last one, put the new one.
if(similarity > *(topScores+p-2)){
     *(similarWordIndexes+p-2) = wordIndex;
     *(topScores+p-2) = similarity;
     // Pushing the new element to the front as much as possible.
     for(int i=p-2;i>0;i--){
		 if(*(topScores+i-1) > *(topScores+i))
               break;
		 // Swapping the elements in topScores
         float tmpf = *(topScores+i-1);
         *(topScores+i-1) = *(topScores+i);
         *(topScores+i) = tmpf;
         // Swapping the elements in similarWordIndexes
         int tmpi = *(similarWordIndexes+i-1);
         *(similarWordIndexes+i-1) = *(similarWordIndexes+i);
         *(similarWordIndexes+i) = tmpi;
     }
}
```
* After all the iterations are finished, the slave has the **most similar P words with their scores**.
* Then the slave sends the MASTER the most similar P words with their scores.
* Since it would be more simple to implement for me, the slave sends all them one by one, NOT as an array.
## `int main(int argc, char** argv)`
* It's the same as in the source code we covered at PS.

## Expected Outputs
Includes the outputs of both `kod.c` and `yedek.c`
## The outputs of `kod.c` with the bonus part
![](https://lh3.googleusercontent.com/UdNc48zCKulEyF7kRGGTJvk1cdIpLnhLrF6l8XiUGqBIGgFxBUFMYNHtN2sp-p5miAIO35fpc09TWYhh1C_ef0ofxgowRQ_lQA5T86EckBgcVHVLa4ft4DetBekMD7TyHMsUJ78QEPbZMPpAQcnuc90ZNVAtndi0QBSCQDWQ-OwCSHH838lFGo2FgbcT0zg7swXmpApCi9cx8uOIVfnTJDzGzAQZFG-NH5-OycAUHNgQQ3ZuuV5E5zlChkuFWgtUpGaXr0dXC_RgXHOVOOX6r-Lm_kbzVcAirwwS4UJ0iqQDzhN9I7Rr_T7-hLWqQaRj4YKGK91bcvxFqK8Aig3mRhWePPrNngU86U33WFt0iKniN0RiJaswngH8D7zBUhqtK-8_SNBzHOetLTiGeKNIotRhxX89PyuEs_S3G8dip8Nn8rhDtYXcmX5uj2jRQlpIjHU7dVzoQd1ggB4j3yuRj287sNHNz4Bxq3WOlZO8MkhFjscgnjBO0L0LNPIhv4tRghvVsfF9PICEcaWWWsaQ6bvE3QLO6Bh261K79DeanjbZlpwwRCmJkPYCpKiY6ctfdsCgfl9dAAhfxSK83fHjv6UFo0Gnz0EtlLfZEG5u0qRVvcxwsfFXvTcybZmknydVXj5nxvyGET82UCgMgFUoOmnmL0SLP1U=w1040-h2096-no)
![enter image description here](https://lh3.googleusercontent.com/BDasfKQM_6swaUHTqcAs6zOwqrNzYuCXAX6AB8k0TQX7jfelOOGn10rm8-wHIQks8KGMYcLX7jSNwjDQ6jnQ-ZKIw1oOOYcSrir0Zg7IGCXtefsb11HPCuIPsZlOmID3mDD8m9mSVfBBlXYvPqkw-ZBXm2r4CpxsDFqQNAvFgSa7yriAwl0XDKptGvysOpkRX32Opf0XPhTlsMYjkpfnK-flehY6uh85uBvDR1aVTxkhJhRpSqe7_4CjtrAOBtfgtdN_JdijmtmY_BL_W2mYryrvJLpQjTA0Rxgy0V52ZzdMv_CpjfFs1p86eQTqczBItlmijCnORwFfGSuONWZT7RBm0BmlVzjLUa1PiPj-LRxtenrp_QH-EqexaTqnLAM8KPvS6c4LhLbZ5DtIOq_0UOm2151mg9APK5f5_jhBfeYs2NLzhq9PNUyZcKWf6J9M6J88OvQ1XJuZkOYWkpPtrlfE8Z9GKN2XEFB1qW2hmTcw-NscO_SvDozYo9fANDi-GivMY_uvmK1jUcF35dGbqVY3yAEXiYuyw9f5GfKhw7MtsHs9MX_CpmFm9lQOFliYXllKsy2LbFyADSG7wR2Y_1IE7sn7F7wNlLsY9EM9lG09pWnG82WK7YzKraefUX__JiHrGQXSPEUJpankawWQJklI48Vr_h0=w1046-h2096-no)
![enter image description here](https://lh3.googleusercontent.com/AX4-KY38vkV-JIs360DxfpV12iN4ItQMmfoC8avkfRNNZkaweAUnmdtEtcE78H9G7IL_nWHn4UI1yUFxkoxFpdQeulj-X9byXhDZZvbx7jkV0vvVqG0NMkvHuaqOgXOcWBNBVYzsDsr5ZAjRwXOVmYAUDtUBg-JC50zw1muEdIT3Tx2OZr91M01r3eN0anmcIUAbpPytrj-c87ddnoF-brKaMe3hkD46hyoGfkew6xrGCtTVPawr8N48teHASg7pvtJAa7i6pDiOSQHc1B8WhuMhUdrulK4mJv7_nKHO7F_YIHJm-1zxo9GmrYtmGaUyNXvTHSdGuDxRwrpAGN6iBKJw4gfWDq0mhAuluzUZpEblcFpJBqaW02eT7vdksxfMFL64EFaYyAdn1XjdPBMky2IUTJuXIjHRzF_W-Rc_dWZosW5pppKiDSxeo7GIj31sWO7e-Ue2p8caLYsp5awr3bJZL4nFVnjWyW_Q4z32ocN13MRwQQAvHmNOpOAnVKjtqTfLPtCywS8vXczDcomWFx5ad8-5BNo0XQah40gGOmcs9FvIVHjeAwS2wccSwO0GSQ_aQcCipI-cpYseIjy63jxGakRUF3xjgCfGROr0VGFT71zBn_sjEd3EpQJm2ehxHhViz_-UkUEn4RDX7eWcSD_QktMPAe4=w1050-h2096-no)
![enter image description here](https://lh3.googleusercontent.com/ig16bKp2lcpdr2pIwPPQ-AzQ--DuErSxQAjtZDG8HZdRku37Y2ByxdDrYTV6zn2YO-eLzExjUmW_c0SQOX3AlkeC80Xb8c1VXCQEZF7KcO6nfyrd8Kr4lGh-mWsWnEtEi3LvcseDJ_A5bJPpgmGfL7cuwRyB9fe6oFPNkbWepp9CoCeG9vyZDq0KVJ4UP4o2-xQqkwEW-0pyCc2tAH0shTqWjCJ1VfGroug_zsp4NhI0pAyK0OoAPY39v0uC29XKV7MQKjnhtngnnlqmEu3G2rVI5-MCLNmxm9WV1M2N_gB5Ww2pkzHbze83QWu_3F4jQNKfpc-DoEaHCykmw9dUoFjZWvj_eOPlHLf8ccDMnFneg-A-W4QzmKhz7s3WF8QTB1pEUHtyhMXuVNqiO6ICiunw77gxbjKx-Jds29S-cXQkcymXuUOYWlA3LVCMSh4QXc-3Cvv-rTyH1nFK_TU84Yo33PG4a2guxKactN1Y7hkBlbeu2m4ztrU5svo4huJty_AS3FiWIM0Z8iqlQB_Q-oWntuqaoV6B8kdweThfbAzhCiRV-baZlWq00Yloz8TEF3JqCcyHQFtyI9--5CXURmAj33EFHnxUh_B0oFzFZigIguj1NVraheO6mUb70qV5_1nDNDUoHhD2gdsB3KBGG9-ANjwy5HM=w1056-h820-no)
## The outputs of `yedek.c` without the bonus part
![enter image description here](https://lh3.googleusercontent.com/Y2BavQTwOvI2M20Vvzxf2SPTxq33pXtPjvTGOSEeEg8Je7__ClPgF1tgDNinAttzPSVdQPwQKvKfRbPgWqASEeMSjhYj2hqRWvOXgDuoTe8jYXiftlkETontvdKErK26Ez8AhPY9WTeBrErBH4l2m9U8sNIwoDGdPQE4sF3oPw5Jw9_3UPiMpfMxSPDnuoSJtVnzNRpeW4Qy-2kJ_XRzulyNhxjp_pE2zf356raDAOAImoicYWcqynleOjTMDfDk8NqgktwIwKO1BrDhFF5vAI5Fp9yS1YQvRPjiEkAkPyVZa3-ht0vL4_w2dNCxsUkKwKOQiTwGtRhhCG7eDt-kD20Fla0tYB9Th1GPqEuLNGp--zsR-P618O-jQsBkvp9UaEYu4eT9vmJlzWvCpZarvq-RIuuKt_w36cVhf2zHrYAdGtmxs3Kx68ApbioqRuZNVLMcbYOug9Iqbg-xzUhpm7AHlWatAGo9SSoYX-o0vmIWpNSlxjn1HPLMx5m1yJCOqcfTFgUX78kpKa-7txbKdB4nNGYW_d_XBjwzI3ApsPUQBHrRYKQkJVLNgCWbcjOMYmJKIR9kQTxNm_J0gTxKS0kcPJrN-jhNN-wARdWNrVdWoHi1fQW3gfkkT6jxCbDqIRFB2ptEwBYmLwZOYw2sAQgeaGg3C9k=w964-h2096-no)
![enter image description here](https://lh3.googleusercontent.com/6I_fJ8k-fLcLMVWNQttQ5ywDmBtCWTWq7cpxGPmJRIPuK94MY4kydS7PN9lZWh9jVgkKHyWHND4HgWx3HWsbnf-d2Mhj8FwYxWj2BBma1hqtL4HZ02GtsQQq0RseIiiuEGunWvVNdNazjN0KXw5T9Qszha3eMm1VzsGM8rrxp2wh3Ijl_amUB27CCbKss828rhUr7M2SoStUEjXiBYTbroOnT-cD5GPrPYmzrT1IVVx7XIaokV82QQPclwS0sKfy9ByapQ1CcPFcs3uq_WMChk4VK51SfmVJyLZ1gwd56GwcYz7xW89SIQ60Rl3TiNpJdJ3hB7KyT5ch5b9TMzr0q94MAspW3jRRlgsvbG_3AFwyKDoYT5zxWjaEZUHxS0lctFHFL1oc9Jx7lUqODiqidvF0fR2mnWtZF_dIXcy2CKDi9WtsDPIf2x-Y3BEXSoADjDqJ5C_2ocDFLDIlnLP4byEvzGutlEXPasWqBshczKUrLll7Ds-P7-o1q_s0Y46XliNsCF2p6KmbyDKrDK7j38mxgpigNzBKOzf2vXb9Bv5OEnzN0i2FRmusLpLxsxmf10-B68bx41_iEi9ZHDjHAJtyZxzrLHF94XRB_OeXphM87Yi9mpWchPiChphUos2fhKpDIAX23F3xnryExJkMuLM3roWFFAw=w946-h2096-no)
![enter image description here](https://lh3.googleusercontent.com/wFhPfW9M4EVyCxtdyORVBJHhaF649p74yVX1EbLASvzWAYlUIfXZ-N1dgjqdfbMnXUwB6J8oF8kmqJ2dN5LUuhlzjXWS21f2RWAMnvY_sb7bdYi_d5lnbUbm0hX3IfJ9AzOX8ilC0GK9JoFCz22sNRmRYB-4T40WZhailfp15wVuwKaYp6NrMUMNwuT6uuEomIN7f9Gn0leL7Q4E8flRdGBKspfWM95lJsC5Qab1sMi0pG_rITK1dLMItU5mYZuKqwvVnwcgi8K_5DmtC3Qs_EN7LeSP6FQ80kN2KIEEw5CLatMu3TzS4-jiH5RA-2rQ30zmeghiOXYz7IDyo0x_gwvssDG2-heHjSd8jowRUJNLNJlVUljlCPlPfw4VYAdA5oaRFksju2PcliIsecwYUojcq-PWIOfl7bOnLzbmNgODJXrMJKP05g6-qyGoEHAqFQYeoQvPHODBZt2ov94Ji1DSYO_BjvC6lTwnzKSZtTbd5aZaaIuFiVrn0Hkk2p0QQwQHdOsTmtaOdJSLS0lBw7X_hrjvTY8usM8_doi_rzs3NQ1o8W_tUtVC-uDUpqnhj9998PIo_7wVIInBq9bs2JSy5UnMBwRfnkJwVPRi1xYljycQdwDfpUwseZXlZWc9gqTx0Y_PhQmSTAUWL8WdAmH7tcpEFFs=w992-h1216-no)
