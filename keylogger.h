#ifndef KEYLOGGER_H
# define KEYLOGGER_H

# include <linux/module.h>
# include <linux/kernel.h>
# include <linux/interrupt.h>
# include <linux/io.h>
# include <linux/list.h>
# include <linux/mutex.h>
# include <linux/time.h>
# include <linux/miscdevice.h>
# include <linux/string.h>
# include <linux/time64.h>
# include <linux/seq_file.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mberengu");
MODULE_DESCRIPTION("keyboard keylogger driver");

#define IRQ_NAME "Keylogger"

# define IRQ_NUM 1		// dmesg | grep -i irq  => [    0.539481] serio: i8042 KBD port at 0x60,0x64 irq 1
# define KEYBOARD_PORT 0x60	//                                                                   ^ -> ^

/*
 *  CHAT GPT :
 *
 * The RELEASED_MASK is used to filter out the eighth bit (bit 7)
 * in the scan code to determine whether a key is pressed or released.
 * When a key is pressed, the eighth bit is typically cleared (0),
 * while when it is released, the eighth bit is typically set (1).
 * This mask is defined as 0x80 to filter out the eighth bit.
 */
# define RELEASED_MASK 0x80
# define HOUR 3600

// id for irq interrupt
static void *dev_id = "Keylogger";

static void keyboard_tasklet(struct tasklet_struct *tasklet);
static LIST_HEAD(keylist);
DECLARE_TASKLET(kbd_tasklet, keyboard_tasklet);
DEFINE_RWLOCK(misc_lock);
DEFINE_MUTEX(tasklet_lock);

struct keycodes_s {
	char		key[15];
	char		maj[2];
	char		variation[4];
	unsigned char	pressed;
	unsigned char	released;
};


struct key_display_format {
	struct tm 		time;
	char			key[15];
	unsigned char		scancode;
	unsigned char		state;
	struct list_head	list;
};

// azerty
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
	{"!", "§", "", 0x35, 0xb5},	// 33

	// NUMBERS
	{"&", "1", "", 0x02, 0x82},
	{"é", "2", "~", 0x03, 0x83},
	{"\"", "3", "#", 0x04, 0x84},
	{"'", "4", "{", 0x05, 0x85},
	{"(", "5", "[", 0x06, 0x86},
	{"-", "6", "|", 0x07, 0x87},
	{"è", "7", "`", 0x08, 0x88},
	{"_", "8", "\\", 0x09, 0x89},
	{"ç", "9", "^", 0x0a, 0x8a},
	{"à", "0", "@", 0x0b, 0x8b},
	{")", "°", "]", 0x0c, 0x8c},
	{"=", "+", "}", 0x0d, 0x8d},	// 12 // 45
	
	// PAD - NUMBERS
	// no pad on laptop
/*	{"0", "", "", 0x00, 0x00},
	{"1", "", "", 0x00, 0x00},
	{"2", "", "", 0x00, 0x00},
	{"3", "", "", 0x00, 0x00},
	{"4", "", "", 0x00, 0x00},
	{"5", "", "", 0x00, 0x00},
	{"6", "", "", 0x00, 0x00},
	{"7", "", "", 0x00, 0x00},
	{"8", "", "", 0x00, 0x00},
	{"9", "", "", 0x00, 0x00}, */	// 0

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
	{"f12", "", "", 0x58, 0xd8},	// 13 // 58
	// left side special
	{"²", "", "", 0x01, 0x81},
	{"tab", "", "", 0x0f, 0x8f},
	{"caps up", "", "", 0x3a, 0xba},
	{"caps down", "", "", 0x3a, 0xfa},
	{"lshift", "", "", 0x2a, 0xaa},
	{"lctrl", "", "", 0x1d, 0x9d},
	{"alt", "", "", 0x38, 0xb8},	// 7 // 65
	// right side special
	{"backspace", "", "", 0x0e, 0x8e},
	{"*", "µ", "", 0x2b, 0xab},
	{"enter", "", "", 0x1c, 0x9c},
	{"rshift", "", "", 0x36, 0xb6},
	{"rctrl", "", "", 0xe0, 0x9d},
	{"arret defil", "", "", 0x46, 0xc6},
	{"<", ">", "", 0x56, 0xd6},	// 7 // 72

	//SPECIAL CASES
	{"window", "", "", 0x5c, 0xdc},	// released [e0]
					// pressed [38]
					// released [e0]
					// released [b8]

	{"alt-gr", "", "", 0x38, 0xb8},	// released [e0]
					// pressed [38]
					// released [e0]
					// released [b8]


	{"left arrow", "", "", 0x4b, 0xcb},	// released [e0]
						// pressed [4b]
						// released [e0]
						// released [cb]

	{"right arrow", "", "", 0x4d, 0xcd},	// released [e0]
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

	{"pause attn", "", "", 0x45, 0xc5}, // release	0xe1
					    // press	0x45
					    // release	0xe1
					    // release	0x9d
					    // release	0xc5	// 14 // 86
};

static int	misc_open(struct inode *inode, struct file *file);
static int 	misc_read(struct seq_file *seq, void *ptr);
static int	misc_release(struct inode *inode, struct file *file);

// events function call

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = misc_open,
	.read = seq_read,
	.release = misc_release
};

// define device
static struct miscdevice miscdev = {
	.name = "kbdlog",
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &fops
};

#endif
