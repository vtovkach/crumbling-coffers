#include <stdio.h>                          
#include "orchestrator/state/client.h"      
#include "orchestrator/config.h"            

int addClientToQueue(struct Client *client)
{
    // Just a place holder for now
    // TODO 
    
    printf("Buffer's Content: ");
    for(int i = 0; i < TCP_SEGMENT_SIZE; i++)
    {
        char cur_char = (char) client->buffer[i];
        if(cur_char == '\0') { cur_char = '*'; } // Indicate pending zero 
        
        putchar(cur_char);
    }
    putchar('\n');
    fflush(stdout);

    return 0;
}