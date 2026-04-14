#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <signal.h>
#include <dirent.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <jpeglib.h>
#include <jerror.h>
#include <png.h>
#include <webp/types.h>
#include <webp/decode.h>
#include <webp/demux.h>
#include <vpx/vpx_decoder.h>

#define THREADS 4 //app threads (in addition to inotify loop)
#define QUEUE_DEPTH 64 //per thread. power of 2

#define MAX_IMG_SIZE 25165824 //24MB

#define MAX_EVENTS 256
#define LEN_NAME 128
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (MAX_EVENTS * (EVENT_SIZE + LEN_NAME))

#define ICON_DIR_IN "/var/www/cocktailkeeper/images"
#define ICON_DIR_OUT "/var/www/cocktailkeeper/img/"
#define ICON_DIR_OUT_LEN 28

#define ICON_WIDTH 156
#define ICON_HEIGHT 156

#define ICON_AR ICON_WIDTH / ICON_HEIGHT

#define IMG_OWNER_USER_ID 33

#define WEIGHTED_AVG 1
#define UNWEIGHTED_AVG 0

void* crop_img(void*);

//GLOBALS
int queuePtr = 0, threadQuePtr = 0, qhead = 0, qtail[THREADS] = {0};
pthread_t thread[THREADS];
sigset_t signal_mask;


struct queue_t {
  int enqueued;
  char inFileName[64];
};
struct queue_t queue_global[THREADS * QUEUE_DEPTH];


struct imageMetaData_t {
  int width, height, colorspace, rawImgSize, destImgSize;
  unsigned char *pixelData, *encodedImg;
};
struct imageMetaData_t __attribute__((aligned(32))) imageMetaData[THREADS];

#include "filters.c"
#include "scaling.c"
#include "seam_carving.c"
#include "codecs.c"



void slog(const char *action, const char *target) {
  time_t t = time(NULL);
  struct tm* time = localtime(&t);
  char time_str[20];
  strftime(time_str, 20, "%F %T", time);
  FILE* fp = fopen("/var/log/ckeeper.scaler.log", "a");
  fprintf(fp, "%s %s %s\n", time_str, action, target);
  fclose(fp);
}



inline int strtail(register char *s1, const register char *s2) {
  register int n = 0;
  while(*s2) {
    *s1++ = *s2++;
    n += 1;
  }
  *s1 = 0;
  return n;
}



void dirscan(const char *basePath, const char *outPath, double ar, int h) {
  struct dirent* dp;
  DIR* dfd = opendir(basePath);
  if(dfd == NULL) return;

  while((dp = readdir(dfd)) != NULL) {
    if(dp->d_type == DT_REG) {
      strtail(queue_global[queuePtr].inFileName, dp->d_name);

      while(queue_global[queuePtr].enqueued) //don't overrun queues
        sleep(1);

      //ADD TO QUEUE
      queue_global[queuePtr].enqueued = 1;
      //wake thread if it's not busy
      if(qhead - qtail[threadQuePtr] < 2)
        pthread_kill(thread[threadQuePtr], SIGCONT);

      //INCREMENT QUEUE PTR
      threadQuePtr += 1;
      qhead += (threadQuePtr == THREADS);
      threadQuePtr = (threadQuePtr == THREADS) ? 0 : threadQuePtr;
      qhead &= QUEUE_DEPTH - 1;
      queuePtr = threadQuePtr * QUEUE_DEPTH + qhead;
    }
  }
  closedir(dfd);
}



int main(void) {
  //check folder perms
  if(access(ICON_DIR_IN, F_OK|R_OK|W_OK) != 0 || access(ICON_DIR_OUT, F_OK|R_OK|W_OK) != 0) {
    slog("DAEMON", "folder permissions failed.");
    return 1;
  }

  daemon(0, 0);
  chdir(ICON_DIR_IN);
  umask(0);
  slog("DAEMON", "imgcrop started.");

  //block/catch signals
  sigemptyset(&signal_mask);
  sigaddset(&signal_mask, SIGCONT);
  sigprocmask(SIG_SETMASK, &signal_mask, NULL); //block SIGCONT (inherited by all threads)

  memset(queue_global, 0, sizeof(queue_global));
  memset(imageMetaData, 0, sizeof(imageMetaData));

  //THREAD POOL
  for(long long coreOffset = 0; coreOffset < THREADS; coreOffset++)
    pthread_create(&thread[coreOffset], NULL, &crop_img, (void*)coreOffset);

  //INOTIFY WATCH
  int ifd, icon_wd, length, msgQueue;
  char buffer[BUF_LEN] __attribute__ ((aligned(__alignof__(struct inotify_event))));

  ifd = inotify_init();
  if(ifd == -1) {
    slog("DAEMON", "inotify init failed.");
    return 2;
  }

  icon_wd = inotify_add_watch(ifd, ICON_DIR_IN, IN_CLOSE_WRITE | IN_MOVED_TO);

  //check for backlog
  dirscan(ICON_DIR_IN, ICON_DIR_OUT, ICON_AR, ICON_HEIGHT);


  //INOTIFY LISTEN
  int pathLen, outLen, nameLen;
  while(1) {
    msgQueue = 0;
    length = read(ifd, buffer, sizeof(buffer));
    while(msgQueue < length) {

      struct inotify_event* event = (struct inotify_event *) &buffer[msgQueue];
      msgQueue += sizeof(struct inotify_event) + event->len;
      if(!event->len) break;

      if(icon_wd != event->wd)
        continue;

      strtail(queue_global[queuePtr].inFileName, event->name);

      if(event->mask & (IN_CLOSE_WRITE | IN_MOVED_TO)) {
        if(event->mask & IN_ISDIR)
          continue;
        else { //ADD TO QUEUE
          while(queue_global[queuePtr].enqueued) //don't overrun queues
            sleep(1);
          queue_global[queuePtr].enqueued = 1;
          //wake thread if it's not busy
          if(qhead - qtail[threadQuePtr] < 2)
            pthread_kill(thread[threadQuePtr], SIGCONT);
        }
      }
      //INCREMENT QUEUE PTR
      threadQuePtr += 1;
      qhead += (threadQuePtr == THREADS);
      threadQuePtr = (threadQuePtr == THREADS) ? 0 : threadQuePtr;
      qhead &= QUEUE_DEPTH - 1;
      queuePtr = threadQuePtr * QUEUE_DEPTH + qhead;
    }
  }

  close(ifd);
  return 0;
}



void* crop_img(void* arg) {

  int fd, rc, err, signumber, threadNumber = (long long)arg;
  int *qPtr = &qtail[threadNumber];
  struct queue_t *_queue = &queue_global[threadNumber * QUEUE_DEPTH];
  struct stat sb;
  unsigned char *srcImg = malloc(MAX_IMG_SIZE);
  char outPath[128], userDir[128];
  strcpy(outPath, ICON_DIR_OUT);
  strcpy(userDir, ICON_DIR_OUT);

  for(;;) {
    while(_queue[*qPtr].enqueued == 0) //no conn avail
      sigwait(&signal_mask, &signumber); //wait for signal

    //open file
    fd = open(_queue[*qPtr].inFileName, O_RDONLY);
    if(fd <= 0) {
      slog("IMG OPEN", _queue[*qPtr].inFileName);
      goto clear_queue;
    }
    fstat(fd, &sb); //file size
    if(sb.st_size < 128 || sb.st_size > MAX_IMG_SIZE) {
      slog("IMG SIZE TOO BIG OR SMALL", _queue[*qPtr].inFileName);
      close(fd);
      goto clear_queue;
    }

    //read src img file
    rc = 0;
    while(rc < sb.st_size)
      rc += read(fd, srcImg + rc, sb.st_size - rc);
    close(fd);

    /* decode */

    //int src_webp = 0;
    if(srcImg[0] == 0xFF && srcImg[1] == 0xD8 && srcImg[2] == 0xFF) { //JPEG
      jpeg_decode(srcImg, sb.st_size, threadNumber);
      //todo add jpeg decode err check
    }
    else if(srcImg[1] == 'P' && srcImg[2] == 'N' && srcImg[3] == 'G') { //PNG
      err = png_decode(srcImg, sb.st_size, threadNumber);
      if(err) {slog("IMG png decompress failed", _queue[*qPtr].inFileName); goto clear_queue;}
    }
    else if(srcImg[8] == 'W' && srcImg[9] == 'E' && srcImg[10] == 'B') { //WEBP
      //src_webp = 1;
      err = webp_decode(srcImg, sb.st_size, threadNumber);
      if(err) {slog("IMG webp decompress failed", _queue[*qPtr].inFileName); goto clear_queue;}
    }
    else if(srcImg[0] == 'B' && srcImg[1] == 'M') { //BMP
      err = bmp_decode(srcImg, sb.st_size, threadNumber);
      if(err == 1) {slog("IMG bitmap compression not supported", _queue[*qPtr].inFileName); goto clear_queue;}
      if(err == 2) {slog("IMG bitmap colorspace only supports 24/32bpp", _queue[*qPtr].inFileName); goto clear_queue;}
    }
    else {
      slog("IMG FORMAT INVALID", _queue[*qPtr].inFileName);
      //unlink(_queue[*qPtr].path);
      goto clear_queue;
    }

    /* scaling */

    double scaleRatio = (double)imageMetaData[threadNumber].height / ICON_HEIGHT;
    int scaleRatioInt = (int)scaleRatio;
    scaleRatioInt += (scaleRatio - scaleRatioInt > 0.75);
    if(scaleRatio > 1.3 && scaleRatio <= 1.75)
      scaleRatioInt = 96; // 2/3

    while(scaleRatioInt > 1) {

    switch(scaleRatioInt) {
      case 2: scale_by_half(UNWEIGHTED_AVG, threadNumber); break; // 1/2
      case 3: scale_by_third(threadNumber); break; // 1/3
      case 4:
      case 5: scale_by_quarter(threadNumber); break; // 1/4
      case 6:
      case 7: scale_by_third(threadNumber); scale_by_half(UNWEIGHTED_AVG, threadNumber); break; // 1/6
      case 8: scale_by_quarter(threadNumber); scale_by_half(UNWEIGHTED_AVG, threadNumber); break; // 1/8
      case 9:
      case 10: scale_by_third(threadNumber); scale_by_third(threadNumber); break; // 1/9
      case 11:
      case 12:
      case 13:
      case 14: scale_by_quarter(threadNumber); scale_by_third(threadNumber); break; // 1/12
      case 96: scale_by_double(threadNumber); scale_by_third(threadNumber); break; // 2/3
      default: scale_by_quarter(threadNumber); scale_by_quarter(threadNumber); break; // 1/16
    }

    scaleRatio = (double)imageMetaData[threadNumber].height / ICON_HEIGHT;
    scaleRatioInt = (int)scaleRatio;
    scaleRatioInt += (scaleRatio - scaleRatioInt > 0.75);
    if(scaleRatio > 1.3 && scaleRatio <= 1.75)
      scaleRatioInt = 96; // 2/3

    }

    //scale max width, since height is not scaled to exact value
    int scaledWidth = imageMetaData[threadNumber].height * ICON_AR; //max width ratio
    if(imageMetaData[threadNumber].width > scaledWidth) //seam carving, width only
      seam_carving(scaledWidth, threadNumber);

    /* encode */

    err = png_encode(threadNumber);
    if(err) {slog("IMG encoding err", _queue[*qPtr].inFileName); goto clear_queue;}

    /* write file */

    //out path
    strtail(userDir + ICON_DIR_OUT_LEN, _queue[*qPtr].inFileName);
    rc = strtail(outPath + ICON_DIR_OUT_LEN, _queue[*qPtr].inFileName);
    strtail(outPath + ICON_DIR_OUT_LEN + rc, ".png");
    for(int i = ICON_DIR_OUT_LEN; i < ICON_DIR_OUT_LEN + 33; i++) {
      outPath[i] = (outPath[i] == '~') ? '/' : outPath[i];
      userDir[i] = (userDir[i] == '~') ? 0 : userDir[i];
    }

    //make user's sub dir
    mkdir(userDir, S_IRWXU|S_IRWXG|S_IRWXO);

    fd = open(outPath, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
    err = 0;
    while(err < imageMetaData[threadNumber].destImgSize && err >= 0)
      err += write(fd, imageMetaData[threadNumber].encodedImg + err, imageMetaData[threadNumber].destImgSize - err); //todo loop write
    fchown(fd, IMG_OWNER_USER_ID, IMG_OWNER_USER_ID);
    //fchmod(fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH); //436
    close(fd);
    if(err == imageMetaData[threadNumber].destImgSize) //delete src img if write succeeds
      unlink(_queue[*qPtr].inFileName);
    else
      slog("IMG write err", outPath);

    clear_queue:
    if(imageMetaData[threadNumber].pixelData) {
      free(imageMetaData[threadNumber].pixelData); //todo reuse
      imageMetaData[threadNumber].pixelData = 0;
    }
    if(imageMetaData[threadNumber].encodedImg) {
      free(imageMetaData[threadNumber].encodedImg); //todo reuse
      imageMetaData[threadNumber].encodedImg = 0;
    }
    /* increment queue tail ptr */
    _queue[*qPtr].enqueued = 0;
    *qPtr += 1;
    *qPtr &= QUEUE_DEPTH - 1;

  } //end main loop
}
