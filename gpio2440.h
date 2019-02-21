#ifndef GPIOD_H
#define GPIOD_H

struct gpiod_ioc_args
{
    int pid;
    unsigned int pin;
    unsigned int value;
};

/*General*/
#define GPIOD_MAXIRQ 0xff
#define GPIOD_MAXEINT 24

/* Device data */
#define GPIOD_DEVNAME "gpio2440"
#define GPIOD_DEVPATH "/dev/gpio2440"

/* Pull Up modes */
#define GPIOD_PUPON 0x0
#define GPIOD_PUPOFF 0x1

/* Use 'g' as magic number */
#define GPIOD_IOC_MAGIC 'g'

/* ioctl commands */
#define GPIOD_IOC_RESET _IO(GPIOD_IOC_MAGIC, 0)

#define GPIOD_IOC_CONFIG _IOW(GPIOD_IOC_MAGIC, 1, unsigned long)

#define GPIOD_IOC_GETDAT _IOR(GPIOD_IOC_MAGIC, 2, unsigned long)
#define GPIOD_IOC_SETDAT _IOW(GPIOD_IOC_MAGIC, 3, unsigned long)

#define GPIOD_IOC_SETIRQ _IOW(GPIOD_IOC_MAGIC, 4, unsigned long)
#define GPIOD_IOC_CLRIRQ _IOW(GPIOD_IOC_MAGIC, 5, unsigned long)

#define GPIOD_IOC_IRQSIG _IOR(GPIOD_IOC_MAGIC, 6, unsigned long)
#define GPIOD_IOC_IRQFLT _IOW(GPIOD_IOC_MAGIC, 7, unsigned long)
#define GPIOD_IOC_PULLUP _IOW(GPIOD_IOC_MAGIC, 8, unsigned long)
#define GPIOD_IOC_FREEPIN _IOW(GPIOD_IOC_MAGIC, 9, unsigned long)


/* IRQ Signals */
#define GPIOD_SIGBASE (40)
#define GPIOD_EINT2SIG(eint) (GPIOD_SIGBASE + eint)
#define GPIOD_SIG2EINT(sig) (sig - GPIOD_SIGBASE)
#define GPIOD_SIGMAX (24)


/* IRQ filter mode */
#define GPIOD_IRQFLT_ON 0x1
#define GPIOD_IRQFLT_OFF 0x0
#define GPIOD_IRQFLT_WMIN 0x0
#define GPIOD_IRQFLT_WMAX 0x3f

#endif // GPIOD_H
