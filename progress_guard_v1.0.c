#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/**ceshi
 * Define a structure that describes the application object
 * name: your application's name
 * restart_cmd: Any shell command you want to execute, Here is the order to restart.
 */
typedef struct {
    const char *name;
    const char *restart_cmd;
} app;

//flag's control file
const char *progress_guard_flag_filename = "/root/progress_guard/progress_guard_flag";
//program log file
const char *progress_guard_log_filename = "/root/progress_guard/progress_guard_log";

/**
 * Record the program log
 * @param log
 */
void progressGuardLog(char *log) {
    time_t tm = time(NULL);
    char date_time[64];
    strftime(date_time, sizeof(date_time), "%Y-%m-%d %H:%M:%S", localtime(&tm));
    char buf[100] = {0};
    FILE *f = NULL;
    if ((f = fopen(progress_guard_log_filename, "a+")) == NULL) {
        perror("log");
        exit(EXIT_FAILURE);
    }
    strcpy(buf, date_time);
    strcat(buf, "\t");
    strcat(buf, log);
    strcat(buf, "\n");
    fwrite(buf, sizeof(char), strlen(buf), f);
    fclose(f);
}

/**
 * Use the file to control the program start and stop
 * @return flag, if ret==1 program start, if ret ==0 program stop
 */
int getRestartStatus(void) {
    int ret = 0;
    FILE *f = NULL;
    f = fopen(progress_guard_flag_filename, "r");
    ret = (f == NULL) ? 0 : 1;
    fclose(f);
    return ret;
}

/**
 * progress Guard,Use the "ps -ef" command to check the process survivability, restart if the process does not exist.
 * @param appList
 * @param size
 */
void progressGuard(const app *appList, int size) {
    FILE *ptr = NULL;
    int status = 0, i = 0;
    char buf[150] = {0};
    char cmd[128] = {0};
    char warn[50] = {0};
    int count;

    while (1) {
        status = getRestartStatus();//According to the flag file to decide whether to check the process
        if (status) {
            for (i = 0; i < size; i++) {
                strcpy(cmd, "ps -ef | grep ");
                strcat(cmd, (appList + i)->name);
                strcat(cmd, " | grep -v grep | wc -l");
                strcpy(warn, "restart ");
                strcat(warn, (appList + i)->name);
                if ((ptr = popen(cmd, "r")) == NULL) {
                    progressGuardLog("popen err");
//                    printf("popen err\n"); when you close your terminal,use printf can't see!
                    pclose(ptr);
                    sleep(1);
                    break;
                }
//            memset(buf, 0, sizeof(buf));
                if ((fgets(buf, sizeof(buf), ptr)) != NULL) {//Get the total number of processes and child processes
                    count = atoi(buf);
                    if (count <=
                        0) {//When the number of processes is less than or equal to 0, the process does not exist
                        system((appList + i)->restart_cmd);
                        progressGuardLog(warn);
                    }
                }
                pclose(ptr);
                sleep(1);
            }
            sleep(15);
        } else {
            break;
        }
    }
}

int main(void) {
    //Enable the daemon
    if (daemon(1, 1) < 0) {
        perror("error daemon.../n");
        exit(EXIT_FAILURE);
    }
    app a1 = {"leanote-linux-amd64",
              "supervisord -c /etc/supervisord.conf ; service mongod stop ; supervisorctl stop all ; supervisorctl reload ; supervisorctl start all"};
    app a2 = {"php-fpm", "service php-fpm restart"};
    app a3 = {"mongod", "supervisorctl stop all ; supervisorctl reload ; supervisorctl start all"};
    app a4 = {"nginx", "service nginx restart"};
    app a5 = {"IntelliJIDEALicenseServer_linux_amd64",
              "nohup /root/IntelliJIDEALicenseServer/IntelliJIDEALicenseServer_linux_amd64 > /dev/null 2>&1 &"};
    app appList[] = {a1, a2, a3, a4, a5};
    progressGuard(appList, 5);
    return 0;
}


