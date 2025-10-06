#define PROJECT_NAME "Webcrawler"
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#define NOB_EXPERIMENTAL_DELETE_OLD

#define BUILD_FOLDER "./dist"
#define VENDORS_FOLDER "./vendors"
#define OBJ_FOLDER "./objs"
#define SRC_FOLDER "./src"
#define D_BUFF_SIZE 10

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#include "./includes/imported/nob.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>


typedef struct __list__ {
  char** buff;
  size_t csize;
  size_t isize;
} path_list;

path_list* plist_init()
{
  path_list* plist = (path_list*)malloc(sizeof(path_list));
  if(!plist) {
    perror("List structure error");
    exit(-1);
  }
  plist->csize = D_BUFF_SIZE;
  plist->isize = 0;
  plist->buff = (char**)malloc(sizeof(char*)*plist->csize);

  if(!plist->buff) {
    perror("list mem init error");
    exit(-1);
  }

  return plist;
}

void __plist_buff_resize(path_list* list)
{
  if(!list) {
    perror("list structure uninitialized: resize");
    exit(-1);
  }

  list->csize *= 2;

  list->buff = (char**)realloc(list->buff, list->csize);

  if(!list->buff) {
    perror("buffer resize error");
    exit(-1);
  }
}

void plist_push(path_list* list, char* path)
{
  if(!list || !list->buff) {
    perror("append failed list uninitialized!");
    exit(-1);
  }

  if(list->isize == list->csize) {
    __plist_buff_resize(list);
  }

  list->buff[list->isize++] = strdup(path);
}

char* plist_pop(path_list* list)
{
  if(list->isize == 0) {
    return "";
  }

  char* result = list->buff[list->isize-1];
  list->isize--;
  return result;
}

void __printl(path_list* list)
{
  for(size_t i = 0; i < list->isize; i++) {
    printf("[INFO] compiling %s\n", list->buff[i]);
  }
}

int plist_empty(path_list* list)
{
  if(list->isize > 0) return 0;
  return 1;
}

void plist_free(path_list* list)
{
  for(size_t i = 0; i < (list->isize); i++) {
    free(list->buff[i]);
  }
  free(list->buff);
  free(list);
}

const char* get_filename_ext(const char* filename)
{
  const char* dot = strrchr(filename,'.');

  if(!dot||dot==filename) return "";

  return dot+1;
}

char* path_mat(const char* start, const char* end)
{
  int start_size = strlen(start);
  int slash_size = strlen("/");
  int end_size = strlen(end);
  
  char* fpath = (char*)malloc(sizeof(char)*(start_size+end_size+slash_size+2));
  sprintf(fpath, "%s/%s\0", start, end);

  return fpath;
}

int ext_check(char* path) {
  const char* ext = get_filename_ext(path);
  
  if(strcmp(ext, "c") == 0) {
    return 0;
  }
  
  return 1;
}

void parse_dirs(path_list* cFiles, DIR* sdir, struct dirent* content, char* path) {
  char* cpath = path;
  while((content = readdir(sdir)) != NULL) {;
    if(strcmp(content->d_name, ".") == 0 || strcmp(content->d_name, "..") == 0) {
      continue;
    }

    struct stat filestat;

    char* statPath = path_mat(cpath, content->d_name);
    if(stat(statPath, &filestat) == -1) {
      printf("[INFO] %s failed compiling\n", statPath);
      perror("stat failed");
      exit(-1);
    }

    if(S_ISDIR(filestat.st_mode)) {
      char* npath = cpath;
      strcat(npath, "/");
      strcat(npath, content->d_name);
      
      DIR* src;
      struct dirent* ncontent;

      if((src = opendir(npath)) == NULL) {
        perror("cwd error: cannot open cwd");
        exit(-1);
      }
      
      parse_dirs(cFiles, src, ncontent, npath);
      closedir(src);
    } else if(strcmp(get_filename_ext(content->d_name), "c") == 0) {
      char* pushPath = path_mat(cpath, content->d_name);
      plist_push(cFiles, pushPath);
    }

  }

  //free(cpath);
  free(content);
}

const char* normalize_path(const char* path)
{
  const char* dot = strrchr(path, '.');

  if(!dot) return "";

  return dot+1;
}

char* cto(const char* cfile)
{
  char* ofile = strdup(cfile);
  
  char* dot = strrchr(ofile, '.');

  if(!dot) return NULL;

  int len = dot-ofile;
  
  char* buffer = (char*)malloc(sizeof(char)*(len+10));

  if(!buffer) {
    perror("buffer uninitialized:");
    exit(-1);
  }

  strncpy(buffer, ofile, len);
  buffer[len] = '\0';
  strcat(buffer, ".o");

  free(ofile);
  
  return buffer;
}

void replace_path(char* target, char* substring, char* replacement)
{
  static char buffer[1024];
  char* pos;

  pos = strstr(target, substring);

  if(pos == NULL) return;

  int strLenToS = pos - target;

  //sprintf(buffer, "%.*s/%s/%s", strLenToS, target, replacement, pos+1);
  strncpy(buffer, target, strLenToS);

  buffer[strLenToS] = '\0';

  strcat(buffer, replacement);
  strcat(buffer, pos+strlen(substring));
  
  strcpy(target, buffer);
;
}

char* get_path_nf(const char* path)
{
  const char* slash = strrchr(path, '/');

  const int pos = slash - path;

  char* buffer = (char*)malloc(sizeof(char)*(pos+10));

  sprintf(buffer, "%.*s\0", pos, path);

  return buffer;
}

int main(int argc, char** argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);
  Nob_Cmd cmd = {0};

  if(!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;
  if(!nob_mkdir_if_not_exists(VENDORS_FOLDER)) return 1;

  if(!nob_mkdir_if_not_exists(OBJ_FOLDER)) return 1;

  DIR* src;
  struct dirent *content;

  if((src = opendir(SRC_FOLDER)) == NULL) {
    perror("cwd error: cannot open cwd");
    return EXIT_FAILURE;
  }

  path_list* cFiles = plist_init();

  char cwd[100];
  char* srcd = (char*)malloc(sizeof(SRC_FOLDER)/sizeof(char));
  

  getcwd(cwd, sizeof(cwd));

  strcat(cwd, normalize_path(SRC_FOLDER));

  parse_dirs(cFiles, src, content, cwd);

  __printl(cFiles);

  path_list* oPathList = plist_init();
 
  while(plist_empty(cFiles) == 0) {
    nob_cmd_append(&cmd, "cc");
    nob_cmd_append(&cmd, "-Wall", "-Wextra", "-I./includes");
    const char* cFileEnr = plist_pop(cFiles);
    char* cFile = strdup(cFileEnr);
    nob_cmd_append(&cmd, "-c", cFile);
    char* oFile = cto(cFileEnr);

    replace_path(oFile, "src", "objs");

    const char* odir = get_path_nf(oFile);

    /* printf("cfile: %s\n ofile %s\n odir %s\n", cFile, oFile, odir); */
    if(!nob_mkdir_if_not_exists(odir)) return 1;
    nob_cmd_append(&cmd, "-o", oFile);
    

    if(!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    //    printf("no error\n");

    plist_push(oPathList, oFile);
  }

  nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-I./includes");
  while(!plist_empty(oPathList)) {
    const char* oFile = plist_pop(oPathList);

    nob_cmd_append(&cmd, oFile);
  }

  nob_cmd_append(&cmd, "-o", BUILD_FOLDER "/" PROJECT_NAME);

  if(!nob_cmd_run_sync_and_reset(&cmd)) return 1;


  if(argc > 1) {
    for(int i = 1; i < argc; i++) {
      if(strcmp(argv[i], "run") == 0) {
        int pipefd[2];
        char* buffer[1024];
        ssize_t size;

        if(pipe(pipefd) < 0) exit(1);
        
        pid_t wbexecProcPID = fork();


        if(wbexecProcPID == 0) {

          close(pipefd[0]);
          
          printf("[INFO] running process\n````````````\n\n");
          dup2(pipefd[1], STDOUT_FILENO);
          close(pipefd[1]);
          
          execv("./dist/" PROJECT_NAME, 0);

          exit(127);
        }else {

          close(pipefd[1]);

          while((size = read(pipefd[0], buffer, sizeof(buffer)-1)) > 0) {
            buffer[size] = '\0';
            printf(ANSI_COLOR_GREEN "%s" ANSI_COLOR_RESET, buffer);
          } 

          close(pipefd[0]);
          waitpid(wbexecProcPID, 0, 0);
          kill(wbexecProcPID, 0);
          
          printf("\n\n```````````\n\n");
          printf("[INFO] process finished successfully\n");
        }
      }
    }
  }
  closedir(src);
  plist_free(cFiles);
  plist_free(oPathList);

  printf("[INFO] nob exit\n");
  return 0;
}
