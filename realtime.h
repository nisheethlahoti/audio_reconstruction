// Sets unbuffered operations on stdin and stdout, and FIFO scheduling on the current thread, with
// priority being a weighted avg. of max and min priorities, with given weights.
void set_realtime(int weight_max, int weight_min);
