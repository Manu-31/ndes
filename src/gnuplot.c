/**
 * @file gnuplot.c
 * @brief Some basic tools to draw probes through gnuplot
 *
 */

#include <stdlib.h>     // exit
#include <unistd.h>     // pipe, unlink
#include <string.h>     // strlen, strdup
#include <errno.h>
#include <stdio.h>      // perror
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <motsim.h>
#include <gnuplot.h>


/**
 * Some definitions fo gnuplot path and commands
 */
#ifndef GnuPlotCmd
#   define GnuPlotCmd "/usr/bin/gnuplot"
#endif
#define GPSetTermWxt "set terminal wxt"
#define GPSetTermPng "set terminal png"
#define GPSetTermPng "set terminal png"
#define GPSetTermQt "set terminal qt"

/*
 * Warning : default value depends on gnuplot version
 */
#define GPSetTermDefault GPSetTermQt

#define GPSetOutput "set output"

/**
 * Multiple source files can be associated to a given
 * terminal, so we need to chain the names
 */
struct fileName_t {
   char * fileName;
   struct fileName_t * next;
};

/**
 * @brief A gnuplot_t structure is needed to draw some probes
 */
struct gnuplot_t {
   struct fileName_t * srcFileName;   //!< Input file names daisy chain

   // Gnuplot specific
   char * outputFileName;
   int    terminalType;
   int                 id;    //!< Ident of gnuplot terminal
   char * title ;
   double xmin, xmax;
   double ymin, ymax;

   // Terminal daisy chaining
   struct gnuplot_t * prev;
   struct gnuplot_t * next; 
};

struct gnuplotProcess_t {
   int stdin;  // stdin du processus
   int stdout; // stdout+stderr
   int pid;

   int nbPlot;
   struct gnuplot_t * lastTerminal;
};

struct gnuplotProcess_t * gnuplotProcess = NULL; 

/**
 * @brief Send a command to a gnuplot process
 */
void GPSendCmd(struct gnuplot_t * gp, char * cmd)
{
   char buffer[512];
   //   int readBytes;
   
   unsigned char cr = 0x0a;

   printf_debug(DEBUG_GNUPLOT, "GPSendCmd IN\n");
/*
   while ((readBytes = read(gnuplotProcess->stdout, buffer, 512)) > 0) {
      write(STDOUT_FILENO, buffer, readBytes);
   }
   printf_debug(DEBUG_GNUPLOT, "GPSendCmd BBB\n");
*/

   if (gp) {
      printf_debug(DEBUG_GNUPLOT, "Selecting terminal (%s)...\n", (gp->title)?gp->title:"Guess my name");
      switch (gp->terminalType) {
         case gnuplotTerminalTypeWxt :
            sprintf(buffer, "%s %d title '%s'",
                    GPSetTermDefault,
                    gp->id,
                    (gp->title)?gp->title:"Guess my name");
         break;
         case gnuplotTerminalTypePng :
            sprintf(buffer, "%s",
                    GPSetTermPng);
             break;
         default :
            motSim_error(MS_FATAL, "unknown terminal type");
         break;
      }
      printf_debug(DEBUG_GNUPLOT, "sending \"%s\"\n", buffer);
      write(gnuplotProcess->stdin, buffer, strlen(buffer));
      write(gnuplotProcess->stdin, &cr, 1);
   }

   printf_debug(DEBUG_GNUPLOT, "sending \"%s\"\n", cmd);
 
   write(gnuplotProcess->stdin, cmd, strlen(cmd));
   write(gnuplotProcess->stdin, &cr, 1);
/*
   while ((readBytes = read(gnuplotProcess->stdout, buffer, 512)) > 0) {
      write(STDOUT_FILENO, buffer, readBytes);
   }
*/
}

/*
 * Terminaison de gnuplot
 */
void on_exitGnuplot(int exitStatus, void * gP)
{
   struct gnuplotProcess_t * gnuplotProcess = (struct gnuplotProcess_t *)gP; 
   struct gnuplot_t * term, *prev;
   struct fileName_t * p;
   int n;

   printf_debug(DEBUG_GNUPLOT, "closing gnuplot terminals ...\n");

   // Terminaison propre du processus
   printf_debug(DEBUG_GNUPLOT, "terminating gnuplot process ...\n");
   GPSendCmd(NULL, "exit");

   //! Waiting for the process death
   waitpid(gnuplotProcess->pid, NULL, 0);

   // Fermeture des tubes de communication
   printf_debug(DEBUG_GNUPLOT, "closing gnuplot pipes ...\n");
   close(gnuplotProcess->stdin);
   close(gnuplotProcess->stdout);

   // Effacement des fichiers temporaires
   for (n=0,term = gnuplotProcess->lastTerminal; term != NULL; term = term->prev, n++) {
      while (term->srcFileName) {
         printf_debug(DEBUG_GNUPLOT, "unlinking \"%s\"\n", term->srcFileName->fileName);
	 
         if (unlink(term->srcFileName->fileName)) {
            perror("unlink ");
         }
	 
         p = term->srcFileName;
         term->srcFileName = p->next;
         free(p);
      }
   }
   assert(n == gnuplotProcess->nbPlot);

   // Libération de la mémoire
   for (n=0,term = gnuplotProcess->lastTerminal; term != NULL; n++) {
      prev = term->prev;
      free(term);
      term = prev;
   }
   assert(n == gnuplotProcess->nbPlot);
}

/**
 * @brief gnuplot process creation
 * We will for a single gnuplot process for all the terminals. This
 * function creates this process as well as comunication means.
 */
int gnuplotProcessCreate()
{
   int  pipeGPin[2];
   int  pipeGPout[2];

   if (gnuplotProcess){
      return 1;
   }

   // Structure creation
   gnuplotProcess = (struct gnuplotProcess_t*) sim_malloc(sizeof(struct gnuplotProcess_t));
   gnuplotProcess->nbPlot = 0;
   gnuplotProcess->lastTerminal = NULL;
 
   // Pipes
   if (pipe(pipeGPin)) {
      perror("pipe ");
      return 0;
   }
   if (pipe(pipeGPout)) {
      perror("pipe ");
      return 0;
   }

   // Let's go !!
   gnuplotProcess->pid = fork();

   if (gnuplotProcess->pid == -1){
      perror("fork ");
      return 0;
   } else if (gnuplotProcess->pid == 0) {
      // Look for gnuplot binary

      // Input from here
      if (dup2(pipeGPin[0], STDIN_FILENO) == -1) {
         perror("dup2 ");
         return 0;
      }
      //close(pipeGPin[0]);
      //close(pipeGPin[1]);
      // Output to here
      if (dup2(pipeGPout[1], STDOUT_FILENO) == -1) {
         perror("dup2 ");
         return 0;
      }
      if (dup2(pipeGPout[1], STDERR_FILENO) == -1) {
         perror("dup2 ");
         return 0;
      }
      //close(pipeGPout[0]);
      //close(pipeGPout[1]);

      // Try to run
      execl(GnuPlotCmd , "gnuplot (forked by NDS)", NULL);
      printf("You should not see this message !\n");
      exit(1);
   }

   // Close useless fds
   close(pipeGPin[0]);
   close(pipeGPout[1]);

   // Don't forget the usefull ones 
   gnuplotProcess->stdin = pipeGPin[1];
   gnuplotProcess->stdout = pipeGPout[0];

   // Prepare the end
   on_exit(on_exitGnuplot, gnuplotProcess);

   return 1;
}

/**
 * @brief Affichage dans une fenetre d'une sonde
 *
 * On passe par un fichier dont le nom est de la forme %d.%d-%s.gp
 * avec, dans l'ordre, le numéro de processus, le numéro de terminal
 * et le titre.
 * 
 * Le nom de fichier est sauvé pour effacer le fichier à la fin du
 * programme (par un on_exitGnuplot)
 */
#define BUFFER_LENGTH 4096
int gnuplot_displayProbe(struct gnuplot_t * gp, int with, struct probe_t * probe)
{
   char fileName[BUFFER_LENGTH];   
   struct fileName_t * p;
   int  fd;
   char cmd[BUFFER_LENGTH];
   
   assert(gp != NULL);

   //! We need a title (wxt terminal)
   if (gp->title == NULL) {
      printf_debug(DEBUG_GNUPLOT, "gp->title\n");
      gnuplot_setTitle(gp, probe_getName(probe));
   }

   //! We dump the probe in a temporary file
   sprintf(fileName, "%d.%d-%s.gp", getpid(), gp->id, gp->title);
   if ((fd = open(fileName, O_CREAT|O_WRONLY, 0644)) == -1) {
      perror("open");
      return 0;
   }
   probe_dumpFd(probe, fd, dumpGnuplotFormat);
   close(fd);

   //! Set filename
   p = (struct fileName_t *)sim_malloc(sizeof(struct fileName_t));
   p->next = gp->srcFileName;
   gp->srcFileName = p;
   gp->srcFileName->fileName = strdup(fileName);

   /*
   //! Set terminal (WARNING : it's done in GPSendCmd, so we only need
   //! to send a void command here)
   switch (gp->terminalType) {
      case gnuplotTerminalTypeWxt :
         sprintf(cmd, "%s %d title '%s'",
                 GPSetTermWxt,
                 gp->id,
                 gp->title);
         GPSendCmd(gp, cmd);
      break;
      case gnuplotTerminalTypePng :
         sprintf(cmd, "%s",
                 GPSetTermPng);
         GPSendCmd(gp, cmd);
      break;
      default :
         motSim_error(MS_FATAL, "unknown terminal type");
      break;
   }
   */

   //! Select ouput if specified
   if (gp->outputFileName) {
      sprintf(cmd, "%s '%s'",
              GPSetOutput,
	      gp->outputFileName);
      GPSendCmd(gp, cmd);
   }

   //! WARNING, to be done : set style in a specific function so that
   //! each instance can have specific style parameters
   sprintf(cmd, "set style fill solid 0.5");
   GPSendCmd(gp, cmd);

   //! Plot the graph
   sprintf(cmd, "plot '%s' using 1:2 with %s title '%s'",
           fileName,
           (with == 1)?"points":(with == 2)?"lines":"boxes",
           probe_getName(probe));
   GPSendCmd(gp, cmd);

   //! Temporary files unlinked on exit


   return 1;
}

int gnuplot_displayProbes(struct gnuplot_t * gp, int with, ...)
{
   char fileName[BUFFER_LENGTH];   
   struct fileName_t * p;
   int  fd;
   char cmd[BUFFER_LENGTH];
   char cmdTmp[BUFFER_LENGTH];

   va_list argp;
   struct probe_t * probe;
   int n = 0;

   assert(gp != NULL);

   sprintf(cmd, "plot");

   va_start(argp, with);
   while ((probe = va_arg(argp, struct probe_t *)) != NULL) {
      if (gp->title == NULL) {
         printf_debug(DEBUG_GNUPLOT, "gp->title\n");
         gnuplot_setTitle(gp, probe_getName(probe));
      }

      // Création d'un fichier contenant le dump
      sprintf(fileName, "%d.%d-%s-%d.gp", getpid(), gp->id, gp->title, n);
      if ((fd = open(fileName, O_CREAT|O_WRONLY, 0644)) == -1) {
         perror("open");
         return errno;
      }
      probe_dumpFd(probe, fd, dumpGnuplotFormat);
      close(fd);

      // Set filename
      p = (struct fileName_t *)sim_malloc(sizeof(struct fileName_t));
      p->next = gp->srcFileName;
      gp->srcFileName = p;
      gp->srcFileName->fileName = strdup(fileName);

      printf_debug(DEBUG_TBD, "Gestion des fichiers temporaires\n");

      sprintf(cmdTmp, "%s '%s' using 1:2 with %s title '%s'",n?",":"",
              fileName,
              (with == 1)?"points":(with == 2)?"lines":"boxes",
              probe_getName(probe));
      n++;
      strcat(cmd, cmdTmp);
   }
   va_end(argp);

   // Plot the graph
   GPSendCmd(gp, cmd);
   return 0;
}

/**
 * @brief Creation of a gnuplot_t instance
 */
struct gnuplot_t * gnuplot_create()
{
   struct gnuplot_t * result = (struct gnuplot_t *)sim_malloc(sizeof(struct gnuplot_t));

   // Création du process
   if (!gnuplotProcess) {
      gnuplotProcessCreate();
   }

   result->srcFileName = NULL;

   result->outputFileName = NULL;
   result->terminalType = gnuplotTerminalTypeWxt;
   result->title = NULL;
   result->id = gnuplotProcess->nbPlot++;

   result->xmin = 0.0;
   result->xmax = 0.0;

   result->ymin = 0.0;
   result->ymax = 0.0;

   result->prev = gnuplotProcess->lastTerminal;
   if (result->prev) {
      result->prev->next = result;
   }
   gnuplotProcess->lastTerminal = result;

   return result;
}

void gnuplot_setXRange(struct gnuplot_t * gp, double xmin, double xmax)
{
   char cmd[BUFFER_LENGTH];

   gp->xmin = xmin;
   gp->xmax = xmax;

   // Set ranges
   if (gp->xmin < gp->xmax) {
      sprintf(cmd, "set xrange [%f:%f]", gp->xmin, gp->xmax);
   } else {
      sprintf(cmd, "set xrange [*:*]");
   }
   GPSendCmd(gp, cmd);
}

void gnuplot_setYRange(struct gnuplot_t * gp, double ymin, double ymax)
{
   char cmd[BUFFER_LENGTH];

   gp->ymin = ymin;
   gp->ymax = ymax;

   // Set ranges
   if (gp->ymin < gp->ymax) {
      sprintf(cmd, "set yrange [%f:%f]", gp->ymin, gp->ymax);
   } else {
      sprintf(cmd, "set yrange [*:*]");
   }
   GPSendCmd(gp, cmd);

}

/**
 * @brief Change the output file name
 * @param gp The gnuplot terminal 
 * @param name The new file name
 */
void gnuplot_setOutputFileName(struct gnuplot_t * gp, char * outputFileName)
{
  gp->outputFileName = sim_malloc(strlen(outputFileName)+1);

  strncpy(gp->outputFileName, outputFileName, strlen(outputFileName) +1);
}

void gnuplot_setTitle(struct gnuplot_t * gp, char * name)
{
  gp->title = strdup(name);
  gp->title = sim_malloc(strlen(name)+1);

  strncpy(gp->title, name, strlen(name) +1);
}

/**
 * @brief Select terminal type
 * @param gp The gnuplot terminal 
 * @param terminalType The new terminal type
 */
void gnuplot_setTerminalType(struct gnuplot_t * gp, int terminalType)
{
  gp->terminalType = terminalType;
}

/**
 * @brief Destructor
 */
void gnuplot_delete(struct gnuplot_t * gp)
{
   printf_debug(DEBUG_TBD, "not yet implemented\n");
}
