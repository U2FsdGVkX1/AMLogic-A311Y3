/*
* Copyright (C) 2026 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: aml_adla_converter
*/

#ifndef ADLA_MODEL_CONVERTER_H
#define ADLA_MODEL_CONVERTER_H

#if defined(_WIN32)
#define ADLA_CONVERTER_API __declspec(dllexport)
#else
#define ADLA_CONVERTER_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum InputModelType
{
    INPUT_MODEL_TYPE_TFLITE = 0,
    INPUT_MODEL_TYPE_ONNX = 1
} InputModelType;

typedef struct ConvertConfig
{
    const char *input_model;
    const char *output_folder;
    const char *output_name;
    const char *profile_name;
    InputModelType input_model_type;
    int log_level;
    int enable_dump_info;
    int enable_dump_graph;
    int enable_dump_intermediate_model;
    int enable_export_rebuilt_model;
} ConvertConfig;

ADLA_CONVERTER_API int amlnn_scan_model(const char *input_model, InputModelType input_model_type);

ADLA_CONVERTER_API int amlnn_convert_model(const ConvertConfig *config);

#ifdef __cplusplus
}
#endif

#endif
