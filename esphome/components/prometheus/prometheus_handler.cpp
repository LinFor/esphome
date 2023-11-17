#include "prometheus_handler.h"
#include "esphome/core/application.h"

namespace esphome {
namespace prometheus {

void PrometheusHandler::handleRequest(AsyncWebServerRequest *req) {
  AsyncResponseStream *stream = req->beginResponseStream("text/plain; version=0.0.4; charset=utf-8");

#ifdef USE_SENSOR
  this->sensor_type_(stream);
  for (auto *obj : App.get_sensors())
    this->sensor_row_(stream, obj);
#endif

#ifdef USE_BINARY_SENSOR
  this->binary_sensor_type_(stream);
  for (auto *obj : App.get_binary_sensors())
    this->binary_sensor_row_(stream, obj);
#endif

#ifdef USE_FAN
  this->fan_type_(stream);
  for (auto *obj : App.get_fans())
    this->fan_row_(stream, obj);
#endif

#ifdef USE_LIGHT
  this->light_type_(stream);
  for (auto *obj : App.get_lights())
    this->light_row_(stream, obj);
#endif

#ifdef USE_COVER
  this->cover_type_(stream);
  for (auto *obj : App.get_covers())
    this->cover_row_(stream, obj);
#endif

#ifdef USE_SWITCH
  this->switch_type_(stream);
  for (auto *obj : App.get_switches())
    this->switch_row_(stream, obj);
#endif

#ifdef USE_NUMBER
  this->number_type_(stream);
  for (auto *obj : App.get_numbers())
    this->number_row_(stream, obj);
#endif

#ifdef USE_CLIMATE
  this->climate_type_(stream);
  for (auto *obj : App.get_climates())
    this->climate_row_(stream, obj);
#endif

#ifdef USE_LOCK
  this->lock_type_(stream);
  for (auto *obj : App.get_locks())
    this->lock_row_(stream, obj);
#endif

  req->send(stream);
}

static const char *const ID_LABEL = "id";
static const char *const NAME_LABEL = "name";
static const char *const METRIC_NAME_LABEL = "metric_name";

std::string PrometheusHandler::get_metric_name_prefix_(EntityBase *obj, const std::string &default_value) {
  auto fres = relabel_map_.find(obj);
  if (fres == relabel_map_.end())
    return default_value;
  auto map = fres->second;
  auto item = map.find(METRIC_NAME_LABEL);
  return item == map.end() ? default_value : item->second;
}

void PrometheusHandler::write_label_(AsyncResponseStream *stream, const std::string &name, const std::string &value) {
  stream->print(name.c_str());
  stream->print("=\"");
  stream->print(value.c_str());
  stream->print('\"');
}

void PrometheusHandler::write_label_(AsyncResponseStream *stream, const std::string &name, const std::string &value,
                                     bool &start_with_comma) {
  if (start_with_comma)
    stream->print(',');
  start_with_comma = true;

  write_label_(stream, name, value);
}

void PrometheusHandler::write_labels_(AsyncResponseStream *stream, EntityBase *obj) {
  auto any_issued = false;
  auto id_issued = false;
  auto name_issued = false;
  auto fres = relabel_map_.find(obj);
  if (fres != relabel_map_.end()) {
    for (const std::pair<std::string, std::string> pair : fres->second) {
      if (METRIC_NAME_LABEL == pair.first)
        continue;

      if (ID_LABEL == pair.first)
        id_issued = true;
      if (NAME_LABEL == pair.first)
        name_issued = true;

      write_label_(stream, pair.first, pair.second, any_issued);
    }
  }
  if (!id_issued) {
    write_label_(stream, ID_LABEL, obj->get_object_id(), any_issued);
  }
  if (!name_issued) {
    write_label_(stream, NAME_LABEL, obj->get_name(), any_issued);
  }
}

void PrometheusHandler::write_metric_start_(AsyncResponseStream *stream, EntityBase *obj,
                                            const std::string &metric_name, const std::string &metric_suffix) {
  stream->print(metric_name.c_str());
  stream->print(metric_suffix.c_str());
  stream->print('{');
  write_labels_(stream, obj);
}

// Type-specific implementation
#ifdef USE_SENSOR
void PrometheusHandler::sensor_type_(AsyncResponseStream *stream) {
  stream->print(F("#TYPE esphome_sensor_value GAUGE\n"));
  stream->print(F("#TYPE esphome_sensor_failed GAUGE\n"));
}
void PrometheusHandler::sensor_row_(AsyncResponseStream *stream, sensor::Sensor *obj) {
  if (obj->is_internal() && !this->include_internal_)
    return;
  auto metric_name = get_metric_name_prefix_(obj, "esphome_sensor");
  if (!std::isnan(obj->state)) {
    // We have a valid value, output this value
    write_metric_start_(stream, obj, metric_name, "_failed");
    stream->print(F("} 0\n"));
    // Data itself
    write_metric_start_(stream, obj, metric_name, "_value");
    stream->print(',');
    write_label_(stream, "unit", obj->get_unit_of_measurement());
    stream->print(F("} "));
    stream->print(value_accuracy_to_string(obj->state, obj->get_accuracy_decimals()).c_str());
    stream->print(F("\n"));
  } else {
    // Invalid state
    write_metric_start_(stream, obj, metric_name, "_failed");
    stream->print(F("} 1\n"));
  }
}
#endif

// Type-specific implementation
#ifdef USE_BINARY_SENSOR
void PrometheusHandler::binary_sensor_type_(AsyncResponseStream *stream) {
  stream->print(F("#TYPE esphome_binary_sensor_value GAUGE\n"));
  stream->print(F("#TYPE esphome_binary_sensor_failed GAUGE\n"));
}
void PrometheusHandler::binary_sensor_row_(AsyncResponseStream *stream, binary_sensor::BinarySensor *obj) {
  if (obj->is_internal() && !this->include_internal_)
    return;
  auto metric_name = get_metric_name_prefix_(obj, "esphome_binary_sensor");
  if (obj->has_state()) {
    // We have a valid value, output this value
    write_metric_start_(stream, obj, metric_name, "_failed");
    stream->print(F("} 0\n"));
    // Data itself
    write_metric_start_(stream, obj, metric_name, "_value");
    stream->print(F("} "));
    stream->print(obj->state);
    stream->print(F("\n"));
  } else {
    // Invalid state
    write_metric_start_(stream, obj, metric_name, "_failed");
    stream->print(F("} 1\n"));
  }
}
#endif

#ifdef USE_FAN
void PrometheusHandler::fan_type_(AsyncResponseStream *stream) {
  stream->print(F("#TYPE esphome_fan_value GAUGE\n"));
  stream->print(F("#TYPE esphome_fan_speed GAUGE\n"));
  stream->print(F("#TYPE esphome_fan_oscillation GAUGE\n"));
}
void PrometheusHandler::fan_row_(AsyncResponseStream *stream, fan::Fan *obj) {
  if (obj->is_internal() && !this->include_internal_)
    return;
  auto metric_name = get_metric_name_prefix_(obj, "esphome_fan");
  // Data itself
  write_metric_start_(stream, obj, metric_name, "_value");
  stream->print(F("} "));
  stream->print(obj->state);
  stream->print(F("\n"));
  // Speed if available
  if (obj->get_traits().supports_speed()) {
    write_metric_start_(stream, obj, metric_name, "_speed");
    stream->print(F("} "));
    stream->print(obj->speed);
    stream->print(F("\n"));
  }
  // Oscillation if available
  if (obj->get_traits().supports_oscillation()) {
    write_metric_start_(stream, obj, metric_name, "_oscillation");
    stream->print(F("} "));
    stream->print(obj->oscillating);
    stream->print(F("\n"));
  }
}
#endif

#ifdef USE_LIGHT
void PrometheusHandler::light_type_(AsyncResponseStream *stream) {
  stream->print(F("#TYPE esphome_light_state GAUGE\n"));
  stream->print(F("#TYPE esphome_light_color GAUGE\n"));
  stream->print(F("#TYPE esphome_light_effect_active GAUGE\n"));
}
void PrometheusHandler::light_row_(AsyncResponseStream *stream, light::LightState *obj) {
  if (obj->is_internal() && !this->include_internal_)
    return;
  auto metric_name = get_metric_name_prefix_(obj, "esphome_light");
  // State
  write_metric_start_(stream, obj, metric_name, "_state");
  stream->print(F("} "));
  stream->print(obj->remote_values.is_on());
  stream->print(F("\n"));
  // Brightness and RGBW
  light::LightColorValues color = obj->current_values;
  float brightness, r, g, b, w;
  color.as_brightness(&brightness);
  color.as_rgbw(&r, &g, &b, &w);
  write_metric_start_(stream, obj, metric_name, "_color");
  stream->print(F(",channel=\"brightness\"} "));
  stream->print(brightness);
  stream->print(F("\n"));
  write_metric_start_(stream, obj, metric_name, "_color");
  stream->print(F(",channel=\"r\"} "));
  stream->print(r);
  stream->print(F("\n"));
  write_metric_start_(stream, obj, metric_name, "_color");
  stream->print(F(",channel=\"g\"} "));
  stream->print(g);
  stream->print(F("\n"));
  write_metric_start_(stream, obj, metric_name, "_color");
  stream->print(F(",channel=\"b\"} "));
  stream->print(b);
  stream->print(F("\n"));
  write_metric_start_(stream, obj, metric_name, "_color");
  stream->print(F(",channel=\"w\"} "));
  stream->print(w);
  stream->print(F("\n"));
  // Effect
  std::string effect = obj->get_effect_name();
  if (effect == "None") {
    write_metric_start_(stream, obj, metric_name, "_effect_active");
    stream->print(F(",effect=\"None\"} 0\n"));
  } else {
    write_metric_start_(stream, obj, metric_name, "_effect_active");
    stream->print(',');
    write_label_(stream, "effect", effect);
    stream->print(F("} 1\n"));
  }
}
#endif

#ifdef USE_COVER
void PrometheusHandler::cover_type_(AsyncResponseStream *stream) {
  stream->print(F("#TYPE esphome_cover_value GAUGE\n"));
  stream->print(F("#TYPE esphome_cover_tilt GAUGE\n"));
  stream->print(F("#TYPE esphome_cover_failed GAUGE\n"));
}
void PrometheusHandler::cover_row_(AsyncResponseStream *stream, cover::Cover *obj) {
  if (obj->is_internal() && !this->include_internal_)
    return;
  auto metric_name = get_metric_name_prefix_(obj, "esphome_cover");
  if (!std::isnan(obj->position)) {
    // We have a valid value, output this value
    write_metric_start_(stream, obj, metric_name, "_failed");
    stream->print(F("} 0\n"));
    // Data itself
    write_metric_start_(stream, obj, metric_name, "_value");
    stream->print(F("} "));
    stream->print(obj->position);
    stream->print(F("\n"));
    if (obj->get_traits().get_supports_tilt()) {
      write_metric_start_(stream, obj, metric_name, "_tilt");
      stream->print(F("} "));
      stream->print(obj->tilt);
      stream->print(F("\n"));
    }
  } else {
    // Invalid state
    write_metric_start_(stream, obj, metric_name, "_failed");
    stream->print(F("} 1\n"));
  }
}
#endif

#ifdef USE_SWITCH
void PrometheusHandler::switch_type_(AsyncResponseStream *stream) {
  stream->print(F("#TYPE esphome_switch_value GAUGE\n"));
}
void PrometheusHandler::switch_row_(AsyncResponseStream *stream, switch_::Switch *obj) {
  if (obj->is_internal() && !this->include_internal_)
    return;
  auto metric_name = get_metric_name_prefix_(obj, "esphome_switch");
  // Data itself
  write_metric_start_(stream, obj, metric_name, "_value");
  stream->print(F("} "));
  stream->print(obj->state);
  stream->print(F("\n"));
}
#endif

#ifdef USE_NUMBER
void PrometheusHandler::number_type_(AsyncResponseStream *stream) {
  stream->print(F("#TYPE esphome_number_value GAUGE\n"));
  stream->print(F("#TYPE esphome_number_failed GAUGE\n"));
}
void PrometheusHandler::number_row_(AsyncResponseStream *stream, number::Number *obj) {
  if (obj->is_internal() && !this->include_internal_)
    return;
  auto metric_name = get_metric_name_prefix_(obj, "esphome_number");
  if (obj->has_state()) {
    // We have a valid value, output this value
    write_metric_start_(stream, obj, metric_name, "_failed");
    stream->print(F("} 0\n"));
    // Data itself
    write_metric_start_(stream, obj, metric_name, "_value");
    stream->print(F("} "));
    stream->print(obj->state);
    stream->print(F("\n"));
  } else {
    // Invalid state
    write_metric_start_(stream, obj, metric_name, "_failed");
    stream->print(F("} 1\n"));
  }
}
#endif

#ifdef USE_CLIMATE
void PrometheusHandler::climate_type_(AsyncResponseStream *stream) {
  stream->print(F("#TYPE esphome_climate_target_temperature_high GAUGE\n"));
  stream->print(F("#TYPE esphome_climate_target_temperature_low GAUGE\n"));
  stream->print(F("#TYPE esphome_climate_target_temperature GAUGE\n"));
  stream->print(F("#TYPE esphome_climate_mode GAUGE\n"));
  stream->print(F("#TYPE esphome_climate_action GAUGE\n"));
  stream->print(F("#TYPE esphome_climate_current_temperature GAUGE\n"));
  stream->print(F("#TYPE esphome_climate_fan_mode GAUGE\n"));
  stream->print(F("#TYPE esphome_climate_swing_mode GAUGE\n"));
  stream->print(F("#TYPE esphome_climate_preset GAUGE\n"));
}
void PrometheusHandler::climate_row_(AsyncResponseStream *stream, climate::Climate *obj) {
  if (obj->is_internal() && !this->include_internal_)
    return;
  auto metric_name = get_metric_name_prefix_(obj, "esphome_climate");

  auto traits = obj->get_traits();
  // Setpoints
  if (traits.get_supports_two_point_target_temperature()) {
    write_metric_start_(stream, obj, metric_name, "_target_temperature_high");
    stream->print(F("} "));
    stream->print(obj->target_temperature_high);
    stream->print(F("\n"));
    write_metric_start_(stream, obj, metric_name, "_target_temperature_low");
    stream->print(F("} "));
    stream->print(obj->target_temperature_low);
    stream->print(F("\n"));
  } else {
    write_metric_start_(stream, obj, metric_name, "_target_temperature");
    stream->print(F("} "));
    stream->print(obj->target_temperature);
    stream->print(F("\n"));
  }
  // Mode
  write_metric_start_(stream, obj, metric_name, "_mode");
  stream->print(F("} "));
  stream->print(obj->mode);
  stream->print(F("\n"));
  // Action
  if (traits.get_supports_action()) {
    write_metric_start_(stream, obj, metric_name, "_action");
    stream->print(F("} "));
    stream->print(obj->action);
    stream->print(F("\n"));
  }
  // Current temperature
  if (traits.get_supports_current_temperature()) {
    write_metric_start_(stream, obj, metric_name, "_current_temperature");
    stream->print(F("} "));
    stream->print(obj->current_temperature);
    stream->print(F("\n"));
  }
  // Fan mode
  if (traits.get_supports_fan_modes()) {
    write_metric_start_(stream, obj, metric_name, "_fan_mode");
    stream->print(F("} "));
    stream->print(obj->fan_mode.value_or(-1));
    stream->print(F("\n"));
  }
  // Swing mode
  if (traits.get_supports_swing_modes()) {
    write_metric_start_(stream, obj, metric_name, "_swing_mode");
    stream->print(F("} "));
    stream->print(obj->swing_mode);
    stream->print(F("\n"));
  }
  // Preset
  if (traits.get_supports_presets()) {
    write_metric_start_(stream, obj, metric_name, "_preset");
    stream->print(F("} "));
    stream->print(obj->preset.value_or(-1));
    stream->print(F("\n"));
  }
}
#endif

#ifdef USE_LOCK
void PrometheusHandler::lock_type_(AsyncResponseStream *stream) {
  stream->print(F("#TYPE esphome_lock_value GAUGE\n"));
}
void PrometheusHandler::lock_row_(AsyncResponseStream *stream, lock::Lock *obj) {
  if (obj->is_internal() && !this->include_internal_)
    return;
  auto metric_name = get_metric_name_prefix_(obj, "esphome_lock");
  // Data itself
  write_metric_start_(stream, obj, metric_name, "_value");
  stream->print(F("} "));
  stream->print(obj->state);
  stream->print(F("\n"));
}
#endif

}  // namespace prometheus
}  // namespace esphome
