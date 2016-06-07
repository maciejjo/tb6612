#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio/consumer.h>
#include <linux/pwm.h>

#define MOTOR_PWM_PERIOD 10000000
#define MOTOR_PWM_DUTY_MUL 100000

enum motor_mode {
	MOTOR_MODE_CW,
	MOTOR_MODE_CCW,
	MOTOR_MODE_STOP
};

/* struct motor_data
 *
 * Describe resources associated with single motor.
 *
 * speed_pwm - PWM signal driving motor speed
 * in1_gpio, in2_gpio - GPIO signals for setting motor direction
 * speed - variable to store current speed
 * mode -  variable to store motor mode (cw, ccw, stop)
 */

struct motor_data {
	struct pwm_device *speed_pwm;
	struct gpio_desc *in1_gpio;
	struct gpio_desc *in2_gpio;
	unsigned int speed;
	enum motor_mode mode;
};

/* struct tb6612_data
 *
 * Describe resources associated with whole chip.
 *
 * motor_a, motor_b - data associated with each motor
 * standby - field describing standby state
 * standby_gpio - GPIO for setting standby state
 *
 */

struct tb6612_data {
	struct motor_data motor_a;
	struct motor_data motor_b;
	unsigned char standby;
	struct gpio_desc *standby_gpio;
};

static ssize_t tb6612_show_motor_a_speed(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tb6612_data *data =
		platform_get_drvdata(to_platform_device(dev));

	return sprintf(buf, "%u\n", data->motor_a.speed);
}

static ssize_t tb6612_store_motor_a_speed(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long tmp;
	int error;
	struct tb6612_data *data =
		platform_get_drvdata(to_platform_device(dev));

	error = kstrtoul(buf, 10, &tmp);

	if (tmp < 0 || tmp > 100)
		return -EINVAL;

	error = pwm_config(data->motor_a.speed_pwm, tmp * MOTOR_PWM_DUTY_MUL,
			MOTOR_PWM_PERIOD);
	if (error < 0)
		return error;

	data->motor_a.speed = tmp;

	return count;
}

static ssize_t tb6612_show_motor_b_speed(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tb6612_data *data =
		platform_get_drvdata(to_platform_device(dev));

	return sprintf(buf, "%u\n", data->motor_b.speed);
}

static ssize_t tb6612_store_motor_b_speed(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long tmp;
	int error;
	struct tb6612_data *data =
		platform_get_drvdata(to_platform_device(dev));

	error = kstrtoul(buf, 10, &tmp);

	if (tmp < 0 || tmp > 100)
		return -EINVAL;

	error = pwm_config(data->motor_b.speed_pwm, tmp * MOTOR_PWM_DUTY_MUL,
			MOTOR_PWM_PERIOD);
	if (error < 0)
		return error;

	data->motor_b.speed = tmp;

	return count;
}

static ssize_t tb6612_show_motor_a_mode(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tb6612_data *data =
		platform_get_drvdata(to_platform_device(dev));

	switch (data->motor_a.mode) {

	case MOTOR_MODE_CW:
		return sprintf(buf, "cw\n");
	case MOTOR_MODE_CCW:
		return sprintf(buf, "ccw\n");
	case MOTOR_MODE_STOP:
		return sprintf(buf, "stop\n");
	default:
		return -EINVAL;

	}


}

static ssize_t tb6612_store_motor_a_mode(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)


{
	struct tb6612_data *data =
		platform_get_drvdata(to_platform_device(dev));

	dev_err(dev, "buffer is %s\n", buf);

	if (strncmp(buf, "stop", strlen("stop")) == 0) {

		gpiod_set_value(data->motor_a.in1_gpio, 0);
		gpiod_set_value(data->motor_a.in2_gpio, 0);
		data->motor_a.mode = MOTOR_MODE_STOP;

	} else if (strncmp(buf, "cw", strlen("cw")) == 0) {

		gpiod_set_value(data->motor_a.in1_gpio, 0);
		gpiod_set_value(data->motor_a.in2_gpio, 1);
		data->motor_a.mode = MOTOR_MODE_CW;

	} else if (strncmp(buf, "ccw", strlen("ccw")) == 0) {

		gpiod_set_value(data->motor_a.in1_gpio, 1);
		gpiod_set_value(data->motor_a.in2_gpio, 0);
		data->motor_a.mode = MOTOR_MODE_CCW;

	} else {

		return -EINVAL;

	}

	return count;
}

static ssize_t tb6612_show_motor_b_mode(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tb6612_data *data =
		platform_get_drvdata(to_platform_device(dev));

	switch (data->motor_b.mode) {

	case MOTOR_MODE_CW:
		return sprintf(buf, "cw\n");
	case MOTOR_MODE_CCW:
		return sprintf(buf, "ccw\n");
	case MOTOR_MODE_STOP:
		return sprintf(buf, "stop\n");
	default:
		return -EINVAL;

	}
}

static ssize_t tb6612_store_motor_b_mode(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)


{
	struct tb6612_data *data =
		platform_get_drvdata(to_platform_device(dev));

	if (strncmp(buf, "stop", strlen("stop")) == 0) {

		gpiod_set_value(data->motor_b.in1_gpio, 0);
		gpiod_set_value(data->motor_b.in2_gpio, 0);
		data->motor_b.mode = MOTOR_MODE_STOP;

	} else if (strncmp(buf, "cw", strlen("cw")) == 0) {

		gpiod_set_value(data->motor_b.in1_gpio, 0);
		gpiod_set_value(data->motor_b.in2_gpio, 1);
		data->motor_b.mode = MOTOR_MODE_CW;

	} else if (strncmp(buf, "ccw", strlen("ccw")) == 0) {

		gpiod_set_value(data->motor_b.in1_gpio, 1);
		gpiod_set_value(data->motor_b.in2_gpio, 0);
		data->motor_b.mode = MOTOR_MODE_CCW;

	} else {

		return -EINVAL;

	}

	return count;
}

static ssize_t tb6612_show_suspend(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tb6612_data *data =
		platform_get_drvdata(to_platform_device(dev));

	return sprintf(buf, "%i\n", data->standby);
}

static ssize_t tb6612_store_suspend(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long tmp;
	int error;
	struct tb6612_data *data =
		platform_get_drvdata(to_platform_device(dev));

	error = kstrtoul(buf, 10, &tmp);

	if (tmp > 1)
		return -EINVAL;

	/* Chip has reverse standby logic, need to negate */
	gpiod_set_value(data->standby_gpio, !tmp);

	data->standby = tmp;

	return count;
}

static DEVICE_ATTR(motor_a_speed, S_IWUSR | S_IRUGO, tb6612_show_motor_a_speed,
		tb6612_store_motor_a_speed);

static DEVICE_ATTR(motor_a_mode, S_IWUSR | S_IRUGO, tb6612_show_motor_a_mode,
		tb6612_store_motor_a_mode);

static DEVICE_ATTR(motor_b_speed, S_IWUSR | S_IRUGO, tb6612_show_motor_b_speed,
		tb6612_store_motor_b_speed);

static DEVICE_ATTR(motor_b_mode, S_IWUSR | S_IRUGO, tb6612_show_motor_b_mode,
		tb6612_store_motor_b_mode);

static DEVICE_ATTR(suspend, S_IWUSR | S_IRUGO, tb6612_show_suspend,
		tb6612_store_suspend);

static struct attribute *tb6612_attributes[] = {
	&dev_attr_motor_a_speed.attr,
	&dev_attr_motor_a_mode.attr,
	&dev_attr_motor_b_speed.attr,
	&dev_attr_motor_b_mode.attr,
	&dev_attr_suspend.attr,
	NULL,
};

static const struct attribute_group tb6612_group = {
	.attrs = tb6612_attributes,
};


static int tb6612_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct pinctrl *pinctrl;
	struct device_node *node = dev->of_node;
	struct tb6612_data *data;
	int error = 0;

	if (node == NULL) {
		dev_err(dev, "Non DT platforms not supported\n");
		return -EINVAL;
	}

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	/* Select pins that are in use */

	pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR(pinctrl))
		dev_warn(&pdev->dev, "Unable to select pin group\n");

	/* Configure PWMs for both motors" */

	data->motor_a.speed_pwm = devm_pwm_get(&pdev->dev, "motor_a");
	if (IS_ERR(data->motor_a.speed_pwm)) {
		dev_err(&pdev->dev, "devm_pwm_get() failed\n");
		return error;
	}

	error = pwm_config(data->motor_a.speed_pwm, 0, MOTOR_PWM_PERIOD);
	if (error) {
		dev_err(&pdev->dev, "pwm_config() failed\n");
		return error;
	}

	error = pwm_set_polarity(data->motor_a.speed_pwm, PWM_POLARITY_NORMAL);
	if (error) {
		dev_err(&pdev->dev, "pwm_set_polarity() failed\n");
		return error;
	}

	error = pwm_enable(data->motor_a.speed_pwm);
	if (error) {
		dev_err(&pdev->dev, "pwm_enable() failed\n");
		return error;
	}

	data->motor_b.speed_pwm = devm_pwm_get(&pdev->dev, "motor_b");
	if (IS_ERR(data->motor_b.speed_pwm)) {
		dev_err(&pdev->dev, "devm_pwm_get() failed\n");
		return error;
	}

	error = pwm_config(data->motor_b.speed_pwm, 0, MOTOR_PWM_PERIOD);
	if (error) {
		dev_err(&pdev->dev, "pwm_config() failed\n");
		return error;
	}

	error = pwm_set_polarity(data->motor_b.speed_pwm, PWM_POLARITY_NORMAL);
	if (error) {
		dev_err(&pdev->dev, "pwm_set_polarity() failed\n");
		return error;
	}


	error = pwm_enable(data->motor_b.speed_pwm);
	if (error) {
		dev_err(&pdev->dev, "pwm_enable() failed\n");
		return error;
	}

	/* Configure GPIOs */

	data->motor_a.in1_gpio = devm_gpiod_get(&pdev->dev,
			"ain1", GPIOD_OUT_LOW);
	if (IS_ERR(data->motor_a.in1_gpio)) {
		dev_err(&pdev->dev, "devm_gpiod_get() failed\n");
		return error;
	}

	data->motor_a.in2_gpio = devm_gpiod_get(&pdev->dev,
			"ain2", GPIOD_OUT_LOW);
	if (IS_ERR(data->motor_a.in1_gpio)) {
		dev_err(&pdev->dev, "devm_gpiod_get() failed\n");
		return error;
	}

	data->motor_b.in1_gpio = devm_gpiod_get(&pdev->dev,
			"bin1", GPIOD_OUT_LOW);
	if (IS_ERR(data->motor_b.in1_gpio)) {
		dev_err(&pdev->dev, "devm_gpiod_get() failed\n");
		return error;
	}

	data->motor_b.in2_gpio = devm_gpiod_get(&pdev->dev,
			"bin2", GPIOD_OUT_LOW);
	if (IS_ERR(data->motor_b.in1_gpio)) {
		dev_err(&pdev->dev, "devm_gpiod_get() failed\n");
		return error;
	}

	data->standby_gpio = devm_gpiod_get(&pdev->dev, "stby", GPIOD_OUT_LOW);
	if (IS_ERR(data->motor_b.in1_gpio)) {
		dev_err(&pdev->dev, "devm_gpiod_get() failed\n");
		return error;
	}

	platform_set_drvdata(pdev, data);

	error = sysfs_create_group(&pdev->dev.kobj, &tb6612_group);
	if (error) {
		dev_err(&pdev->dev, "sysfs_create_group() failed (%d)\n",
				error);
		return error;
	}

	/* Set initial values */



	return 0;
}

static int tb6612_remove(struct platform_device *pdev)
{
	struct tb6612_data *data = platform_get_drvdata(pdev);

	pwm_disable(data->motor_a.speed_pwm);
	pwm_disable(data->motor_b.speed_pwm);

	gpiod_set_value(data->motor_a.in1_gpio, 0);
	gpiod_set_value(data->motor_a.in2_gpio, 0);
	gpiod_set_value(data->motor_b.in1_gpio, 0);
	gpiod_set_value(data->motor_b.in2_gpio, 0);
	gpiod_set_value(data->standby_gpio, 0);

	sysfs_remove_group(&pdev->dev.kobj, &tb6612_group);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id tb6612_match[] = {
	{ .compatible = "toshiba,tb6612fng", },
	{ },
};
MODULE_DEVICE_TABLE(of, tb6612_match);
#endif

static struct platform_driver tb6612_driver = {
	.probe	= tb6612_probe,
	.remove = tb6612_remove,
	.driver = {
		.name	= "tb6612",
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(tb6612_match),
#endif
	},
};
module_platform_driver(tb6612_driver);

MODULE_AUTHOR("Adam Olek, Maciej Sobkowski <maciejjo@maciejjo.pl");
MODULE_DESCRIPTION("Toshiba TB6612FNG Driver IC for Dual DC motor");
MODULE_LICENSE("GPL");
