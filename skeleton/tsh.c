// Net IDs: jjk613, bpv512 
//
//
/***************************************************************************
 *  Title: MySimpleShell 
 * -------------------------------------------------------------------------
 *    Purpose: A simple shell implementation 
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.1 $
 *    Last Modification: $Date: 2006/10/13 05:25:59 $
 *    File: $RCSfile: tsh.c,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: tsh.c,v $
 *    Revision 1.1  2005/10/13 05:25:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.4  2002/10/24 21:32:47  sempi
 *    final release
 *
 *    Revision 1.3  2002/10/23 21:54:27  sempi
 *    beta release
 *
 *    Revision 1.2  2002/10/15 20:37:26  sempi
 *    Comments updated
 *
 *    Revision 1.1  2002/10/15 20:20:56  sempi
 *    Milestone 1
 *
 ***************************************************************************/
#define __MYSS_IMPL__

/************System include***********************************************/
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

/************Private include**********************************************/
#include "tsh.h"
#include "io.h"
#include "interpreter.h"
#include "runtime.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

#define BUFSIZE 80

/************Global Variables*********************************************/

/************Function Prototypes******************************************/
/* handles SIGINT and SIGSTOP signals */	
static void sig_handler(int);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

int main (int argc, char *argv[])
{
  /* Initialize command buffer */
  char* cmdLine = malloc(sizeof(char*)*BUFSIZE);

  /* shell initialization */
  if (signal(SIGINT, sig_handler) == SIG_ERR) PrintPError("SIGINT");
  if (signal(SIGTSTP, sig_handler) == SIG_ERR) PrintPError("SIGTSTP");
  if (signal(SIGCHLD, sig_handler) == SIG_ERR) PrintPError("SIGCHLD");

  while (!forceExit) /* repeat forever */
  {
    /* read command line */
    getCommandLine(&cmdLine, BUFSIZE);
   // printf("\n");
    if(strcmp(cmdLine, "exit") == 0)
    {
      forceExit=TRUE;
      continue;
    }

    /* checks the status of background jobs */
    //implement this to get rid of jobs that finished
    CheckJobs();

    /* interpret command and line
     * includes executing of commands */
    Interpret(cmdLine);
//    printf("Not getting here! \n");

  }

  /* shell termination */
  free(cmdLine);
  return 0;
} /* end main */

static void sig_handler(int signo)
{
  switch(signo)
  {
    
    case SIGTSTP:
    
      StopProcessHandler();
      break;
    case SIGINT:
  //    forceExit = TRUE;
      InterruptProcessHandler();
      break;
    case SIGCHLD:
     // printf("SIGCHLD\n");
      ChildHandler();
      break;
    default:
      break;
    
  }
}

