#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#ifdef WORK_THREAD_MODEL
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif /* WORK_THREAD_MODEL */

#include IMPL

#define DICT_FILE "./dictionary/words.txt"

#ifdef WORK_THREAD_MODEL
#define THREAD_SIZE	10
#define STRING_COUNT	1000
#define BUF_SIZE	(THREAD_SIZE * STRING_COUNT)

pthread_mutex_t WaitAllReady_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  WaitAllReady_cond = PTHREAD_COND_INITIALIZER;
int WaitAllReady = 0;

pthread_mutex_t WaitMainThread_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  WaitMainThread_cond = PTHREAD_COND_INITIALIZER;
int WaitMainThread = 0;

pthread_mutex_t WaitWorking_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  WaitWorking_cond = PTHREAD_COND_INITIALIZER;
int WaitWorking = THREAD_SIZE;

pthread_mutex_t WaitNextRound_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  WaitNextRound_cond = PTHREAD_COND_INITIALIZER;
int WaitNextRound = 0;

pthread_mutex_t WaitAppend_mutex = PTHREAD_MUTEX_INITIALIZER;

#endif /* WORK_THREAD_MODEL */

static double diff_in_second(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    if (t2.tv_nsec-t1.tv_nsec < 0) {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (diff.tv_sec + diff.tv_nsec / 1000000000.0);
}

#ifdef WORK_THREAD_MODEL
void* thread_do (void *args) {
	char (*Data)[MAX_LAST_NAME_SIZE] = (char (*)[MAX_LAST_NAME_SIZE])args;
	entry* e = NULL;
	while (1) {
		pthread_mutex_lock(&WaitAllReady_mutex);
		++WaitAllReady;
		pthread_cond_signal(&WaitAllReady_cond);
		pthread_mutex_unlock(&WaitAllReady_mutex);

		/* Wait Main Thread Trigger Child Thrad */
		pthread_mutex_lock(&WaitMainThread_mutex);
		while(!WaitMainThread)
			pthread_cond_wait(&WaitMainThread_cond, &WaitMainThread_mutex);
		pthread_mutex_unlock(&WaitMainThread_mutex);

		for(int i = 0 ; i < STRING_COUNT ; ++i) {
			if (Data[i][0] == '\0')
				break;
			pthread_mutex_lock(&WaitAppend_mutex);
			append(Data[i], e);
			pthread_mutex_unlock(&WaitAppend_mutex);
			Data[i][0] = '\0';
		}

		pthread_mutex_lock(&WaitWorking_mutex);
		--WaitWorking;
		pthread_cond_signal(&WaitWorking_cond);
		pthread_mutex_unlock(&WaitWorking_mutex);

		pthread_mutex_lock(&WaitNextRound_mutex);
		while(!WaitNextRound)
			pthread_cond_wait(&WaitNextRound_cond, &WaitNextRound_mutex);
		pthread_mutex_unlock(&WaitNextRound_mutex);

	}
	pthread_exit(NULL);
}

void WaitAndTriggerChildThread() {

	pthread_mutex_lock(&WaitAllReady_mutex);
	while (WaitAllReady != THREAD_SIZE)
		pthread_cond_wait(&WaitAllReady_cond, &WaitAllReady_mutex);
	pthread_mutex_unlock(&WaitAllReady_mutex);

	/* Trigger all child thread to exe thread_do */
	pthread_mutex_lock(&WaitMainThread_mutex);
	WaitMainThread = 1;
	pthread_cond_broadcast(&WaitMainThread_cond);
	pthread_mutex_unlock(&WaitMainThread_mutex);

	WaitWorking = THREAD_SIZE;
	WaitNextRound = 0;

	pthread_mutex_lock(&WaitWorking_mutex);
	while ( WaitWorking != 0 )
		pthread_cond_wait(&WaitWorking_cond, &WaitWorking_mutex);
	pthread_mutex_unlock(&WaitWorking_mutex);

	pthread_mutex_lock(&WaitNextRound_mutex);
	WaitNextRound = 1;
	pthread_cond_broadcast(&WaitNextRound_cond);
	pthread_mutex_unlock(&WaitNextRound_mutex);

	WaitAllReady = 0;
	WaitMainThread = 0;
}

#endif /* WORK_THREAD_MODEL */

int main(int argc, char *argv[])
{
    FILE *fp;
    int i = 0;
#ifndef WORK_THREAD_MODEL
    char line[MAX_LAST_NAME_SIZE];
#endif
    struct timespec start, end;
    double cpu_time1, cpu_time2;

#ifdef WORK_THREAD_MODEL
    pthread_t threads[THREAD_SIZE];
    char Thread_Data[THREAD_SIZE * STRING_COUNT][MAX_LAST_NAME_SIZE] = {{0}};
    int cur_str_count = 0;
#endif /* WORK_THREAD_MODEL */

    /* check file opening */
    fp = fopen(DICT_FILE, "r");
    if (fp == NULL) {
        printf("cannot open the file\n");
        return -1;
    }

    /* build the entry */
    entry *pHead, *e;
    pHead = (entry *) malloc(sizeof(entry));
    printf("size of entry : %lu bytes\n", sizeof(entry));
    e = pHead;
    e->pNext = NULL;

#if defined(__GNUC__)
    __builtin___clear_cache((char *) pHead, (char *) pHead + sizeof(entry));
#endif
    clock_gettime(CLOCK_REALTIME, &start);

#ifdef WORK_THREAD_MODEL

    /* Thread Creation */
    for (int j = 0 ; j < THREAD_SIZE ; ++j)
	pthread_create(&threads[j], NULL, thread_do, &Thread_Data[j * STRING_COUNT]);

    while (fgets(Thread_Data[cur_str_count], sizeof(Thread_Data[cur_str_count]), fp)) {

	    while(Thread_Data[cur_str_count][i] != '\0')
		    ++i;

	    Thread_Data[cur_str_count][i - 1] = '\0';
	    i = 0 ;
	    ++cur_str_count;

	    /* Reach BUF_SIZE to trigger child thread */
	    if (cur_str_count >= BUF_SIZE) {
		    cur_str_count = 0;
		    WaitAndTriggerChildThread();
	    }
    }

    /* if can't get data from file, handle rest of data */
    if (cur_str_count != 0)
	    WaitAndTriggerChildThread();

    /* Clean Up */
    for (int i = 0 ; i < THREAD_SIZE ; ++i)
	    pthread_cancel(threads[i]);

#else
    while (fgets(line, sizeof(line), fp)) {
        while (line[i] != '\0')
            i++;
        line[i - 1] = '\0';
        i = 0;
        e = append(line, e);
    }
#endif /* WORK_THREAD_MODEL */
    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time1 = diff_in_second(start, end);

    /* close file as soon as possible */
    fclose(fp);

    e = pHead;

    /* the givn last name to find */

    char input[MAX_LAST_NAME_SIZE] = "zyxel";
    e = pHead;

    assert(findName(input, e) &&
           "Did you implement findName() in " IMPL "?");
    assert(0 == strcmp(findName(input, e)->lastName, "zyxel"));

#if defined(__GNUC__)
    __builtin___clear_cache((char *) pHead, (char *) pHead + sizeof(entry));
#endif
    /* compute the execution time */
    clock_gettime(CLOCK_REALTIME, &start);
    findName(input, e);
    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time2 = diff_in_second(start, end);

    FILE *output;
#if defined(OPT)
    output = fopen("opt.txt", "a");
#else
    output = fopen("orig.txt", "a");
#endif
    fprintf(output, "append() findName() %lf %lf\n", cpu_time1, cpu_time2);
    fclose(output);

    printf("execution time of append() : %lf sec\n", cpu_time1);
    printf("execution time of findName() : %lf sec\n", cpu_time2);

    if (pHead->pNext) free(pHead->pNext);
    free(pHead);

    return 0;
}
