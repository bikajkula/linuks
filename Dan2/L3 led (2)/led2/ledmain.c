#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "utils.h"
#include "common.h"

/* ---------------- Module properties ---------------- */
MODULE_LICENSE("Dual BSD/GPL");
/* --------------------------------------------------- */

/* --------------- Constants definition -------------- */

/* NOTE: Check Broadcom BCM8325 datasheet, page 91+
 * NOTE: GPIO Base address is set to 0x7E200000,
 *       but it is VC CPU BUS address, while the
 *       ARM physical address is 0x3F200000, what
 *       can be seen in pages 5-7 of Broadcom
 *       BCM8325 datasheet, having in mind that
 *       total system ram is 0x3F000000 (1GB - 16MB)
 *       instead of 0x20000000 (512 MB)
 */

/* GPIO registers base address. */
#define GPIO_BASE_ADDR      (0x3F200000)
#define GPIO_ADDR_SPACE_LEN (0xB4)

/* GPIO pins used for leds. */
#define ACT_LED_GPIO (47)

// led blink interval
#define ACT_LED_BLINK_PERIOD_MSEC (1000) /* 1s */

/* --------------------------------------------------- */

/* --------------- Declarations ---------------------- */

/* Declaration of led driver functions */
static int led_init(void);
static void led_exit(void);
static long led_ioctl(struct file *, unsigned int, unsigned long);

/* LED struct. */
enum led_state {LED_STATE_OFF = 0, LED_STATE_ON = 1, LED_STATE_BLINK = 3};

enum led_power {LED_POWER_OFF = 0, LED_POWER_ON = 1};

struct led_dev {
    void __iomem   *regs;               /* Virtual address where the physical GPIO address is mapped */
	u8             gpio;                /* GPIO pin that the LED is connected to */
	u32            blink_period_msec;   /* LED blink period in msec */
    enum led_state state;               /* LED state */
    enum led_power power;               /* LED current power state */
	struct hrtimer blink_timer;         /* Blink timer */
	ktime_t        kt;                  /* Blink timer period */
};

/* --------------------------------------------------- */

/* -------- Global variables of the driver ----------- */

/* Structure that declares the usual file access functions. */
static const struct file_operations led_fops =
{
	owner :            THIS_MODULE,
	unlocked_ioctl :   led_ioctl
};

/* Active led (green) */
static struct led_dev act_led =
{
    regs              : NULL,
    gpio              : ACT_LED_GPIO,
	blink_period_msec : ACT_LED_BLINK_PERIOD_MSEC,
    state             : LED_STATE_BLINK,
    power             : LED_POWER_OFF
};

/* Major number. */
static int led_major;

/* --------------------------------------------------- */

/* ------------------ Functions ---------------------- */

/* Timer callback function called each time the timer expires
   flashes the LED */
static enum hrtimer_restart blink_timer_callback(struct hrtimer *param)
{
    struct led_dev* dev = container_of(param, struct led_dev, blink_timer);

	if(dev->state == LED_STATE_BLINK)
    {
        if (dev->power == LED_POWER_OFF){
            dev->power = LED_POWER_ON;
            SetGpioPin(dev->regs, dev->gpio);
        }else{
            dev->power = LED_POWER_OFF;
            ClearGpioPin(dev->regs, dev->gpio);
        }
    }

    hrtimer_forward(&dev->blink_timer, ktime_get(), dev->kt);

    return HRTIMER_RESTART;
}

/*
 * Initialization:
 *  1. Register device driver
 *  2. Map GPIO Physical address space to virtual address
 *  3. Initialize GPIO pins
 *  4. Init and start the high resoultion timer
 */
static int led_init(void)
{
    int result = -1;

    printk(KERN_INFO "Inserting led module\n");

    /* Registering device. */
    result = register_chrdev(0, "led", &led_fops);
    if (result < 0)
    {
        printk(KERN_INFO "led: cannot obtain major number, ERROR %d\n", result);
        return result;
    }

    led_major = result;
    printk(KERN_INFO "led major number is %d\n", led_major);

    /* map the GPIO register space from PHYSICAL address space to virtual address space */
	act_led.regs = ioremap(GPIO_BASE_ADDR, GPIO_ADDR_SPACE_LEN);
    if(!act_led.regs)
    {
        result = -ENOMEM;
        goto fail_no_virt_mem;
    }

    /* Initialize GPIO pins. */
    /* LEDS */
    SetGpioPinDirection(act_led.regs, act_led.gpio, GPIO_DIRECTION_OUT);
    SetGpioPin(act_led.regs, act_led.gpio);

    /* Initialize high resolution timer. */
    hrtimer_init(&act_led.blink_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	// timer interval defined as (TIMER_SEC + TIMER_NANO_SEC)
    act_led.kt = ktime_set(0 /*TIMER_SEC*/, act_led.blink_period_msec*1000000 /*TIMER_NANO_SEC*/);
    act_led.blink_timer.function = &blink_timer_callback;
    hrtimer_start(&act_led.blink_timer, act_led.kt, HRTIMER_MODE_REL);

   return 0;

fail_no_virt_mem:
    /* Freeing the major number. */
    unregister_chrdev(led_major, "led");

    return result;
}

/*
 * Cleanup:
 *  1. stop the timer
 *  2. release GPIO pins (clear all outputs, set all as inputs and pull-none to minimize the power consumption)
 *  3. Unmap GPIO Physical address space from virtual address
 *  4. Unregister device driver
 */
static void led_exit(void)
{
    printk(KERN_INFO "Removing led module\n");

    /* Release high resolution timer. */
    hrtimer_cancel(&act_led.blink_timer);

    /* Clear GPIO pins (turn off led). */
    ClearGpioPin(act_led.regs, act_led.gpio);

    /* Set GPIO pins as inputs and disable pull-ups. */
    SetGpioPinDirection(act_led.regs, act_led.gpio, GPIO_DIRECTION_IN);

    /* Unmap GPIO Physical address space. */
    if (act_led.regs)
    {
        iounmap(act_led.regs);
    }

    /* Freeing the major number. */
    unregister_chrdev(led_major, "led");
}

/* File ioctl function. */
static long led_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
    {
        case LED_STATE_OFF:
           act_led.state = LED_STATE_OFF;
           act_led.power = LED_POWER_OFF;
           ClearGpioPin(act_led.regs, act_led.gpio);
        break;
        case LED_STATE_ON:
           act_led.state = LED_STATE_ON;
           act_led.power = LED_POWER_ON;
           SetGpioPin(act_led.regs, act_led.gpio);
        break;
        case LED_STATE_BLINK:
           act_led.state = LED_STATE_BLINK;
        break;
    }
    /* Success. */
	return 0;
}

/* Declaration of the init and exit functions. */
module_init(led_init);
module_exit(led_exit);

