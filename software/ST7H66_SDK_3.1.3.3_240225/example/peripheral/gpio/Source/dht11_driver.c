

#include "dht11_driver.h"
#include "clock.h"
#include "log.h"


#define CHECK_OPS(ops, ...) \
if(ops)                     \
{                           \
    ops(__VA_ARGS__);               \
} 

#define DHT_READ_RETRY_MAX    500

static dht_driver_ops_t *dht_ops = NULL;

static uint8_t dht_start(void)
{
    uint8_t get_wait_cnt = 0;

    CHECK_OPS(dht_ops->gpio_init, DHT_GPIO_MODE_OUT);

    CHECK_OPS(dht_ops->gpio_set, DHT_GPIO_STATE_LOW);

    CHECK_OPS(dht_ops->delay_us, 20 * 1000);

    CHECK_OPS(dht_ops->gpio_set, DHT_GPIO_STATE_HIGHT);

    CHECK_OPS(dht_ops->gpio_init, DHT_GPIO_MODE_IN);

    CHECK_OPS(dht_ops->delay_us, 20);

    if((dht_ops->gpio_get) && (dht_ops->gpio_get() == DHT_GPIO_STATE_LOW))
    {
        while((dht_ops->gpio_get() == DHT_GPIO_STATE_LOW) && (get_wait_cnt < DHT_READ_RETRY_MAX))
        {
            get_wait_cnt++;
            CHECK_OPS(dht_ops->delay_us, 1);
        }

        if(get_wait_cnt >= DHT_READ_RETRY_MAX)
        {
            return 0;
        }
        else
        {
            get_wait_cnt = 0;
        }

        while((dht_ops->gpio_get() == DHT_GPIO_STATE_HIGHT) && (get_wait_cnt < DHT_READ_RETRY_MAX))
        {
            get_wait_cnt++;
            CHECK_OPS(dht_ops->delay_us, 1);
        }

        if(get_wait_cnt >= DHT_READ_RETRY_MAX)
        {
            return 0;
        }

        return 1;
    }

    return 0;
}

static uint8_t dht_get_byte_data(void)
{
    uint8_t get_wait_cnt = 0;
	uint8_t temp;

	for(int i = 0; i < 8; i++)
	{
		temp <<= 1;

        while((dht_ops->gpio_get() == DHT_GPIO_STATE_LOW) && (get_wait_cnt < DHT_READ_RETRY_MAX))
        {
            get_wait_cnt++;
            CHECK_OPS(dht_ops->delay_us, 1);
        }

        if(get_wait_cnt >= DHT_READ_RETRY_MAX)
        {
            return 0;
        }
        else
        {
            get_wait_cnt = 0;
        }

        CHECK_OPS(dht_ops->delay_us, 28);

		(dht_ops->gpio_get() == DHT_GPIO_STATE_HIGHT) ? (temp |= 0x01) : (temp &= ~0x01);

        while((dht_ops->gpio_get() == DHT_GPIO_STATE_HIGHT) && (get_wait_cnt < DHT_READ_RETRY_MAX))
        {
            get_wait_cnt++;
            CHECK_OPS(dht_ops->delay_us, 1);
        }

        if(get_wait_cnt >= DHT_READ_RETRY_MAX)
        {
            return 0;
        }
	}

	return temp;
}

void dht_driver_init(dht_driver_ops_t *ops)
{
    dht_ops = ops;
}

uint8_t dht_get_temp_humi_data(dht_driver_data_t *data)
{
    uint8_t buffer[5] = {0};

    if(data == NULL)
    {
        return 0;
    }

	if(dht_start())
	{
		buffer[0] = dht_get_byte_data();
		buffer[1] = dht_get_byte_data();
		buffer[2] = dht_get_byte_data();
		buffer[3] = dht_get_byte_data();
		buffer[4] = dht_get_byte_data();

        data->temp = buffer[2] * 100 + buffer[3];

        data->humi = buffer[0] * 100 + buffer[1];
	}

    return ((buffer[0] + buffer[1] + buffer[2] + buffer[3]) == buffer[4]) ? 1 : 0;
}
