#include <linux/init.h>
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/workqueue.h>
#include <linux/time.h>

#define DELAY_NANOSEC	1000	// 1us = 1000ns

struct completion cp;
struct hrtimer timer;

static enum hrtimer_restart hrtimer_func(struct hrtimer *time)
{
    printk(KERN_ALERT "In timer function\n");
    complete(&cp);
    return HRTIMER_NORESTART;
}

static int timer_init(void)
{
    struct timeval tm;

    init_completion(&cp);
    hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    timer.function = hrtimer_func;
    hrtimer_start(&timer,
		  ktime_set(0, DELAY_NANOSEC),
		  HRTIMER_MODE_REL);
    do_gettimeofday(&tm);
    printk(KERN_ALERT "Before sleep, time: %ld!\n", tm.tv_usec);
    wait_for_completion(&cp);

    do_gettimeofday(&tm);
    printk(KERN_ALERT "After  sleep, time: %ld!\n", tm.tv_usec);
    return 0;
}

static void timer_exit(void)
{
    printk(KERN_ALERT "Goodbye, cruel world\n");
}

module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");

