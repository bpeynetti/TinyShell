/***************************************************************************
 *  Title: Runtime environment 
 * -------------------------------------------------------------------------
 *    Purpose: Runs commands
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.1 $
 *    Last Modification: $Date: 2006/10/13 05:24:59 $
 *    File: $RCSfile: runtime.c,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: runtime.c,v $
 *    Revision 1.1  2005/10/13 05:25:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.6  2002/10/24 21:32:47  sempi
 *    final release
 *
 *    Revision 1.5  2002/10/23 21:54:27  sempi
 *    beta release
 *
 *    Revision 1.4  2002/10/21 04:49:35  sempi
 *    minor correction
 *
 *    Revision 1.3  2002/10/21 04:47:05  sempi
 *    Milestone 2 beta
 *
 *    Revision 1.2  2002/10/15 20:37:26  sempi
 *    Comments updated
 *
 *    Revision 1.1  2002/10/15 20:20:56  sempi
 *    Milestone 1
 *
 ***************************************************************************/
#define __RUNTIME_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

/************Private include**********************************************/
#include "runtime.h"
#include "io.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

/************Global Variables*********************************************/

#define NBUILTINCOMMANDS (sizeof BuiltInCommands / sizeof(char*))



/* the pids of the background processes */
bgjobL *bgjobs = NULL;
bgjobL *lastJob = NULL;
// bgjobL *lastJob = NULL; //ptr to end of the job list

// the next job id 
int nextJobId = 1;
// foreground process
int fgpid = 0;

/************Function Prototypes******************************************/
/* run command */
static void RunCmdFork(commandT*, bool);
/* runs an external program command after some checks */
static void RunExternalCmd(commandT*, bool);
/* resolves the path and checks for exutable flag */
static bool ResolveExternalCmd(commandT*);
/* forks and runs a external program */
static void Exec(commandT*, bool);
/* runs a builtin command */
static void RunBuiltInCmd(commandT*);
/* checks whether a command is a builtin command */
static bool IsBuiltIn(char*);
/************External Declaration*****************************************/

/**************Implementation***********************************************/
int total_task;
void RunCmd(commandT** cmd, int n)
{
  int i;
  int count;
  total_task = n;
  //printf("Command name: %s \n",cmd[0]->argv[0]);
  //printf("Command argument count: %d \n",cmd[0]->argc-1);
  for ( count=1;count<cmd[0]->argc;count++){
        //printf("Argument %d : %s \n",count,cmd[0]->argv[count]);
  }
  if(n == 1)
    RunCmdFork(cmd[0], TRUE);
  else{
    RunCmdPipe(cmd[0], cmd[1]);
    for(i = 0; i < n; i++)
      ReleaseCmdT(&cmd[i]);
  }
}

void RunCmdFork(commandT* cmd, bool fork)
{
  if (cmd->argc<=0)
    //printf("You typed enter! \n");
    return;
  if (IsBuiltIn(cmd->argv[0]))
  {
    RunBuiltInCmd(cmd);
  }
  else
  {
    RunExternalCmd(cmd, fork);
  }
}

void RunCmdBg(commandT* cmd)
{
  // TODO
  //currently at another place ---> in runbuiltincmd!
}

void RunCmdPipe(commandT* cmd1, commandT* cmd2)
{
}

void RunCmdRedirOut(commandT* cmd, char* file)
{
	//redirection implemented in Exec
}

void RunCmdRedirIn(commandT* cmd, char* file)
{
	//redirection implemented in Exec
}


/*Try to run an external command*/
static void RunExternalCmd(commandT* cmd, bool fork)
{
  if (ResolveExternalCmd(cmd)){
    Exec(cmd, fork);
  }
  else {
    printf("%s: command not found\n", cmd->argv[0]);
    fflush(stdout);
    ReleaseCmdT(&cmd);
  }
}

/*Find the executable based on search list provided by environment variable PATH*/
static bool ResolveExternalCmd(commandT* cmd)
{
  char *pathlist, *c;
  char buf[1024];
  int i, j;
  struct stat fs;

  if(strchr(cmd->argv[0],'/') != NULL){
    if(stat(cmd->argv[0], &fs) >= 0){
      if(S_ISDIR(fs.st_mode) == 0)
        if(access(cmd->argv[0],X_OK) == 0){/*Whether it's an executable or the user has required permisson to run it*/
          cmd->name = strdup(cmd->argv[0]);
          return TRUE;
        }
    }
    return FALSE;
  }
  pathlist = getenv("PATH");
  if(pathlist == NULL) return FALSE;
  i = 0;
  while(i<strlen(pathlist)){
    c = strchr(&(pathlist[i]),':');
    if(c != NULL){
      for(j = 0; c != &(pathlist[i]); i++, j++)
        buf[j] = pathlist[i];
      i++;
    }
    else{
      for(j = 0; i < strlen(pathlist); i++, j++)
        buf[j] = pathlist[i];
    }
    buf[j] = '\0';
    strcat(buf, "/");
    strcat(buf,cmd->argv[0]);
    if(stat(buf, &fs) >= 0){
      if(S_ISDIR(fs.st_mode) == 0)
        if(access(buf,X_OK) == 0){/*Whether it's an executable or the user has required permisson to run it*/
          cmd->name = strdup(buf);
          return TRUE;
        }
    }
  }
  return FALSE; /*The command is not found or the user don't have enough priority to run.*/
}

static void Exec(commandT* cmd, bool forceFork)
{
        pid_t pid;
        int status;
        //printf("%s \n",cmd->name);
        
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask,SIGCHLD);
        sigaddset(&mask,SIGINT);
        sigaddset(&mask,SIGTSTP);
        //blocks the signals specified above
        sigprocmask(SIG_BLOCK, &mask, NULL);

        //make copy of arguments without the command
        //int i=0;
        //char* argArray[cmd->argc-1];
        //for (i=0;i<cmd->argc-1;i++){
        //        argArray[i] = strdup(cmd->argv[i+1]);
        //}
        pid = fork();
        //check for fork error
        if (pid < 0){
                printf("Fork Error\n");//err_sys("fork error");
        }
        // //successful fork
        // else {
        if (pid==0){
        //child
	            setpgid(0, 0);
	            //unblocks the signals specified above
	            sigprocmask(SIG_UNBLOCK, &mask, NULL);
	            
	            //check for redirections
	            
	//copy this to redirect in call
	            //check redirection in
	            if (cmd->is_redirect_in==1){
			int fdin;
			fdin = open(cmd->redirect_in,O_RDONLY,S_IRUSR|S_IWUSR);
			if (fdin<0){
				//error
				perror("Cannot open redirect in file");
				exit(0);
			}
			
			//printf("Redirect in %s\n",cmd->redirect_in);
	            	//yes, so redirect with dup2
	            	dup2(fdin,STDIN_FILENO);
	            	close(fdin);
		    }
	//copy this to redirect out call
	            //check redirection out
	            if (cmd->is_redirect_out==1){
			//create or open the file to use
			int fdout;
			fdout = creat(cmd->redirect_out,S_IRUSR|S_IWUSR);
			if (fdout<0){
				perror("cannot open redirect out file");
				exit(0);
			}
			//printf("Redirect out %s\n",cmd->redirect_out);
	            	//yes, so redirect with dup2
	            	dup2(fdout,STDOUT_FILENO);
			close(fdout);
	            }
	            
                execv(cmd->name,cmd->argv);
          }
          else{
            //parent
            sigprocmask(SIG_UNBLOCK, &mask, NULL);
            //we add the job to the background if it was specified as background
            if (cmd->bg ==1){
                AddJob(pid,BACKGROUND, cmd->cmdline); //add job to background jobs
            }
            //otherwise the job goes in the foreground
            else {
                //printf("Adding job %d \n",pid);
                fgpid = pid;
                AddJob(pid, FOREGROUND, cmd->cmdline);
                WaitFg(pid);
            }
          }
    //}
}


static bool IsBuiltIn(char* cmd)
{
  int i=0;
  int numberOfCommands = 5;
 // printf("You are trying to run the command %s \n",cmd);
  char* commands[5] = {"exit", "jobs","fg", "bg", "cd"};
  for (i=0;i<numberOfCommands;i++){
        if (strcmp(cmd,commands[i])==0){
                //printf("And it's built-in! \n");
                return TRUE;
        }
  }
  return FALSE;
}


static void RunBuiltInCmd(commandT* cmd)
{
        //running built-in command
        int i=0;
        //check if it's exit
        if (strcmp(cmd->argv[0],"exit")==0){
                return;
        }


        //check for echo
	/*
        if (strcmp(cmd->argv[0],"echo")==0){
                for (i=1;i<cmd->argc;i++){
                        //check for environment var
                        if (cmd->argv[i][0]=="$"){
                                //get the 1st through n elements of this argument
                                char* envVar = malloc(strlen((cmd->argv[i]))*sizeof(char));
                                memcpy(envVar,cmd->argv[i]+sizeof(char),sizeof(char)*strlen(cmd->argv[i]));
                                printf("%s ",getenv(envVar));
                        }
                        else{
                                printf("%s ",cmd->argv[i]);
                        }
                }
                printf("\n");
        }
	*/
	if (strcmp(cmd->argv[0], "jobs")==0){
		PrintJobs();
	}
	
	if (strcmp(cmd->argv[0], "fg")==0){
	    //bring process to foreground
	    //the jid should have been given in the command so in argv[1]
		
		if (cmd->argv[1]==NULL){
			//no job id given
			printf("No job id \n");
		}
		else {
			//get the job id as an integer
			pid_t jid = atoi(cmd->argv[1]);
			//find job
			bgjobL* jobTofg = FindJob(jid,FALSE);
			//now change state to foreground
			if (jobTofg==NULL){
				printf("Job does not exist \n");
				//job does not exist
				return;
			}
			else {
				if (jobTofg->state==STOPPED || jobTofg->state==BACKGROUND){
					//continue if it wasn't already there
					kill(-(jobTofg->pid),SIGCONT);
				}

				//and bring state to FOREGROUND
				jobTofg->state = FOREGROUND;
//				printf("Job %d brought to foreground: %s \n",jobTofg->jid,jobTofg->cmdline);
				//now wait for it to finish
				fgpid = jobTofg->pid;
				WaitFg(jobTofg->pid);
				
			}
			
		}
	}

  if (strcmp(cmd->argv[0], "bg") == 0)
  {
    if (cmd->argv[1] == NULL)
    {
      //no job id given
      printf("No job id \n");
    }
    else
    {
      //get the job id as an integer
      pid_t jid = atoi(cmd->argv[1]);
      //find the job in the list
      bgjobL* jobToBg = FindJob(jid, FALSE);
      if (jobToBg == NULL)
      {
        printf("Job does not exist \n");
        return;
      }
      else
      {
	if (jobToBg->state == STOPPED || jobToBg->state == BACKGROUND)
	{
		kill(-(jobToBg->pid), SIGCONT);
	}
      	jobToBg->state = BACKGROUND; 
      }
    }
  }
  
  if (strcmp(cmd->argv[0], "cd") == 0)
  {
  	if (cmd->argv[1] == NULL)
  	{
  		if (chdir(getenv("HOME")) != 0)
  		{
  			printf("error in cd\n");
  		}
  	}
  	else
  	{
  		if (chdir(cmd->argv[1]) != 0)
  		{
  			printf("error in cd\n");
  		}
  	}
  }

}

void PrintJobs()
{
	bgjobL* current = bgjobs;
	const char* state;
	while (current != NULL)
	{
		if (current->state == BACKGROUND)
		{
			state = "Running";
			printf("[%d]   %s                 %s &\n", current->jid, state, current->cmdline);
			fflush(stdout);
		}
		else if (current->state == STOPPED)
		{
			state = "Stopped";
			printf("[%d]   %s                 %s\n", current->jid, state, current->cmdline);
			fflush(stdout);
		}

		current = current->next;
		
	}
}



commandT* CreateCmdT(int n)
{
  int i;
  commandT * cd = malloc(sizeof(commandT) + sizeof(char *) * (n + 1));
  cd -> name = NULL;
  cd -> cmdline = NULL;
  cd -> is_redirect_in = cd -> is_redirect_out = 0;
  cd -> redirect_in = cd -> redirect_out = NULL;
  cd -> argc = n;
  for(i = 0; i <=n; i++)
    cd -> argv[i] = NULL;
  return cd;
}

/*Release and collect the space of a commandT struct*/
void ReleaseCmdT(commandT **cmd){
  int i;
  if((*cmd)->name != NULL) free((*cmd)->name);
  if((*cmd)->cmdline != NULL) free((*cmd)->cmdline);
  if((*cmd)->redirect_in != NULL) free((*cmd)->redirect_in);
  if((*cmd)->redirect_out != NULL) free((*cmd)->redirect_out);
  for(i = 0; i < (*cmd)->argc; i++)
    if((*cmd)->argv[i] != NULL) free((*cmd)->argv[i]);
  free(*cmd);
}


// job stuff
void AddJob(pid_t pid, int state, char* cmdline){
    //modify this so that there is a pointer to the end of the job list
    
    bgjobL* current = bgjobs;
    bgjobL* nextJob = (bgjobL*) malloc(sizeof(bgjobL));
    //now assign the same pid, state, and all that
    //and job id is whatever's next
    nextJob->state = state;
    nextJob->pid = pid;
    nextJob->jid = nextJobId;
    nextJob->cmdline = cmdline;
    nextJob->printedJob = FALSE;
    nextJob->next = NULL;
    
    
    //now look at what to do 
    if (state == BACKGROUND)
    {
        nextJobId++;
    }
    else{
        nextJob->jid = 0;
    }
    
    //add the job to end of list of jobs
    if (current == NULL)
    {
        bgjobs = nextJob;
        lastJob = nextJob;
    }
    else
    {
        //add it to the end of the list of jobs
        while (current->next != NULL){
            current = current->next;
        }
        current->next = nextJob;
    }
    lastJob = nextJob;
}

bgjobL* FindJob(pid_t id, bool Process){
    bgjobL* current = bgjobs;
    pid_t check;
    //iterate over all jobs until you find it
    //Process  = true for pid, false for job id
    //if you cant find it, return null
    while (current!=NULL)
    {
        //capture the check we need
        if (Process==TRUE){
            check = current->pid;
        }
        else{
            check = current->jid;
        }
        //now check it, otherwise iterate to next job
        if (check == id)
        {
            return current;
        }
        else
        {
            current = current->next;   
        }
    }
    return NULL;
}

void UpdateJobs(pid_t pid, int state)
{
  bgjobL* job;
  job = FindJob(pid, TRUE);
  
 // printf("%d\n", job->state);
  //printf("%s\n", job->cmdline);
  
  if (job != NULL)
  {
    if (state == DONE && job->state != BACKGROUND)
    {
	job->printedJob = TRUE;
    }
    job->state = state;
    if (state == STOPPED)
    {
      if (job->jid == 0)
      {
        job->jid = nextJobId;
        nextJobId += 1;
      }
      //NEED TO PRINT OUT THAT THE JOB WAS STOPPED
      fflush(stdout);
    }
   // printf("%d\n", state);
  }
}

void ReleaseJob(bgjobL* job)
{
    free(job->cmdline);
    free(job);
}

void CheckJobs(){
    bgjobL* thisJob = bgjobs;
    bgjobL* prevJob = NULL;
    
    //iterate over all jobs and kill if necessary
    while (thisJob != NULL){
        if (thisJob->state == DONE)
        {
            if (thisJob->printedJob == FALSE){
              printf("[%d]   Done                 %s \n", thisJob->jid,thisJob->cmdline);
              thisJob->printedJob = TRUE;
	    }
            else {
            	//give control to the shell
              fgpid = - 1;
            }

            if (prevJob == NULL){
                bgjobs = thisJob->next;
                ReleaseJob(thisJob);
                thisJob = bgjobs;
            } 
            else 
            {
                //skip over and join previous with next
                prevJob->next = thisJob->next;
                ReleaseJob(thisJob);
                thisJob = prevJob->next;
            }
            fflush(stdout);
        }
        else{
            //just continue
            prevJob = thisJob; 
            thisJob = thisJob->next;
        }
      
    }
    if (bgjobs==NULL)
    {
        //no more jobs, reset job id
        nextJobId = 1;
    }
}

void WaitFg(pid_t pid)
{
    bgjobL* job = FindJob(pid,TRUE);
    //while job still in the foreground
    while (job->state==FOREGROUND && job!=NULL)
    {
        //not killed at some point 
            //wait 1 second for job to finish
            sleep(1);
    }
}
//---------------------------------------------------------------------------------------

//SIGNAL HANDLERS

//handle SIGINT signal (control c) 
void InterruptProcessHandler()
{
  //kills the foreground process if you're in the parent
  if (fgpid > 0)
  {
  //	printf("This is killing a process %d \n",fgpid);
        //kills the process, throws a SIGINT
        kill(-fgpid, SIGINT);
  }
 // printf("Can't kill me just yet \n");
}


//handle SIGTSTP signal (process stopped)
void StopProcessHandler()
{
  //kills the foreground process if you're in the parent
  if (fgpid >= 0)
  {
    //stops the process, throws a SIGTSTP
    kill(-fgpid, SIGTSTP);
    //returns control to shell
    fgpid = -1;
  }
}

void ChildHandler()
{
  int wpid;
  int status;
  do
  {
    //waitpid waits for the child process to execute
    //WNOHANG - returns immediately if no child has exited
    //WUNTRACED - return if a child has stopped
    wpid = waitpid(-1, &status, WNOHANG | WUNTRACED);
    printf("wpid: %d\n", wpid);
    printf("fgpid: %d\n", fgpid);
    if (wpid == fgpid)
    {
      //return control to terminal
      fgpid = -1;
    }
    //options are child process is stopped or done
    //can check which it is using WIFSTOPPED macro
    if (WIFSTOPPED(status))//evaluates if child process is stopped
    {
      UpdateJobs(wpid, STOPPED);
      printf("WE HAVE STOPPED\n");
    }
    else
    {
      UpdateJobs(wpid, DONE);
    }
  }while(wpid>0);
}                              
