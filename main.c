#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>

#define BUF_LEN 1024

// TODO: Handle adding trailing directory slash if not present

/**
 * Handle file saved
 */
void handleFile(char *dir, char *fileName, char *script, char *argv[]){
  int pathLen, dirLen;
  char *path;


  // Fork
  pid_t pid = fork();
  if (pid < 0){
    fprintf(stderr, "Error: fork()\n");
  }

  if (pid == 0){
    // Child

    // Concatenate dir with filename
    dirLen = strlen(dir);
    if (dir[dirLen-2] != '/'){
      // Add trailing slash
      pathLen = dirLen + strlen(fileName) + 1;
      path = malloc(pathLen + 1);
      strcpy(path, dir);
      path[dirLen] = '/';
      strcpy(path+dirLen+1, fileName);
    } else {
      // No need for slash
      pathLen = dirLen + strlen(fileName);
      path = malloc(pathLen + 1);
      strcpy(path, dir);
      strcpy(path+dirLen, fileName);
    }

    printf("Child handling file '%s'\n", path);

    // Open file and assign to stdin
    int fd = open(path, O_RDONLY);
    if (fd < 0){
      perror("child-open");
      exit(EXIT_FAILURE);
    }
    if (dup2(fd, STDIN_FILENO) < 0){
      perror("child-dup2");
      exit(EXIT_FAILURE);
    }
    close(fd);

    // Pass off to script
    //execl(script, script, NULL);
    execv(script, argv);
    perror("child-execl");
    
    exit(EXIT_FAILURE);
  }
}


/**
 * Main body loop
 * @param fd inotify watch descriptor
 */
int mainLoop(int fd, char *dir, char *script, char *argv[]){
  char buf[BUF_LEN] __attribute__ ((aligned(8)));
  struct inotify_event *eInote;

  for (;;){
    int len = read(fd, buf, BUF_LEN);
    if (len < 0){
      perror("read()");
      return EXIT_FAILURE;
    }

    if (len == 0)
      return EXIT_SUCCESS;
    
    if (len < sizeof(struct inotify_event)){
      fprintf(stderr, "Partial read\n");
      continue;
    }

    // Cast to event struct
    eInote = (struct inotify_event *) buf;
    handleFile(dir, eInote->name, script, argv);
  }
  return EXIT_SUCCESS;
}


/**
 * Takes two args: path to watch and script to handle it
 */
int main(int argc, char *argv[]){
  char *watchPath, *script;
  int fdWatch, fdInote;

  // Check args
  if (argc < 3){
    fprintf(stderr, "Usage: %s [path to watch] [script]\n", argv[0]);
    return EXIT_FAILURE;
  }
  watchPath = argv[1];
  script = argv[2];
  
  // Setup inotify
  fdInote = inotify_init();
  if (fdInote <= 0){
    fprintf(stderr, "System Error: inotify_init\n");
    return EXIT_FAILURE;
  }
  fdWatch = inotify_add_watch(fdInote, watchPath, IN_CLOSE_WRITE);
  if (fdWatch <= 0){
    fprintf(stderr, "System Error: inotify_add_watch\n");
    return EXIT_FAILURE;
  }

  // Watch loop
  int ret = mainLoop(fdInote, watchPath, script, argv + 2);

  // Shutdown
  if (inotify_rm_watch(fdInote, fdWatch)){
    fprintf(stderr, "System Error: inotify_rm_watch\n");
    return EXIT_FAILURE;
  }
  return ret;
}
