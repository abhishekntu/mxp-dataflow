#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <math.h>
#include "vbx.h"
#include "vbx_port.h"
#include "vbx_common.h"
#include "vectorblox_mxp_lin.h"
#define N 256
#define M 65536
#define ops 5

uint32_t i;


void kernel_mxp(int8_t **X, int8_t **Y, int nx, int ny)
{
int8_t a = 2;
int8_t b = 2;
int8_t c = 2;
int8_t *sc_X = *X;
int8_t *sc_Y = *Y;
vbx_timestamp_t time_start,time_stop;
vbx_dcache_flush_all(); // not required since using the uncached region 

vbx_byte_t *vx = vbx_sp_malloc( ny*sizeof(vbx_byte_t));
vbx_byte_t *vy = vbx_sp_malloc( ny*sizeof(vbx_byte_t));
vbx_byte_t *vara = vbx_sp_malloc( ny*sizeof(vbx_byte_t));
vbx_set_vl(ny);

vbx_timestamp_start();
time_start = vbx_timestamp();
for(i=0;i<M*N;i=i+M){
	vbx_dma_to_vector( vx, sc_X+i,ny*sizeof(vbx_byte_t));
	vbx( SVB,VMUL,vy,a,vx);
	vbx( VVB,VMUL,vy,vy,vx);
	vbx( SVB,VMUL,vara,b,vx);
	vbx( VVB,VADD,vy,vy,vara );
	vbx( SVB,VADD,vy,c,vy);
	vbx_dma_to_host( sc_Y+i, vy,ny*sizeof(vbx_byte_t));
	vbx_sync();
}
time_stop = vbx_timestamp();
vbx_timestamp_t cycles = time_stop - time_start;
double seconds = ((double) cycles) / ((double) vbx_timestamp_freq());

printf("Maximum Throughput for Poly2 kernel on MXP in (Gops/sec) is -> %g Gops/sec\n",(nx*ny*ops)/(seconds*pow(10,9)));
vbx_sp_free();
vbx_shared_free(sc_X);
vbx_shared_free(sc_Y);
}


void init_data(int8_t **X, int8_t **Y, int nx, int ny)
{
int8_t *t_X = *X;
int8_t *t_Y = *Y;

t_X=vbx_shared_malloc(nx*ny*sizeof(int8_t));
t_Y=vbx_shared_malloc(nx*ny*sizeof(int8_t));

for(i=0;i<nx*ny;i++){
	t_X[i]=1;
	t_Y[i]=0;
}

}

int main(int argc, char *argv[])
{

#ifdef MXP
VectorBlox_MXP_Initialize("mxp0","cma");
#endif
 
int8_t *X, *Y;
int nx = N;
int ny = M;
init_data(&X, &Y, nx, ny);
kernel_mxp(&X,&Y,nx,ny);
return 0;
}
