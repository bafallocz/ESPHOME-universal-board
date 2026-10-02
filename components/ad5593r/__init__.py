import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

CODEOWNERS = ["@bafallocz"]
DEPENDENCIES = ["i2c"]
MULTI_CONF = True

ad5593r_ns = cg.esphome_ns.namespace("ad5593r")
AD5593RComponent = ad5593r_ns.class_("AD5593RComponent", cg.Component, i2c.I2CDevice)

CONF_AD5593R_ID = "ad5593r_id"
CONF_GAIN_2X = "gain_2x"
CONF_REFERENCE_VOLTAGE = "reference_voltage"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AD5593RComponent),
            cv.Optional(CONF_GAIN_2X, default=True): cv.boolean,
            cv.Optional(CONF_REFERENCE_VOLTAGE, default=2.5): cv.voltage,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x11))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    cg.add(var.set_gain_2x(config[CONF_GAIN_2X]))
    cg.add(var.set_reference_voltage(config[CONF_REFERENCE_VOLTAGE]))
