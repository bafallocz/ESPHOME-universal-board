import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_CHANNEL,
    CONF_ID,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_VOLT,
)

from . import AD5593RComponent, CONF_AD5593R_ID, ad5593r_ns

DEPENDENCIES = ["ad5593r"]

AD5593RSensor = ad5593r_ns.class_("AD5593RSensor", sensor.Sensor, cg.PollingComponent)


def validate_channel(value):
    if isinstance(value, str) and value.lower() in ["temperature", "temp"]:
        return 8
    try:
        val = int(value)
        if 0 <= val <= 8:
            return val
    except (ValueError, TypeError):
        pass
    raise cv.Invalid("Channel must be an integer between 0 and 7, or 'temperature'")


CONFIG_SCHEMA = (
    sensor.sensor_schema(
        AD5593RSensor,
    )
    .extend(
        {
            cv.GenerateID(CONF_AD5593R_ID): cv.use_id(AD5593RComponent),
            cv.Required(CONF_CHANNEL): validate_channel,
        }
    )
    .extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    cg.add_define("USE_SENSOR")
    parent = await cg.get_variable(config[CONF_AD5593R_ID])
    ch = config[CONF_CHANNEL]
    var = cg.new_Pvariable(config[CONF_ID], parent, ch)
    await cg.register_component(var, config)

    # Apply default units and classes if not overridden by the user
    sensor_conf = dict(config)
    if ch == 8:
        if sensor.CONF_UNIT_OF_MEASUREMENT not in config:
            sensor_conf[sensor.CONF_UNIT_OF_MEASUREMENT] = UNIT_CELSIUS
        if sensor.CONF_DEVICE_CLASS not in config:
            sensor_conf[sensor.CONF_DEVICE_CLASS] = DEVICE_CLASS_TEMPERATURE
        if sensor.CONF_STATE_CLASS not in config:
            sensor_conf[sensor.CONF_STATE_CLASS] = STATE_CLASS_MEASUREMENT
        if sensor.CONF_ACCURACY_DECIMALS not in config:
            sensor_conf[sensor.CONF_ACCURACY_DECIMALS] = 1
    else:
        if sensor.CONF_UNIT_OF_MEASUREMENT not in config:
            sensor_conf[sensor.CONF_UNIT_OF_MEASUREMENT] = UNIT_VOLT
        if sensor.CONF_DEVICE_CLASS not in config:
            sensor_conf[sensor.CONF_DEVICE_CLASS] = DEVICE_CLASS_VOLTAGE
        if sensor.CONF_STATE_CLASS not in config:
            sensor_conf[sensor.CONF_STATE_CLASS] = STATE_CLASS_MEASUREMENT
        if sensor.CONF_ACCURACY_DECIMALS not in config:
            sensor_conf[sensor.CONF_ACCURACY_DECIMALS] = 3

    await sensor.register_sensor(var, sensor_conf)
    cg.add(parent.register_adc_sensor(var))
