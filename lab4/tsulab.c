#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/time.h>

MODULE_DESCRIPTION("Tomsk State University Kernel Module");
MODULE_AUTHOR("jeffers1n");

#define PROCFS_FILE_NAME "tsulab"

static struct proc_dir_entry *time_tracker_entry = NULL;
static struct timespec64 module_start_time;

static long calculate_time_difference(void) {
    struct timespec64 current_time;
    long long seconds_difference;
    long long total_minutes;

    ktime_get_real_ts64(&current_time); 

    seconds_difference = current_time.tv_sec - module_start_time.tv_sec; 
    total_minutes = seconds_difference / 60; 

    return total_minutes;
}

static ssize_t time_tracker_read(struct file *file, char __user *buffer, size_t len, loff_t *offset)
{
    char output_buffer[32];
    size_t output_size;
    long minutes_elapsed;

    if (*offset > 0)
        return 0;

    minutes_elapsed = calculate_time_difference(); 

    output_size = snprintf(output_buffer, sizeof(output_buffer), "%ld\n", minutes_elapsed); 
    if (copy_to_user(buffer, output_buffer, output_size))
        return -EFAULT;

    *offset += output_size;
    return output_size;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops time_tracker_file_ops = {
    .proc_read = time_tracker_read,
};
#else
static const struct file_operations time_tracker_file_ops = {
    .read = time_tracker_read,
};
#endif

static int __init time_tracker_init(void)
{
    struct tm initial_time;
    time64_t initial_time_sec;

    initial_time.tm_year = 1997 - 1900;
    initial_time.tm_mon = 8 - 1;
    initial_time.tm_mday = 29;
    initial_time.tm_hour = 12;
    initial_time.tm_min = 0;
    initial_time.tm_sec = 0;

    initial_time_sec = mktime64(initial_time.tm_year + 1900, initial_time.tm_mon + 1, initial_time.tm_mday,
                                 initial_time.tm_hour, initial_time.tm_min, initial_time.tm_sec);

    module_start_time.tv_sec = initial_time_sec;
    module_start_time.tv_nsec = 0;

    time_tracker_entry = proc_create(PROCFS_ENTRY_NAME, 0644, NULL, &time_tracker_file_ops);
    if (!time_tracker_entry) {
        pr_err("Error creating /proc/%s\n", PROCFS_ENTRY_NAME);
        return -ENOMEM;
    }

    pr_info("/proc/%s has been successfully created\n", PROCFS_ENTRY_NAME);
    return 0;
}

static void __exit time_tracker_exit(void)
{
    proc_remove(time_tracker_entry);
    pr_info("/proc/%s has been successfully removed\n", PROCFS_ENTRY_NAME);
}

module_init(time_tracker_init);
module_exit(time_tracker_exit);

MODULE_LICENSE("GPL");
