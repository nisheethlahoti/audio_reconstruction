#!/usr/bin/awk -f
# Tells the average and standard deviation of time gaps in the log files.

{
	num += 1;
	sum += $2;
	sqsum += $2 * $2
} END {
	avg=sum/num;
	stddev=sqrt(sqsum/num-avg*avg);
	print "Average: " avg;
	print "Std. deviation: " stddev
}
