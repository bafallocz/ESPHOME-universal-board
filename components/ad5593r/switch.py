import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_CHANNEL, CONF_ID

from . import AD5593RComponent, CONF_AD5593R_ID, ad5593r_ns

DEPENDENCIES = ["ad5593r"]

AD5593RSwitch = ad5593r_ns.class_("AD5593RSwitch", switch.Switch)

CONFIG_SCHEMA = switch.switch_schema(AD5593RSwitch).extend(
    {
        cv.GenerateID(CONF_AD5593R_ID): cv.use_id(AD5593RComponent),
        cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=7),
    }
)


async def to_code(config):
    cg.add_define("USE_SWITCH")
    parent = await cg.get_variable(config[CONF_AD5593R_ID])
    var = cg.new_Pvariable(config[CONF_ID], parent, config[CONF_CHANNEL])
    await switch.register_switch(var, config)
    cg.add(parent.register_gpio_output(var))
