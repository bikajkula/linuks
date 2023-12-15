/* Download it from http://j.mp/GQ2yuD */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>

#include <linux/delay.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/input-polldev.h>

static const char buf[] = {0xf0, 0x55, 0xfb, 0x0};

static char buff[6];

struct nunchuk_dev {
	struct input_polled_dev *polled_input;
	struct i2c_client *i2c_client;
};


static void nunchuk_read_registers(struct i2c_client *client)
{
	mdelay(10);

	if (i2c_master_send(client, buf + 3, 1) >= 0)
		pr_info("Okay\n");
	else
		pr_info("Not okay\n");
		
	mdelay(10);
	
	if (i2c_master_recv(client, buff, 6) >= 0)
		pr_info("Okay\n");
	else
		pr_info("Not okay\n");

	pr_info("Called nunchuk_read_registers\n");
}

static void nunchuk_poll(struct input_polled_dev *dev)
{
	struct nunchuk_dev *nunchuk;

	int err = -1;

	int cpressed;
	int zpressed;

	nunchuk = dev->private;

	mdelay(10);

	i2c_master_send(nunchuk->i2c_client, buf + 3, 1);

	mdelay(10);

	err = i2c_master_recv(nunchuk->i2c_client, buff, 6);

	zpressed = buff[5] & 0x1; // 0x1 = 0001
	cpressed = buff[5] & 0x2; // 0x2 = 0010
	cpressed >>= 1;

	input_event(dev->input, EV_KEY, BTN_C, cpressed);
	input_event(dev->input, EV_KEY, BTN_Z, zpressed);
	input_sync(dev->input);

	//pr_info("bajtova primljeno: %d", err);

	pr_info("\nx-axis: %d\n", buff[0]);
	pr_info("y-axis: %d\n", buff[1]);
	pr_info("x-axis accel: %d\n", buff[2]);
	pr_info("y-axis accel: %d\n", buff[3]);
	pr_info("z-axis accel: %d\n", buff[4]);
	pr_info("button Z: %d\n", zpressed);
	pr_info("button C: %d\n", cpressed);
	udelay(1000);
}

static int nunchuk_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct input_polled_dev* polled_input;
	struct input_dev* input;
	
	struct nunchuk_dev *nunchuk;
	
	nunchuk = devm_kzalloc(&client->dev, sizeof(struct nunchuk_dev), GFP_KERNEL);
	if (!nunchuk) {
		dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
	}
	
	polled_input = devm_input_allocate_polled_device(&client->dev);
	if (!polled_input)
        	return -ENOMEM;
	
	nunchuk->i2c_client = client;
	nunchuk->polled_input = polled_input;
	
	polled_input->private = nunchuk;
	polled_input->poll = nunchuk_poll;
	polled_input->poll_interval = 50;
	
	i2c_set_clientdata(client, nunchuk);
	
	input = polled_input->input;
	input->dev.parent = &client->dev;
	input->name = "Wii Nunchuk";
	input->id.bustype = BUS_I2C;
	
	set_bit(EV_KEY, input->evbit);
	set_bit(BTN_C, input->keybit);
	set_bit(BTN_Z, input->keybit);
	
	if(input_register_polled_device(polled_input) != 0){
		return -ENOMEM;
	}

	pr_info("Called nunchuk_probe\n");
	
	if(i2c_master_send(client, buf, 2) >= 0)
		pr_info("Okay\n");
	else
		pr_info("Not okay\n");
	
	udelay(1000);
	
	if(i2c_master_send(client, buf + 2, 2) >= 0)
		pr_info("Okay\n");
	else
		pr_info("Not okay\n");
		
	nunchuk_read_registers(client);
	nunchuk_read_registers(client);
	
	if ((buff[5] & 0b00000001) == 0)
		pr_info("Z pressed\n");
	else
		pr_info("Z not pressed\n");

	if ((buff[5] & 0b00000010) == 0)
		pr_info("C pressed\n");
	else
		pr_info("C not pressed\n");

	return 0;
}

static int nunchuk_remove(struct i2c_client *client)
{
	pr_info("Called nunchuk_remove\n");
	return 0;
}

static const struct i2c_device_id nunchuk_id[] = {
	{ "nunchuk", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, nunchuk_id);

#ifdef CONFIG_OF
static struct of_device_id nunchuk_dt_match[] = {
	{ .compatible = "nintendo,nunchuk" },
	{ },
};
#endif

static struct i2c_driver nunchuk_driver = {
	.driver = {
		.name = "nunchuk",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(nunchuk_dt_match),
	},
	.probe = nunchuk_probe,
	.remove = nunchuk_remove,
	.id_table = nunchuk_id,
};

module_i2c_driver(nunchuk_driver);

MODULE_LICENSE("GPL");

