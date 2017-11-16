#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define NSAMPLES 1000
#define EXPORT_PATH      "/sys/class/gpio/export"
#define SW_VAL_PATH      "/sys/class/gpio/gpio15/value"
#define SW_INT_PATH      "/sys/class/gpio/gpio15/edge"
#define SW_DIR_PATH      "/sys/class/gpio/gpio15/direction"

int configure_pins();

const char *sw = "15"; // Linux GPIO representation

void close_fd(int);

int main(int argc, char *argv[]){
	struct timespec tstart = {0,0}, tend = {0,0};
	int count=0, fd;
	double period;
	unsigned char value0, value1;  // catch rising edge
	
	configure_pins();

	while((fd = open(SW_VAL_PATH, O_RDONLY)) <= 0);
	printf("Opened SW value sucessfully\n");

	while(1){
		while(count <= NSAMPLES){// discard first, add last
			if(count == 1){ // discard first sample
				clock_gettime(CLOCK_MONOTONIC, &tstart);
			}

			lseek(fd, 0, SEEK_SET);
			if(read(fd, &value0, 1) > 1)
				perror("could not read the file");
			lseek(fd, 0, SEEK_SET);
			if(read(fd, &value1, 1) > 1)
				perror("could not read the file");
			if((value0 == '0' && value1 == '1') || (count > 1 && value0 == '1' && value1 == '0')){   // rising edge
				count++;  // acquired sample
			}
		}
		clock_gettime(CLOCK_MONOTONIC, &tend);
		count = 0;
		period = (((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec))*1000; // period in ms
		period /= NSAMPLES;
	        printf("Period (ms): %.3f \n", period);
		printf("Frequency (kHz): %.3f \n", 1/period);
	}

	close_fd(fd);	

	return EXIT_SUCCESS;
}

void close_fd(int fd){
	// close file descriptor
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
