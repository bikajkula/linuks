/* Download it from http://j.mp/GOFKfU */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/slab.h>

struct nunchuk_dev {
	struct input_polled_dev *polled_input;
	struct i2c_client *i2c_client;
};

static int nunchuk_read_registers(struct i2c_client *client,
				   u8 *recv)
{
	u8 buf[1];
	int ret;

	/* Ask the device to get ready for a read */

	mdelay(10);

	buf[0] = 0x00;
	ret = i2c_master_send(client, buf, 1);
	if (ret != 1) {
		dev_err(&client->dev, "i2c send failed (%d)\n", ret);
		return ret < 0 ? ret : -EIO;
	}

	mdelay(10);

	/* Now read registers */

	ret = i2c_master_recv(client, recv, 6);
	if (ret != 6) {
		dev_err(&client->dev, "i2c recv failed (%d)\n", ret);
		return ret < 0 ? ret : -EIO;
	}

	return 0;
}

static void nunchuk_poll(struct input_polled_dev *polled_input)
{
	u8 recv[6];
	int zpressed, cpressed;

	/* Retrieve the physical i2c device */

	struct nunchuk_dev *nunchuk = polled_input->private;
	struct i2c_client *client = nunchuk->i2c_client;

	/* Get the state of the device registers */

	if (nunchuk_read_registers(client, recv) < 0)
		return;

	zpressed = (recv[5] & BIT(0)) ? 0 : 1;

	if (zpressed)
		dev_info(&client->dev, "Z button pressed\n");

	cpressed = (recv[5] & BIT(1)) ? 0 : 1;

	if (cpressed)
		dev_info(&client->dev, "C button pressed\n");

	/* Send events to the INPUT subsystem */
	input_event(polled_input->input,
		    EV_KEY, BTN_Z, zpressed);

	input_event(polled_input->input,
		    EV_KEY, BTN_C, cpressed);

	input_sync(polled_input->input);
}

static int nunchuk_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int ret;
	u8 buf[2];

	struct input_polled_dev *polled_input;
	struct input_dev *input;
	struct nunchuk_dev *nunchuk;

	pr_info("Called nunchuk_probe\n");

	nunchuk = devm_kzalloc(&client->dev, sizeof(struct nunchuk_dev), GFP_KERNEL);
	if (!nunchuk) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	/* Initialize device */

	buf[0] = 0xf0;
	buf[1] = 0x55;

	ret = i2c_master_send(client, buf, 2);
	if (ret != 2) {
		dev_err(&client->dev, "i2c send failed (%d)\n", ret);
		return ret < 0 ? ret : -EIO;
	}

	udelay(1000);

	buf[0] = 0xfb;
	buf[1] = 0x00;

	ret = i2c_master_send(client, buf, 2);
	if (ret != 2) {
		dev_err(&client->dev, "i2c send failed (%d)\n", ret);
		return ret < 0 ? ret : -EIO;
	}

	/* Register input device */

	polled_input = input_allocate_polled_device();
	if (!polled_input) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	/* Implement pointers between logical and physical */

	nunchuk->i2c_client = client;
	nunchuk->polled_input = polled_input;
	polled_input->private = nunchuk;
	i2c_set_clientdata(client, nunchuk);
	input = polled_input->input;
	input->dev.parent = &client->dev;

	/* Configure input device */

	input->name = "Wii Nunchuk";
	input->id.bustype = BUS_I2C;

	set_bit(EV_KEY, input->evbit);
	set_bit(BTN_C, input->keybit);
	set_bit(BTN_Z, input->keybit);

	polled_input->poll = nunchuk_poll;
	polled_input->poll_interval = 50;

	/* Register the input device when everything is ready */

	ret = input_register_polled_device(polled_input);
	if (ret < 0) {
		dev_err(&client->dev, "cannot register input device (%d)\n",
			ret);
		goto register_input_fail;
	}

	return 0;

register_input_fail:
	input_free_polled_device(polled_input);
	return ret;
}

static int nunchuk_remove(struct i2c_client *client)
{
	struct nunchuk_dev *nunchuk = i2c_get_clientdata(client);
	pr_info("Called nunchuk_remove\n");

	input_unregister_polled_device(nunchuk->polled_input);
	input_free_polled_device(nunchuk->polled_input);

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
