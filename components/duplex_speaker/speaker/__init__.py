import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import speaker
from esphome.const import CONF_ID

CODEOWNERS = ["@ragio579"]
DEPENDENCIES = ["speaker"]

duplex_speaker_ns = cg.esphome_ns.namespace("duplex_speaker")
DuplexSpeaker = duplex_speaker_ns.class_(
    "DuplexSpeaker", speaker.Speaker, cg.Component
)

CONFIG_SCHEMA = speaker.SPEAKER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(DuplexSpeaker),
    }
).extend(cv.COMPONENT_SCHEMA)


def _final_validate(config):
    from esphome.components.audio import set_stream_limits
    return set_stream_limits(
        min_bits_per_sample=16,
        max_bits_per_sample=16,
        min_channels=1,
        max_channels=1,
        min_sample_rate=16000,
        max_sample_rate=16000,
    )(config)


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await speaker.register_speaker(var, config)
