// Scalpel Copyright (C) 2005-11 by Golden G. Richard III and 
// 2007-11 by Vico Marziale.
//
// Written by Golden G. Richard III and Vico Marziale.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301, USA.
//
// Thanks to Kris Kendall, Jesse Kornblum, et al for their work 
// on Foremost.  Foremost 0.69 was used as the starting point for 
// Scalpel, in 2005.
//

#include "scalpel.h"

// globals defined in scalpel.h

// signal has been caught by signal handler
int signal_caught;

// current wildcard character
char wildcard;

// width of tty, for progress bar
int ttywidth;

char *__progname;

void usage() {

  printf( 
	 "Scalpel carves files or data fragments from a disk image based on a set of\n"
	 "file carving patterns, which include headers, footers, and other information.\n\n"

	 "Usage: scalpel [-b] [-c <config file>] [-d] [-e] [-h] [-i <file>]\n"
	 "[-n] [-o <outputdir>] [-O] [-p] [-q <clustersize>] [-r]\n"  

	 /*	 "[-s] [-m <blockmap file>] [-M <blocksize>] [-n] [-o <outputdir>]\n" */
	 /*	 "[-O] [-p] [-q <clustersize>] [-r] [-s <num>] [-u <blockmap file>]\n" */

	 "[-v] [-V] <imgfile> [<imgfile>] ...\n\n"



	 "Options:\n"

	 "-b  Carve files even if defined footers aren't discovered within\n"
	 "    maximum carve size for file type [foremost 0.69 compat mode].\n"

	 "-c  Choose configuration file.\n"
	 
	 "-d  Generate header/footer database; will bypass certain optimizations\n"
	 "    and discover all footers, so performance suffers.  Doesn't affect\n"
	 "    the set of files carved.  **EXPERIMENTAL**\n"
  
	 "-e  Do nested header/footer matching, to deal with structured files that may\n"
	 "    contain embedded files of the same type.  Applicable only to\n"
	 "    FORWARD / NEXT patterns.\n"

	 "-h  Print this help message and exit.\n"

	 "-i  Read names of disk images from specified file.  Note that minimal parsing of\n"
	 "    the pathnames is performed and they should be formatted to be compliant C\n"
	 "    strings; e.g., under Windows, backslashes must be properly quoted, etc.\n"
  
	 /*

	 "-m  Use and update carve coverage blockmap file.  If the blockmap file does\n"
	 "    not exist, it is created. For new blockmap files, 512 bytes is used as\n"
	 "    a default blocksize unless the -M option overrides this value. In the\n"
	 "    blockmap file, the first 32bit unsigned int in the file identifies the\n"
	 "    block size.  Thereafter each 32bit unsigned int entry in the blockmap\n"
	 "    file corresponds to one block in the image file.  Each entry counts how\n"
	 "    many carved files contain this block. Requires more system resources.\n"
	 "    This feature is currently experimental.\n"

	 "-M  Set blocksize for new coverage blockmap file.\n"

	 */


	 "-n  Don't add extensions to extracted files.\n"

	 "-o  Set output directory for carved files.\n"
  
	 "-O  Don't organize carved files by type. Default is to organize carved files\n"
	 "    into subdirectories.\n"

	 "-p  Perform image file preview; audit log indicates which files\n"
	 "    would have been carved, but no files are actually carved.  Useful for\n"
	 "    indexing file or data fragment locations or supporting in-place file\n"
	 "    carving.\n"

	 "-q  Carve only when header is cluster-aligned.\n"
  
	 "-r  Find only first of overlapping headers/footers [foremost 0.69 compat mode].\n"

	 /*	 

	  "-s  Skip num bytes in each disk image before carving.\n"


	 "-u  Use (but don't update) carve coverage blockmap file when carving.\n"
	 "    Carve only sections of the image whose entries in the blockmap are 0.\n"
	 "    These areas are treated as contiguous regions.  This feature is\n"
	 "    currently experimental.\n"

	 */

	 "-V  Print copyright information and exit.\n"

	 "-v  Verbose mode.\n"
	  );
}


// signal handler, sets global variable 'signal_caught' which is
// checked periodically during carve operations.  Allows clean
// shutdown.
void catch_alarm(int signum) {
  signal_caught = signum;
  signal(signum, catch_alarm);

#ifdef __DEBUG
  fprintf(stderr, "\nCaught signal: %s.\n", (char *)strsignal(signum));
#endif

  fprintf(stderr, "\nKill signal detected. Cleaning up...\n");
}


int
extractSearchSpecData(struct scalpelState *state,
		      struct SearchSpecLine *s, char **tokenarray) {

  int err = 0;

  // process one line from config file:
  //     token[0] = suffix
  //     token[1] = case sensitive?
  //     token[2] = maximum carve size
  //     token[3] = begintag
  //     token[4] = endtag
  //     token[5] = search type (optional)

  s->suffix = (char *)malloc(MAX_SUFFIX_LENGTH * sizeof(char));
  checkMemoryAllocation(state, s->suffix, __LINE__, __FILE__, "s->suffix");
  s->begin = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
  checkMemoryAllocation(state, s->begin, __LINE__, __FILE__, "s->begin");
  s->end = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
  checkMemoryAllocation(state, s->end, __LINE__, __FILE__, "s->end");
  s->begintext = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
  checkMemoryAllocation(state, s->begintext, __LINE__, __FILE__, "s->begintext");
  s->endtext = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
  checkMemoryAllocation(state, s->endtext, __LINE__, __FILE__, "s->endtext");

  if(!strncasecmp(tokenarray[0],
		  SCALPEL_NOEXTENSION_SUFFIX,
		  strlen(SCALPEL_NOEXTENSION_SUFFIX))) {
    s->suffix[0] = SCALPEL_NOEXTENSION;
    s->suffix[1] = 0;
  }
  else {
    memcpy(s->suffix, tokenarray[0], MAX_SUFFIX_LENGTH);
  }

  // case sensitivity check
  s->casesensitive = (!strncasecmp(tokenarray[1], "y", 1) ||
		      !strncasecmp(tokenarray[1], "yes", 3));

  //#ifdef _WIN32
  //    s->length = _atoi64(tokenarray[2]);
  //#else
  //  s->length = atoull(tokenarray[2]);
  //#endif


  char split[MAX_STRING_LENGTH];
  char *maxcarvelength;

  strcpy(split, tokenarray[2]);
  maxcarvelength = strchr(split, ':');
  if(!maxcarvelength) {
    s->minlength = 0;
    s->length = strtoull(split, 0, 10);
  }
  else {
    *maxcarvelength = 0;
    maxcarvelength++;
    s->minlength = strtoull(split, 0, 10);
    s->length = strtoull(maxcarvelength, 0, 10);
  }

  // determine search type for this needle
  s->searchtype = SEARCHTYPE_FORWARD;
  if(!strncasecmp(tokenarray[5], "REVERSE", strlen("REVERSE"))) {
    s->searchtype = SEARCHTYPE_REVERSE;
  }
  else if(!strncasecmp(tokenarray[5], "NEXT", strlen("NEXT"))) {
    s->searchtype = SEARCHTYPE_FORWARD_NEXT;
  }
  // FORWARD is the default, but OK if the user defines it explicitly
  else if(!strncasecmp(tokenarray[5], "FORWARD", strlen("FORWARD"))) {
    s->searchtype = SEARCHTYPE_FORWARD;
  }

  // regular expressions must be handled separately

  if(isRegularExpression(tokenarray[3])) {
  
#ifdef GPU_THREADING
		// GPU execution does not support regex needles.
			fprintf(stderr, "ERROR: GPU search for regex headers is not supported!\n");
			fprintf(stderr, "Please modify the config file for non-regex headers only.\n");
			exit(1);
#endif

    // copy RE, zap leading/training '/' and prepare for regular expression compilation
    s->beginisRE = 1;
    strcpy(s->begin, tokenarray[3]);
    strcpy(s->begintext, tokenarray[3]);
    s->beginlength = strlen(tokenarray[3]);
    s->begin[s->beginlength] = 0;
    // compile regular expression  
    err = regncomp(&(s->beginstate.re), s->begin+1, s->beginlength-2,
		   REG_EXTENDED | (REG_ICASE * (!s->casesensitive)));
    if (err) {
      return SCALPEL_ERROR_BAD_HEADER_REGEX;
    }
  }
  else {
    // non-regular expression header
    s->beginisRE = 0;
    strcpy(s->begintext, tokenarray[3]);
    s->beginlength = translate(tokenarray[3]);
    memcpy(s->begin, tokenarray[3], s->beginlength);
    init_bm_table(s->begin, s->beginstate.bm_table, s->beginlength,
		  s->casesensitive);
  }

  if(isRegularExpression(tokenarray[4])) {
  
#ifdef GPU_THREADING
		// GPU execution does not support regex needles.
			fprintf(stderr, "ERROR: GPU search for regex footers is not supported!\n");
			fprintf(stderr, "Please modify the config file for non-regex footers only.\n");
			exit(1);
#endif  
  
    // copy RE, zap leading/training '/' and prepare for for regular expression compilation
    s->endisRE = 1;
    strcpy(s->end, tokenarray[4]);
    strcpy(s->endtext, tokenarray[4]);
    s->endlength = strlen(tokenarray[4]);
    s->end[s->endlength] = 0;
    // compile regular expression
    err = regncomp(&(s->endstate.re), s->end+1, s->endlength-2,
		   REG_EXTENDED | (REG_ICASE * (!s->casesensitive)));

    if(err) {
      return SCALPEL_ERROR_BAD_FOOTER_REGEX;
    }
  }
  else {
    s->endisRE = 0;
    strcpy(s->endtext, tokenarray[4]);
    s->endlength = translate(tokenarray[4]);
    memcpy(s->end, tokenarray[4], s->endlength);
    init_bm_table(s->end, s->endstate.bm_table, s->endlength, s->casesensitive);
  }

  return SCALPEL_OK;
}



int processSearchSpecLine(struct scalpelState *state, char *buffer, int lineNumber) {

  char *buf = buffer;
  char *token;
  char **tokenarray = (char **)malloc(6 * sizeof(char[MAX_STRING_LENGTH + 1]));
  int i = 0, err = 0, len = strlen(buffer);

  checkMemoryAllocation(state, tokenarray, __LINE__, __FILE__, "tokenarray");

  // murder CTRL-M (0x0d) characters
  //  if(buffer[len - 2] == 0x0d && buffer[len - 1] == 0x0a) {
  if (len >= 2 && buffer[len - 2] == 0x0d && buffer[len - 1] == 0x0a) {
    buffer[len - 2] = buffer[len - 1];
    buffer[len - 1] = buffer[len];
  }

  buf = (char *)skipWhiteSpace(buf);
  token = strtok(buf, " \t\n");

  // lines beginning with # are comments
  if(token == NULL || token[0] == '#') {
    return SCALPEL_OK;
  }

  // allow wildcard to be changed
  if(!strncasecmp(token, "wildcard", 9)) {
    if((token = strtok(NULL, " \t\n")) != NULL) {
      translate(token);
    }
    else {
      fprintf(stdout,
	      "Warning: Empty wildcard in configuration file line %d. Ignoring.\n",
	      lineNumber);
      return SCALPEL_OK;
    }

    if(strlen(token) > 1) {
      fprintf(stderr, "Warning: Wildcard can only be one character,"
	      " but you specified %d characters.\n"
	      "         Using the first character, \"%c\", as the wildcard.\n",
	      (int)strlen(token), token[0]);
    }

    wildcard = token[0];
    return SCALPEL_OK;
  }

  while (token && (i < NUM_SEARCH_SPEC_ELEMENTS)) {
    tokenarray[i] = token;
    i++;
    token = strtok(NULL, " \t\n");
  }

  switch (NUM_SEARCH_SPEC_ELEMENTS - i) {
  case 2:
    tokenarray[NUM_SEARCH_SPEC_ELEMENTS - 1] = (char *)"";
    tokenarray[NUM_SEARCH_SPEC_ELEMENTS - 2] = (char *)"";
    break;
  case 1:
    tokenarray[NUM_SEARCH_SPEC_ELEMENTS - 1] = (char *)"";
    break;
  case 0:
    break;
  default:
    fprintf(stderr,
	    "\nERROR: In line %d of the configuration file, expected %d tokens,\n"
	    "       but instead found only %d.\n",
	    lineNumber, NUM_SEARCH_SPEC_ELEMENTS, i);
    return SCALPEL_ERROR_NO_SEARCH_SPEC;
    break;

  }

  if((err =
      extractSearchSpecData(state, &(state->SearchSpec[state->specLines]),
			    tokenarray))) {
    switch (err) {
    case SCALPEL_ERROR_BAD_HEADER_REGEX:
      fprintf(stderr,
	      "\nERROR: In line %d of the configuration file, bad regular expression for header.\n",
	      lineNumber);
      break;
    case SCALPEL_ERROR_BAD_FOOTER_REGEX:
      fprintf(stderr,
	      "\nERROR: In line %d of the configuration file, bad regular expression for footer.\n",
	      lineNumber);
      break;

    default:
      fprintf(stderr,
	      "\nERROR: Unknown error on line %d of the configuration file.\n",
	      lineNumber);
    }
  }
  state->specLines++;
  return SCALPEL_OK;
}


// process configuration file
int readSearchSpecFile(struct scalpelState *state) {

  int lineNumber = 0, status;
  FILE *f;

  char *buffer =
    (char *)malloc((NUM_SEARCH_SPEC_ELEMENTS * MAX_STRING_LENGTH + 1) *
		   sizeof(char));
  checkMemoryAllocation(state, buffer, __LINE__, __FILE__, "buffer");

  f = fopen(state->conffile, "r");
  if(f == NULL) {
    fprintf(stderr,
	    "ERROR: Couldn't open configuration file:\n%s -- %s\n",
	    state->conffile, strerror(errno));
    free(buffer);
    return SCALPEL_ERROR_FATAL_READ;
  }

  while (fgets(buffer, NUM_SEARCH_SPEC_ELEMENTS * MAX_STRING_LENGTH, f)) {
    lineNumber++;

    if(state->specLines > MAX_FILE_TYPES) {
      fprintf(stderr, "Your conf file contains too many file types.\n");
      fprintf(stderr,
	      "This version was compiled with MAX_FILE_TYPES == %d.\n",
	      MAX_FILE_TYPES);
      fprintf(stderr, "Increase MAX_FILE_TYPES, recompile, and try again.\n");
      free(buffer);
      return SCALPEL_ERROR_TOO_MANY_TYPES;
    }

    if((status =
	processSearchSpecLine(state, buffer, lineNumber)) != SCALPEL_OK) {
      free(buffer);
      return status;
    }
  }

  // add an empty object to the end of the list as a marker

  state->SearchSpec[state->specLines].suffix = NULL;
  state->SearchSpec[state->specLines].casesensitive = 0;
  state->SearchSpec[state->specLines].length = 0;
  state->SearchSpec[state->specLines].begin = NULL;
  state->SearchSpec[state->specLines].beginlength = 0;
  state->SearchSpec[state->specLines].end = NULL;
  state->SearchSpec[state->specLines].endlength = 0;

  // GGRIII: offsets field is uninitialized--it doesn't
  // matter, since we won't use this entry.

  fclose(f);
  free(buffer);
  return SCALPEL_OK;
}

// Register the signal-handler that will write to the audit file and
// close it if we catch a SIGINT or SIGTERM 
void registerSignalHandlers() {
  if(signal(SIGINT, catch_alarm) == SIG_IGN) {
    signal(SIGINT, SIG_IGN);
  }
  if(signal(SIGTERM, catch_alarm) == SIG_IGN) {
    signal(SIGTERM, SIG_IGN);
  }

#ifndef _WIN32
  // *****GGRIII:  is this problematic?
  // From foremost 0.69:
  /* Note: I haven't found a way to get notified of 
     console resize events in Win32.  Right now the statusbar
     will be too long or too short if the user decides to resize 
     their console window while foremost runs.. */

  //    signal(SIGWINCH, setttywidth);
#endif
}

// initialize state variable and copy command line arguments
void initializeState(char **argv, struct scalpelState *state) {

  char **argvcopy = argv;
  int sss;
  int i;

  // Allocate memory for state 
  state->imagefile = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
  checkMemoryAllocation(state, state->imagefile, __LINE__, __FILE__,
			"state->imagefile");
  state->inputFileList = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
  checkMemoryAllocation(state, state->inputFileList, __LINE__, __FILE__,
			"state->inputFileList");
  state->conffile = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
  checkMemoryAllocation(state, state->conffile, __LINE__, __FILE__,
			"state->conffile");
  state->outputdirectory = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
  checkMemoryAllocation(state, state->conffile, __LINE__, __FILE__,
			"state->outputdirectory");
  state->invocation = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
  checkMemoryAllocation(state, state->invocation, __LINE__, __FILE__,
			"state->invocation");

  // GGRIII: memory allocation made more sane, because we're storing
  // more information in Scalpel than foremost had to, for each file
  // type.
  sss = (MAX_FILE_TYPES + 1) * sizeof(struct SearchSpecLine);
  state->SearchSpec = (struct SearchSpecLine *)malloc(sss);
  checkMemoryAllocation(state, state->SearchSpec, __LINE__, __FILE__,
			"state->SearchSpec");
  state->specLines = 0;

  // GGRIII: initialize header/footer offset data, carved file count,
  // et al.  The header/footer database is re-initialized in "dig.c"
  // after each image file is processed (numfilestocarve and
  // organizeDirNum are not). Storage for the header/footer offsets
  // will be reallocated as needed.

  for(i = 0; i < MAX_FILE_TYPES; i++) {
    state->SearchSpec[i].offsets.headers = 0;
    state->SearchSpec[i].offsets.footers = 0;
    state->SearchSpec[i].offsets.numheaders = 0;
    state->SearchSpec[i].offsets.numfooters = 0;
    state->SearchSpec[i].offsets.headerstorage = 0;
    state->SearchSpec[i].offsets.footerstorage = 0;
    state->SearchSpec[i].numfilestocarve = 0;
    state->SearchSpec[i].organizeDirNum = 0;
  }

  state->fileswritten = 0;
  state->skip = 0;
  state->organizeMaxFilesPerSub = MAX_FILES_PER_SUBDIRECTORY;
  state->modeVerbose = FALSE;
  state->modeNoSuffix = FALSE;
  state->useInputFileList = FALSE;
  state->carveWithMissingFooters = FALSE;
  state->noSearchOverlap = FALSE;
  state->generateHeaderFooterDatabase = FALSE;
  state->updateCoverageBlockmap = FALSE;
  state->useCoverageBlockmap = FALSE;
  state->coverageblocksize = 0;
  state->blockAlignedOnly = FALSE;
  state->organizeSubdirectories = TRUE;
  state->previewMode = FALSE;
  state->handleEmbedded = FALSE;
  state->auditFile = NULL;

  // default values for output directory, config file, wildcard character,
  // coverage blockmap directory
  strncpy(state->outputdirectory, SCALPEL_DEFAULT_OUTPUT_DIR,
	  strlen(SCALPEL_DEFAULT_OUTPUT_DIR));
  strncpy(state->conffile, SCALPEL_DEFAULT_CONFIG_FILE, MAX_STRING_LENGTH);
  state->coveragefile = state->outputdirectory;
  wildcard = SCALPEL_DEFAULT_WILDCARD;
  signal_caught = 0;
  state->invocation[0] = 0;

  // copy the invocation string into the state
  do {
    strncat(state->invocation,
	    *argvcopy, MAX_STRING_LENGTH - strlen(state->invocation));
    strncat(state->invocation,
	    " ", MAX_STRING_LENGTH - strlen(state->invocation));
    ++argvcopy;
  }
  while (*argvcopy);

  registerSignalHandlers();
}

// parse command line arguments
void processCommandLineArgs(int argc, char **argv, struct scalpelState *state) {
  char i;
  int numopts = 1;

  while ((i = getopt(argc, argv, "behvVu:ndpq:rc:o:s:i:m:M:O")) != -1) {
    numopts++;
    switch (i) {

    case 'V':
      fprintf(stdout, SCALPEL_COPYRIGHT_STRING);
      exit(1);

    case 'h':
      usage();
      exit(1);

    case 's':
      numopts++;
      state->skip = strtoull(optarg, NULL, 10);
      fprintf(stdout,
	      "Skipping the first %lld bytes of each image file.\n",
	      state->skip);
      break;

    case 'c':
      numopts++;
      strncpy(state->conffile, optarg, MAX_STRING_LENGTH);
      break;

    case 'd':
      state->generateHeaderFooterDatabase = TRUE;
      break;

    case 'e':
      state->handleEmbedded = TRUE;
      break;

      /*

    case 'm':
      numopts++;
      state->updateCoverageBlockmap = TRUE;
      state->useCoverageBlockmap = TRUE;
      state->coveragefile = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
      checkMemoryAllocation(state, state->coveragefile, __LINE__,
			    __FILE__, "state->coveragefile");
      strncpy(state->coveragefile, optarg, MAX_STRING_LENGTH);
      break;

    case 'u':
      numopts++;
      state->useCoverageBlockmap = TRUE;
      state->coveragefile = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
      checkMemoryAllocation(state, state->coveragefile, __LINE__,
			    __FILE__, "state->coveragefile");
      strncpy(state->coveragefile, optarg, MAX_STRING_LENGTH);
      break;

    case 'M':
      numopts++;
      state->coverageblocksize = strtoul(optarg, NULL, 10);
      if(state->coverageblocksize <= 0) {
	fprintf(stderr,
		"\nERROR: Invalid blocksize for -M command line option.\n");
	exit(1);
      }
      break;

      */

    case 'o':
      numopts++;
      strncpy(state->outputdirectory, optarg, MAX_STRING_LENGTH);
      break;

    case 'O':
      state->organizeSubdirectories = FALSE;
      break;

    case 'p':
      state->previewMode = TRUE;
      break;

    case 'b':
      state->carveWithMissingFooters = TRUE;
      break;

    case 'i':
      state->useInputFileList = TRUE;
      state->inputFileList = optarg;
      break;

    case 'n':
      state->modeNoSuffix = TRUE;
      fprintf(stdout, "Extracting files without filename extensions.\n");
      break;

    case 'q':
      numopts++;
      state->blockAlignedOnly = TRUE;
      state->alignedblocksize = strtoul(optarg, NULL, 10);
      if(state->alignedblocksize <= 0) {
	fprintf(stderr,
		"\nERROR: Invalid blocksize for -q command line option.\n");
	exit(1);
      }
      break;

    case 'r':
      state->noSearchOverlap = TRUE;
      break;

    case 'v':
      state->modeVerbose = TRUE;
      break;

    default:
      exit(1);
    }
  }

  // check for incompatible options

  if((state->useInputFileList || argc - numopts > 1) &&
     (state->updateCoverageBlockmap || state->useCoverageBlockmap)) {


    fprintf(stderr, "%d %d\n", argc, numopts);

    fprintf(stderr,
	    "\nCoverage blockmaps can be processed only if a single image filename is\n"
	    "specified on the command line.\n");
    exit(1);
  }
}

// full pathnames for all files used
void convertFileNames(struct scalpelState *state) {

  char fn[MAX_STRING_LENGTH]; // should be [PATH_MAX +1] from limits.h

  if(realpath(state->outputdirectory, fn)) {
    strncpy(state->outputdirectory, fn, MAX_STRING_LENGTH);
  }
  else {
    //		perror("realpath");
  }

  if(realpath(state->conffile, fn)) {
    strncpy(state->conffile, fn, MAX_STRING_LENGTH);
  }
  else {
    //		perror("realpath");
  }

}


// GGRIII: for each file, build header/footer offset database first,
// then carve files based on this database.  Need to clear the
// header/footer offset database after processing of each file.

void digAllFiles(char **argv, struct scalpelState *state) {

  int i = 0, j = 0;
  FILE *listoffiles = NULL;

  if(state->useInputFileList) {
    fprintf(stdout, "Batch mode: reading list of images from %s.\n",
	    state->inputFileList);
    listoffiles = fopen(state->inputFileList, "r");
    if(listoffiles == NULL) {
      fprintf(stderr, "Couldn't open file:\n%s -- %s\n",
	      (*(state->inputFileList) ==
	       '\0') ? "<blank>" : state->inputFileList, strerror(errno));
      handleError(state, SCALPEL_ERROR_FATAL_READ);
      exit(-1);
    }
    j = 0;
    do {
      j++;

      if(fgets(state->imagefile, MAX_STRING_LENGTH, listoffiles) == NULL) {
	if (!feof(listoffiles)) {
		fprintf(stderr,
			"Error reading line %d of %s. Skipping line.\n",
			j, state->inputFileList);
		}
	continue;
      }
      if(state->imagefile[strlen(state->imagefile) - 1] == '\n') {
	state->imagefile[strlen(state->imagefile) - 1] = '\x00';
      }

      // GGRIII: this function now *only* builds the header/footer
      // database.  Carving is handled afterward, in carveImageFile().

      if((i = digImageFile(state)) != SCALPEL_OK) {
	handleError(state, i);
		continue;
      }
      else {
	// GGRIII: "digging" is now complete and header/footer database
	// has been built.  The function carveImageFile() performs
	// extraction of files based on this database.

	if((i = carveImageFile(state)) != SCALPEL_OK) {
	  handleError(state, i);
	}
      }
    }
    while (!feof(listoffiles));
    fclose(listoffiles);
  }
  else {
    do {
      state->imagefile = *argv;

      // GGRIII: this function now *only* builds the header/footer
      // database.  Carving is handled afterward, in carveImageFile().

      if((i = digImageFile(state))) {
	handleError(state, i);
	continue;
      }
      else {
	// GGRIII: "digging" is now complete and header/footer database
	// has been built.  The function carveImageFile() performs extraction 
	// of files based on this database.

	if((i = carveImageFile(state))) {
	  handleError(state, i);
	}
      }
      ++argv;
    }
    while (*argv);
  }
}

int main(int argc, char **argv) {

  time_t starttime = time(0);
  struct scalpelState state;


  ///////// JUST  A TEST FOR WIN32 TSK LINKAGE ///////////
  //      TSK_TCHAR *imgs=_TSK_T("c:\\128MB.dd");
  //      TSK_IMG_INFO *img = tsk_img_open(_TSK_T("raw"), 1, (const TSK_TCHAR **)&imgs);
  //      printf("TSK size is %d bytes\n", (int)img->size);
  ///////// END JUST  A TEST FOR WIN32 TSK LINKAGE ///////////


  if(ldiv(SIZE_OF_BUFFER, SCALPEL_BLOCK_SIZE).rem != 0) {
    fprintf(stderr, SCALPEL_SIZEOFBUFFER_PANIC_STRING);
    exit(-1);
  }

#ifndef __GLIBC__
  setProgramName(argv[0]);
#endif

  fprintf(stdout, SCALPEL_BANNER_STRING);

  initializeState(argv, &state);
  processCommandLineArgs(argc, argv, &state);
  convertFileNames(&state);

  // read configuration file
  int err;
  if((err = readSearchSpecFile(&state))) {
    // problem with config file
    handleError(&state, err);
    exit(-1);
  }

  setttywidth();

  argv += optind;
  if(*argv != NULL || state.useInputFileList) {
    // prepare audit file and make sure output directory is empty.
    int err;
    if((err = openAuditFile(&state))) {
      handleError(&state, err);
      exit(-1);
    }
    
  	// Initialize the backing store of buffer to read-in, process image data.
  	init_store();

  	// Initialize threading model for cpu or gpu search.
  	init_threading_model(&state);
  		
    
    digAllFiles(argv, &state);
    closeAuditFile(state.auditFile);
  }
  else {
    usage();
    fprintf(stdout, "\nERROR: No image files specified.\n\n");
  }

#ifdef _WIN32
  fprintf(stdout,
	  "\nScalpel is done, files carved = %I64u, elapsed  = %ld secs.\n",
	  state.fileswritten, (int)time(0) - starttime);
#else
  fprintf(stdout,
	  "\nScalpel is done, files carved = %llu, elapsed  = %ld secs.\n",
	  state.fileswritten, (int)time(0) - starttime);
#endif

  return 0;
}
