#pragma once

#include <map>
#include <utility>

#include "esphome/components/web_server_base/web_server_base.h"
#include "esphome/core/component.h"
#include "esphome/core/controller.h"
#include "esphome/core/entity_base.h"

namespace esphome {
namespace prometheus {

class PrometheusHandler : public AsyncWebHandler, public Component {
 public:
  PrometheusHandler(web_server_base::WebServerBase *base) : base_(base) {}

  /** Determine whether internal components should be exported as metrics.
   * Defaults to false.
   *
   * @param include_internal Whether internal components should be exported.
   */
  void set_include_internal(bool include_internal) { include_internal_ = include_internal; }

  /** Add the new label for an entity's metric.
   *
   * @param obj The entity for which to set the label
   * @param name The label name
   * @param value The label value
   */
  void add_label(EntityBase *obj, const std::string &name, const std::string &value) {
    relabel_map_[obj].insert(std::make_pair(name, value));
  }

  /** Add the value for an entity's "id" label.
   *
   * @param obj The entity for which to set the "id" label
   * @param value The value for the "id" label
   */
  void add_label_id(EntityBase *obj, const std::string &value) { relabel_map_id_.insert({obj, value}); }

  /** Add the value for an entity's "name" label.
   *
   * @param obj The entity for which to set the "name" label
   * @param value The value for the "name" label
   */
  void add_label_name(EntityBase *obj, const std::string &value) { relabel_map_name_.insert({obj, value}); }

  bool canHandle(AsyncWebServerRequest *request) override {
    if (request->method() == HTTP_GET) {
      if (request->url() == "/metrics")
        return true;
    }

    return false;
  }

  void handleRequest(AsyncWebServerRequest *req) override;

  void setup() override {
    this->base_->init();
    this->base_->add_handler(this);
  }
  float get_setup_priority() const override {
    // After WiFi
    return setup_priority::WIFI - 1.0f;
  }

 protected:
  /// Gets metric name prefix (i.e. 'esphome_sensor') with default fallback
  std::string get_metric_name_prefix_(EntityBase *obj, const std::string &default_value);
  /// Write single label with provided label name and value
  void write_label_(AsyncResponseStream *stream, const std::string &name, const std::string &value);
  void write_label_(AsyncResponseStream *stream, const std::string &name, const std::string &value,
                    bool &start_with_comma);
  /// Write all common labels (id, name) taking into account configured relabelling
  void write_labels_(AsyncResponseStream *stream, EntityBase *obj);
  /// Write metric name and common labels without trailing '}' to allow append custom metric labels (i.e. unit, channel,
  /// effect and so on)
  void write_metric_start_(AsyncResponseStream *stream, EntityBase *obj, const std::string &metric_name,
                           const std::string &metric_suffix);

#ifdef USE_SENSOR
  /// Return the type for prometheus
  void sensor_type_(AsyncResponseStream *stream);
  /// Return the sensor state as prometheus data point
  void sensor_row_(AsyncResponseStream *stream, sensor::Sensor *obj);
#endif

#ifdef USE_BINARY_SENSOR
  /// Return the type for prometheus
  void binary_sensor_type_(AsyncResponseStream *stream);
  /// Return the sensor state as prometheus data point
  void binary_sensor_row_(AsyncResponseStream *stream, binary_sensor::BinarySensor *obj);
#endif

#ifdef USE_FAN
  /// Return the type for prometheus
  void fan_type_(AsyncResponseStream *stream);
  /// Return the sensor state as prometheus data point
  void fan_row_(AsyncResponseStream *stream, fan::Fan *obj);
#endif

#ifdef USE_LIGHT
  /// Return the type for prometheus
  void light_type_(AsyncResponseStream *stream);
  /// Return the Light Values state as prometheus data point
  void light_row_(AsyncResponseStream *stream, light::LightState *obj);
#endif

#ifdef USE_COVER
  /// Return the type for prometheus
  void cover_type_(AsyncResponseStream *stream);
  /// Return the switch Values state as prometheus data point
  void cover_row_(AsyncResponseStream *stream, cover::Cover *obj);
#endif

#ifdef USE_SWITCH
  /// Return the type for prometheus
  void switch_type_(AsyncResponseStream *stream);
  /// Return the switch Values state as prometheus data point
  void switch_row_(AsyncResponseStream *stream, switch_::Switch *obj);
#endif

#ifdef USE_NUMBER
  /// Return the type for prometheus
  void number_type_(AsyncResponseStream *stream);
  /// Return the sensor state as prometheus data point
  void number_row_(AsyncResponseStream *stream, number::Number *obj);
#endif

#ifdef USE_CLIMATE
  /// Return the type for prometheus
  void climate_type_(AsyncResponseStream *stream);
  /// Return the sensor state as prometheus data point
  void climate_row_(AsyncResponseStream *stream, climate::Climate *obj);
#endif

#ifdef USE_LOCK
  /// Return the type for prometheus
  void lock_type_(AsyncResponseStream *stream);
  /// Return the lock Values state as prometheus data point
  void lock_row_(AsyncResponseStream *stream, lock::Lock *obj);
#endif

  web_server_base::WebServerBase *base_;
  bool include_internal_{false};
  std::map<EntityBase *, std::map<std::string, std::string>> relabel_map_;
  std::map<EntityBase *, std::string> relabel_map_id_;
  std::map<EntityBase *, std::string> relabel_map_name_;
};

}  // namespace prometheus
}  // namespace esphome
