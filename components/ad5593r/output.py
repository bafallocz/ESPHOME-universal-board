import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_CHANNEL, CONF_ID

from . import AD5593RComponent, CONF_AD5593R_ID, ad5593r_ns

DEPENDENCIES = ["ad5593r"]

AD5593RChannel = ad5593r_ns.class_("AD5593RChannel", output.FloatOutput)

CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.Required(CONF_ID): cv.declare_id(AD5593RChannel),
        cv.GenerateID(CONF_AD5593R_ID): cv.use_id(AD5593RComponent),
        cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=7),
    }
)


async def to_code(config):
    cg.add_define("USE_OUTPUT")
    parent = await cg.get_variable(config[CONF_AD5593R_ID])
    var = cg.new_Pvariable(config[CONF_ID], parent, config[CONF_CHANNEL])
    await output.register_output(var, config)
    cg.add(parent.register_dac_channel(var))
