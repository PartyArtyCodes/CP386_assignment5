/*----------------------
Assignment: CP386 A5
Authors: Jayden Hunt, Arthur Severinets
Date: 2026-07-20
----------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#define INPUT_FILE "sample_in_banker.txt"
#define MAX_LINE   1024


int n = 0;                 
int m = 0;                
int *available  = NULL;  
int **maximum   = NULL;  
int **allocation = NULL; 
int **need      = NULL;   

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;
int *safe_sequence = NULL; 
int  turn = 0;             


static int **alloc_matrix(int rows, int cols)
{
    int **mat = malloc(rows * sizeof(int *));
    for (int i = 0; i < rows; i++)
        mat[i] = calloc(cols, sizeof(int));
    return mat;
}

static void free_matrix(int **mat, int rows)
{
    if (!mat) return;
    for (int i = 0; i < rows; i++)
        free(mat[i]);
    free(mat);
}

static void print_vector(const int *v)
{
    for (int j = 0; j < m; j++)
        printf("%d%s", v[j], (j == m - 1) ? "" : " ");
    printf("\n");
}

static void print_matrix(int **mat)
{
    for (int i = 0; i < n; i++)
        print_vector(mat[i]);
}


static bool is_safe(int *seq_out)
{
    int  *work   = malloc(m * sizeof(int));
    bool *finish = calloc(n, sizeof(bool));
    int   count  = 0;

    memcpy(work, available, m * sizeof(int));

    while (count < n) {
        bool found = false;
        for (int i = 0; i < n; i++) {
            if (finish[i])
                continue;
            bool can_run = true;
            for (int j = 0; j < m; j++) {
                if (need[i][j] > work[j]) {
                    can_run = false;
                    break;
                }
            }
            if (can_run) {
                for (int j = 0; j < m; j++)
                    work[j] += allocation[i][j];
                finish[i] = true;
                if (seq_out)
                    seq_out[count] = i;
                count++;
                found = true;
            }
        }
        if (!found)
            break;
    }

    free(work);
    free(finish);
    return count == n;
}


static void request_resources(int customer, const int *request)
{

    for (int j = 0; j < m; j++) {
        if (request[j] > need[customer][j]) {
            printf("Request exceeds maximum need, request is denied\n");
            return;
        }
    }


    for (int j = 0; j < m; j++) {
        if (request[j] > available[j]) {
            printf("Resources are not available, request is denied\n");
            return;
        }
    }


    for (int j = 0; j < m; j++) {
        available[j]            -= request[j];
        allocation[customer][j] += request[j];
        need[customer][j]       -= request[j];
    }

    if (is_safe(NULL)) {
        printf("State is safe, and request is satisfied\n");
    } else {

        for (int j = 0; j < m; j++) {
            available[j]            += request[j];
            allocation[customer][j] -= request[j];
            need[customer][j]       += request[j];
        }
        printf("State is not safe, and request is denied\n");
    }
}

static void release_resources(int customer, const int *release)
{
 
    for (int j = 0; j < m; j++) {
        if (release[j] > allocation[customer][j]) {
            printf("Release exceeds allocated resources, release is denied\n");
            return;
        }
    }

    for (int j = 0; j < m; j++) {
        allocation[customer][j] -= release[j];
        need[customer][j]       += release[j];
        available[j]            += release[j];
    }
    printf("The resources have been released successfully\n");
}


static void print_status(void)
{
    printf("Available Resources:\n");
    print_vector(available);
    printf("Maximum Resources:\n");
    print_matrix(maximum);
    printf("Allocated Resources:\n");
    print_matrix(allocation);
    printf("Need Resources:\n");
    print_matrix(need);
}


static void *customer_thread(void *arg)
{
    int id = *(int *)arg;
    free(arg);

    pthread_mutex_lock(&mutex);


    while (safe_sequence[turn] != id)
        pthread_cond_wait(&cond, &mutex);

    printf("--> Customer/Thread %d\n", id);

    printf("    Allocated resources: ");
    for (int j = 0; j < m; j++)
        printf(" %d", allocation[id][j]);
    printf("\n");

    printf("    Needed:");
    for (int j = 0; j < m; j++)
        printf(" %d", need[id][j]);
    printf("\n");

    printf("    Available: ");
    for (int j = 0; j < m; j++)
        printf(" %d", available[j]);
    printf("\n");

    printf("    Thread has started\n");
    printf("    Thread has finished\n");
    printf("    Thread is releasing resources\n");

    
    for (int j = 0; j < m; j++) {
        available[j] += allocation[id][j];
        allocation[id][j] = 0;
        need[id][j] = maximum[id][j];
    }

    printf("    New Available: ");
    for (int j = 0; j < m; j++)
        printf(" %d", available[j]);
    printf("\n");


    turn++;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

static void run_threads(void)
{
    safe_sequence = malloc(n * sizeof(int));

    if (!is_safe(safe_sequence)) {
        printf("System is not in a safe state, cannot run threads\n");
        free(safe_sequence);
        safe_sequence = NULL;
        return;
    }

    printf("Safe Sequence is: ");
    for (int i = 0; i < n; i++)
        printf("%d%s", safe_sequence[i], (i == n - 1) ? "" : " ");
    printf("\n");

    turn = 0;
    pthread_t *threads = malloc(n * sizeof(pthread_t));

    for (int i = 0; i < n; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&threads[i], NULL, customer_thread, id);
    }

    for (int i = 0; i < n; i++)
        pthread_join(threads[i], NULL);

    free(threads);
    free(safe_sequence);
    safe_sequence = NULL;
}


static void load_maximum_from_file(void)
{
    FILE *fp = fopen(INPUT_FILE, "r");
    if (!fp) {
        fprintf(stderr, "Error: cannot open %s\n", INPUT_FILE);
        exit(EXIT_FAILURE);
    }


    char line[MAX_LINE];
    n = 0;
    while (fgets(line, sizeof(line), fp)) {
        bool has_digit = false;
        for (char *p = line; *p; p++) {
            if (*p >= '0' && *p <= '9') {
                has_digit = true;
                break;
            }
        }
        if (has_digit)
            n++;
    }

    maximum    = alloc_matrix(n, m);
    allocation = alloc_matrix(n, m);
    need       = alloc_matrix(n, m);

    rewind(fp);
    int row = 0;
    while (fgets(line, sizeof(line), fp) && row < n) {
        bool has_digit = false;
        for (char *p = line; *p; p++) {
            if (*p >= '0' && *p <= '9') {
                has_digit = true;
                break;
            }
        }
        if (!has_digit)
            continue;

        int col = 0;
        char *tok = strtok(line, " ,\t\r\n");
        while (tok && col < m) {
            maximum[row][col] = atoi(tok);
            need[row][col]    = maximum[row][col]; 
            col++;
            tok = strtok(NULL, " ,\t\r\n");
        }
        row++;
    }
    fclose(fp);
}

//Main
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <resource 1> <resource 2> ... <resource m>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    m = argc - 1;
    available = malloc(m * sizeof(int));
    for (int j = 0; j < m; j++)
        available[j] = atoi(argv[j + 1]);

    load_maximum_from_file();

    printf("Number of Customers: %d\n", n);
    printf("Currently Available resources:");
    for (int j = 0; j < m; j++)
        printf(" %d", available[j]);
    printf("\n");
    printf("Maximum resources from file:\n");
    print_matrix(maximum);

    char line[MAX_LINE];
    int *values = malloc(m * sizeof(int));

    while (1) {
        printf("Enter Command: ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin))
            break; 

        line[strcspn(line, "\r\n")] = '\0';

        char *cmd = strtok(line, " \t");
        if (!cmd)
            continue;

        if (strcmp(cmd, "Exit") == 0) {
            break;
        } else if (strcmp(cmd, "Status") == 0) {
            print_status();
        } else if (strcmp(cmd, "Run") == 0) {
            run_threads();
        } else if (strcmp(cmd, "RQ") == 0 || strcmp(cmd, "RL") == 0) {
            bool is_request = (strcmp(cmd, "RQ") == 0);

            char *tok = strtok(NULL, " \t");
            if (!tok) {
                printf("Invalid input, use one of RQ, RL, Status, Run, Exit\n");
                continue;
            }
            int customer = atoi(tok);
            if (customer < 0 || customer >= n) {
                printf("Invalid input, use one of RQ, RL, Status, Run, Exit\n");
                continue;
            }

            bool valid = true;
            for (int j = 0; j < m; j++) {
                tok = strtok(NULL, " \t");
                if (!tok) {
                    valid = false;
                    break;
                }
                values[j] = atoi(tok);
            }
            if (!valid) {
                printf("Invalid input, use one of RQ, RL, Status, Run, Exit\n");
                continue;
            }

            if (is_request)
                request_resources(customer, values);
            else
                release_resources(customer, values);
        } else {
            printf("Invalid input, use one of RQ, RL, Status, Run, Exit\n");
        }
    }

    free(values);
    free(available);
    free_matrix(maximum, n);
    free_matrix(allocation, n);
    free_matrix(need, n);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return EXIT_SUCCESS;
}
