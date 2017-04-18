#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#define MALLOC_SIZE (512*1024*1024*sizeof(char)) //512MB
#define ARRAY_SIZE 100
#define ALLOC_PERIOD_US 500

static pthread_t reader_thread;
char* malloc_p[ARRAY_SIZE] = {NULL};

static void *reader_server(void* data)
{
	int i = 0, end_i = 0;
	unsigned int size = 0;
	char* tmp_p;
	while(1) {
		for (i=0;i<ARRAY_SIZE;i++) {
			//printf("mallocp = %p\n", malloc_p[i]);
			if (malloc_p[i] != NULL) {
				end_i = i;
			}
		}
		printf("reader start end_i = %d\n", end_i);
		if (end_i != 0) {
			for (i=0;i<end_i;i++) {
				tmp_p = malloc_p[i];
				//printf("reader i = %d\n", i);
				for(size = 0;size < MALLOC_SIZE;size++) {
					//printf("%d\n", *tmp_p);
					*tmp_p = (*tmp_p + 0x5)%255;
					//printf("%d\n", *tmp_p);
					tmp_p++;
					if (size % 4096 == 0) {//step on new page
						//printf("sleep on page boundary\n");
						usleep(ALLOC_PERIOD_US);
					}
				}
			}
		}
		printf("reader sleep\n");
		sleep(2);
	}
	return 0;		
}

int setPidOutOfMemoryAdj(void)
{
	FILE *oom_adj = fopen("/proc/self/oom_score_adj", "we");
	printf("Set OOM_ADJ to max\n");
	if (oom_adj) {
		fputs("-1000", oom_adj);
		fclose(oom_adj);
	} else {
		/* fallback to kernels <= 2.6.35 */
		oom_adj = fopen("/proc/self/oom_adj", "we");
		if (oom_adj) {
			fputs("-17", oom_adj);
			fclose(oom_adj);
		}
	}
	return 0;
}

int main()
{
	int i = 0;
	unsigned int size = 0;
	char* tmp_p;
	setPidOutOfMemoryAdj();
	pthread_create(&reader_thread, NULL, reader_server, NULL);

	printf("Start to allocate buffer\n");
	for (i=0;i<ARRAY_SIZE;i++) {
		malloc_p[i] = malloc(MALLOC_SIZE);
		if (malloc_p[i] != NULL) {
			printf("Succeed to allocate buffer size=%lumb\n", (MALLOC_SIZE/1024)/1024);
			tmp_p = malloc_p[i];
			for(size = 0;size < MALLOC_SIZE;size++) {
				if (size % 4096 == 0) {//step on new page
					//printf("sleep on page boundary\n");
					usleep(ALLOC_PERIOD_US);
				}
				*tmp_p = rand()%255;
				//printf("%d\n", *tmp_p);
				tmp_p++;
			}
		} else {
			printf("FAIL to allocate buffer %s\n", strerror(errno));
			i--; //redo i
		}
	}
	printf("Done allocating buffer\n");
	pthread_join(reader_thread, NULL);
}
