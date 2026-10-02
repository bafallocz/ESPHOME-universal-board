import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_CHANNEL, CONF_ID

from . import AD5593RComponent, CONF_AD5593R_ID, ad5593r_ns

DEPENDENCIES = ["ad5593r"]

CONF_PULLDOWN = "pulldown"

AD5593RBinarySensor = ad5593r_ns.class_("AD5593RBinarySensor", binary_sensor.BinarySensor)

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(AD5593RBinarySensor).extend(
    {
        cv.GenerateID(CONF_AD5593R_ID): cv.use_id(AD5593RComponent),
        cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=7),
        cv.Optional(CONF_PULLDOWN, default=False): cv.boolean,
    }
)


async def to_code(config):
    cg.add_define("USE_BINARY_SENSOR")
    parent = await cg.get_variable(config[CONF_AD5593R_ID])
    var = cg.new_Pvariable(config[CONF_ID], parent, config[CONF_CHANNEL])
    await binary_sensor.register_binary_sensor(var, config)
    cg.add(parent.register_gpio_input(var, config[CONF_PULLDOWN]))
