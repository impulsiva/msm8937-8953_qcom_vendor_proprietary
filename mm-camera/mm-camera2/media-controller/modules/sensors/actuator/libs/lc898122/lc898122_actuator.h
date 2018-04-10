/**
 * lc898122_actuator.h
 *
 * DESCRIPTION
 *  Auto-Generated by Actuator tool.
 *
 * Copyright (c) 2014-2015 Qualcomm Technologies, Inc. All Rights Reserved.
 *   Qualcomm Technologies Proprietary and Confidential.
 */
  {
    .actuator_params =
    {
      .module_name = "onsemi",
      .actuator_name = "lc898122",
      .i2c_addr = 0x48,
      .i2c_freq_mode = SENSOR_I2C_MODE_FAST,
      .i2c_data_type = CAMERA_I2C_WORD_DATA,
      .i2c_addr_type = CAMERA_I2C_WORD_ADDR,
      .act_type = ACTUATOR_TYPE_HVCM,
      .data_size = 10,
      .reg_tbl =
      {
        .reg_tbl_size = 1,
        .reg_params =
        {
          {
            .reg_write_type = ACTUATOR_WRITE_DAC,
            .hw_mask = 0x00000000,
            .reg_addr = 0x0380,
            .hw_shift = 0,
            .data_shift = 6,
          },
        },
      },
      .init_setting_size = 3,
      .init_settings =
      {
        {
          .reg_addr = 0x0302,
          .addr_type = CAMERA_I2C_WORD_ADDR,
          .reg_data = 0x01,
          .data_type = CAMERA_I2C_BYTE_DATA,
          .i2c_operation = ACTUATOR_I2C_OP_WRITE,
          .delay = 0,
        },
        {
          .reg_addr = 0x0303,
          .addr_type = CAMERA_I2C_WORD_ADDR,
          .reg_data = 0xF1,
          .data_type = CAMERA_I2C_BYTE_DATA,
          .i2c_operation = ACTUATOR_I2C_OP_WRITE,
          .delay = 0,
        },
        {
          .reg_addr = 0x0304,
          .addr_type = CAMERA_I2C_WORD_ADDR,
          .reg_data = 0x0400,
          .data_type = CAMERA_I2C_WORD_DATA,
          .i2c_operation = ACTUATOR_I2C_OP_WRITE,
          .delay = 0,
        },
      },
    }, /* actuator_params */

    .actuator_tuned_params =
    {
      .scenario_size =
      {
        1, /* MOVE_NEAR */
        1, /* MOVE_FAR */
      },
      .ringing_scenario =
      {
        /* MOVE_NEAR */
        {
          200,
        },
        /* MOVE_FAR */
        {
          200,
        },
      },
      .initial_code = 70,
      .region_size = 1,
      .region_params =
      {
        {
          .step_bound =
          {
            200, /* Macro step boundary*/
            0, /* Infinity step boundary*/
          },
          .code_per_step = 1,
          .qvalue = 128,
        },
      },
      .damping =
      {
        /* damping[MOVE_NEAR] */
        {
          /* Scenario 0 */
          {
            .ringing_params =
            {
              /* Region 0 */
              {
                .damping_step = 0x3FF,
                .damping_delay = 6000,
                .hw_params = 0x00000000,
              },
            },
          },
        },
        /* damping[MOVE_NEAR] */
        {
          /* Scenario 0 */
          {
            .ringing_params =
            {
              /* Region 0 */
              {
                .damping_step = 0x3FF,
                .damping_delay = 6000,
                .hw_params = 0x00000000,
              },
            },
          },
        },
      },
    }, /* actuator_tuned_params */
  },