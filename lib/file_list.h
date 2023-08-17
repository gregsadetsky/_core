unsigned int extract_bpm(const char *input) {
  int len = strlen(input);
  int i = 0;
  bool foundBPM = false;

  while (i < len) {
    // Check for the pattern "bpm"
    if (input[i] == 'b' && input[i + 1] == 'p' && input[i + 2] == 'm') {
      foundBPM = true;
      i += 3;  // Skip "bpm"
      break;
    }
    i++;
  }

  // If "bpm" is found, extract the number
  if (foundBPM) {
    char numberBuffer[10];
    int j = 0;

    while (i < len && isdigit(input[i])) {
      numberBuffer[j] = input[i];
      i++;
      j++;
    }

    numberBuffer[j] = '\0';
    int bpmValue = atoi(numberBuffer);

    return bpmValue;
  } else {
    return 1;  // If "bpm" is not found, return 1 to indicate failure
  }
}

unsigned int extract_beats(const char *input) {
  int len = strlen(input);
  int i = 0;
  bool foundBeats = false;

  while (i < len) {
    // Check for the pattern "bpm"
    if (input[i] == 'b' && input[i + 1] == 'e' && input[i + 2] == 'a' &&
        input[i + 3] == 't' && input[i + 4] == 's') {
      foundBeats = true;
      i += 5;  // Skip "beats"
      break;
    }
    i++;
  }

  // If "bpm" is found, extract the number
  if (foundBeats) {
    char numberBuffer[10];
    int j = 0;

    while (i < len && isdigit(input[i])) {
      numberBuffer[j] = input[i];
      i++;
      j++;
    }

    numberBuffer[j] = '\0';
    int bpmValue = atoi(numberBuffer);

    return bpmValue;
  } else {
    return 1;  // If "bpm" is not found, return 1 to indicate failure
  }
}

#define WAV_HEADER_SIZE 44
#define FileList_max 255
typedef struct FileList {
  char **name;
  uint32_t *size;
  unsigned int *bpm;
  unsigned int *beats;
  unsigned int num;
} FileList;

FileList *list_files(const char *dir) {
  FileList *filelist = malloc(sizeof(FileList));
  filelist->name = malloc(sizeof(char *) * FileList_max);
  filelist->size = malloc(sizeof(FSIZE_t) * FileList_max);
  filelist->bpm = malloc(sizeof(unsigned int) * FileList_max);
  filelist->beats = malloc(sizeof(unsigned int) * FileList_max);
  filelist->num = 0;

  char cwdbuf[FF_LFN_BUF] = {0};
  FRESULT fr; /* Return value */
  char const *p_dir;
  if (dir[0]) {
    p_dir = dir;
  } else {
    fr = f_getcwd(cwdbuf, sizeof cwdbuf);
    if (FR_OK != fr) {
      printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
      return filelist;
    }
    p_dir = cwdbuf;
  }
  DIR dj;      /* Directory object */
  FILINFO fno; /* File information */
  memset(&dj, 0, sizeof dj);
  memset(&fno, 0, sizeof fno);
  fr = f_findfirst(&dj, &fno, p_dir, "*");
  if (FR_OK != fr) {
    printf("f_findfirst error: %s (%d)\n", FRESULT_str(fr), fr);
    return filelist;
  }
  while (fr == FR_OK && fno.fname[0]) { /* Repeat while an item is found */
    /* Create a string that includes the file name, the file size and the
     attributes string. */
    const char *pcWritableFile = "writable file",
               *pcReadOnlyFile = "read only file", *pcDirectory = "directory";
    const char *pcAttrib;
    /* Point pcAttrib to a string that describes the file. */
    if (fno.fattrib & AM_DIR) {
      pcAttrib = pcDirectory;
    } else if (fno.fattrib & AM_RDO) {
      pcAttrib = pcReadOnlyFile;
    } else {
      pcAttrib = pcWritableFile;
    }
    /* Create a string that includes the file name, the file size and the
     attributes string. */
    // printf("%s [%s] [size=%llu]\n", fno.fname, pcAttrib, fno.fsize);

    // Check if the filename ends with ".wav"
    if (strstr(fno.fname, ".wav") && strstr(fno.fname, "bpm") &&
        strstr(fno.fname, "beats")) {
      // Allocate memory for the name field and copy the filename into it
      filelist->name[filelist->num] = malloc(strlen(fno.fname) + 1);
      strcpy(filelist->name[filelist->num], fno.fname);
      filelist->size[filelist->num] = (uint32_t)(fno.fsize - WAV_HEADER_SIZE);
      filelist->bpm[filelist->num] = extract_bpm(fno.fname);
      filelist->beats[filelist->num] = extract_beats(fno.fname);
      filelist->num++;
    }

    fr = f_findnext(&dj, &fno); /* Search for next item */
  }
  f_closedir(&dj);
  return filelist;
}