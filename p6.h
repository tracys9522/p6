struct timeval {
time_t tv_sec; /* seconds */
suseconds_t tv_usec; /* microseconds */
};
int sec = (int) (tv.tv_sec);
double msec = (double) (tv.tv_usec/1000);