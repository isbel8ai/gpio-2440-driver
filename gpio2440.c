#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

#include "regs-gpio.h"
#include "gpio2440.h"

#include <mach/hardware.h>
#include <mach/gpio-fns.h>
#include <asm/irq.h>

#include <mach/regs-gpio.h>

#include <plat/gpio-core.h>

MODULE_DESCRIPTION ( "gpio2440 driver" );
MODULE_AUTHOR ( "Sistemas Embebidos" );
MODULE_LICENSE ( "GPL" );


static pid_t process[GPIOD_MAXIRQ];

static void gpiod_rstirq(unsigned int irq){
    if (irq >= 0) {
        if (process[irq] >= 0) {
            free_irq(irq, NULL);
            process[irq] = -1;
        }
    }
}

static void gpiod_reset(void) {
    int i = 0;
    for (i = 0; i < GPIOD_MAXEINT; i++) {
        gpiod_rstirq(IRQ_EINT(i));
    }
}

static int gpiod_irq_sig(int irq) {

    if (irq < IRQ_EINT4) {
        return GPIOD_SIGBASE + irq - IRQ_EINT0;
    } else {
        return GPIOD_SIGBASE + irq - IRQ_EINT4 + 4;
    }
}

static struct task_struct *gpiod_find_task(pid_t procid) {
    struct task_struct *taskp;
    rcu_read_lock();
    taskp = find_task_by_vpid(procid);
    rcu_read_unlock();
    return taskp;
}

static irqreturn_t gpiod_interrupt(int irq, void *dev_id) {
    int signum = gpiod_irq_sig(irq);
//    printk(KERN_INFO GPIOD_DEVNAME ": handled irq %i.\n", irq );

    if (process[irq] > 0) {
        int ret;
        struct siginfo info;
        struct task_struct *taskp;

        /* send the signal */
        memset(&info, 0, sizeof(struct siginfo));
        info.si_signo = signum;
        info.si_code = SI_USER;

        taskp = gpiod_find_task(process[irq]);

        if (!taskp) {
            printk(KERN_WARNING GPIOD_DEVNAME ": No such process %i.\n",
                   process[irq]);
            return IRQ_NONE;
        }

        ret = send_sig_info(signum, &info, taskp);
        if (ret < 0) {
            printk(KERN_WARNING GPIOD_DEVNAME ": Error sending signal %i.\n",
                    signum);
            return IRQ_NONE;
        }
    }
    return IRQ_HANDLED;
}

static long gpiod_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
        {
    int procid = ((struct gpiod_ioc_args*)arg)->pid;
    unsigned int pin = ((struct gpiod_ioc_args*)arg)->pin;
    unsigned int value = ((struct gpiod_ioc_args*)arg)->value;
    int irq = gpio_to_irq(pin);
    int retval = 0;

    if ((process[pin] > -1) && (procid != process[pin])) {
        if ((gpiod_find_task(process[pin]) > 0)) {
            printk(KERN_WARNING GPIOD_DEVNAME
                    ": Pin %i in use by process %i.\n", pin, process[pin]);
            return -EBUSY;
        }
    }

    switch (cmd) {
        case GPIOD_IOC_RESET:
            gpiod_reset();
            break;       
        case GPIOD_IOC_GETDAT:
            retval = s3c2410_gpio_getpin(pin);
            ((struct gpiod_ioc_args*)arg)->value = retval;
            break;
        case GPIOD_IOC_IRQSIG:
            if (irq < 0) {
                printk(KERN_WARNING GPIOD_DEVNAME
                    ": No interrupt for pin %x.\n", pin);
                return -EPERM;
            }
            return gpiod_irq_sig(irq);
            break;
        case GPIOD_IOC_CONFIG:
            s3c2410_gpio_cfgpin(pin, value);
            process[pin] = procid;
            break;
        case GPIOD_IOC_PULLUP:
            s3c2410_gpio_pullup(pin, value);
            break;
        case GPIOD_IOC_SETDAT:
            s3c2410_gpio_setpin(pin, value);
            break;
        case GPIOD_IOC_SETIRQ:
            if (irq < 0) {
                printk(KERN_WARNING GPIOD_DEVNAME
                        ": No interrupt for pin %x.\n", pin);
                return -EPERM;
            }
            retval = request_irq(irq, gpiod_interrupt, IRQF_DISABLED | value,
                    GPIOD_DEVNAME, NULL);
            if (retval) {
                free_irq(irq, NULL);
                request_irq(irq, gpiod_interrupt, IRQF_DISABLED | value,
                        GPIOD_DEVNAME, NULL);
            }
            return 0;
            break;
        case GPIOD_IOC_CLRIRQ:
            if (irq < 0) {
                printk(KERN_WARNING GPIOD_DEVNAME
                        ": No interrupt for pin %x.\n", pin);
                return -EPERM;
            }
            free_irq(irq, NULL);
            return 0;
            break;
        case GPIOD_IOC_IRQFLT:
            s3c2410_gpio_irqfilter(pin, GPIOD_IRQFLT_ON, S3C2410_EINTFLT_PCLK |
                    S3C2410_EINTFLT_WIDTHMSK(GPIOD_IRQFLT_WMAX));
        break;
        case GPIOD_IOC_FREEPIN:
            gpiod_rstirq(irq);
        break;
        default:
            printk(KERN_WARNING GPIOD_DEVNAME ": Invalid command.\n");
            return -EINVAL;
    }
    return retval;
}

static const struct file_operations gpiod_fops = {
    .owner		= THIS_MODULE,
    .unlocked_ioctl		= gpiod_ioctl,
};

static struct miscdevice gpiod_button_misc_device = {
    MISC_DYNAMIC_MINOR,
    GPIOD_DEVNAME,
    &gpiod_fops
};

static int gpiod_init_module(void) {
    printk(KERN_INFO GPIOD_DEVNAME ": Start module\n");
    if (misc_register(&gpiod_button_misc_device)) {
        printk(KERN_WARNING GPIOD_DEVNAME ": Couldn't register device.\n");
        return -EBUSY;
    }
    gpiod_reset();
    return 0;
}

static void gpiod_exit_module(void) {
    printk(KERN_INFO GPIOD_DEVNAME ": Exit module\n");
    gpiod_reset();
    misc_deregister ( &gpiod_button_misc_device );
}

module_init ( gpiod_init_module );
module_exit ( gpiod_exit_module );
