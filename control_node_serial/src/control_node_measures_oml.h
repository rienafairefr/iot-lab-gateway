/*
 * This file was automatically generated by oml2-scaffold V2.10.1
 * for control_node_measures version 1.0.0.
 * Please do not edit.
 */

#ifndef CONTROL_NODE_MEASURES_OML_H
#define CONTROL_NODE_MEASURES_OML_H

#ifdef __cplusplus
extern "C" {
#endif

/* Get size_t and NULL from <stddef.h>.  */
#include <stddef.h>

#include <oml2/omlc.h>

typedef struct {
  OmlMP* consumption;
  OmlMP* radio;

} oml_mps_t;


#ifdef OML_FROM_MAIN
/*
 * Only declare storage once, usually from the main
 * source, where OML_FROM_MAIN is defined
 */

static OmlMPDef oml_consumption_def[] = {
  {"timestamp_s", OML_UINT64_VALUE},
  {"timestamp_us", OML_UINT32_VALUE},
  {"power", OML_DOUBLE_VALUE},
  {"voltage", OML_DOUBLE_VALUE},
  {"current", OML_DOUBLE_VALUE},
  {NULL, (OmlValueT)0}
};

static OmlMPDef oml_radio_def[] = {
  {"timestamp_s", OML_UINT64_VALUE},
  {"timestamp_us", OML_UINT32_VALUE},
  {"rssi", OML_INT32_VALUE},
  {NULL, (OmlValueT)0}
};

static oml_mps_t g_oml_mps_storage;
oml_mps_t* g_oml_mps_control_node_measures = &g_oml_mps_storage;

static inline void
oml_register_mps()
{
  g_oml_mps_control_node_measures->consumption = omlc_add_mp("consumption", oml_consumption_def);
  g_oml_mps_control_node_measures->radio = omlc_add_mp("radio", oml_radio_def);

}

#else
/*
 * Not included from the main source, only declare the global pointer
 * to the MPs and injection helpers.
 */

extern oml_mps_t* g_oml_mps_control_node_measures;

#endif /* OML_FROM_MAIN */

static inline void
oml_inject_consumption(OmlMP* mp, uint64_t timestamp_s, uint32_t timestamp_us, double power, double voltage, double current)
{
  OmlValueU v[5];

  omlc_zero_array(v, 5);

  omlc_set_uint64(v[0], timestamp_s);
  omlc_set_uint32(v[1], timestamp_us);
  omlc_set_double(v[2], power);
  omlc_set_double(v[3], voltage);
  omlc_set_double(v[4], current);

  omlc_inject(mp, v);

}

static inline void
oml_inject_radio(OmlMP* mp, uint64_t timestamp_s, uint32_t timestamp_us, int32_t rssi)
{
  OmlValueU v[3];

  omlc_zero_array(v, 3);

  omlc_set_uint64(v[0], timestamp_s);
  omlc_set_uint32(v[1], timestamp_us);
  omlc_set_int32(v[2], rssi);

  omlc_inject(mp, v);

}


/* Compatibility with old applications */
#ifndef g_oml_mps
# define g_oml_mps	g_oml_mps_control_node_measures
#endif

#ifdef __cplusplus
}
#endif

#endif /* CONTROL_NODE_MEASURES_OML_H */
