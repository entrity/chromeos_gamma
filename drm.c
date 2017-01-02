#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#define DEBUG_STR "debug"
#define SAVE_ORIG "bak"
#define SAVE_FILE "save"
#define LOAD_FILE "load"
#define DIVIDE_2 "divide"
#define MULTIP_2 "mult"
#define ORIG_GAMMA_FILE "orig_gamma_table.dat"

#define GET_GAMMA(dat) drmModeCrtcGetGamma(dat->fd, dat->crtc_id, dat->crtc->gamma_size, dat->r, dat->g, dat->b)
#define SET_GAMMA(dat) drmModeCrtcSetGamma(dat->fd, dat->crtc_id, dat->crtc->gamma_size, dat->r, dat->g, dat->b)

struct crtc_data {
  uint16_t *r, *g, *b; // gammas
  int fd;
  uint32_t crtc_id;
  drmModeCrtc *crtc;
};

/*GLOBALS*/

char do_debug, do_back, do_multiply, do_divide;
char *do_save, *do_load;
int back_fd = -2, save_fd = -2, load_fd = -2;

/*STATIC FUNCTIONS*/

static void error(char * msg)
{
  perror(msg);
  exit(errno || -1);
}

static void save_table(int fd, struct crtc_data *dat)
{
  GET_GAMMA(dat);
  size_t gamma_size = dat->crtc->gamma_size;
  write(fd, dat->r, gamma_size*sizeof(uint16_t));
  write(fd, dat->g, gamma_size*sizeof(uint16_t));
  write(fd, dat->b, gamma_size*sizeof(uint16_t));
  puts("wrote one table\n");
}

static void load_table(int fd, struct crtc_data *dat)
{
  size_t gamma_size = dat->crtc->gamma_size;
  read(fd, dat->r, gamma_size*sizeof(uint16_t));
  read(fd, dat->g, gamma_size*sizeof(uint16_t));
  read(fd, dat->b, gamma_size*sizeof(uint16_t));
  puts("read one table\n");
  SET_GAMMA(dat);
}

static void debug(struct crtc_data *dat)
{
  size_t gamma_size = dat->crtc->gamma_size;
  GET_GAMMA(dat);
  for (int gidx = 0; gidx < gamma_size; gidx++) {
    printf("\t\tgamma(%3d) %5d %5d %5d\n", gidx, dat->r[gidx], dat->g[gidx], dat->b[gidx]);
  }
}

static void divide_gamma(struct crtc_data *dat)
{
  GET_GAMMA(dat);
  size_t gamma_size = dat->crtc->gamma_size;
  for (int i = 0; i < gamma_size; i++) {
    dat->r[i] = dat->r[i] / 2;
    dat->g[i] = dat->g[i] / 2;
    dat->b[i] = dat->b[i] / 2;
  }
  // SET_GAMMA(dat);
  lseek(dat->fd, 0, SEEK_SET);
  int ret = drmModeCrtcSetGamma(dat->fd, dat->crtc_id, dat->crtc->gamma_size, dat->r, dat->g, dat->b);
  printf("set gamma after dividing: %d\n", ret);
}

static void multiply_gamma()
{
  
}

void iterate_crtcs(int fd)
{
  int save_fd, backup_fd, load_fd;
  drmModeResPtr res = drmModeGetResources(fd);
  printf("crtc ct %15d\n", res->count_crtcs);
  
  // allocate/init crtc_data
  struct crtc_data *dat = (struct crtc_data *) malloc(sizeof(struct crtc_data));
  dat->fd = fd;
  
  // open files as necessary
  if (do_back) {
    printf(ORIG_GAMMA_FILE "\n");
    back_fd = open(ORIG_GAMMA_FILE, O_WRONLY | O_CREAT);
    if (back_fd < 0) error("open backup file");
  }
  if (do_save) {
    save_fd = open(do_save, O_WRONLY | O_CREAT);
    if (save_fd < 0) error("open save file");
  }
  if (do_load) {
    load_fd = open(do_load, O_RDONLY);
    if (load_fd < 0) error("open load file");
  }
  
  // Iterate through crtcs for this fd
  for (int crtc_idx = 0; crtc_idx < res->count_crtcs; crtc_idx++) {
    dat->crtc_id = res->crtcs[crtc_idx];
    dat->crtc = drmModeGetCrtc(fd, dat->crtc_id);
    dat->r = calloc(dat->crtc->gamma_size, sizeof(uint16_t));
    dat->g = calloc(dat->crtc->gamma_size, sizeof(uint16_t));
    dat->b = calloc(dat->crtc->gamma_size, sizeof(uint16_t));
    printf("\tcrtc id? %15d\t%3d\n", dat->crtc_id, dat->crtc->gamma_size);
    
    // Process gamma as desired
    if (do_debug)
      debug(dat);
    if (do_back)
      save_table(back_fd, dat);
    if (do_save)
      save_table(save_fd, dat);
    if (do_load)
      load_table(load_fd, dat);
    if (do_divide)
      divide_gamma(dat);
    if (do_multiply)
      multiply_gamma(dat);
    
    printf("crtc gotten\n");
    free(dat->r); free(dat->g); free(dat->b);
  }
  
  // close files as necessary
  if (do_back) close(backup_fd);
  if (do_save) close(save_fd);
  if (do_load) close(load_fd);
  // cleanup
  free(dat);
}

void iterate_fds()
{
  int ret;
  int fds[DRM_MAX_MINOR];
  drmModeRes *fd_res[DRM_MAX_MINOR];
  int fd_ct = 0;
  
  printf("DRM_MAX_MINOR %15d\n", DRM_MAX_MINOR); // 16
  for (int fd_idx = 0; fd_idx < DRM_MAX_MINOR; fd_idx++) {
    fds[fd_idx] = 0;
    char * filename = NULL;
    ret = asprintf(&filename, "/dev/dri/card%d", fd_idx);
    assert(ret != -1);
    assert(filename);
    
    int fd = open(filename, O_RDWR);
    free(filename);
    
    if (fd < 0) continue;
    fd_res[fd_ct] = drmModeGetResources(fd);
    if (!fd_res[fd_ct]) {
      close(fd);
      continue;
    }
    
    fds[fd_ct] = fd;
    fd_ct ++;
    
    iterate_crtcs(fd);
    close(fd);
  }
}

int main(int argn, char *argc[])
{
  for (int i = 1; i < argn; i++) {
    printf("arg %2d\t%s\n", i, argc[i]);
    if (!strcmp(argc[i], DEBUG_STR)) do_debug = 1;
    if (!strcmp(argc[i], SAVE_ORIG)) do_back = 1;
    if (!strcmp(argc[i], SAVE_FILE)) do_save = argc[++i];
    if (!strcmp(argc[i], LOAD_FILE)) do_load = argc[++i];
    if (!strcmp(argc[i], DIVIDE_2))  do_divide = 1;
    if (!strcmp(argc[i], MULTIP_2))  do_multiply = 1;
  }
  iterate_fds();
  return 0;
}
