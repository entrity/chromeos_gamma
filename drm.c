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

#define SAVE_ORIG "bak"
#define SAVE_FILE "save"
#define LOAD_FILE "load"
#define DIVIDE_2 "/2"
#define MULTIP_2 "x2"
#define ORIG_GAMMA_FILE "orig_gamma_table.dat"

struct gamma_table {
   uint16_t *r, *g, *b;
};

// callback for processing a RGB gamma triplet from any function
// called by iterate_crtcs
static void (*gamma_processor)(size_t, uint16_t*, uint16_t*, uint16_t*);

// pointer to data used by gamma_processor function. this is not
// the gamma data but any supplemental data, such as a file descriptor
static void *gamma_processor_data;

static void error()
{
  fprintf(stderr, "err %d\n", errno);
  exit(errno || -1);
}

static void save_table(size_t gamma_size, uint16_t *r, uint16_t *g, uint16_t *b)
{
  int gamma_out_fd = *((int *) gamma_processor_data);
  write(gamma_out_fd, r, gamma_size*sizeof(uint16_t));
  write(gamma_out_fd, g, gamma_size*sizeof(uint16_t));
  write(gamma_out_fd, b, gamma_size*sizeof(uint16_t));
  puts("wrote one table\n");
}

void set_gamma(int fd, uint32_t crtc_id)
{
  // int r, g, b;
  // drmModeCrtc *crtc = drmModeGetCrtc(fd, crtc_id);
  // int res = drmModeCrtcSetGamma(fd, crtc_id, crtc->gamma_size, r, g, b);
  // printf("Attempted to set gamma: %3d\n", res);
}

void iterate_crtcs(int fd)
{
  uint16_t *r, *g, *b;
  drmModeResPtr res = drmModeGetResources(fd);
  printf("crtc ct %15d\n", res->count_crtcs);
  for (int crtc_idx = 0; crtc_idx < res->count_crtcs; crtc_idx++) {
    uint32_t crtc_id = res->crtcs[crtc_idx];
    drmModeCrtc *crtc = drmModeGetCrtc(fd, crtc_id);
    printf("\tcrtc id? %15d\t%3d\n", crtc_id, crtc->gamma_size);
    
    r = calloc(crtc->gamma_size, sizeof(uint16_t));
    g = calloc(crtc->gamma_size, sizeof(uint16_t));
    b = calloc(crtc->gamma_size, sizeof(uint16_t));
    drmModeCrtcGetGamma(fd, crtc_id, crtc->gamma_size, r, g, b);
    
    // for (int gidx = 0; gidx < crtc->gamma_size; gidx++) {
    //   printf("\t\tgamma(%3d) %5d %5d %5d\n", gidx, r[gidx], g[gidx], b[gidx]);
    // }
    gamma_processor(crtc->gamma_size, r, g, b);
    
    free(r); free(g); free(b);
    printf("crtc gotten\n");
  }
  
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

static void save_file(char *filepath)
{
  int gamma_out_fd = open(filepath, O_WRONLY | O_CREAT);
  if (gamma_out_fd < 0) error();
  printf("Opened file %d %s\n", gamma_out_fd, filepath);
  gamma_processor = save_table;
  gamma_processor_data = &gamma_out_fd;
  iterate_fds();
  close(gamma_out_fd);
}

static void load_file(char *filepath)
{
  
}

static void divide_gamma_by_2()
{
  
}

static void multiply_gamma_by_2()
{
  
}

int main(int argn, char *argc[])
{
  for (int i = 1; i < argn; i++) {
    printf("arg %2d\t%s\n", i, argc[i]);
    if (!strcmp(argc[i], SAVE_ORIG))
      save_file(ORIG_GAMMA_FILE);
    if (!strcmp(argc[i], SAVE_FILE))
      save_file(argc[++i]);
    if (!strcmp(argc[i], LOAD_FILE))
      load_file(argc[++i]);
    if (!strcmp(argc[i], DIVIDE_2))
      divide_gamma_by_2();
    if (!strcmp(argc[i], MULTIP_2))
      multiply_gamma_by_2();
  }
  // org_connectors();
  return 0;
}
