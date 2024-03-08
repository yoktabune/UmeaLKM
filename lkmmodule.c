#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/rwsem.h> 
#include <linux/uaccess.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/ioctl.h>

#define MYDEVICE_TYPE 'k'
#define IOCTL_DUMP_HASH_TABLE _IOR(MYDEVICE_TYPE, 1, size_t)


MODULE_LICENSE("GPL");

struct fake_device {
	struct rw_semaphore rwsem;
} virtual_device;

struct cdev *mcdev;
int major_number;
int ret;

static struct class *cls;
static struct device *dev;


dev_t dev_num;

#define DEVICE_NAME "mydevice"


char key[30];
char value[30];

void updateKeyValuePair(char *input) {
    char *chkey, *chvalue;

    chkey = strsep(&input, ":");
    printk(KERN_INFO "mydevice: chkey is %s\n", chkey);
    if (strlen(chkey) < sizeof(key)) {
        strcpy(key, chkey);
    }

    chvalue = strsep(&input, ":");
    printk(KERN_INFO "mydevice: chvalue is %s\n", chvalue);
    if (strlen(chvalue) < sizeof(value)) {
        strcpy(value, chvalue);
    }
}
#define SIZE 20
int items = 0;

struct DataItem {
   char key[30];  
   char data[30]; 
};

struct DataItem* hashArray[SIZE]; 
struct DataItem* dummyItem;
struct DataItem* item;

int hashCode(char *key) {
    int hash = 0;
    int c;

    while (c = *key++)
        hash += c;

    return hash % SIZE;
}

struct DataItem* search(char *key) {
    int hashIndex = hashCode(key);

    while (hashArray[hashIndex] != NULL) {
        if (strcmp(hashArray[hashIndex]->key, key) == 0)
            return hashArray[hashIndex];
        
        ++hashIndex;
        hashIndex %= SIZE;
    }
    
    return NULL; // Not found
}


void insert(char *key, char *data) {
    int hashIndex = hashCode(key);
    struct DataItem *item = (struct DataItem*) vmalloc(sizeof(struct DataItem));
    strcpy(item->key, key);
    strcpy(item->data, data);

    // Handling collision with linear probing
    while (hashArray[hashIndex] != NULL && strcmp(hashArray[hashIndex]->key, "-1") != 0) {
        // If the key already exists, update the value
        if (strcmp(hashArray[hashIndex]->key, key) == 0) {
            strcpy(hashArray[hashIndex]->data, data);
            vfree(item); // Free the allocated memory as it's an update operation
            return;
        }
        ++hashIndex;
        hashIndex %= SIZE;
    }

    hashArray[hashIndex] = item;
    items++;
}


void display(void) {
   int i;
   printk(KERN_INFO "mydevice: items: %i\n", items);
   for(i = 0; i < SIZE; i++) {
      if(hashArray[i] != NULL && strcmp(hashArray[i]->key, "-1") != 0)
         printk(KERN_INFO "mydevice: (%s,%s)", hashArray[i]->key, hashArray[i]->data);
      else
         printk(KERN_INFO "mydevice: ~~ ");
   }
   printk(KERN_INFO "\n");
}


int device_open(struct inode *inode, struct file *filp){

	printk(KERN_INFO "mydevice: opened device\n");
	return 0;
}

ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset){
    ssize_t data_size = 0; // Kopyalanan veri miktarını tutmak için

    down_read(&virtual_device.rwsem);
    printk(KERN_INFO "mydevice: Reading from device\n");

    char userKey[30]; // Boyut ihtiyacınıza göre ayarlayın
    if (copy_from_user(userKey, bufStoreData, min(bufCount, sizeof(userKey)-1)) != 0)
        return -EFAULT;

    userKey[min(bufCount, sizeof(userKey)-1)] = '\0'; // Null-terminasyon garantisi

    struct DataItem* item = search(userKey);
    up_read(&virtual_device.rwsem);

    if(item != NULL) {
        printk(KERN_INFO "mydevice: Element with key '%s' found, value '%s'\n", item->key, item->data);
        data_size = min(strlen(item->data) + 1, bufCount); // Kopyalanacak veri miktarını hesapla
        if (copy_to_user(bufStoreData, item->data, data_size) != 0)
            return -EFAULT;
    } else {
        printk(KERN_INFO "mydevice: Element with key '%s' not found\n", userKey);
        char failMsg[] = "Not Found";
        data_size = min(sizeof(failMsg), bufCount); // Kopyalanacak veri miktarını hesapla
        if (copy_to_user(bufStoreData, failMsg, data_size) != 0)
            return -EFAULT;
    }
    
    *curOffset += data_size; // Mevcut offseti güncelle
    return data_size; // Kullanıcı alanına kopyalanan verinin boyutunu döndür
}



char ourInput[100];
ssize_t device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset){
    down_write(&virtual_device.rwsem);
    printk(KERN_INFO "mydevice: writing to device\n");

    ssize_t not_copied = copy_from_user(ourInput, bufSourceData, bufCount);
    if (not_copied == bufCount) {
        // Eğer hiçbir veri başarıyla kopyalanmadıysa, hata döndür
        up_write(&virtual_device.rwsem);
        return -EFAULT;
    }

    updateKeyValuePair(ourInput);
    insert(key, value);
    display();
    up_write(&virtual_device.rwsem);

    printk(KERN_INFO "mydevice: unlocked device\n");
    // Başarıyla kopyalanan bayt sayısını döndür
    return bufCount - not_copied;
}

long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    down_read(&virtual_device.rwsem);
    char *buffer;
    size_t bufferSize = 0;
    int i, error = 0;

    // Calculate buffer size needed
    for (i = 0; i < SIZE; i++) {
        if (hashArray[i] != NULL && strcmp(hashArray[i]->key, "-1") != 0) {
            bufferSize += strlen(hashArray[i]->key) + strlen(hashArray[i]->data) + 2; // For ':' and '\n'
        }
    }

    // Allocate buffer
    buffer = vmalloc(bufferSize + 1); // +1 for null terminator
    if (!buffer) return -ENOMEM;

    buffer[0] = '\0'; // Start with an empty string

    // Fill the buffer with hash table content
    for (i = 0; i < SIZE && !error; i++) {
        if (hashArray[i] != NULL && strcmp(hashArray[i]->key, "-1") != 0) {
            error = snprintf(buffer + strlen(buffer), bufferSize - strlen(buffer), 
                             "%s:%s\n", hashArray[i]->key, hashArray[i]->data) < 0;
        }
    }

    if (!error) {
        // Copy the buffer to user space
        if (copy_to_user((char __user *)arg, buffer, strlen(buffer) + 1)) {
            error = -EFAULT;
        }
    }

    vfree(buffer);
    up_read(&virtual_device.rwsem);
    return error ? error : bufferSize;
}

int device_close(struct inode *inode, struct file *filp){
	printk(KERN_INFO "mydevice: closed device\n");
	return 0;
}


struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_close,
	.write = device_write,
    .unlocked_ioctl = device_ioctl,
	.read = device_read
};

static int driver_entry(void){
	ret = alloc_chrdev_region(&dev_num,0,1,DEVICE_NAME);
	if(ret < 0) {
		printk(KERN_ALERT "mydevice: failed to allocate a major number.\n");
		return ret;
	}
	
	major_number = MAJOR(dev_num);
	printk(KERN_INFO "mydevice: major number is %d\n", major_number);
	
	mcdev = cdev_alloc();
	mcdev->ops = &fops;
	mcdev->owner = THIS_MODULE;

	ret = cdev_add(mcdev, dev_num, 1);

	if(ret < 0) {
		printk(KERN_ALERT "mydevice: unable to add cdev to kernel\n");
		return ret;
	}

	init_rwsem(&virtual_device.rwsem);

	dummyItem = (struct DataItem*) vmalloc(sizeof(struct DataItem));
    strcpy(dummyItem->data, "-1");  
    strcpy(dummyItem->key, "-1");

    cls = class_create(DEVICE_NAME);
    if (IS_ERR(cls)) {
        cdev_del(mcdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(cls);
    }

    dev = device_create(cls, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(dev)) {
        class_destroy(cls);
        cdev_del(mcdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(dev);
    }

    printk(KERN_INFO "mydevice: device class created correctly\n");

	return 0;
}

static void driver_exit(void){
    device_destroy(cls, dev_num);
    class_destroy(cls);
	cdev_del(mcdev);
	unregister_chrdev_region(dev_num, 1);
	printk(KERN_ALERT "mydevice: unloaded module\n");

}

module_init(driver_entry);
module_exit(driver_exit);