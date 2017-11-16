#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <string.h>
#include <errno.h>

// high priority or an error occured in SW
#define POLL_GPIO POLLPRI | POLLERR 

#define TIMEOUT 60

#define NSAMPLES 1000
#define EXPORT_PATH      "/sys/class/gpio/export"
#define SW_VAL_PATH      "/sys/class/gpio/gpio15/value"
#define SW_INT_PATH      "/sys/class/gpio/gpio15/edge"
#define SW_DIR_PATH      "/sys/class/gpio/gpio15/direction"


const char *sw = "15"; // Linux GPIO representation

void close_fd(int);
int configure_pins();
void set_realtime_prio();

int main(int argc, char *argv[]){
    int fd, count, poll_ret;
    double period;
    char value;
    struct pollfd poll_gpio;
    struct timespec tstart = {0,0}, tend={0,0};
    
    set_realtime_prio();
    configure_pins();
    
    while((fd = open(SW_VAL_PATH, O_RDONLY)) <= 0);
    printf("Opened SW value sucessfully\n");
    
    // file descriptor from SW is being polled
    poll_gpio.fd = fd; 
    // poll events in GPIO 
    poll_gpio.events = POLL_GPIO;
    poll_gpio.revents = 0;

    read(fd, &value, 1);

    while(1){
    
	clock_gettime(CLOCK_MONOTONIC, &tstart);
        for(count = 0; count <  NSAMPLES; count++){
            lseek(fd, 0, SEEK_SET);
            read(fd, &value, 1); // read GPIO value
            poll_ret = poll(&poll_gpio, 1, TIMEOUT*1000);
            
            if(!poll_ret){
                printf("Timeout\n");
                return 0;
            }
           
            else{
                
                if(poll_ret == -1){
                    perror("poll");
                    return EXIT_FAILURE;
                }
            
                if((poll_gpio.revents) & (POLL_GPIO)){
                    lseek(fd, 0, SEEK_SET);
                    read(fd, &value, 1); // read GPIO value
                    //printf("GPIO changed state!\n");
                }
            }
        } 
        clock_gettime(CLOCK_MONOTONIC, &tend); 
        period = (((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec))*1000; // period in ms

	//period = 1.0e-9*((tend.tv_nsec) - (tstart.tv_nsec));  // period in 
	

        period /= NSAMPLES; // divide by n samples
	printf("Period (ms): %.3f \n", period);
        printf("Frequency (kHz): %.3f \n", 1/period);
	
	}

    close(fd); //close value file

    return EXIT_SUCCESS;
}

void set_realtime_prio(){
    pthread_t this_thread = pthread_self(); // operates in the current running thread
    struct sched_param params;
    int ret;
    
    // set max prio 
    params.sched_priority = sched_get_priority_max(SCHED_FIFO);    
    ret = pthread_setschedparam(this_thread, SCHED_FIFO, &params);

    if(ret != 0){
	perror("Unsuccessful in setting thread realtime prio\n");
    }

}

void close_fd(int fd){
	// close file from file descriptor
    if(close(fd) < 0){
        perror("Warning: Unable to close file correctly\n");
    }
}

int configure_pins(){
    int fd_export, fd_edge, fd_input;
    
    /*******************EXPORT*******************/
    // open export file
	if((fd_export = open(EXPORT_PATH, O_WRONLY)) <= 0){
		perror("Unable to open export file\n");
		return EXIT_FAILURE;
	}else printf("Opened export file successfully\n");
    // export SW GPIO
    if(write(fd_export, sw, strlen(sw)) < 0){
		if(errno != EBUSY){ // does not end if pin is already exported
       		perror("Unable to export SW GPIO\n");
			close_fd(fd_export);
			return EXIT_FAILURE;
		}
		perror("Warning: Unable to export SW GPIO\n");
    } else printf("Exported SW successfully\n");

	// close export file
	close_fd(fd_export);
    
    
    /******************DIRECTION******************/
    // open direction file
    if((fd_input = open(SW_DIR_PATH, O_WRONLY)) <=0){
        perror("Unable to open direction file for PIN 15\n");
        return EXIT_FAILURE;
    } else printf("Opened direction file successfully\n");
	if(write(fd_input, "in", 2) < 0){ // configure as input
        if(errno != EBUSY){
            perror("Unable to change direction from SW\n");
            close_fd(fd_input);
            return EXIT_FAILURE;
        }
        perror("Warning: unable to change direction from SW\n");
    }else printf("Changed direction from SW successfully\n");

    close_fd(fd_input); // close direction file
    
    
    /********************EDGE*********************/    
    while((fd_edge = open(SW_INT_PATH, O_RDWR)) <=0);
    printf("Opened edge file successfully\n");
    while(write(fd_edge, "rising", 6) < 0);
    printf("Changed edge from SW successfully\n");
    close_fd(fd_edge);

    return EXIT_SUCCESS;
}

