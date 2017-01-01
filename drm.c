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

void set_gamma(int fd, uint32_t crtc_id)
{
  int r, g, b;
  drmModeCrtc *crtc = drmModeGetCrtc(fd, crtc_id);
  int res = drmModeCrtcSetGamma(fd, crtc_id, crtc->gamma_size, r, g, b);
  printf("Attempted to set gamma: %3d\n", res);
}

void foo(int fd)
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
    // printf("\t\tr %3d %3d %3d\n", r, g, b);
    // free(crtc);
    free(r); free(g); free(b);
    printf("crtc gotten\n");
  }
  // free(res);
  puts("Marker 1\n");
}

void org_connectors()
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
    printf("Got dri dev file %d\n", fd_idx);
    fd_ct ++;
    
    foo(fd);
    puts("Marker 2\n");
    // close(fd);
  }
}

int main(char *argc, int argn)
{
  org_connectors();
  return 0;
}
