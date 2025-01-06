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

static struct proc_dir_entry *proc_entry = NULL;
static struct timespec64 module_start_time;

static long calculate_minutes_since_1997(void) {
    struct timespec64 current_time;
    long long time_difference_sec;
    long long minutes_diff;

    ktime_get_real_ts64(&current_time); 

    time_difference_sec = current_time.tv_sec - module_start_time.tv_sec; 
    minutes_diff = time_difference_sec / (60); 

    return minutes_diff;
}

static ssize_t proc_file_read(struct file *file, char __user *buffer, size_t len, loff_t *offset)
{
    char output_buffer[32];
    size_t output_length;
    long minutes_elapsed;

    if (*offset > 0)
        return 0;

    minutes_elapsed = calculate_minutes_since_1997(); 

    output_length = snprintf(output_buffer, sizeof(output_buffer), "%ld\n", minutes_elapsed); 
    if (copy_to_user(buffer, output_buffer, output_length))
        return -EFAULT;

    *offset += output_length;
    return output_length;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_operations = {
    .proc_read = proc_file_read,
};
#else
static const struct file_operations proc_file_operations = {
    .read = proc_file_read,
};
#endif

static int __init module_init_function(void)
{
    struct tm start_time_structure;
    time64_t start_time_seconds;

    start_time_structure.tm_year = 1997 - 1900;
    start_time_structure.tm_mon = 8 - 1 ;
    start_time_structure.tm_mday = 29;
    start_time_structure.tm_hour = 12;
    start_time_structure.tm_min = 0;
    start_time_structure.tm_sec = 0;
    
    start_time_seconds = mktime64(start_time_structure.tm_year + 1900, start_time_structure.tm_mon + 1, 
                                   start_time_structure.tm_mday, start_time_structure.tm_hour, 
                                   start_time_structure.tm_min, start_time_structure.tm_sec);

    module_start_time.tv_sec = start_time_seconds;
    module_start_time.tv_nsec = 0;

    proc_entry = proc_create(PROCFS_FILE_NAME, 0644, NULL, &proc_file_operations);
    if (!proc_entry) {
        pr_err("Failed to create /proc/%s\n", PROCFS_FILE_NAME);
        return -ENOMEM;
    }

    pr_info("/proc/%s created\n", PROCFS_FILE_NAME);
    return 0;
}

static void __exit module_exit_function(void)
{
    proc_remove(proc_entry);
    pr_info("/proc/%s removed\n", PROCFS_FILE_NAME);
}

module_init(module_init_function);
module_exit(module_exit_function);

MODULE_LICENSE("GPL");
